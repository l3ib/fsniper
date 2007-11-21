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

#define PICESIZE 512

#define DEBUG 0

#define IS_SPACE(c) (((c) == ' ') || ((c) == '\t') || ((c) == '\n'))
#define IS_END_KEY(c) (((c) == '\0') || ((c) == '{') || ((c) == '=') || ((c) == '}') || ((c) == '#'))
#define IS_END_VALUE(c) (((c) == '\0') || ((c) == '}') || ((c) == '\n') || ((c) == '#'))

void keyval_node_free_all(struct keyval_node * node) {
	/* free will do nothing if these are null */
	free(node->name);
	free(node->value);
	free(node->comment);
	
	if (node->children) keyval_node_free_all(node->children);
	if (node->next) keyval_node_free_all(node->next);
	
	free(node);
}

void keyval_node_write(struct keyval_node * node, size_t depth, FILE * file) {
	char * tabs = "";

	if (depth) {
		size_t count = depth;
		tabs = calloc(depth, sizeof(char));
		while (--count) strncat(tabs, "\t", 1);
	}
	
	if (node->children) {
		struct keyval_node * child = node->children;
		if (node->name) fprintf(file, "%s%s {\n", tabs, node->name);

		while (child) {
			keyval_node_write(child, depth + 1, file);
			child = child->next;
		}
		if (node->name) fprintf(file, "%s}\n", tabs);
	} else {
		if (node->name) {
			fprintf(file, "%s%s = %s", tabs, node->name, node->value);
			if (node->comment) fprintf(file, " # %s", node->comment);
		} else if (node->comment) fprintf(file, "%s# %s\n", tabs, node->comment);
		fputc('\n', file);
	}

	if (depth) free(tabs);
}

/* returns 'data' + some offset (skips leading whitespace) */
char * skip_leading_whitespace(char * data) {
	while (*data && IS_SPACE(*data)) {
		data++;
	}

	return data;
}

/* returns the length of 'string' if trailing whitespace was to be
 * removed. */
size_t skip_trailing_whitespace(char * string) {
	size_t len;
	
	if (!*string) return 0;
	
	len = strlen(string) - 1;

	while (IS_SPACE(string[len])) {
		len--;
	}

	return len + 1;
}

/* returns a null-terminated string containing the first n characters of
 * 'source', with trailing whitespace removed. */
char * sanitize_str(char * source, size_t n) {
	char * ret;
	size_t len;
	
	if (n == 0) return 0;
	
	ret = malloc(n + 1);

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

/* removes # comments from 'string' and replaces them with whitespace. */
void strip_comments(char * string) {
	for (; *string; string++) {
		if (*string == '#') {
			/* we've found a comment. it should last until the end of the line. */
			for (; *string != '\n'; string++) {
				if (*string == '\0') return;
				*string = ' ';
			}
		}
	}
}

/* collapses multi-line statements into one line. the '\' character is used to
 * indicate that a statement continues onto the next line. */
void collapse(char * string) {
	for (; *string; string++) {
		if (*string == '\\') {
			/* the dreaded \ has been encountered. first change this character into
			 * a space. then change the end-of-line character into a space if there
			 * is no 'data' between this \ and the end of the line. */
			char * pos = string++;
			
			for (; *string; string++) {
				if (*string == '\n') {
					*string = ' ';
					/* also change the \ into a space */
					*pos = ' ';
				} else if (*string != '\t' && *string != ' ') {
					/* we have found a non-space character between the \ and the
					 * end-of-line. abort mission. */
					 break;
				}
			}
		}
	}
}

/* transforms sequences of more than one space into a single space. returns a
 * new string which must be freed. */
char * strip_multiple_spaces(char * string) {
	size_t len = 0;
	char * result = malloc(sizeof(char) * (strlen(string) + 1));
	unsigned char seen_space = 0;

	for (;;) {
		if (IS_SPACE(*string)) {
			if (!seen_space) seen_space = 1 + (*string == '\n');
		} else {
			if (seen_space) {
				result[len++] = (seen_space == 1) ? ' ' : '\n';
				seen_space = 0;
			}

			result[len++] = *string;
		}

		if (*string == '\0') {
			len--;
			break;
		}
		string++;
	}

	result[len] = '\0';

	return realloc(result, len + 1);
}

/* the parser. probably full of bugs. ph34r.
 * stops if it encounters } or end of string. */
struct keyval_node * keyval_parse_node(char ** _data) {
	struct keyval_node * head = malloc(sizeof(struct keyval_node));
	struct keyval_node * child = NULL; /* last child found */

	char * data = *_data;

	head->head = head;
	head->name = head->value = head->comment = NULL;
	head->children = head->next = NULL;

	while (*data) {
		size_t count = 0;
		struct keyval_node * node;
		char * name = NULL;
		char type_key, type_value = 0;

		data = skip_leading_whitespace(data);
		
		/* obscure bug lying in wait. i had it as data[count++] but the macro
		 * expansion pwnt it. don't make the same mistake! */
		while (1) {
			if (IS_END_KEY(data[count])) {
				break;
			}
			count++;
		}
		
		type_key = data[count];

		if (type_key == '}') {
			data += count + 1;
			break;
		}

		/* count now contains the length of the key + trailing whitespace... with
		 * some offset */
		if (type_key != '#') name = sanitize_str(data, count);
		/* is this node just a key-value pair or is it a section? */
		if (type_key == '=' || type_key == '#') {
			/* it's a key-value pair. */
			data = skip_leading_whitespace(data + count + 1);

			/* the value is all characters until some...*/

			if ((*data == '\0') || (*data == '\n')) {
				/* malformed... expected a value, got end of line */
			}

			count = 0;
			/* how many characters occur until end of line? */
			if (type_key != '#') while (1) {
				if (IS_END_VALUE(data[count])) {
					/*printf("%c fails\n", data[count]);*/
					if (data[count - 1] != '\\') {
						type_value = data[count];
						break; /* stop unless literal */
					}
				} else {
					/*printf("%c passes\n", data[count]);*/
				}
				count++;
			}

			node = malloc(sizeof(struct keyval_node));
			node->value = count ? sanitize_str(data, count) : NULL;
			node->next = node->children = NULL;

			data += count;

			if (type_key == '#' || type_value == '#') {
				data = skip_leading_whitespace(data + ((type_value == '#') ? 1 : 0));
				count = 0;
				/* there's a comment to be made here. */
				while (1) {
					if (data[count] == '\n') break;
					count++;
				}

				node->comment = sanitize_str(data, count);
				data += count;
			} else node->comment = NULL;

			if (*data && (*data != '}')) data++;

		} else if (data[count] == '{') {
			/* it's a section. */
			char * d = data + count + 1;
			node = keyval_parse_node(&d);
			data = d;
		} else {
			/* stop. */
			break;
		}

		node->name = name;

		if (child) {
			child->next = node;
		} else {
			head->children = node;
		}

		child = node;
	}

	*_data = data;
	return head;
}

struct keyval_node * keyval_parse_string(const char * data) {
	struct keyval_node * head;
	char * data2 = strdup(data);
	char * data3;
	
	/* preprocessing */
	collapse(data2);
	data3 = strip_multiple_spaces(data2);
	free(data2);

	/* to make sure that keyval_parse_node doesn't mess up our pointer to data3 */
	data2 = data3;
	head = keyval_parse_node(&data2);
	
	free(data3);

	return head;
}

struct keyval_node * keyval_parse_file(const char * filename) {
	char buf[PICESIZE];
	FILE * file = fopen(filename, "r");
	char * data = malloc(PICESIZE);

	struct keyval_node * node;

	data[0] = '\0';
	if (!file) return NULL;

	while(fgets(buf, PICESIZE, file)) {
		data = realloc(data, strlen(data) + strlen(buf) + 1);
		data = strcat(data, buf);
	}
	fclose(file);

	node = keyval_parse_string(data);
	free(data);

	return node;
}
#if 0
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
#endif
