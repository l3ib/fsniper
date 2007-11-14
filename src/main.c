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
#include "keyvalcfg.h"
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
struct keyval_section *config = NULL; 

/* global watchnode */
struct watchnode *node = NULL;

/* used for verbose printfs throughout sniper */
int verbose = 0;  

/* synchronous mode, no forking for handlers */
int syncmode = 0;

/* whether to log to stdout or not */
int logtostdout = 0;

/* structure for maintaining pipes */
struct pipe_list
{
	int pfd[2];
	struct pipe_list* next;
};

struct pipe_list* pipe_list_head = NULL;
struct pipe_list * pipe_list_remove(struct pipe_list * head,
	struct pipe_list * element);

/* frees memory.  called from more than one place because fork'd copies
 * still have globals hanging around before they are exited.
 *
 * frees:
 * - config
 * - watchnode elements
 * - pipe list
 *
 * called from:
 * - handle_quit_signal (in case of main exiting) 
 * - exit_handler (in case of handler exiting)
 */
void free_all_globals()
{
	struct watchnode *cur, *prev;
	struct pipe_list* tmp_pipe;

	/* free config here */ 
	keyval_section_free_all(config);

	/* free watchnode elements */
	cur = node;
	while (cur) {
		if (cur->path)
			free(cur->path);
		prev = cur;
		cur = cur->next;
		free(prev);
	}

	/* free / close any remaining pipes in the list */    
	tmp_pipe = pipe_list_head->next;
	while(tmp_pipe)
		tmp_pipe = pipe_list_remove(pipe_list_head, tmp_pipe);

	free(pipe_list_head);
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
	int ifd, len, i = 0, selectret = 0, maxfd, forkret, retryselect;
	char buf[BUF_LEN]; 
	char *configdir;
	char *configfile;
	char *home;
	char *pidfilename;
	char *error_str;
	char *version_str = "sniper SVN";
	char *pbuf;
	FILE *pidfile;
	DIR *dir;
	fd_set set;
	struct passwd *uid;
	struct inotify_event *event;
	struct argument *argument = argument_new();
	struct pipe_list *pipe_list_cur;
	struct pipe_list *pipe_list_tmp;

	/* alloc pipe list */
	pipe_list_head = malloc(sizeof(struct pipe_list));
	pipe_list_head->next = NULL;

	/* set up signals for exiting/reaping */ 
	signal(SIGINT, &handle_quit_signal); 
	signal(SIGTERM, &handle_quit_signal);
	signal(SIGCHLD, &handle_child_signal);


	/* add command line arguments */
	argument_register(argument, "help", "Prints this help text.", 0);
	argument_register(argument, "version", "Prints version information.", 0);
	argument_register(argument, "daemon", "Run as a daemon.", 0);
	argument_register(argument, "verbose", "Turns on debug text.", 0);
	argument_register(argument, "sync", "Sync mode (for debugging).", 0);
	argument_register(argument, "log-to-stdout", "Log to stdout alongside the usual log file.", 0);

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

	if (argument_exists(argument, "log-to-stdout")) {
		logtostdout = 1;
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
	config = keyval_parse(configfile);
	free(configfile);

	/* create a pid file */
	uid = getpwuid(getuid());
	pidfilename = malloc(strlen("/tmp/sniper-") + strlen(uid->pw_name) + strlen(".pid") + 1);
	sprintf(pidfilename, "/tmp/sniper-%s.pid", uid->pw_name);
	pidfile = fopen(pidfilename, "w");
	fprintf(pidfile, "%d", getpid());
	fclose(pidfile);
	free(pidfilename);

	/* add nodes to the inotify descriptor */
	node = add_watches(ifd);

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
				if (event->len)
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
