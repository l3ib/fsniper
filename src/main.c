#include <sys/inotify.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "keyvalcfg.h"
#include "watchnode.h"
#include "add_watches.h"
#include "handle_event.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_EFENCE
#include <efence.h>
#endif

/* size of the event structure, not counting name */
#define EVENT_SIZE  (sizeof (struct inotify_event))
/* reasonable guess as to size of 1024 events */
#define BUF_LEN        (1024 * (EVENT_SIZE + 16))

/* one global config */ 
struct keyval_section *config = NULL; 

/* global watchnode */
struct watchnode *node = NULL;

/* used for verbose printfs throughout sniper */
unsigned char verbose;  

void handle_quit_signal(int signum) 
{
	struct watchnode* cur, *prev;

	if (verbose) printf("received signal %d. exiting.\n", signum);
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
	exit(0); 
} 

int main(int argc, char** argv)
{
	int fd, len, i = 0;
	char buf[BUF_LEN]; 
	char *configdir;
	char *home;
	DIR *dir;
	struct inotify_event *event;
	char *configfile = "test.conf";

/* set up signals for exiting */ 
	signal(SIGINT, &handle_quit_signal); 
	signal(SIGTERM, &handle_quit_signal);

/* check for ~/.config/sniper/ and create it if needed */
	home = getenv("HOME");
	if (!home)
	{
		printf("error: no HOME environment variable set\n");
		exit(1);
	}

	configdir = malloc(strlen(home) + strlen ("/.config") + strlen ("/sniper") + 1);

	sprintf(configdir, "%s/.config", home);
	if ( (dir = opendir(configdir)) == NULL)
		mkdir(configdir, S_IRWXU | S_IRWXG | S_IRWXO);
	closedir(dir);

	sprintf(configdir, "%s/.config/sniper", home);
	if ( (dir = opendir(configdir)) == NULL)
		mkdir(configdir, S_IRWXU | S_IRWXG | S_IRWXO);
	closedir(dir);

	free(configdir);

	fd = inotify_init();
	if (fd < 0)
		perror("inotify_init");

	if (verbose) printf("parsing config file: %s\n", configfile);
	config = keyval_parse(configfile);
	node = add_watches(fd);

/* wait for inotify events and then handle them */
	while (1)
	{
		len = read(fd, buf, BUF_LEN);
		while (i < len)
		{
			event = (struct inotify_event *) &buf[i];
			if (event->len)
				handle_event(event);
			i += EVENT_SIZE + event->len;
		}
		i = 0;
	}
}
