#include "keyvalcfg.h"
#include "../common/error.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char ** argv) {
	char * error;
	
	if (argc == 1) {
		fprintf(stderr, "what file am i testing?\n");
		return 1;
	}

	/* make error string */
	error = strdup_printf("keyval: error: expected value after `x =` on line 1\nkeyval: in file `%s`\n", argv[1]);

	if (keyval_test_error(argv[1], error)) {
		free(error);
		return 1;
	}
	free(error);
	return 0;
}
