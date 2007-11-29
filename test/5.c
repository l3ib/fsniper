#include "keyvalcfg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int test_multiline(struct keyval_node * cfg) {
	struct keyval_node * value;
	char * cookie;

	if (!(value = keyval_node_find(cfg, "value"))) {
		fprintf(stderr, "error: `value` key not found\n");
		return 1;
	}

	cookie = keyval_node_get_value_string(value);

	if (strcmp(cookie, "cookies\nand\nmilk")) {
		fprintf(stderr, "error: `value` key has wrong value\n");
		free(cookie);
		return 1;
	}

	free(cookie);

	return 0;
}

int main(int argc, char ** argv) {
	struct keyval_node * cfg;

	if (argc == 1) {
		fprintf(stderr, "what file am i testing?\n");
		return 1;
	}

	if (!(cfg = keyval_parse_file(argv[1]))) {
		char * error = keyval_get_error();
		fprintf(stderr, "%s", error);
		free(error);
		return 1;
	}

	if (test_multiline(cfg)) {
		keyval_node_free_all(cfg);
		return 1;
	}
	keyval_write(cfg, "5.out");
	keyval_node_free_all(cfg);

	if (!(cfg = keyval_parse_file("5.out"))) {
		char * error = keyval_get_error();
		fprintf(stderr, "%s", error);
		if (unlink("5.out")) {
			fprintf(stderr, "failed to remove 5.out\n");
		}
		free(error);
		return 1;
	}

	if (test_multiline(cfg)) {
		keyval_node_free_all(cfg);
		if (unlink("5.out")) {
			fprintf(stderr, "failed to remove 5.out\n");
		}
		return 1;
	}

	keyval_node_free_all(cfg);
	if (unlink("5.out")) {
		fprintf(stderr, "failed to remove 5.out\n");
	}
	return 0;
}
