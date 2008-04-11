#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "util.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_EFENCE
#include <efence.h>
#endif

/* you must free this return value */
char* get_config_dir()
{
    char *configdir, *home;
    DIR *dir;

    home = getenv("HOME");
    if (!home)
    {
        printf("error: no HOME environment variable set\n");
        exit(1);
    }

    configdir = malloc(strlen(home) + strlen("/.config/") + strlen(PACKAGE_NAME) + 1);

    sprintf(configdir, "%s/.config", home);
    if ( (dir = opendir(configdir)) == NULL)
        mkdir(configdir, S_IRWXU | S_IRWXG | S_IRWXO);
    closedir(dir);

    sprintf(configdir, "%s/.config/%s", home, PACKAGE_NAME);
    if ( (dir = opendir(configdir)) == NULL)
        mkdir(configdir, S_IRWXU | S_IRWXG | S_IRWXO);
    closedir(dir);

    return configdir;
}

