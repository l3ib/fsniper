#include "keyval_parse.h"
#include "keyval_tokenize.h"

#include <stdlib.h>
#include <string.h>

#include <stdarg.h>

#define PICESIZE 512

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

/* take tokens from start to end (including end) and make a string out
 * of them. converts whitespace nodes to single spaces. */
static char * keyval_tokens_to_string(struct keyval_token * start,
		struct keyval_token * end) {
	char * s = 0;
	size_t s_alloc = 1;


	/* handle the first one manually, unless it's a space or newline */
	while ((start->flags == KEYVAL_TOKEN_WHITESPACE) || (start->flags == KEYVAL_TOKEN_SEPARATOR && *start->data == '\n')) {
		if (!start->next) return NULL;
		start = start->next;
	}

	/* work backwards to find the first non-space token */
	while (end->flags == KEYVAL_TOKEN_WHITESPACE || (end->flags == KEYVAL_TOKEN_SEPARATOR && *end->data == '\n')) {
		end = end->prev;
	}

	s = calloc(start->length + 1, sizeof(char));
	s_alloc = start->length + 1;
	strncpy(s, start->data, start->length);

	start = start->next;
	while (start != end->next) {
		if (start->flags == KEYVAL_TOKEN_WHITESPACE) {
			/* add a single space character */
			s = realloc(s, s_alloc + 1);
			s[s_alloc - 1] = ' ';
			s[s_alloc] = '\0';
			s_alloc += 1;
		} else {
			s = realloc(s, s_alloc + start->length);
			strncpy(s + s_alloc - 1, start->data, start->length);
			s_alloc += start->length;
			s[s_alloc - 1] = '\0';
		}

		start = start->next;
	}

	return s;
}

struct keyval_node * keyval_parse_comment(struct keyval_token ** token_) {
	struct keyval_token * token = *token_;
	struct keyval_token * first = token;

	while (token) {
		if (token->flags == KEYVAL_TOKEN_SEPARATOR) {
			if (*token->data == '\n') {
				/* the end! */
				struct keyval_node * node = calloc(1, sizeof(struct keyval_node));
				node->comment = keyval_tokens_to_string(first, token->prev);
				*token_ = token;
				return node;
			}
		}

		if (!token->next) {
			/* trailing null means we're at the end of a file... */
			struct keyval_node * node = calloc(1, sizeof(struct keyval_node));
			node->comment = keyval_tokens_to_string(first, token);
			*token_ = token;
			return node;
		}
		token = token->next;
	}

	return 0;
}

struct keyval_node * keyval_parse_list(struct keyval_token ** token_) {
	struct keyval_token * token = *token_;
	struct keyval_token * last = token;
	struct keyval_node * head = 0;

	while (token) {
		if (token->flags == KEYVAL_TOKEN_SEPARATOR) {
			switch (*token->data) {
				case ']':
					{
						/* same as below, except we return */
						struct keyval_node * node = calloc(1, sizeof(struct keyval_node));
						node->value = keyval_tokens_to_string(last, token->prev);
						head = keyval_node_append(head, node);
						*token_ = token;
						return head;
					}
					break;
				case ',':
					{
						struct keyval_node * node = calloc(1, sizeof(struct keyval_node));
						node->value = keyval_tokens_to_string(last, token->prev);
						head = keyval_node_append(head, node);
						last = token = token->next;
					}
				default:
					token = token->next;
					break;
			}
		} else token = token->next;
	}

	/* ran out of tokens... error */

	return 0;
}

struct keyval_node * keyval_parse_value(struct keyval_token ** token_) {
	struct keyval_token * token = *token_;
	struct keyval_token * first = token;

	while (token) {
		if (token->flags == KEYVAL_TOKEN_SEPARATOR) {
			switch (*token->data) {
				case '[':
					if (token->next) {
						struct keyval_node * node = calloc(1, sizeof(struct keyval_node));;
						token = token->next;
						node->children = keyval_parse_list(&token);
						*token_ = token;
						return node;
					}
				case '#':
					{
						/* the value */
						struct keyval_node * node = calloc(1, sizeof(struct keyval_node));
						struct keyval_node * comment;
						node->value = keyval_tokens_to_string(first, token->prev);
						/* and the comment */
						token = token->next;
						comment = keyval_parse_comment(&token);
						node->comment = comment->comment;
						free(comment);
						*token_ = token;
						return node;
					}
				case '\n':
				case '}':
					{
						/* the end! */
						struct keyval_node * node = calloc(1, sizeof(struct keyval_node));
						node->value = keyval_tokens_to_string(first, token->prev);
						*token_ = token;
						return node;
					}
					break;
				default: break;
			}
		}

		if (!token->next) {
			/* trailing null means we're at the end of a file... */
			struct keyval_node * node = calloc(1, sizeof(struct keyval_node));
			node->comment = keyval_tokens_to_string(first, token);
			*token_ = token;
			return node;
		}
		token = token->next;
	}

	return 0;
}

struct keyval_node * keyval_parse_section(struct keyval_token ** token_) {
	struct keyval_token * token = *token_;
	struct keyval_token * last = token;
	struct keyval_node * head_node = calloc(1, sizeof(struct keyval_node));

	while (token) {
		if (token->flags == KEYVAL_TOKEN_SEPARATOR) {
			switch (*token->data) {
				case '{':
					/* section start */
					{
						struct keyval_node * node;
						struct keyval_token * t = token;
						token = token->next;
						node = keyval_parse_section(&token);
						node->name = keyval_tokens_to_string(last, t->prev);
						if (!token || !(token->flags == KEYVAL_TOKEN_SEPARATOR && *token->data == '}')) {
							/* error! zomg! */
							keyval_append_error_va("keyval: error: section `%s` never closed\nkeyval: error: (declared on line %d)\n", node->name, t->line);
							return 0;
						}
						head_node->children = keyval_node_append(head_node->children, node);
						if (token) last = token->next;
						token = token->next;
					}
					break;
				case '}':
					/* section end */
					*token_ = token;
					return head_node;
					break;
				case '#':
					/* a comment node */
					{
						struct keyval_node * node;
						if (token->next) {
							token = token->next;
							node = keyval_parse_comment(&token);
							head_node->children = keyval_node_append(head_node->children, node);
							last = token->next;
						}
					}
					break;
				case '=':
					/* a key-value pair
					 * everything since the last newline is the key */
					{
						struct keyval_node * node;
						struct keyval_token * t = token;
						if (token->next) {
							token = token->next;
							node = keyval_parse_value(&token);
							node->name = keyval_tokens_to_string(last, t->prev);
							head_node->children = keyval_node_append(head_node->children, node);
							last = token->next;
						}
						break;
					}
				default:
					token = token->next;
					break;
			}
		} else token = token->next;
	}

	*token_ = token;
	return head_node;
}

struct keyval_node * keyval_parse_file(const char * filename) {
	char buf[PICESIZE];
	FILE * file = fopen(filename, "r");
	char * data = calloc(PICESIZE, sizeof(char));

	struct keyval_node * node;

	if (!file) return NULL;

	while (fgets(buf, PICESIZE, file)) {
		data = realloc(data, strlen(data) + strlen(buf) + 1);
		data = strcat(data, buf);
	}
	fclose(file);

	node = keyval_parse_string(data);
	free(data);

	return node;
}

struct keyval_node * keyval_parse_string(char * data) {
	struct keyval_token * token = keyval_tokenize(data, "{}#=\n[],");
	struct keyval_node * node = keyval_parse_section(&token);

	return node;
}
