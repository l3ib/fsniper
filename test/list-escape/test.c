#include "keyvalcfg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int test_multiline(struct keyval_node * cfg) {
	struct keyval_node * list;
	struct keyval_node * element;
	char * many_lines;
	char * normal;

	if (!(list = keyval_node_find(cfg, "favourite list"))) {
		fprintf(stderr, "error: `favourite list` key not found\n");
		return 1;
	}

	if (keyval_node_get_value_type(list) != KEYVAL_TYPE_LIST) {
		fprintf(stderr, "error: `favourite list` is of wrong type\n");
		return 1;
	}

	if (!(element = keyval_node_get_children(list))) {
		fprintf(stderr, "error: failed to get first element of `favourite list`\n");
		return 1;
	}

	if (!(many_lines = keyval_node_get_value_string(element))) {
		fprintf(stderr, "error: failed to get first value of `favourite list`\n");
		return 1;
	}

	if (strcmp(many_lines, "list element\nconsisting of\nseveral lines.")) {
		fprintf(stderr, "error: first value of `favourite list` is wrong\n");
		free(many_lines);
		return 1;
	}
	free(many_lines);

	if (!(element = keyval_node_get_next(element))) {
		fprintf(stderr, "error: failed to get second element of `favourite list`\n");
		return 1;
	}

	if (!(normal = keyval_node_get_value_string(element))) {
		fprintf(stderr, "error: failed to get second value of `favourite list`\n");
		return 1;
	}

	if (strcmp(normal, "and a normal one,]")) {
		fprintf(stderr, "error: second value of `favourite list` is wrong\n");
		fprintf(stderr, "got: `%s`\n", normal);
		free(normal);
		return 1;
	}
	free(normal);

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
	keyval_write(cfg, "6.out");
	keyval_node_free_all(cfg);

	if (!(cfg = keyval_parse_file("6.out"))) {
		char * error = keyval_get_error();
		if (unlink("6.out")) {
			fprintf(stderr, "failed to remove 6.out\n");
		}
		free(error);
		return 1;
	}

	if (test_multiline(cfg)) {
		keyval_node_free_all(cfg);
		if (unlink("6.out")) {
			fprintf(stderr, "failed to remove 6.out\n");
		}
		return 1;
	}

	keyval_node_free_all(cfg);
	if (unlink("6.out")) {
		fprintf(stderr, "failed to remove 6.out\n");
	}
	return 0;
}
