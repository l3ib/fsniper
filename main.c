#include <sys/inotify.h>
#include <sys/select.h>
#include <stdio.h>
#include <unistd.h>
#include "add_watches.h"

/* size of the event structure, not counting name */
#define EVENT_SIZE  (sizeof (struct inotify_event))
/* reasonable guess as to size of 1024 events */
#define BUF_LEN        (1024 * (EVENT_SIZE + 16))
#define watch_directory "/home/andrew/tmp/inotify/tmp"

int main(int argc, char** argv)
{
	int fd, len, i = 0;
	char buf[BUF_LEN];
	fd = inotify_init();
	if (fd < 0)
		perror("inotify_init");

	add_watches(fd, "test.conf");
	/*int wd;
	wd = inotify_add_watch(fd, watch_directory, IN_CREATE);
	if (wd < 0)
		perror("inotify_add_watch");*/

	while (1)
	{
		len = read(fd, buf, BUF_LEN);
		while (i < len) {
			struct inotify_event *event;
			event = (struct inotify_event *) &buf[i];
			if (event->len)
				printf ("name=%s\n", event->name);
			i += EVENT_SIZE + event->len;
		}
		i = 0;
	}
}
