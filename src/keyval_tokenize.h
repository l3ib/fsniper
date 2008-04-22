#ifndef KEYVALCFG_TOKENIZE_INCLUDED
#define KEYVALCFG_TOKENIZE_INCLUDED

#include <stdio.h>

enum keyval_token_flags {
	KEYVAL_TOKEN_NORMAL,
	KEYVAL_TOKEN_SEPARATOR,
	KEYVAL_TOKEN_WHITESPACE
};

struct keyval_token {
	const char * data;
	size_t length;
	size_t line;

	enum keyval_token_flags flags;

	struct keyval_token * head;
	struct keyval_token * prev;
	struct keyval_token * next;
};

/* separates s into pieces, with each character of separators getting its own
 * token. things are automatically split by whitespace. */
struct keyval_token * keyval_tokenize(const char * s, char * separators);

void keyval_token_free_all(struct keyval_token * token);

#endif
