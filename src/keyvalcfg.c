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
#include <stdarg.h>

#define PICESIZE 512

#define IS_SPACE(c) (((c) == ' ') || ((c) == '\t') || ((c) == '\n'))
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

static void keyval_append_error_va(const char * format, ...) {
	char * e = malloc(100 * sizeof(char));
	size_t r;

	va_list ap;
	va_start(ap, format);
	
	r = vsnprintf(e, 100, format, ap);
	e = realloc(e, r + 1);
	if (r > 99) {
		/* we need to do this again */
		r = vsnprintf(e, r + 1, format, ap);
	}
	va_end(ap);
	keyval_append_error(e);
	free(e);
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

struct keyval_node * keyval_node_find(struct keyval_node * head, char * name) {
	return keyval_node_find_next(head->children, name);
}

struct keyval_node * keyval_node_find_next(struct keyval_node * node,
                                           char * name) {
	for (; node; node = node->next) {
		if (!node->name) continue;
		if (!strcmp(node->name, name)) return node;
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

static unsigned char keyval_node_has_list_value(struct keyval_node * node) {
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
