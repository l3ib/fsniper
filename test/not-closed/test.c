#include "keyvalcfg.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../common/error.h"

int main(int argc, char ** argv) {
	char * error;
	
	if (argc == 1) {
		fprintf(stderr, "what file am i testing?\n");
		return 1;
	}

	error = strdup_printf("keyval: error: section `x` never closed near line 2\nkeyval: in file `%s`\n", argv[1]);
	if (keyval_test_error(argv[1], error)) {
		free(error);
		return 1;
	}
	
	free(error);
	
	return 0;
}
