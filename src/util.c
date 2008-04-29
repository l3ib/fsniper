/*
 * fsniper - daemon to run scripts based on changes in files monitored by inotify
 * Copyright (C) 2006, 2007, 2008  Andrew Yates and David A. Foster
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

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

