#include <magic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include "keyvalcfg.h"
#include "watchnode.h"

void handle_event(struct watchnode* node, struct inotify_event* event)
{
	char* filename;
	char* mimetype;
	char* temp;
	magic_t magic;

	for (; node; node = node->next)
	{
		if (node->wd == event->wd)
			break;
	}

	if (!node)
		return;

	filename = malloc(strlen(node->path) + strlen("/") + strlen(event->name) + 1);
	strcpy(filename, node->path);
	strcat(filename, "/");
	strcat(filename, event->name);
	
/* does perror work here? should we also call exit()? */
	if ( (magic = magic_open(MAGIC_MIME)) == NULL)
		perror("magic_open");

	if (magic_load(magic, NULL) < 0)
		perror("magic_load");

	mimetype = magic_file(magic, filename);
	if (mimetype == NULL)
		perror("magic_file");

	free(filename);


/* todo:
	 parse mimetype
	 find handlers (if any) for that mimetype
	 execute handlers, continuing on to the next if exit code != 0
*/
	magic_close(magic);
}
