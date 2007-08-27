#include <magic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>
#include "keyvalcfg.h"
#include "watchnode.h"
#include "util.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_EFENCE
#include <efence.h>
#endif

extern struct keyval_section *config;
extern struct watchnode *node;
extern int verbose;

void handle_event(struct inotify_event* event, int writefd)
{
	char abort;
	char isextension;
	char foundslash;
	char *extension;
	char *filename;
	char *handlerexec;
	const char *mimetype;
	magic_t magic;
	int i, j, sysret, attempts;
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
		isextension = 1;

		for (i=0; child->name[i]; i++)
		{
			if (child->name[i] == '/')
				isextension = 0;
		}

		if (isextension == 1)
		{
			for (i=strlen(filename)-1; filename[i]; i--)
			{
				if (filename[i] == '.')
					extension = strdup(filename + i);
			}										 
			
			if (strlen(extension) != strlen(child->name))
				abort = 1;

			for (i=0, j=0; abort == 0 && filename[i] && child->name[j]; i++, j++)
			{
				if (extension[i] != child->name[j])
				{
					abort = 1;
					break;
				}
			}

			free(extension);

			if (abort == 0)
				break;
		}
		
		/* parse the mimetype to see if it matches the one in the config */
		for (i=0, j=0; isextension == 0 && mimetype[i] && child->name[j] && abort == 0; i++, j++)
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

	/* dup the fd */
	dup2(writefd, fileno(stdout));

/* find the handlers which correspond to the mimetype, and continue executing them
   until we've run them all or one returns 0 */	
	handler = child->keyvals;

	/* delay attempts are limited! */
	attempts = 0;

	/* TODO: configify or constify attempts */
	while (handler && attempts < 5)
	{
		if (strcmp(handler->key, "handler") != 0)
			break;

		handlerexec = malloc(strlen(handler->value) + strlen(filename) + strlen(" \"") + strlen("\"") + 1);
		strcpy(handlerexec, handler->value);
		strcat(handlerexec, " \"");
		strcat(handlerexec, filename);
		strcat(handlerexec, "\"");
		
		write(writefd, "Executing: ", 11);
		write(writefd, handlerexec, strlen(handlerexec));
		write(writefd, "\n", 1);
		sysret = WEXITSTATUS(system(handlerexec));		

		free(handlerexec);

		/* do somethng based on return code! */

		if (sysret == 0)
		{
			/* the handler handled it!  break out of the handler loop and exit this thread */
			break;
		}
		else if (sysret == DELAY_RET_CODE)
		{
			/* go to sleep for a while */
			/* TODO: get config value for this delay time */
			attempts++;
			write(writefd, "Handler indicated delay, sleeping...", 37); 
			sleep(5 * 60);
			write(writefd, "Handler process resuming.", 26);
		} 
		else
		{
			/* some other return code, means try the next handler */
			handler = handler->next;
		}
	}

	/* TODO: constify/configify this again */
	if (attempts == 5)
		write(writefd, "Handler gave up on retries.", 28); 

	free(filename);
	magic_close(magic);

	/* close down our pipe */
	close(writefd);
	exit(0);
}
