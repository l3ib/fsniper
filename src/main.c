#include <sys/inotify.h>
#include <sys/select.h>
#include <stdio.h>
#include <unistd.h>
#include "keyvalcfg.h"
#include "watchnode.h"
#include "add_watches.h"
#include "handle_event.h"

/* size of the event structure, not counting name */
#define EVENT_SIZE  (sizeof (struct inotify_event))
/* reasonable guess as to size of 1024 events */
#define BUF_LEN        (1024 * (EVENT_SIZE + 16))
#define watch_directory "/home/andrew/tmp/inotify/tmp"

int main(int argc, char** argv)
{
	int fd, len, i = 0;
	char buf[BUF_LEN];
	struct inotify_event *event;
	struct keyval_section* config;
	struct watchnode* node;
	fd = inotify_init();
	if (fd < 0)
		perror("inotify_init");

	config = keyval_parse("test.conf");
	node = add_watches(fd, config);

	while (1)
	{
		len = read(fd, buf, BUF_LEN);
		while (i < len)
		{
			event = (struct inotify_event *) &buf[i];
			if (event->len)
			{
				printf ("name=%s\n", event->name);
				handle_event(config, node, event);
			}
			i += EVENT_SIZE + event->len;
		}
		i = 0;
	}
	keyval_section_free_all(config);
/* TODO: free the linked list stuff (path in each node and the actual nodes) */
}
