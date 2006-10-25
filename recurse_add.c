#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <wordexp.h>
#include "keyvalcfg.h"

void recurse_add(int fd, char* directory)
{
	struct stat dir_stat;
  struct dirent* entry;
	char* path;
  DIR* dir = opendir(directory);

	if (dir == NULL)
	{
		return;
	}

	while ( (entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0)
			continue;
		if (strcmp(entry->d_name, "..") == 0)
			continue;
		path = malloc(strlen(directory) + strlen(entry->d_name) + 2);
		sprintf(path, "%s/%s", directory, entry->d_name);
		if (stat(path, &dir_stat) == -1) {
			continue;
		}
		if (S_ISDIR(dir_stat.st_mode))
		{
			printf("adding watch for: %s\n", path);
			inotify_add_watch(fd, path, IN_CREATE);
			recurse_add(fd, path);
		}
		free(path);
	}
}
