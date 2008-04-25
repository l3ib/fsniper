#include "keyvalcfg.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

unsigned char check_rationals(struct keyval_node * cfg) {
	struct keyval_node * rationals;
	struct keyval_node * child;
	char * comment;
	
	if (!(rationals = keyval_node_find(cfg, "rationals"))) return 1;

	if (!(comment = keyval_node_get_comment(keyval_node_get_children(rationals)))) {
		fprintf(stderr, "error: comment for `rationals` not found\n");
		return 1;
	}

	if (strcmp(comment,
	           "i couldn't come up with anything more clever than these...")) {
		fprintf(stderr, "error: comment for `rationals` differs\n");
		return 1;
	}

	free(comment);

	child = keyval_node_find(rationals, "zero");
	if (!child) {
		fprintf(stderr, "error: `zero` key not found in `rationals`\n");
		return 1;
	}
	if (keyval_node_get_value_int(child) != 0) {
		fprintf(stderr, "error: `zero` key has different value in `rationals`\n");
		return 1;
	}
	
	child = keyval_node_find(rationals, "unity");
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

unsigned char check_favourites(struct keyval_node * irrationals) {
	struct keyval_node * favourites;
	struct keyval_node * child;
	
	if (!(favourites = keyval_node_find(irrationals, "favourites"))) {
		fprintf(stderr, "error: `favourites` section not found in `irrationals`\n");
		return 1;
	}
	
	if (!(child = keyval_node_find(favourites, "golden ratio"))) {
		fprintf(stderr, "error: `golden ratio` key not found in `favourites`\n");
		return 1;
	}
	
	if (keyval_node_get_value_type(child) != KEYVAL_TYPE_DOUBLE) {
		fprintf(stderr, "error: wrong type for `golden ratio` in `favourites`\n");
		return 1;
	}
	
	if (keyval_node_get_value_double(child) != 1.61803399) {
		fprintf(stderr, "warning: value for `golden ratio` in `favourites` differs\n");
	}
	
	return 0;
}

unsigned char check_even_numbers(struct keyval_node * cfg) {
	struct keyval_node * evens;
	struct keyval_node * child;
	
	char * dots;

	if (!(evens = keyval_node_find(cfg, "even numbers"))) return 1;
	
	child = keyval_node_get_children(evens);
	if (keyval_node_get_value_type(child) != KEYVAL_TYPE_INT) {
		fprintf(stderr, "error: wrong type in `even numbers`\n");
		return 1;
	}
	
	if (keyval_node_get_value_int(child) != 2) {
		fprintf(stderr, "error: wrong value in `even numbers`\n");
		return 1;
	}
	
	child = keyval_node_get_next(child);
	if (keyval_node_get_value_type(child) != KEYVAL_TYPE_INT) {
		fprintf(stderr, "error: wrong type in `even numbers`\n");
		return 1;
	}
	
	if (keyval_node_get_value_int(child) != 4) {
		fprintf(stderr, "error: wrong value in `even numbers`\n");
		return 1;
	}

	child = keyval_node_get_next(child);
	if (keyval_node_get_value_type(child) != KEYVAL_TYPE_INT) {
		fprintf(stderr, "error: wrong type in `even numbers`\n");
		return 1;
	}
	
	if (keyval_node_get_value_int(child) != 6) {
		fprintf(stderr, "error: wrong value in `even numbers`\n");
		return 1;
	}

	child = keyval_node_get_next(child);
	if (keyval_node_get_value_type(child) != KEYVAL_TYPE_INT) {
		fprintf(stderr, "error: wrong type in `even numbers`\n");
		return 1;
	}
	
	if (keyval_node_get_value_int(child) != 8) {
		fprintf(stderr, "error: wrong value in `even numbers`\n");
		return 1;
	}

	child = keyval_node_get_next(child);
	if (keyval_node_get_value_type(child) != KEYVAL_TYPE_INT) {
		fprintf(stderr, "error: wrong type in `even numbers`\n");
		return 1;
	}
	
	if (keyval_node_get_value_int(child) != 10) {
		fprintf(stderr, "error: wrong value in `even numbers`\n");
		return 1;
	}

	child = keyval_node_get_next(child);
	if (keyval_node_get_value_type(child) != KEYVAL_TYPE_STRING) {
		fprintf(stderr, "error: wrong type in `even numbers`\n");
		return 1;
	}
	
	if (!(dots = keyval_node_get_value_string(child))) {
		fprintf(stderr, "error: no ... at end of `even numbers``\n");
		return 1;
	}
	
	if (strcmp(dots, "...")) {
		fprintf(stderr, "error: no ... at end of `even numbers``\n");
		free(dots);
		return 1;
	}

	free(dots);

	return 0;
}

unsigned char check_odd_numbers(struct keyval_node * cfg) {
	struct keyval_node * odds;
	struct keyval_node * child;
	
	char * dots;

	if (!(odds = keyval_node_find(cfg, "odd numbers"))) {
		fprintf(stderr, "error: `odd numbers` not found\n");
		return 1;
	}
	
	child = keyval_node_get_children(odds);
	if (keyval_node_get_value_type(child) != KEYVAL_TYPE_INT) {
		fprintf(stderr, "error: wrong type in `odd numbers`\n");
		return 1;
	}
	
	if (keyval_node_get_value_int(child) != 1) {
		fprintf(stderr, "error: wrong value in `odd numbers`\n");
		return 1;
	}
	
	child = keyval_node_get_next(child);
	if (keyval_node_get_value_type(child) != KEYVAL_TYPE_INT) {
		fprintf(stderr, "error: wrong type in `odd numbers`\n");
		return 1;
	}
	
	if (keyval_node_get_value_int(child) != 3) {
		fprintf(stderr, "error: wrong value in `odd numbers`\n");
		return 1;
	}

	child = keyval_node_get_next(child);
	if (keyval_node_get_value_type(child) != KEYVAL_TYPE_INT) {
		fprintf(stderr, "error: wrong type in `odd numbers`\n");
		return 1;
	}
	
	if (keyval_node_get_value_int(child) != 5) {
		fprintf(stderr, "error: wrong value in `odd numbers`\n");
		return 1;
	}

	child = keyval_node_get_next(child);
	if (keyval_node_get_value_type(child) != KEYVAL_TYPE_INT) {
		fprintf(stderr, "error: wrong type in `odd numbers`\n");
		return 1;
	}
	
	if (keyval_node_get_value_int(child) != 7) {
		fprintf(stderr, "error: wrong value in `odd numbers`\n");
		return 1;
	}

	child = keyval_node_get_next(child);
	if (keyval_node_get_value_type(child) != KEYVAL_TYPE_INT) {
		printf("->%s\n", child->value);
		fprintf(stderr, "error: wrong type in `odd numbers`\n");
		return 1;
	}
	
	if (keyval_node_get_value_int(child) != 9) {
		fprintf(stderr, "error: wrong value in `odd numbers`\n");
		return 1;
	}
	
	child = keyval_node_get_next(child);
	if (keyval_node_get_value_type(child) != KEYVAL_TYPE_INT) {
		fprintf(stderr, "error: wrong type in `odd numbers`\n");
		return 1;
	}
	
	if (keyval_node_get_value_int(child) != 11) {
		fprintf(stderr, "error: wrong value in `odd numbers`\n");
		return 1;
	}

	child = keyval_node_get_next(child);
	if (keyval_node_get_value_type(child) != KEYVAL_TYPE_STRING) {
		fprintf(stderr, "error: wrong type in `odd numbers`\n");
		return 1;
	}
	
	if (!(dots = keyval_node_get_value_string(child))) {
		fprintf(stderr, "error: no ... at end of `odd numbers``\n");
		return 1;
	}
	
	if (strcmp(dots, "...")) {
		fprintf(stderr, "error: no ... at end of `odd numbers``\n");
		free(dots);
		return 1;
	}

	free(dots);

	return 0;
}

unsigned char check_irrationals(struct keyval_node * cfg) {
	struct keyval_node * irrationals;
	struct keyval_node * child;
	char * comment;

	if (!(irrationals = keyval_node_find(cfg, "irrationals"))) return 1;

	if (check_favourites(irrationals)) return 1;
	
	if (!(child = keyval_node_find(irrationals, "pi"))) {
		fprintf(stderr, "error: `pi` key not found in `irrationals`\n");
		return 1;
	}
	
	if (keyval_node_get_value_type(child) != KEYVAL_TYPE_DOUBLE) {
		fprintf(stderr, "error: wrong type for `pi` in `irrationals`\n");
		return 1;
	}
	
	if (keyval_node_get_value_double(child) != 3.14159265358979) {
		fprintf(stderr, "warning: value for `pi` in `irrationals` differs\n");
	}
	
	if (!(comment = keyval_node_get_comment(child))) {
		fprintf(stderr, "error: missing comment for `pi` in `irrationals`\n");
		return 1;
	}
	
	if (strcmp(comment, "ratio of the circumference to the diameter of a circle")) {
		fprintf(stderr, "error: comment for `pi` differs\n");
		free(comment);
		return 1;
	}
	free(comment);
	
	if (!(child = keyval_node_find(irrationals, "root two"))) {
		fprintf(stderr, "error: `root two` key not found in `irrationals`\n");
		return 1;
	}
	
	if (keyval_node_get_value_type(child) != KEYVAL_TYPE_DOUBLE) {
		fprintf(stderr, "error: wrong type for `root two` in `irrationals`\n");
		return 1;
	}
	
	if (keyval_node_get_value_double(child) != 1.4142135623731) {
		fprintf(stderr, "warning: value for `root two` in `irrationals` differs\n");
	}
	
	if (!(comment = keyval_node_get_comment(child))) {
		fprintf(stderr, "error: missing comment for `root two` in `irrationals`\n");
		return 1;
	}
	
	if (strcmp(comment, "positive solution to x^2 = 2")) {
		fprintf(stderr, "error: comment for `root two` differs\n");
		free(comment);
		return 1;
	}
	free(comment);
	
	return 0;
}

int main(int argc, char ** argv) {
	struct keyval_node * cfg;
	struct keyval_node * child;
	char * comment;

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
	
	if (!(child = keyval_node_find(cfg, "this parser rocks"))) {
		fprintf(stderr, "error: missing `this parser rocks`\n");
		return 1;
	}
	
	if (keyval_node_get_value_type(child) != KEYVAL_TYPE_BOOL) {
		fprintf(stderr, "error: wrong type for `this parser rocks`\n");
		return 1;
	}
	
	if (!keyval_node_get_value_bool(child)) {
		fprintf(stderr, "error: wrong value for `this parser rocks`\n");
		return 1;
	}
	
	if (!(comment = keyval_node_get_comment(child))) {
		fprintf(stderr, "error: missing comment for `this parser rocks`\n");
		return 1;
	}
	
	if (strcmp(comment, "haha, sure")) {
		fprintf(stderr, "error: wrong comment for `this parser rocks`\n");
		free(comment);
		return 1;
	}
	
	free(comment);
	
	if (check_rationals(cfg)) goto error;
	if (check_irrationals(cfg)) goto error;
	if (check_even_numbers(cfg)) goto error;
	if (check_odd_numbers(cfg)) goto error;
	
	keyval_node_free_all(cfg);
	
	return 0;

	error:
	keyval_node_free_all(cfg);
	return 1;
}
