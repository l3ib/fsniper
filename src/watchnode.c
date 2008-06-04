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

/**
 * Creates a new watchnode at node->next and populates it with passed parameters.
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

