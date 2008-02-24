#include "keyval_tokenize.h"
#include <string.h>
#include <stdlib.h>

static unsigned char is_space(char c) {
	return (c == ' ') || (c == '\t');
}

static struct keyval_token * keyval_token_append(struct keyval_token * head,
		struct keyval_token * token) {

	if (head) {
		while (head->next) head = head->next;
		token->prev = head;
		token->head = head->head;
		head->next = token;
	} else {
		token->next = token->prev = 0;
		token->head = token;
		head = token;
	}

	return head->head;
}

struct keyval_token * keyval_tokenize(char * s, char * separators) {
	char * last = s; /* last non-separating non-space character */
	struct keyval_token * token = 0;
	while (*s) {
		enum keyval_token_flags flags = KEYVAL_TOKEN_NORMAL;
		char * split_point = 0;
		size_t length = 0;
		if (*s == '\\') {
			/* take stuff to the left of this character */
			struct keyval_token * a = calloc(1, sizeof(struct keyval_token));
			a->data = last;
			a->length = s - last;
			a->flags = KEYVAL_TOKEN_NORMAL;

			if (token) keyval_token_append(token, a);
			else token = keyval_token_append(token, a);

			s += 2;
			last = s - 1;
			continue;
		}

		if (strchr(separators, *s)) {
			split_point = s;
			flags = KEYVAL_TOKEN_SEPARATOR;
			length = 1;
		} else if (is_space(*s)) {
			split_point = s;
			flags = KEYVAL_TOKEN_WHITESPACE;
			/* how much whitespace is there? */
			while (is_space(s[length])) length++;
		}

		/* all the stuff from last up to (but not including) *s should be in its
		 * own token. the character *s should be in its own token. */

		if (split_point) {
			struct keyval_token * a = 0;
			struct keyval_token * b = calloc(1, sizeof(struct keyval_token));

			if (s - last) {
				/* only if this is not the first character */
				a = calloc(1, sizeof(struct keyval_token));
				a->data = last;
				a->length = s - last;
				a->flags = KEYVAL_TOKEN_NORMAL;
			}

			b->data = s;
			b->flags = flags;
			b->length = length;

			last = s + length;

			/* trivial linked list manipulation */

			if (a) {
				if (token) keyval_token_append(token, a);
				else token = keyval_token_append(token, a);
				token = a;
			}

			if (token) keyval_token_append(token, b);
			else token = keyval_token_append(token, b);
			token = b;

			if (flags == KEYVAL_TOKEN_WHITESPACE) s += length - 1;
		}
		s++;
	}

	if (s - last) {
		/* some stuff at the end */
		struct keyval_token * a = calloc(1, sizeof(struct keyval_token));
		a->data = last;
		a->length = s - last;
		a->flags = KEYVAL_TOKEN_NORMAL;
		if (token) {
			a->head = token->head;
			token->next = a;
		} else a->head = a;
	}

	return token->head;
}
