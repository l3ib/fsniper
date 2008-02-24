#ifndef KEYVALCFG_TOKENIZE_INCLUDED
#define KEYVALCFG_TOKENIZE_INCLUDED

#include <stdio.h>

enum keyval_token_flags {
	KEYVAL_TOKEN_NORMAL,
	KEYVAL_TOKEN_SEPARATOR,
	KEYVAL_TOKEN_WHITESPACE
};

struct keyval_token {
	char * data;
	size_t length;

	enum keyval_token_flags flags;

	struct keyval_token * head;
	struct keyval_token * prev;
	struct keyval_token * next;
};

/* separates s into pieces, with each character of separators getting its own
 * token. things are automatically split by whitespace. */
struct keyval_token * keyval_tokenize(char * s, char * separators);

#endif
