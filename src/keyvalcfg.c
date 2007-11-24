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
#include <ctype.h>

#define PICESIZE 512

#define IS_SPACE(c) (((c) == ' ') || ((c) == '\t') || ((c) == '\n'))
#define IS_END_KEY(c) (((c) == '\0') || ((c) == '{') || ((c) == '=') || ((c) == '}') || ((c) == '#'))
#define IS_END_VALUE(c) (((c) == '\0') || ((c) == '}') || ((c) == '\n') || ((c) == '#'))
#define IS_END_LIST(c) (((c) == '\0') || ((c) == ']') || ((c) == ','))

/* error string of craziness */
static char * error = NULL;

static void keyval_append_error(char * e) {
	if (error) {
		error = realloc(error, strlen(error) + strlen(e) + 1);
		strcat(error, e);
	} else {
		error = strdup(e);
	}
}

char * keyval_get_error(void) {
	char * ret = error;
	error = NULL;
	
	return ret;
}

char * keyval_node_get_name(struct keyval_node * node) {
	if (!node->name) return NULL;
	return strdup(node->name);
}

char * keyval_node_get_comment(struct keyval_node * node) {
	if (!node->comment) return NULL;
	return strdup(node->comment);
}

struct keyval_node * keyval_node_get_children(struct keyval_node * node) {
	return node->children;
}

struct keyval_node * keyval_node_get_next(struct keyval_node * node) {
	return node->next;
}

void keyval_node_free_all(struct keyval_node * node) {
	/* free will do nothing if these are null */
	free(node->name);
	free(node->value);
	free(node->comment);
	
	if (node->children) keyval_node_free_all(node->children);
	if (node->next) keyval_node_free_all(node->next);
	
	free(node);
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

/* takes a node whose value contains newlines and sanitizes it into a list
 * structure. */
void keyval_node_clear_newlines(struct keyval_node * node) {
	char * pos = NULL;
	char * cur = node->value;
	struct keyval_node * children = NULL;

	if (!cur) return;

	while ((pos = strchr(cur, '\n'))) {
		struct keyval_node * element = malloc(sizeof(struct keyval_node));
		element->name = element->comment = NULL;

		element->head = children ? children->head : element;
		element->next = element->children = NULL;

		element->value = sanitize_str(cur, pos - cur);
		cur = pos + 1;

		if (children) children->next = element;
		children = element;
	}
	
	if (cur != node->value) {
		/* we have yet to handle the part after the last newline. */
		struct keyval_node * element = malloc(sizeof(struct keyval_node));
		element->name = element->comment = NULL;

		element->head = children ? children->head : element;
		element->next = element->children = NULL;

		element->value = sanitize_str(cur, strlen(cur));
		children->next = element;

		free(node->value);
		node->value = NULL;
		node->children = children->head;
	}
}

void keyval_node_write(struct keyval_node * node, size_t depth, FILE * file) {
	char * tabs = "";

	if (depth) {
		size_t count = depth;
		tabs = calloc(depth, sizeof(char));
		while (--count) strncat(tabs, "\t", 1);
	}
	
	/*printf("(%d)", keyval_node_get_value_type(node));*/
	
	keyval_node_clear_newlines(node);
	
	if (keyval_node_get_value_type(node) == KEYVAL_TYPE_LIST) {
		/* yay, a list! */
		struct keyval_node * child;
		fprintf(file, "%s%s = [", tabs, node->name);

		for (child = node->children; child; child = child->next) {
			fprintf(file, "%s", child->value);
			if (child->next) fprintf(file, ", ");
			else fprintf(file, "]\n");
		}
	} else if (node->children) {
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

unsigned char keyval_write(struct keyval_node * head, const char * filename) {
	FILE * out;
	
	out = fopen(filename, "w");
	if (!out) return 0; /* error */
	
	keyval_node_write(head, 0, out);
	fclose(out);

	return 1;
}

/* returns 'data' + some offset (skips leading whitespace) */
char * skip_leading_whitespace(char * data, size_t * lines) {
	while (*data && IS_SPACE(*data)) {
		if (lines && (*data == '\n')) (*lines)++;
		data++;
	}

	return data;
}

/* returns 'data' + some offset (skips leading whitespace)
 * this version stops at the end of line. */
char * skip_leading_whitespace_line(char * data) {
	while (*data && IS_SPACE(*data)) {
		if (*data == '\n') {
			break;
		}
		data++;
	}

	return data;
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
			if (*string == '\n') seen_space = 2;
			else if (!seen_space) seen_space = 1;
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

struct keyval_node * keyval_node_get_value_list(struct keyval_node * node) {
	struct keyval_node * list = NULL;
	struct keyval_node * last = list;

	char * value;

	if (keyval_node_get_value_type(node) != KEYVAL_TYPE_LIST) return NULL;
	
	value = node->value + 1;

	while (*value != ']') {
		struct keyval_node * element;
		size_t count = 0;

		value = skip_leading_whitespace(value, NULL);
		if (*value == ',') {
			value++;
			continue;
		}

		while (!IS_END_LIST(value[count])) count++;
		
		/* count now contains the length of this list element. */
		element = malloc(sizeof(struct keyval_node));
		element->value = sanitize_str(value, count);
		
		/* make sure element has all irrelevant fields set to null */
		element->name = element->comment = NULL;
		element->children = element->next = NULL;
		
		if (last) {
			element->head = last;
			last->next = element;
		} else element->head = list = element;
		last = element;
		
		value += count;
	}
	
	return list;
}


/* the parser. probably full of bugs. ph34r.
 * stops if it encounters } or end of string. */
struct keyval_node * keyval_parse_node(char ** _data, char * sec_name, size_t * l) {
	struct keyval_node * head = malloc(sizeof(struct keyval_node));
	struct keyval_node * child = NULL; /* last child found */

	char * data = *_data;
	size_t line = l ? *l : 1;

	head->head = head;
	head->name = head->value = head->comment = NULL;
	head->children = head->next = NULL;

	while (*data) {
		size_t count = 0;
		struct keyval_node * node;
		char * name = NULL;
		char type_key, type_value = 0;

		data = skip_leading_whitespace(data, &line);
		
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
		/* is this node just a key-value node or is it a section? */
		if (type_key == '=' || type_key == '#') {
			/* it's a key-value node. */
			data = skip_leading_whitespace_line(data + count + 1);

			/* the value is all characters until some...*/

			if ((type_key != '#') && ((*data == '\0') || (*data == '\n') || (*data == '#'))) {
				/* malformed... expected a value, got end of line */
				/* we want something like
				 * keyvalcfg: error: expected a value after key = in section section
				 */
				char * e;
				size_t len = strlen(name) +
				             strlen("keyvalcfg: error: expected a value after ` =`\n") +
				             strlen("keyvalcfg: (near line )\n") + 5 /* XXX: FIXED LINE NUMBER LENGTH */
				             + 1;
				if (sec_name) len += strlen(" in section") + strlen(sec_name);
				e = malloc(len);
				if (sec_name) {
					snprintf(e, len, "keyvalcfg: error: expected a value after `%s =` in section `%s`\nkeyvalcfg: (near line %d)\n", name, sec_name, line);
				} else {
					snprintf(e, len, "keyvalcfg: error: expected a value after `%s =`\nkeyvalcfg: (near line %d)\n", name, line);
				}
				keyval_append_error(e);
				free(e);
			}

			count = 0;
			/* how many characters occur until end of line? */
			if (type_key != '#') while (1) {
				if (IS_END_VALUE(data[count])) {
					/*printf("%c fails\n", data[count]);*/
					/* XXX: check this */
					if (data[count] == '\n') line++;
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

			if (keyval_node_get_value_type(node) == KEYVAL_TYPE_LIST) {
				node->children = keyval_node_get_value_list(node);
				free(node->value);
				node->value = NULL;
			}

			data += count;

			if (type_key == '#' || type_value == '#') {
				data = skip_leading_whitespace(data + ((type_value == '#') ? 1 : 0), &line);
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
			/* let us track error hierarchy. what's the current error state? */
			char * e = error;
			size_t e_len = error ? strlen(error) : 0;
			line += 1;
			node = keyval_parse_node(&d, name, &line);
			if (sec_name && ((error != e) || ((error) && (strlen(error) != e_len)))) {
				/* something went wrong in there. */
				e = malloc(strlen(sec_name ? sec_name : "(none)") +
				           strlen("keyvalcfg: (in section ``)\n") + 1);
				sprintf(e, "keyvalcfg: (in section `%s`)\n", sec_name ? sec_name : "(none)");
				keyval_append_error(e);
				free(e);
			}
			if (d[-1] != '}') {
				/* something went wrong. malformed config file. */
				/* ERROR */
				char * e = malloc(strlen(name) +
				                  strlen("keyvalcfg: error: section `` never closed (missing `}`)")
				                  + 2);
				sprintf(e, "keyvalcfg: error: section `%s` never closed (missing `}`)\n", name);
				keyval_append_error(e);
				free(e);
			}
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
	if (l) *l = line;
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
	head = keyval_parse_node(&data2, NULL, NULL);
	
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

struct keyval_node * keyval_node_find(struct keyval_node * head, char * name) {
	for (; head; head = head->next) {
		if (!head->name) continue;
		if (!strcmp(head->name, name)) return head;
	}
	
	return NULL;
}

unsigned char keyval_node_get_value_bool(struct keyval_node * node) {
	if (!node->value) return 0;

	switch (*node->value) {
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

char * keyval_node_get_value_string(struct keyval_node * node) {
	return node->value ? strdup(node->value) : NULL;
}

int keyval_node_get_value_int(struct keyval_node * node) {
	return node->value ? atoi(node->value) : 0;
}

double keyval_node_get_value_double(struct keyval_node * node) {
	return node->value ? strtod(node->value, NULL) : 0.0;
}

unsigned char keyval_node_has_list_value(struct keyval_node * node) {
	struct keyval_node * child;
	
	/* a node is a list if:
	 * it has no value,
	 * it has children,
	 * its children do not have children,
	 * its children do not have names.
	 */
	
	if (node->value) {
		if (node->value && (*node->value == '[')) {
			size_t len = strlen(node->value);
			if (node->value[len - 1] == ']') return 1;
		}
	} else {
		unsigned result = 1;
		if (!node->children) return 0;

		for (child = node->children; child; child = child->next) {
			if (child->name || child->children) result = 0;
		}

		return result;
	}
	
	return 0;
}

enum keyval_value_type keyval_node_get_value_type(struct keyval_node * node) {
	enum keyval_value_type type = KEYVAL_TYPE_INT;
	char * s;
	
	if (keyval_node_has_list_value(node)) return KEYVAL_TYPE_LIST;
	if (!node->value) return KEYVAL_TYPE_NONE;
	
	if (strlen(node->value) == 1) {
		switch (*node->value) {
			case 't':
			case 'T':
			case 'y':
			case 'Y':
			case 'f':
			case 'F':
			case 'n':
			case 'N':
				return KEYVAL_TYPE_BOOL;
			default: break;
		}
	}
	
	if (strcasecmp(node->value, "true") == 0) return KEYVAL_TYPE_BOOL;
	if (strcasecmp(node->value, "false") == 0) return KEYVAL_TYPE_BOOL;
	if (strcasecmp(node->value, "yes") == 0) return KEYVAL_TYPE_BOOL;
	if (strcasecmp(node->value, "no") == 0) return KEYVAL_TYPE_BOOL;

	for (s = node->value; *s; s++) {
		if (!isdigit(*s)) {
			/* hmm, a non-digit. if it's a '.', we could still be a decimal number. */
			if (*s == '.') {
				/* two dots encountered */
				if (type == KEYVAL_TYPE_DOUBLE) return KEYVAL_TYPE_STRING;
				type = KEYVAL_TYPE_DOUBLE; /* first dot */
			} else return KEYVAL_TYPE_STRING;
		}
	}
	
	return type;
}

char * keyval_list_to_string(struct keyval_node * node) {
	struct keyval_node * element;
	size_t len = 0;
	char * string;
	
	if (keyval_node_get_value_type(node) != KEYVAL_TYPE_LIST) return NULL;
	
	/* pass one: count length */
	for (element = node->children; element; element = element->next) {
		len += strlen(element->value) + 1;
	}
	
	/* len really is one more than the wanted length, but that turns out to be
	 * fine since we need space for the trailing newline */
	string = calloc(len, sizeof(char));
	
	/* pass two: concatenate! */
	for (element = node->children; element; element = element->next) {
		strcat(string, element->value);
		if (element->next) strcat(string, "\n");
	}

	return string;
}
