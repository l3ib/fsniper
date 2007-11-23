/* keyvalcfg - a key->value config file parser (and writer.)
 * Copyright (C) 2006  Javeed Shaikh <syscrash2k@gmail.com>

 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA */

#ifndef _KEYVALCFG_H_
#define _KEYVALCFG_H_

/* a key->value config file parser (and writer.) see example.txt for an
 * example config file. */

struct keyval_node {
	/* the name of the key or section. can be NULL. */
	char * name;
	
	/* if this is a simple key-value pair, this contains the value. if this is
	 * a section or just a comment, the value is NULL. */
	char * value;
	
	/* any comment associated with this node. the whole node could be just
	 * a comment!*/
	char * comment;

	/* self explanatory, can be null if appropriate */
	struct keyval_node * children;
	struct keyval_node * head;
	struct keyval_node * next;
};

/* traverses the linked list pointed to by 'head' and frees it and all
 * children. */
void keyval_node_free_all(struct keyval_node * head);

/* parses a config file, the infamous parser func */
struct keyval_node * keyval_parse_file(const char * filename);

/* parses a string */
struct keyval_node * keyval_parse_string(const char * data);

/* writes the stuff out to a file. returns 1 on success, 0 on
 * failure. if filename is NULL, writes to stdout. */
unsigned char keyval_write(struct keyval_node * head, const char * filename);

/* gets the name of a node. */
char * keyval_node_get_name(struct keyval_node * node);

/* finds the first keyval_node with name 'name' in a linked list.
 * returns NULL on failure. note that this does not check children. */
struct keyval_node * keyval_node_find(struct keyval_node * head, char * name);

/* joins elements of a list of strings, separating them by newline characters. */
char * keyval_list_to_string(struct keyval_node * node);

enum keyval_value_type {
	KEYVAL_TYPE_NONE,
	KEYVAL_TYPE_BOOL,
	KEYVAL_TYPE_STRING,
	KEYVAL_TYPE_INT,
	KEYVAL_TYPE_DOUBLE,
	KEYVAL_TYPE_LIST
};

/* determines the value type of a node. */
enum keyval_value_type keyval_node_get_value_type(struct keyval_node * node);

/* convenience functions for getting values. figure it out. */
unsigned char keyval_node_get_value_bool(struct keyval_node * node);
char * keyval_node_get_value_string(struct keyval_node * node);
int keyval_node_get_value_int(struct keyval_node * node);
double keyval_node_get_value_double(struct keyval_node * node);

/* returns the error string or NULL if no error occurred. the internal error
 * condition will be reset once this is called. */
/* it's up to you to free the return value. */
char * keyval_get_error(void);

#endif
