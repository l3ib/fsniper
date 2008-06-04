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

#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <wordexp.h>
#include "keyval_node.h"
#include "watchnode.h"
#include "log.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_EFENCE
#include <efence.h>
#endif

extern struct keyval_node *config;
extern struct watchnode *g_watchnode;
extern int verbose;


/* recursively add watches */
void recurse_add(struct watchnode* node, int fd, char *directory, struct keyval_node* child)
{
    struct stat dir_stat;
    struct dirent *entry;
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
            if (verbose) log_write("Watching directory: %s\n", path);
            
            node = watchnode_create(node, inotify_add_watch(fd, path, IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_DELETE),
                               strdup(path), child);

            recurse_add(node, fd, path, child);
        }
        free(path);
    }
}

/* parses the config and adds inotify watches to the fd */
struct watchnode* add_watches(int fd)
{
    struct stat dir_stat;
    struct keyval_node* child;
    struct keyval_node* startchild = NULL;
    struct keyval_node* recurse;
    char* directory;
    wordexp_t wexp;
    struct watchnode* firstnode;
    struct watchnode* node;
   
    firstnode = malloc(sizeof(struct watchnode));
    node = firstnode;
    node->path = NULL;
    node->section = NULL;
    node->next = NULL;

    /* find watch child from main cfg item */
    for (child = config->children; child; child = child->next)
        if (strcmp(child->name, "watch") == 0)
        {
            startchild = child;
            break;
        }

    if (!startchild)
    {
        if (verbose) log_write("No start child found!");
        return firstnode;
    }

    /* get children of this */
    for (child = startchild->children; child; child = child->next)
    {
        wordexp(child->name, &wexp, 0);
        directory = strdup(wexp.we_wordv[0]);
        if (stat(directory, &dir_stat) == -1)
        {
            log_write("Error: directory \"%s\" does not exist.\n", directory);
            continue;        
        }
        if (!S_ISDIR(dir_stat.st_mode))
        {
            log_write("Error: \"%s\" is not a directory.\n", directory);
            continue;
        }
        if (verbose) log_write("Watching directory: %s\n", directory);

        /* create the node and advance the pointer */
        node = watchnode_create(node, inotify_add_watch(fd, directory, IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_DELETE),
                           strdup(wexp.we_wordv[0]), child);

        if ((recurse = keyval_node_find(child, "recurse")))
            if (recurse->value && keyval_node_get_value_bool(recurse))
                recurse_add(node, fd, directory, child);
        wordfree(&wexp);
        free(directory);
    }
    return firstnode;
}

