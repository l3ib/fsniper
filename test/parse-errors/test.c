#include "keyvalcfg.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "error.h"

/* argv[1] is the file name, argv[2] is the expected error */
int main(int argc, char ** argv) {
	char * error;
	
	if (argc != 3) {
		fprintf(stderr, "what file am i testing?\n");
		return 1;
	}

	error = strdup_printf("%s\nkeyval: in file `%s`\n", argv[2], argv[1]);
	if (keyval_test_error(argv[1], error)) {
		free(error);
		return 1;
	}
	
	free(error);
	
	return 0;
}
