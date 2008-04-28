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
extern struct watchnode *node;
extern int verbose;

/* recursively add watches */
void recurse_add(int fd, char *directory)
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
            node->wd = inotify_add_watch(fd, path, IN_CLOSE_WRITE | IN_MOVED_TO);
            node->path = strdup(path);
            node->next = malloc(sizeof(struct watchnode));
            node->next->next = NULL;
            node = node->next;
            recurse_add(fd, path);
        }
        free(path);
    }
}

/* parses the config and adds inotify watches to the fd */
struct watchnode* add_watches(int fd)
{
    struct keyval_node* child;
    struct keyval_node* startchild = NULL;
    struct keyval_node* recurse;
    char* directory;
    wordexp_t wexp;
    struct watchnode* firstnode;
    node = malloc(sizeof(struct watchnode));
    node->next = NULL;
    firstnode = node;

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
        if (verbose) log_write("Watching directory: %s\n", directory);
        node->wd = inotify_add_watch(fd, directory, IN_CLOSE_WRITE | IN_MOVED_TO);
        node->path = strdup(wexp.we_wordv[0]);
        node->section = child;
        node->next = malloc(sizeof(struct watchnode));
        node->next->path = NULL;
        node->next->section = NULL;
        node->next->next= NULL;
        node = node->next;
        if ((recurse = keyval_node_find(child, "recurse")))
            if (recurse->value && keyval_node_get_value_bool(recurse))
                recurse_add(fd, directory);
        wordfree(&wexp);
        free(directory);
    }
    return firstnode;
}
