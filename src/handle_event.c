#include <magic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include "keyvalcfg.h"
#include "watchnode.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_EFENCE
#include <efence.h>
#endif

extern struct keyval_section *config;
extern struct watchnode *node;
extern unsigned char *verbose;

void handle_event(struct inotify_event* event)
{
	char abort;
	char foundslash;
	char *filename;
	char *handlerexec;
	const char *mimetype;
	magic_t magic;
	int i, j;
	struct keyval_section *child;
	struct keyval_pair *handler;
	
/* find the node that corresponds to the event's descriptor */
	for (; node; node = node->next)
	{
		if (node->wd == event->wd)
			break;
	}

	if (!node)
		return;

/* combine the name inotify gives with the full path to the file */
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

/* parse the mimetype to see if it matches the mimetype in the config. */
	abort = 0;
	for (child = node->section->children; child; child = child->next)
	{
		abort = 0;
		foundslash = 0;

		for (i=0, j=0; mimetype[i] && child->name[j] && abort == 0; i++, j++)
		{
			if (!mimetype[i])
			{
				abort = 1;
				break;
			}
			else if (!child->name[j])
			{
				abort = 1;
				break;
			}

			if (foundslash == 0 && child->name[j] == '/')
				foundslash = 1;

			if (mimetype[i] != child->name[j] && child->name[j] != '*')
			{
				abort = 1;
				break;
			}
			else if (child->name[j] == '*' && foundslash == 0)
			{
				while (mimetype[i] != '/')
					i++;
				while (child->name[j] != '/')
					j++;
				foundslash = 1;
			}
			else if (child->name[j] == '*' && foundslash == 1)
				break;
		}
		if (abort == 0)
			break;
	}

	if (abort == 1)
	{
		free(filename);
		magic_close(magic);
		exit(-1);
	}

/* find the handlers which correspond to the mimetype, and continue executing them
   until we've run them all or one returns 0 */	
	for (handler = child->keyvals; handler; handler = child->keyvals->next)
	{
		if (strcmp(handler->key, "handler") != 0)
			break;

		handlerexec = malloc(strlen(handler->value) + strlen(filename) + strlen(" \"") + strlen("\"") + 1);
		strcpy(handlerexec, handler->value);
		strcat(handlerexec, " \"");
		strcat(handlerexec, filename);
		strcat(handlerexec, "\"");

		printf("executing: %s\n", handlerexec);
		if (system(handlerexec) == 0)
		{
			free(handlerexec);
			break;
		}
		else
			free(handlerexec);
	}

	free(filename);
	magic_close(magic);

	exit(0);
}
