#include "keyvalcfg.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

unsigned char check_rationals(struct keyval_node * cfg) {
	struct keyval_node * rationals;
	struct keyval_node * child;
	char * comment;
	
	if (!(rationals = keyval_node_find(keyval_node_get_children(cfg),
	                                   "rationals"))) return 0;

	if (!(comment = keyval_node_get_comment(keyval_node_get_children(rationals)))) {
		fprintf(stderr, "error: comment for `rationals` not found\n");
		return 1;
	}

	if (strcmp(comment,
	           "i couldn't come up with anything more clever than these...")) {
		fprintf(stderr, "error: comment in `rationals` differs\n");
		return 1;
	}

	free(comment);

	child = keyval_node_find(rationals->children, "zero");
	if (!child) {
		fprintf(stderr, "error: `zero` key not found in `rationals`\n");
		return 1;
	}
	if (keyval_node_get_value_int(child) != 0) {
		fprintf(stderr, "error: `zero` key has different value in `rationals`\n");
		return 1;
	}
	
	child = keyval_node_find(rationals->children, "unity");
	if (!child) {
		fprintf(stderr, "error: `unity` key not found in `rationals`\n");
		return 1;
	}
	if (keyval_node_get_value_int(child) != 1) {
		fprintf(stderr, "error: `unity` key has different value in `rationals`\n");
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
	
	if (check_rationals(cfg)) return 1;
	
	keyval_node_free_all(cfg);
	
	return 0;
}
