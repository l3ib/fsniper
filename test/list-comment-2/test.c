#include "keyvalcfg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int test_list_comment(struct keyval_node * cfg) {
	struct keyval_node * list;
	struct keyval_node * element;
	char * comment;

	if (!(list = keyval_node_find(cfg, "commented list"))) {
		fprintf(stderr, "error: `commented list` not found\n");
		return 1;
	}

	if (!(comment = keyval_node_get_comment(list))) {
		fprintf(stderr, "error: `commented list` has no comment\n");
		return 1;
	}

	if (strcmp(comment, "hello!")) {
		fprintf(stderr, "error: `commented list` has wrong comment\n");
		fprintf(stderr, "got: `%s`\nexpected: `hello!`\n", comment);
		free(comment);
		return 1;
	}

	free(comment);

	if (!(element = keyval_node_get_children(list))) {
		fprintf(stderr, "error: missing first element of `commented list`\n");
		return 1;
	}

	if (keyval_node_get_value_int(element) != 1) {
		fprintf(stderr, "error: first element of `commented list` has wrong value\n");
		return 1;
	}

	if (!(comment = keyval_node_get_comment(element))) {
		fprintf(stderr, "error: first element of `commented list` has no comment\n");
		return 1;
	}

	if (strcmp(comment, "one")) {
		fprintf(stderr, "error: first element of `commented list` has wrong comment\n");
		free(comment);
		return 1;
	}

	free(comment);

	if (!(element = keyval_node_get_next(element))) {
		fprintf(stderr, "error: missing second element of `commented list`\n");
		return 1;
	}

	if (keyval_node_get_value_int(element) != 2) {
		fprintf(stderr, "error: second element of `commented list` has wrong value\n");
		return 1;
	}

	if (!(comment = keyval_node_get_comment(element))) {
		fprintf(stderr, "error: second element of `commented list` has no comment\n");
		return 1;
	}

	if (strcmp(comment, "two")) {
		fprintf(stderr, "error: second element of `commented list` has wrong comment\n");
		free(comment);
		return 1;
	}

	free(comment);

	if (!(element = keyval_node_get_next(element))) {
		fprintf(stderr, "error: missing third element of `commented list`\n");
		return 1;
	}

	if (keyval_node_get_value_int(element) != 3) {
		fprintf(stderr, "error: third element of `commented list` has wrong value\n");
		return 1;
	}

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

	if (test_list_comment(cfg)) {
		fprintf(stderr, "error: from original file\n");
		keyval_node_free_all(cfg);
		return 1;
	}
	keyval_write(cfg, "test.out");
	keyval_node_free_all(cfg);

	if (!(cfg = keyval_parse_file("test.out"))) {
		char * error = keyval_get_error();
		if (unlink("test.out")) {
			fprintf(stderr, "failed to remove test.out\n");
		}
		free(error);
		return 1;
	}

	if (test_list_comment(cfg)) {
		fprintf(stderr, "error: from second file\n");
		keyval_node_free_all(cfg);
		if (unlink("test.out")) {
			fprintf(stderr, "failed to remove test.out\n");
		}
		return 1;
	}

	keyval_node_free_all(cfg);
	if (unlink("test.out")) {
		fprintf(stderr, "failed to remove test.out\n");
	}
	return 0;
}
