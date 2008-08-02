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

/**
 * Adds an inotify watch to the list of watches.
 */
struct watchnode* watch_dir(struct watchnode* node, int ifd, char *dir, struct keyval_node* kv_section)
{
    struct watchnode *retval;

    if (verbose) log_write("Watching directory: %s\n", dir);

    /* warp to end of watchnode list */
    while (node->next)
        node = node->next;

    retval = watchnode_create(node,
                              inotify_add_watch(ifd, dir, IN_CLOSE_WRITE | IN_MOVED_TO |
                                                          IN_CREATE | IN_DELETE),
                              strdup(dir),
                              kv_section);

    return retval;
}

/**
 * Removes an inotify watch and removes its watchnode from the list.
 *
 * You only need to pass a directory name into this function, it will find the
 * watchnode for you and remove it.
 *
 * It returns 1 on success, 0 on failure.
 */
int unwatch_dir(char *dir, int ifd)
{
    struct watchnode *node;

    for (node = g_watchnode; node->next; node = node->next)
        if (strcmp(node->next->path, dir) == 0)
            break;

    if (node->next)
    {
        if (verbose) log_write("Unwatching directory: %s\n", dir);
        inotify_rm_watch(ifd, node->next->wd);
        watchnode_free(node); /* pass in the item before the one we want to remove */

        return 1;
    }

    return 0;
}

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
            node = watch_dir(node, fd, path, child);
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
        if (!child->name) continue; /* skip comment nodes */
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

        /* create the node and advance the pointer */
        node = watch_dir(node, fd, wexp.we_wordv[0], child);

        if ((recurse = keyval_node_find(child, "recurse")))
            if (recurse->value && keyval_node_get_value_bool(recurse))
                recurse_add(node, fd, directory, child);
        
        wordfree(&wexp);
        free(directory);
    }
    return firstnode;
}

