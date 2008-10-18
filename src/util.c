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
#include "keyval_node.h"

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

void validate_config(struct keyval_node *config)
{
    struct keyval_node *watch;
    struct keyval_node *dir;
    struct keyval_node *match;
    struct keyval_node *child;
    int failure = 0;

    /* watch, delay_time and delay_repeats should be the only top level blocks */
    for (child = config->children; child; child = child->next)
    {
			if (!child->name) continue;
        if (strcmp(child->name, "watch") == 0)
        {
            watch = child;
        }
        else if (strcmp(child->name, "delay_time") != 0 && strcmp(child->name, "delay_repeats") != 0)
        {
            fprintf(stderr,"fsniper: invalid top-level block: %s\n",child->name);
            failure = 1;
            exit(-1); /* we can't recover from a missing top-level block */
        }
    }

    for (dir = watch->children; dir; dir = dir->next)
    {
        for (match = dir->children; match; match = match->next)
        {
            /* the only valid nonblock inside of a path block is recurse */
            if (match->value != NULL)
            {
                if (strcmp(match->name, "recurse") != 0)
                {
                    fprintf(stderr,"fsniper: invalid element inside of %s block: %s\n", \
                            dir->name, match->name);
                    failure = 1;
                }
            }
                
            /* only handler elements can go inside match blocks */
            for (child = match->children; child; child = child->next)
            {
							if (!child->name) continue;
							if (strcmp(child->name, "handler") != 0)
                {
                    fprintf(stderr,"fsniper: invalid element inside %s's \"%s\" match block: %s\n", \ 
                           dir->name, match->name,child->name);
                    failure = 1;
                }
            }
        }
    }

    if (failure)
        exit(-1);

}
