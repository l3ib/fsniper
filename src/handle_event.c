#include <magic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include "keyvalcfg.h"
#include "watchnode.h"

extern struct keyval_section *config;

void handle_event(struct watchnode* node, struct inotify_event* event)
{
	char abort;
	char foundslash;
	char* filename;
	const char* mimetype;
	magic_t magic;
	int i;
	struct keyval_section* child;

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

	abort = 0;
	for (child = node->section->children; child; child = child->next)
	{
		abort = 0;
		foundslash = 0;

		for (i=0; mimetype[i] && child->name[i] && abort == 0; i++)
		{
			if (child->name[i] == '/')
				foundslash = 1;

			if (mimetype[i] != child->name[i] && child->name[i] != '*')
			{
				abort = 1;
				break;
			}
			else if (child->name[i] == '*' && foundslash == 0)
			{
				while (child->name[i] != '/')
					i++;
				foundslash = 1;
			}
			else if (child->name[i] == '*' && foundslash == 1)
				break;
		}
		if (abort == 0)
			break;
	}

	if (abort == 1)
		return;

	printf("settled on cn: %s\n", child->name); 
	magic_close(magic);

/* todo:
	 find handlers (if any) for that mimetype
	 execute handlers, continuing on to the next if exit code != 0
*/
}
