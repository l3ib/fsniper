#include "keyvalcfg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

char * strdup_printf(const char * format, ...) {
	size_t len;
	va_list ap;
	char * s = calloc(100, sizeof(char));

	va_start(ap, format);

	len = vsnprintf(s, 100, format, ap);
	s = realloc(s, len + 1);

	if (len > 99) {
		vsnprintf(s, len + 1, format, ap);
	}

	va_end(ap);

	return s;
}

int keyval_test_error(char * filename, char * expected) {
	struct keyval_node * cfg;
	char * error;

	if ((cfg = keyval_parse_file(filename))) {
		fprintf(stderr, "error: expected error parsing %s, but none encountered\n",
		        filename);
		keyval_node_free_all(cfg);
		return 1;
	}

	error = keyval_get_error();
	if (strcmp(error, expected)) {
		fprintf(stderr, "error: got different error than expected:\n");
		fprintf(stderr, "\tgot: `%s`\n\texpected: %s", error, expected);
		free(error);
		return 1;
	}

	free(error);

	return 0;
}
