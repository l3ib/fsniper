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
#include "watchnode.h"
#include <stdio.h>

/**
 * Creates a new watchnode at node->next and populates it with passed parameters.
 *
 * It is expected that path be a duplicated string. watchnode_free will attempt to free
 * it.
 *
 * Returns this newly created watchnode.
 */
struct watchnode* watchnode_create(struct watchnode* node, int wd, char* path, struct keyval_node* section)
{
    node->next = malloc(sizeof(struct watchnode));
    node->next->wd = wd;
    node->next->path = path;
    node->next->section = section;
    node->next->next= NULL;

    return node->next;
}

/**
 * Frees the passed node's next node.  Sets the passed node's next to point to next->next.
 *
 * Always remember to pass in the node BEFORE the node you want to remove.
 */
void watchnode_free(struct watchnode* prevnode)
{
    struct watchnode *delnode;

    if (!prevnode->next)
    {
        fprintf(stderr, "watchnode_free(): received an invalid parameter.\n");
        return;
    }

    delnode = prevnode->next;
    
    /* remove from linked list */    
    prevnode->next = delnode->next;

    /* free delnode */
    free(delnode->path);
    free(delnode);
}

