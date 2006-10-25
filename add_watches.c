#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <wordexp.h>
#include "keyvalcfg.h"
#include "recurse_add.h"

/* parses the config and adds inotify watches to the fd */
void add_watches(int fd, char* configfile)
{
	struct keyval_section *config;
	struct keyval_section *child;
	char* directory;
	wordexp_t wexp;

	config = keyval_parse(configfile);
	for (child = config->children; child; child = child->next)
	{
		wordexp(child->name, &wexp, 0);
		directory = strdup(wexp.we_wordv[0]);
		inotify_add_watch(fd, directory, IN_CREATE);
		if (keyval_pair_find(child->keyvals, "recurse")->value != NULL && strcmp(keyval_pair_find(child->keyvals, "recurse")->value, "true") == 0)
			recurse_add(fd, directory);
		wordfree(&wexp);
		free(directory);
	}
}
