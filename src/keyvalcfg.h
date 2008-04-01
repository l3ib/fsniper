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

/* used to generalize linked list functions. beware. tricks. */
struct linked_list {
    struct linked_list * first;
    struct linked_list * next;
};

/* a linked list of key->value stuff. */

struct keyval_pair {
    struct keyval_pair * first;
    struct keyval_pair * next;

    char * key;
    char * value;
};

/* a section of key->value pairs. the main section (the whole damn file)
 * has name = NULL. yes, i know you love nested sections. */
struct keyval_section {
    /* this is a linked list too. why? because you can have the same section
     * specified 100 times. each time with different values. blame that
     * bastard yates. this is used only for children. */
    struct keyval_section * first;
    struct keyval_section * next;

    char * name;
    struct keyval_pair * keyvals; /* the key->value pairs in this section. */

    /* child sections */
    struct keyval_section * children;
};

/* appends a new key->value pair to 'first'. returns the new pair. */
void llist_append(struct linked_list * first, struct linked_list * item);

/* traverses the linked list pointed to by 'first' and frees everything,
 * including key->value pairs and child sections. */
void keyval_section_free_all(struct keyval_section * first);

/* frees a keyval structure and contents. */
void keyval_pair_free(struct keyval_pair * keyval);

/* frees all keyval structures and contents (treating 'keyval' as the first
 * in the linked list. */
void keyval_pair_free_all(struct keyval_pair * keyval);

/* finds the keyval_pair with key 'key'. returns NULL on failure. */
struct keyval_pair * keyval_pair_find(struct keyval_pair * first,
                                      char * key);

/* finds the keyval_section with name 'name', returns NULL on failure. */
struct keyval_section * keyval_section_find(struct keyval_section * first,
                                            char * name);

/* convenience functions for getting values. figure it out. */
unsigned char keyval_pair_get_value_bool(struct keyval_pair * pair);
char * keyval_pair_get_value_string(struct keyval_pair * pair);
int keyval_pair_get_value_int(struct keyval_pair * pair);
double keyval_pair_get_value_double(struct keyval_pair * pair);

/* parses a config file, the infamous parser func */
struct keyval_section * keyval_parse(const char * filename);

/* writes the section data out to a file. returns 1 on success, 0 on
 * failure. if filename is NULL, writes to stdout. */
unsigned char keyval_write(struct keyval_section * section,
                           const char * filename);

#endif
