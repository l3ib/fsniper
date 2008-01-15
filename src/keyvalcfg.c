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


#include "keyvalcfg.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef USE_EFENCE
#include <efence.h>
#endif

#define PICESIZE 512

#define DEBUG 0

static struct linked_list * llist_find_end(struct linked_list * list) {
	while (list->next) list = list->next;
	return list;
}

void llist_append(struct linked_list * first, struct linked_list * item) {
	item->first = first;
	first = llist_find_end(first);
	first->next = item;
	item->next = NULL;
}

void keyval_pair_free(struct keyval_pair * keyval) {
	free(keyval->key);
	free(keyval->value);
	free(keyval);
}

void keyval_pair_free_all(struct keyval_pair * keyval) {
	while (keyval) {
		struct keyval_pair * next = keyval->next;
		keyval_pair_free(keyval);
		keyval = next;
	}
}

void keyval_section_free_all(struct keyval_section * first) {
	while (first) {
		struct keyval_section * next = first->next;
		if (first->name) free(first->name);
		if (first->keyvals) keyval_pair_free_all(first->keyvals);
		if (first->children) keyval_section_free_all(first->children);
		free(first);
		first = next;
	}
}

/* writes 'section' and all its children to 'file' */
static void keyval_section_write(struct keyval_section * section,
	unsigned recursion, FILE * file) {
	
	struct keyval_pair * pair;
	unsigned count = recursion;
	struct keyval_section * children = section->children;
	char * recursion_tabs = malloc(recursion + 1); /* one tab per recurse */
	recursion_tabs[0] = '\0';

	while (count--) strncat(recursion_tabs, "\t", 1);

	if (section->name) {
		fprintf(file, "%s%s {\n", recursion_tabs + 1, section->name);
	}

	/* write out all of the key value pairs in this section */
	pair = section->keyvals;
	while (pair) {
		fprintf(file, "%s%s = %s;\n", recursion_tabs, pair->key, pair->value);
		pair = pair->next;
	}

	/* recurse (for child sections) */
	while (children) {
		keyval_section_write(children, recursion + 1, file);
		children = children->next;
	}

	if (section->name) fprintf(file, "%s}\n", recursion_tabs + 1);

	free(recursion_tabs);
}

unsigned char keyval_write(struct keyval_section * section,
	const char * filename) {

	FILE * file = stdout;
	if (filename && !(file = fopen(filename, "w"))) return 0;

	keyval_section_write(section, 0, file);
	if (filename) fclose(file);

	return 1;
}

/* returns 'data' + some offset (skips leading whitespace) */
static char * skip_leading_whitespace(char * data) {
	while (*data && ((*data == ' ') || (*data == '\t') || (*data == '\n'))) {
		data++;
	}

	return data;
}

/* returns the length of 'string' if trailing whitespace was to be
 * removed. */
static size_t skip_trailing_whitespace(char * string) {
	size_t len = strlen(string) - 1;

	while ((string[len] == ' ') || (string[len] == '\t')
		|| (string[len] == '\n')) len--;

	return len + 1;
}

/* returns a null-terminated string containing the first n characters of
 * 'source', with trailing whitespace removed. */
static char * sanitize_str(char * source, size_t n) {
	char * ret = malloc(n + 1);
	size_t len;

	/* copy the first n chars of source into ret */
	ret = strncpy(ret, source, n);
	ret[n] = '\0';

	/* remove trailing whitespace */
	len = skip_trailing_whitespace(ret);

	if (len != n) {
		ret = realloc(ret, len + 1);
		ret[len] = '\0';
	}

	return ret;
}

/* removes c-style comments from 'string'; replaces them with whitespace.
 * not the most elegant method of doing this (it modifies the argument it
 * receives.) */
/*static void strip_comments(char * string) {
	for (; *string; string++) {
		if (string[0] == '/' && string[1] == '*') {*/
				/* we're starting a comment */
/*				size_t pos = 0;
				while (string[pos] && !(string[pos] == '*' && string[pos + 1] == '/')) {
					string[pos] = ' ';
					pos++;
				}
				if (!string[pos]) return;
				string[pos] = ' ';
				string[pos + 1] = ' ';

				string += pos + 1;
		}
	}
}*/

/* removes # comments from 'string' and replaces them with whitespace. */
static void strip_comments(char * string) {
	for (; *string; string++) {
		if (*string == '#') {
			/* we've found a comment. it should last until the end of the line. */
			for (; *string && (*string != '\n'); string++) {
				*string = ' ';
			}
		}
	}
}

/* the parser. probably full of bugs. ph34r. */
static struct keyval_section * keyval_parse_section(char * _data,
	size_t * bytes_read) {

	char * data = _data;
	struct keyval_section * section = malloc(sizeof(struct keyval_section));
	section->children = section->next = NULL;
	section->first = section;
	section->keyvals = NULL;
	section->name = NULL;

	/* get rid of comments in the data */
	strip_comments(data);

	while (*data) {
		size_t count = 0;
		char * key;
		char * value;

		/* skip leading whitespace */
		data = skip_leading_whitespace(data);

		if (*data == '}') {
			/* we're done parsing this section. */
			data++;
			break;
		}

		/* read up to '=' or '{', this is our key */
		while (data[count] && ((data[count] != '=') && (data[count] != '{')))
			count++;

		if (!data[count]) {
			if (bytes_read) *bytes_read += data - _data + count;
			return section;
		}

		/* count now contains the length of the key including trailing
		 * whitespace. store the key. */
		key = sanitize_str(data, count);

		data += count + 1;

		if (data[-1] == '{') {
			/* key was really a section name. parse it. */
			struct keyval_section * child;
			count = 0;

			child = keyval_parse_section (data, &count);
			data += count + 1;

			child->name = key;
			if (section->children) {
				/* append to the existing list */
				llist_append((struct linked_list*)section->children,
					(struct linked_list*)child);
			} else {
				/* start a new list */
				child->first = child;
				child->next = NULL;
				section->children = child;
			}
		} else {
			struct keyval_pair * pair;

			/* key has a value. store it. */

			/* skip any whitespace leading up to the value. */
			data = skip_leading_whitespace(data);

			/* read until a newline character is encountered. */
			for (count = 0; data[count] && (data[count] != '\n'); count++);

			/* count contains the length of the value including trailing
			 * whitespace. */
			value = sanitize_str(data, count);
			
			data += count + 1;

			/* throw key and value into a keyval pair */
			pair = malloc(sizeof(struct keyval_pair));
			pair->key = key;
			pair->value = value;

			/* append the pair to the current list of pairs if possible */
			if (section->keyvals) {
				llist_append((struct linked_list*)section->keyvals,
					(struct linked_list*)pair);
			} else {
				pair->next = NULL;
				pair->first = pair;
				section->keyvals = pair;
			}
		}
	}

	if (bytes_read) *bytes_read += data - _data;
	return section;
}

struct keyval_section * keyval_parse(const char * filename) {
	char buf[PICESIZE];
	FILE * file = fopen(filename, "r");
	char * data = malloc(PICESIZE);

	struct keyval_section * section;

	data[0] = '\0';
	if (!file) return NULL;

	while(fgets(buf, PICESIZE, file)) {
		data = realloc(data, strlen(data) + strlen(buf) + 1);
		data = strcat(data, buf);
	}
	fclose(file);

	section = keyval_parse_section(data, NULL);
	free(data);

	return section;
}

struct keyval_pair * keyval_pair_find(struct keyval_pair * first,
	char * key) {

	for (; first; first = first->next) {
		if (strcmp(first->key, key) == 0) return first;
	}

	return NULL; /* no match */
}

struct keyval_section * keyval_section_find(struct keyval_section * first,
	char * name) {

	for (; first; first = first->next) {
		if (strcmp(first->name, name) == 0) return first;
	}

	return NULL; /* no match */
}

/* the convenience functions for getting values and shizer. */
unsigned char keyval_pair_get_value_bool(struct keyval_pair * pair) {
	switch (*pair->value) {
		case 't':
		case 'T':
		case 'y':
		case 'Y':
		case '1':
			return 1;
		default:
			return 0;
	}
}

char * keyval_pair_get_value_string(struct keyval_pair * pair) {
	return strdup(pair->value);
}

int keyval_pair_get_value_int(struct keyval_pair * pair) {
	return atoi(pair->value);
}

double keyval_pair_get_value_double(struct keyval_pair * pair) {
	return strtod(pair->value, NULL);
}
