#include "keyvalcfg.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define ERROR "keyval: error: section `x` never closed near line 20\n" \
              "keyval: in file `4.cfg`\n"

int main(int argc, char ** argv) {
	struct keyval_node * cfg;
	char * error;
	
	if (argc == 1) {
		fprintf(stderr, "what file am i testing?\n");
		return 1;
	}

	if ((cfg = keyval_parse_file(argv[1]))) {
		fprintf(stderr, "error: expected error parsing %s, but none encountered\n", argv[1]);
		return 1;
	}
	
	error = keyval_get_error();
	if (strcmp(error, ERROR)) {
		fprintf(stderr, "error: got different error than expected:\n");
		fprintf(stderr, "\tgot: `%s`\n\texpected: %s", error, ERROR);
		free(error);
		return 1;
	}
	
	free(error);
	
	return 0;
}
