#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include "keyvalcfg.h"
#include "watchnode.h"

void handle_event(struct keyval_section* config, struct watchnode* node, struct inotify_event* event)
{
	char* execute;
	char* mimetype;
	char* temp;
	char* temp2;
	FILE* pipe;

	for (; node; node = node->next)
	{
		if (node->wd == event->wd)
			break;
	}

	if (!node)
		return;

	execute = malloc(strlen(node->path) + strlen("/") + strlen(event->name) + strlen("file -ib ") + 1);
	strcat(execute, "file -ib ");
	strcat(execute, node->path);
	strcat(execute, "/");
	strcat(execute, event->name);
 
	if ( (pipe = popen(execute, "r")) == NULL )
		perror("popen");

	free(execute);

	temp2 = strdup("");
	temp = malloc(30);
	mimetype = malloc(1);
	while (fgets(temp, 29, pipe) != NULL)
	{
		free(mimetype);
		mimetype = malloc(strlen(temp2) + strlen(temp));
		strcat(mimetype, temp2);
		strcat(mimetype, temp);
		free(temp2);
	} 
	free(temp);

/* todo:
	 parse mimetype (free it later?)
	 find handlers (if any) for that mimetype
	 execute handlers, continuing on to the next if exit code != 0
*/
}
