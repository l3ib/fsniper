/*
 * fsniper - daemon to run scripts based on changes in files monitored by inotify
 * Copyright (C) 2006, 2007, 2008  Andrew Yates and David A. Foster
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <sys/inotify.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include "keyval_parse.h"
#include "argparser.h"
#include "watchnode.h"
#include "add_watches.h"
#include "handle_event.h"
#include "util.h"
#include "log.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_EFENCE
#include <efence.h>
#endif

extern int errno;

extern FILE* _logfd;

/* size of the event structure, not counting name */
#define EVENT_SIZE  (sizeof (struct inotify_event))
/* reasonable guess as to size of 1024 events */
#define BUF_LEN        (1024 * (EVENT_SIZE + 16))

/* one global config */ 
struct keyval_node *config = NULL; 

/* global watchnode */
struct watchnode *g_watchnode = NULL;

/* used for verbose printfs throughout fsniper */
int verbose = 0;  

/* synchronous mode, no forking for handlers */
int syncmode = 0;

int logtype = 0;
char *log_arg = NULL;

/* inotify file descriptor */
int ifd;

/* the actual config file. needed by SIGHUP. */
char *configfile;

/* structure for maintaining pipes */
struct pipe_list
{
    int pfd[2];
    struct pipe_list* next;
};

struct pipe_list* pipe_list_head = NULL;
struct pipe_list * pipe_list_remove(struct pipe_list * head,
				    struct pipe_list * element);

/* frees all watchnode elements */
void free_watchnodes()
{
    struct watchnode *cur, *prev;
    if (!g_watchnode)
        return;

    cur = g_watchnode->next;

    while (cur) {
        if (cur->path)
            free(cur->path);
        prev = cur;
        cur = cur->next;
        free(prev);
    }
}

/* frees memory.  called from more than one place because fork'd copies
 * still have globals hanging around before they are exited.
 *
 * frees:
 * - config
 * - configfile
 * - watchnode elements
 * - pipe list
 *
 * called from:
 * - handle_quit_signal (in case of main exiting) 
 * - exit_handler (in case of handler exiting)
 */
void free_all_globals()
{
    struct pipe_list* tmp_pipe;

    /* free config here */ 
    keyval_node_free_all(config);

    /* free watchnode elements */
    free_watchnodes();
    free(g_watchnode);

    /* free / close any remaining pipes in the list */    
    if (pipe_list_head)
    {
        tmp_pipe = pipe_list_head->next;
        while(tmp_pipe)
            tmp_pipe = pipe_list_remove(pipe_list_head, tmp_pipe);
        free(pipe_list_head);
    }
   
    free(configfile);
}

char *get_pid_filename()
{
    struct passwd *uid;
    char *pidfilename;
    uid = getpwuid(getuid());
    pidfilename = malloc(strlen("/tmp/fsniper-") + strlen(uid->pw_name) + strlen(".pid") + 1);
    sprintf(pidfilename, "/tmp/fsniper-%s.pid", uid->pw_name);
    return pidfilename;
}

void write_pid_file(char *pidfilename)
{
    FILE *pidfile;
    mode_t umask_old;
    umask_old = umask(0177);
    pidfile = fopen(pidfilename, "w");
    umask(umask_old);
    fprintf(pidfile, "%d", getpid());
    fclose(pidfile);
}

void remove_pid_file(char *pidfilename)
{
    remove(pidfilename);
    free(pidfilename);
}

/* handler for any quit signal.  cleans up memory and exits gracefully. */
void handle_quit_signal(int signum) 
{
    if (verbose) log_write("Received signal %d, exiting.\n", signum);

    /* kill anything in our process group, including all handler childs */
    killpg(getpid(), SIGTERM);
	
    /* free things */
    free_all_globals();

    /* shut down log */
    log_close();

    remove_pid_file(get_pid_filename());

    /* return an error if there was one */
    if (signum < 0)
	exit(signum);

    exit(0); 
} 

/* handler for reaping children after the fork is done. */
void handle_child_signal()
{
    union wait status;
    while (wait3(&status, WNOHANG, 0) > 0) {} 
}

/* handler for HUP. reloads the config file. */
void handle_hup_signal()
{
    char * error;
    log_write("Received SIGHUP, reloading config file.\n");
    keyval_node_free_all(config);
    config = keyval_parse_file(configfile);
    if ((error = keyval_get_error())) {
        fprintf(stderr, "%s", error);
        free(error);
        exit(1);
    }
    close(ifd);
    free_watchnodes();
    ifd = inotify_init();
    g_watchnode = add_watches(ifd);
}


/* handler for any child process receiving a quit signal. */
void handle_child_quit_signal(int signum)
{
    exit(0);
}

/* deletes an element from the linked list and returns a pointer to the
 * next element. */
struct pipe_list * pipe_list_remove(struct pipe_list * head,
				    struct pipe_list * element) {

    struct pipe_list * next = element->next;
    struct pipe_list * prev;

    /* find the previous element (the one before 'element') */
    for (prev = head; prev->next != element; prev = prev->next);

    free(element);
	
    return (prev->next = next);
}

int main(int argc, char** argv)
{
    int ifd, len = 0, i = 0, selectret = 0, maxfd, retryselect, pid;
    char buf[BUF_LEN]; 
    char *configdir;
    char *pidfilename;
    char *statusfilename;
    char *statusbin;
    char *error_str;
    char *version_str = PACKAGE_STRING;
    char *pbuf;
    char *filename;
    FILE *pidfile;
    FILE *statusfile;
    fd_set set;
    struct inotify_event *event;
    struct argument *argument = argument_new();
    struct pipe_list *pipe_list_cur;
    struct stat file_stat;
    struct watchnode *node;

    /* alloc pipe list */
    pipe_list_head = malloc(sizeof(struct pipe_list));
    pipe_list_head->next = NULL;

    /* set up signals for exiting/reaping */ 
    signal(SIGINT, &handle_quit_signal); 
    signal(SIGTERM, &handle_quit_signal);
    signal(SIGCHLD, &handle_child_signal);
    signal(SIGHUP, &handle_hup_signal);


    /* add command line arguments */
    argument_register(argument, "help", "Prints this help text.", 0);
    argument_register(argument, "version", "Prints version information.", 0);
    argument_register(argument, "daemon", "Run as a daemon.", 0);
    argument_register(argument, "verbose", "Turns on debug text.", 0);
    argument_register(argument, "sync", "Sync mode (for debugging).", 0);
    argument_register(argument, "log-to-stdout", "Deprecated, use \"--log-to=stdout\" instead", 0);
    argument_register(argument, "log-to", "Log messages with specified way. " \
                                "Can be: stdout, file, syslog. \"file\" by default.", 1);

    if ((error_str = argument_parse(argument, argc, argv))) {
	fprintf(stderr, "Error in arguments: %s", error_str);
	free(error_str);
	return -1;
    }

    if (argument_exists(argument, "help")) {
	char *help_txt = argument_get_help_text(argument);
	printf("%s", help_txt);
	free(help_txt);
	return 0;
    }

    if (argument_exists(argument, "version")) {
	printf("%s\n", version_str);
	return 0;
    }

    if (argument_exists(argument, "verbose")) {
	verbose = 1;
    }

    if (argument_exists(argument, "daemon") && fork())
	return 0;

    if (argument_exists(argument, "sync"))
	syncmode = 1;


    if (argument_exists(argument, "log-to-stdout"))
        fprintf(stderr, "Warning, this option is deprecated, " \
                        "please use new syntax: \"--log-to=stdout\".\n");

    logtype = LOG_FILE;
    if (argument_exists(argument, "log-to") && \
        (log_arg = argument_get_value(argument, "log-to")) != NULL)
    {
        if (strcmp(log_arg, "stdout") == 0)
	    logtype = LOG_STDOUT;

        if (strcmp(log_arg, "syslog") == 0)
	    logtype = LOG_SYS;
    }

    /* get config dir (must free this) */
    configdir = get_config_dir();	

    /* if a config file has not been specified, use default */
    if (argument_get_extra(argument))
    {
	configfile = strdup(argument_get_extra(argument));
    }
    else
    {
	configfile = malloc (strlen(configdir) + strlen ("/config") + 1);
	sprintf(configfile, "%s/config", configdir);
    }

    argument_free(argument);
    free(configdir);

    if (access(configfile, R_OK) != 0)
    {
	fprintf(stderr, "error: could not open config file: %s\n", configfile);
	return -1;
    }

    /* create a pid file */
    pidfilename = get_pid_filename();
	
    if (stat(pidfilename, &file_stat) == 0) /* pidfile exists */
    {
	pidfile = fopen(pidfilename, "r");
		
	if (fscanf(pidfile, "%d", &pid) == 1) /* pidfile has a pid inside */
	{
	    char *binaryname;
	    char *scanformat; 
	    if ((binaryname = strrchr(argv[0], '/')) != NULL)
	    {
		binaryname++;
	    }
	    else
	    {
		binaryname = argv[0];
	    }

	    scanformat = malloc(strlen("Name:   %") + strlen(binaryname) + strlen("s") + 1);
	    statusfilename = malloc(strlen("/proc/") + 6 + strlen("/status") + 1);
	    sprintf(statusfilename, "/proc/%d/status", pid);

	    if (stat(statusfilename, &file_stat) != 0) /* write pid file if the process no longer exists */
	    {
		write_pid_file(pidfilename);
	    }
	    else /* process exists, so check owner and binary name */
	    {
		statusfile = fopen(statusfilename, "r");
		statusbin = malloc(strlen(binaryname) + 2); /* the binary name may start with "fsniper" but be longer */
		sprintf(scanformat, "Name:   %%%ds", strlen(binaryname) + 1);
		fscanf(statusfile, scanformat, statusbin);
		free(statusfilename);
		fclose(statusfile);
		fclose(pidfile);
				
		if (strcmp(binaryname, statusbin) == 0 && file_stat.st_uid == getuid())
		    /* exit if the process is fsniper and is owned by the current user */
		{
		    printf("%s: already running instance found with pid %d. exiting.\n", binaryname, pid);
		    exit(1);
		}
		else /* the pid file contains an old pid, one that isn't fsniper, or one not owned by the current user */
		{
		    write_pid_file(pidfilename);
		}
	    }
	}
	else /* pidfile is invalid */
	{
	    fclose(pidfile);
	    write_pid_file(pidfilename);
	}
    }
    else /* the pidfile doesn't exist */
    {
	write_pid_file(pidfilename);
    }
    free(pidfilename);

    /* start up log */
    if (!log_open())
    {
	fprintf(stderr, "Error: could not start log.\n");
	return -1;
    }

    ifd = inotify_init();
    if (ifd < 0)
    {
	perror("inotify_init");
	return -1;
    }

    if (verbose) log_write("Parsing config file: %s\n", configfile);
    config = keyval_parse_file(configfile);

    if ((error_str = keyval_get_error())) {
        fprintf(stderr, "%s", error_str);
        free(error_str);
        exit(1);
    }

    validate_config(config);

    /* add nodes to the inotify descriptor */
    g_watchnode = add_watches(ifd);

    /* wait for events and then handle them */
    while (1)
    {		
	/* set up fds and max */
	FD_ZERO(&set);
	FD_SET(ifd, &set);
	maxfd = ifd;
	for (pipe_list_cur = pipe_list_head->next; pipe_list_cur; pipe_list_cur = pipe_list_cur->next)
	{
	    FD_SET(pipe_list_cur->pfd[0], &set);
	    if (pipe_list_cur->pfd[0] > maxfd)
		maxfd = pipe_list_cur->pfd[0];
	}

	retryselect = 1;
	while (retryselect)
	{
	    /* use select to get activity on any of the fds */
	    selectret = select(maxfd + 1, &set, NULL, NULL, NULL);

	    if (selectret == -1)
	    {
		if (errno == EINTR)
		    retryselect = 1;
		else
		    handle_quit_signal(-2);
	    } else
		retryselect = 0;
	}
		
	/* handle any events on the inotify fd */
	if (FD_ISSET(ifd, &set))
	{
	    len = read(ifd, buf, BUF_LEN);
	    while (i < len)
	    {
		event = (struct inotify_event *) &buf[i];
		if (event->len && (event->mask & IN_CLOSE_WRITE || event->mask & IN_MOVED_TO))
		{
		    /* if sync mode, just call handle_exec */
		    if (syncmode == 1)
		    {
			handle_event(event, fileno(_logfd));
		    }
		    else
		    {
			/* create new pipe_list entry */
			for (pipe_list_cur = pipe_list_head; pipe_list_cur->next != NULL; pipe_list_cur = pipe_list_cur->next) {}

			pipe_list_cur->next = malloc(sizeof(struct pipe_list));
			pipe_list_cur->next->next = NULL;

			/* create pipe */
			pipe(pipe_list_cur->next->pfd);

			if (fork() == 0) 
			{
			    /* child, close 0 */
			    close(pipe_list_cur->next->pfd[0]);					
			    log_close();
			    signal(SIGINT, &handle_child_quit_signal);
			    signal(SIGTERM, &handle_child_quit_signal);
			    handle_event(event, pipe_list_cur->next->pfd[1]);
			} else {
			    /* parent, close 1 */
			    close(pipe_list_cur->next->pfd[1]);
			}
		    }
		}
                else if (event->len && (event->mask & IN_CREATE && event->mask & IN_ISDIR))
                {
                    for (node = g_watchnode->next; node; node = node->next)
                        if (node->wd == event->wd)
                            break;

                    if (node)
                    {
                        /* combine the name inotify gives with the full path to the file */
                        filename = malloc(strlen(node->path) + strlen("/") + strlen(event->name) + 1);
                        sprintf(filename, "%s/%s", node->path, event->name);
                        watch_dir(node, ifd, strdup(filename), node->section);
                        free(filename);
                    }
                }
		else if (event->len && (event->mask & IN_DELETE && event->mask & IN_ISDIR))
                {
                    for (node = g_watchnode->next; node; node = node->next)
                        if (node->wd == event->wd)
                            break;

                    if (node)
                    {
                        /* combine the name inotify gives with the full path to the file */
                        filename = malloc(strlen(node->path) + strlen("/") + strlen(event->name) + 1);
                        sprintf(filename, "%s/%s", node->path, event->name);
                        unwatch_dir(filename, ifd);
                        free(filename);
                    }
                }
                i += EVENT_SIZE + event->len;
            }
	    i = 0;
	}
		
	/* now lets see if we have any pipe activity */
	pipe_list_cur = pipe_list_head->next;
	while (pipe_list_cur)
	{
	    if (FD_ISSET(pipe_list_cur->pfd[0], &set))
	    {
		len = read(pipe_list_cur->pfd[0], buf, BUF_LEN);
		if (len == 0)
		{
		    close(pipe_list_cur->pfd[0]);
		    /* remove this item from the list */
		    pipe_list_cur = pipe_list_remove(pipe_list_head, pipe_list_cur);
					
		} else {
		    /* print it somewhere */
		    pbuf = malloc(len + 1);
		    snprintf(pbuf, len, "%s", buf);
		    log_write("%s\n", pbuf);
		    free(pbuf);
		    pipe_list_cur = pipe_list_cur->next;
		}
	    } else {
		pipe_list_cur = pipe_list_cur->next;

	    }


	}
    }
}
