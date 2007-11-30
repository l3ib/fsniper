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

/* returns a null-terminated string containing the first n characters of
 * 'source', with trailing whitespace removed.
 * this version skips the '\' character in a clever way. */
char * sanitize_str_unescape(char * source, size_t n) {
	char * ret;
	size_t len;
	size_t i, j;
	
	if (n == 0) return 0;
	
	ret = calloc(n + 1, sizeof(char));

	/* copy the first n chars of source into ret */

	for (i = 0, j = 0; i < n; i++, j++) {
		if (source[i] == '\\') {
			/* bump i up by 1 */
			i++;
		}
		ret[j] = source[i];
	}

	/* remove trailing whitespace */
	len = skip_trailing_whitespace(ret);

	if (len != n) {
		ret = realloc(ret, len + 1);
		ret[len] = '\0';
	}

	return ret;
}

static char * escape(char * string) {
	/* allocate a buffer twice the size of the original string, plus one for the
	 * null */
	char * result;
	size_t i, j, len;

	if (!string) return NULL;

	len = strlen(string);
	result = calloc(1 + 2*len, sizeof(char));

	for (i = 0, j = 0; i < len; i++, j++) {
		switch (string[i]) {
			case '\\':
			case '#':
			case '}':
			case '\n':
			case ',':
			case ']':
			case '[':
				result[j++] = '\\';
			default:
				result[j] = string[i];
				break;
		}
	}

	return realloc(result, 1+strlen(result));
}

void keyval_node_write(struct keyval_node * node, size_t depth, FILE * file) {
	char * tabs = "";

	if (depth) {
		size_t count = depth;
		tabs = calloc(depth, sizeof(char));
		while (--count) strncat(tabs, "\t", 1);
	}
	
	if (keyval_node_get_value_type(node) == KEYVAL_TYPE_LIST) {
		/* yay, a list! */
		struct keyval_node * child;
		fprintf(file, "%s%s = [", tabs, node->name);

		for (child = node->children; child; child = child->next) {
			char * value = escape(child->value);
			fprintf(file, "%s", value);
			free(value);
			if (child->next) fprintf(file, ", ");
			if (child->comment) {
				fprintf(file, " # %s\n", child->comment);
			}
			if (!child->next) fprintf(file, "]");
		}

		if (node->comment) fprintf(file, " # %s\n", node->comment);
		else fprintf(file, "\n");
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
			char * value = escape(node->value);
			fprintf(file, "%s%s = %s", tabs, node->name, value);
			free(value);
			if (node->comment) fprintf(file, " # %s", node->comment);
		} else if (node->comment) fprintf(file, "%s# %s\n", tabs, node->comment);
		fputc('\n', file);
	}

	if (depth) free(tabs);
}

void keyval_node_write_debug(struct keyval_node * node, size_t depth) {
	char * tabs = "";

	if (depth) {
		size_t count = depth;
		tabs = calloc(depth, sizeof(char));
		while (--count) strncat(tabs, "\t", 1);
	}
	
	if (node->name) printf("%sname: `%s`\n", tabs, node->name);
	if (node->value) printf("%svalue: `%s`\n", tabs, node->value);
	
	if (node->children) keyval_node_write_debug(node->children, depth + 1);
	if (node->next) keyval_node_write_debug(node->next, depth);

	if (depth) free(tabs);
}

unsigned char keyval_write(struct keyval_node * head, const char * filename) {
	FILE * out = stdout;
	
	if (filename) {
		out = fopen(filename, "w");
		if (!out) return 0; /* error */
	}
	
	keyval_node_write(head, 0, out);
	if (filename) fclose(out);

	return 1;
}

/* returns 'data' + some offset (skips leading whitespace) */
char * skip_leading_whitespace(char * data, size_t * lines) {
	if (!data) return NULL;
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

/* transforms sequences of more than one space into a single space. returns a
 * new string which must be freed. */
char * strip_multiple_spaces(char * string) {
	size_t len = 0;
	char * result;
	unsigned char seen_space = 0;

	if (!string) return NULL;
	result = malloc(sizeof(char) * (strlen(string) + 1));

	for (;;) {
		if (IS_SPACE(*string)) {
			if (*string == '\n') seen_space = 2;
			else if (!seen_space) seen_space = 1;
		} else {
			if (seen_space) {
				result[len++] = (seen_space == 2) ? '\n' : ' ';
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

struct keyval_node * keyval_parse_list(char ** _data, size_t * l) {
	struct keyval_node * last = NULL;

	size_t line = *l;
	char * data;

	size_t count = 0;

	data = *_data;

	while (*data) {
		unsigned char abort = 0;
		struct keyval_node * cur = NULL;
		char * value;
		data = skip_leading_whitespace_line(data);
		count = 0;

		while (!abort) {
			switch (data[count]) {
				case '\\':
					count++;
					break;
				case '#':
					if (last) {
						unsigned char abort_comment = 0;
						size_t count_comment = 0;
						size_t i;
						/* read in the comment */
						char * data_comment = skip_leading_whitespace_line(data + count + 1);
						while (!abort_comment) {
							switch (data_comment[count_comment]) {
								case '\0':
									goto abort;
								case '\n':
									line++;
									last->comment = sanitize_str(data_comment, count_comment);
									/* turn all comment data into spaces. is this really
									 * necessary? */
									for (i = count; i < count + count_comment + data_comment - data; i++) {
										data[i] = ' ';
									}
									abort_comment = 1;
									break;
							}
							if (!abort_comment) count_comment++;
						}
					}
					break;
				case '\0':
					abort:
					if (last) keyval_node_free_all(last->head);
					return NULL;
				case ',':
				case ']':
					cur = calloc(1, sizeof(struct keyval_node));
					value = sanitize_str_unescape(data, count);
					cur->value = strip_multiple_spaces(skip_leading_whitespace(value,
					                                   NULL));
					free(value);
					/*printf("->%s\n", cur->value);*/
					if (last) {
						last->next = cur;
						cur->head = last->head;
					} else cur->head = cur;

					last = cur;

					if (data[count] == ']') {
						data += count + 1;
						goto leave;
					}
					data += count + 1;
					abort = 1;
					break;
				default:
					break;
			}
			if (!abort) count++;
		}
	}

	leave:
	*_data = data;
	*l = line;
	return last->head;
}

/* the parser. probably full of bugs. ph34r.
 * stops if it encounters } or end of string.
 * returns NULL on failure. */
struct keyval_node * keyval_parse_node(char ** _data, char * sec_name, size_t * l) {
	struct keyval_node * head = calloc(1, sizeof(struct keyval_node));
	struct keyval_node * child = NULL; /* last child found */

	char * data = *_data;
	size_t line = l ? *l : 1;

	head->head = head;

	while (*data) {
		char * name = NULL;
		char * comment = NULL;
		char * value = NULL;

		char type_key;
		size_t count = 0;
		unsigned char abort = 0;
		
		unsigned char abort_comment;
		size_t count_comment = 0;
		char * data_comment;
		
		struct keyval_node * node = NULL;

		data = skip_leading_whitespace(data, &line);

		/* the key lasts until = or {
		 * error out if encountered un-escaped '}'
		 * escaping characters works like this: if you encounter a \, just skip the
		 * next character [and turn the \ into a space.]
		 */
		while (!abort) {
			switch (data[count]) {
				case '\\':
					count++; /* skip the next character */
					break;
				case '#':
					/* the rest of the line is a comment... */
					/* read it in... */
					/* COMMENT */
					count_comment = 0;
					abort_comment = 0;
					data_comment = skip_leading_whitespace_line(data + count + 1);
					while (!abort_comment) {
						size_t i;
						switch (data_comment[count_comment]) {
							case '\n':
								comment = sanitize_str(data_comment, count_comment);
								if (count == 0) {
									goto comment_only;
								}
								/* need to make all comment data into spaces now. this helps
								 * determine whether the file is malformed or this is just
								 * a section header. */
								for (i = count; i < count + count_comment; i++) {
									data[i] = ' ';
								}
								abort_comment = 1;
								break;
							case '\0':
								comment = sanitize_str(data_comment, count_comment);
								if (count) {
									/* ERROR */
									/* not supposed to reach end of string... */
									keyval_append_error_va("keyval: error: unexpected end of string on line %d\n", line);
									goto abort_node;
								} else {
									comment_only:
									node = calloc(1, sizeof(struct keyval_node));
									data += count_comment + 2;
									goto done_node;
								}
								break;
						}
						count_comment++;
					}
					/*abort = 1;*/
					break;
				case '}':
				case '\0':
					/* only error out if this condition is met. otherwise we've just
					 * reached the end of a section, and should leave. */
					if (count) {
						/* ERROR */
						goto abort_node;
					} else {
						/* LEAVE */
						/* helps better report line number errors */
						if (data[count - 1] == '\n') line--;
						goto leave;
					}
				case '=':
					type_key = '=';
					abort = 1;
					break;
				case '{':
					type_key = '{';
					abort = 1;
					break;
				case '\n':
					keyval_append_error_va("keyval: error: unexpected end of line on line %d\n", ++line);
					goto abort_node;
					break;
				default:
					break;
			}
			count++;
		}
		
		/* now count stores the length of the key (or name), including trailing
		 * whitespace and possibly multiple spaces inside */

		if (!(count - 1)) {
			keyval_append_error_va("keyval: error: stray `%c` on line %d\n", type_key, line);
			goto abort_node;
		}
		name = sanitize_str_unescape(data, count - 1);
		
		/* (we want to swallow the type character too) */
		data += count + count_comment;

		/* depending on what character we ended at, we'll do different things. */

		if (type_key == '{') {
			/* open section. */
			/* SECTION */

			if (!(node = keyval_parse_node(&data, name, &line))) {
				/* add 'from section'... to error message, and abort */
				goto abort_node;
			}
			if (*data != '}') {
				/* a section was never closed! error!! */
				/* we cannot exactly know the line here. there are newlines at the end
				 * of blank lines. cannot be certain etc. CONFUSED?? */
				keyval_append_error_va("keyval: error: section `%s` never closed near line %d\n", name, line);
				goto abort_node;
			}
			/* make sure we don't read the '}' thinking it's our turn to close */
			data += 1;
		} else if (type_key == '=') {
			/* read in the value. */
			/* VALUE */
			unsigned char comment_found = 0;
			unsigned char list_found = 0;
			char * v;

			node = calloc(1, sizeof(struct keyval_node));

			data = skip_leading_whitespace_line(data);

			/* the value lasts until we reach a '#' or '}' or '\n', unless they've
			 * been escaped. */
			count = 0;
			abort = 0;
			while (!abort) {
				find_value:
				switch (data[count]) {
					case '[':
						if (count == 0) {
							char * d = data + 1;
							/* this has to be a list. */
							if (!(node->children = keyval_parse_list(&d, &line))) {
								keyval_append_error_va("keyval: error: list `%s` not terminated near line %d\n", name, line);
								goto abort_node;
							}
							data = d;
							if (data[-1] != ']') {
								/* error. list improperly terminated. */
								keyval_append_error_va("keyval: error: list `%s` not terminated near line %d\n", name, line);
								goto abort_node;
							}
							list_found = 1;
							/*data = skip_leading_whitespace_line(data + count + 1);*/
							count = 0;
							goto find_value;
						}
						break;
					case '\\':
						count++; /* skip the next character */
						if (data[count] == '\n') line++; /* but update line count! */
						break;
					case '#':
						/* read in the value, then the comment. */
						if (!list_found) {
							if (count == 0) {
								goto missing_value;
							}
							v = sanitize_str_unescape(data, count);
							value = strip_multiple_spaces(skip_leading_whitespace(v, NULL));
							free(v);
						}
						data = skip_leading_whitespace_line(data + count + 1);
						comment_found = 1;
						count = 0;
						break;
					case '\n':
						/* only increment the line count if this value is not in error.
						 * otherwise we would be saying that the error occurred on the
						 * wrong line below. */
						if (count) line++;
						if (list_found && !comment_found) {
							abort = 1;
							break;
						}
					case '}':
					case '\0':
						/* read in the value. */
						if (comment_found) {
							comment = sanitize_str(data, count);
							/*printf("read comment: `%s`\n", comment);*/
							if (list_found) abort = 1;
						} else {
							if (count == 0) {
								missing_value:
								/* there was supposed to be a value but there isn't. error. */
								keyval_append_error_va("keyval: error: expected value after `%s =` on line %d\n", name, line);
								goto abort_node;
							}
							v = sanitize_str_unescape(data, count);
							value = strip_multiple_spaces(skip_leading_whitespace(v, NULL));
							free(v);
						}
						abort = 1;
						/* don't double-count newlines... otherwise we'll send the newline
						 * into the next iteration */
						if (data[count] == '\n') count++;
						break;
				}
				if (!abort)	count++;
			}
			data += count;
		}

		done_node:

		node->name = name;
		node->value = value;
		node->comment = comment;
		
		if (child) {
			child->next = node;
		} else {
			head->children = node;
		}

		child = node;
		continue;
		
		abort_node:
		if (node) keyval_node_free_all(node);
		free(name);
		free(value);
		free(comment);
		goto error;
	}
	
	leave:

	*_data = data;
	if (l) *l = line;
	return head;
	
	error:
	/* cleanup stuff when errors occur */
	keyval_node_free_all(head);
	if (sec_name) keyval_append_error_va("keyval: (in section `%s`)\n", sec_name);
	return NULL;
}

struct keyval_node * keyval_parse_string(const char * data) {
	struct keyval_node * head;
	char * data2 = strdup(data);
	char * data3;
	
	/* to make sure that keyval_parse_node doesn't mess up our pointer to data2 */
	data3 = data2;
	head = keyval_parse_node(&data3, NULL, NULL);
	
	free(data2);

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

	if (!node) keyval_append_error_va("keyval: in file `%s`\n", filename);

	return node;
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
