#include <sys/inotify.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "keyvalcfg.h"
#include "watchnode.h"
#include "add_watches.h"
#include "handle_event.h"

/* size of the event structure, not counting name */
#define EVENT_SIZE  (sizeof (struct inotify_event))
/* reasonable guess as to size of 1024 events */
#define BUF_LEN        (1024 * (EVENT_SIZE + 16))

/* one global config */ 
struct keyval_section *config; 

/* watchnode global for this file */
struct watchnode *node;
  
void handle_quit_signal(int signum) 
{
	/* free config here */ 
	keyval_section_free_all(config);
	/* free watchnode elements */
  while (node) {
    struct watchnode* next = node->next;
    if (node->path) free(node->path);
    free(node);
    node = next;
	}
	exit(0); 
} 

int main(int argc, char** argv)
{
	int fd, len, i = 0;
	char buf[BUF_LEN]; 
	struct inotify_event *event;

/* set up signals for exiting */ 
	signal(SIGINT, &handle_quit_signal); 
	signal(SIGTERM, &handle_quit_signal);

	fd = inotify_init();
	if (fd < 0)
		perror("inotify_init");

	config = keyval_parse("test.conf");
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
