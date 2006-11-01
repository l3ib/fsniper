#include <sys/inotify.h>
#include <sys/select.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "keyvalcfg.h"
#include "add_watches.h"

/* size of the event structure, not counting name */
#define EVENT_SIZE  (sizeof (struct inotify_event))
/* reasonable guess as to size of 1024 events */
#define BUF_LEN        (1024 * (EVENT_SIZE + 16))

/* one global config */
struct keyval_section *config;

void handle_quit_signal(int signum)
{
	/* free config here */
	keyval_section_free_all(config);
	exit(0);
}

int main(int argc, char** argv)
{
	int fd, len, i = 0;
	char buf[BUF_LEN];

	/* set up signals for exiting */
	signal(SIGINT, &handle_quit_signal);
	signal(SIGTERM, &handle_quit_signal);

	fd = inotify_init();
	if (fd < 0)
		perror("inotify_init");

	config = keyval_parse("test.conf");

	add_watches(fd);

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
