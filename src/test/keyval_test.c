#include "keyvalcfg.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CFGFILE "keyval_test.cfg"
#define TEST(x) printf("Testing %s... ", x)
#define TEST_PASS printf("passed.\n")
#define TEST_FAIL test_failed()

void test_failed(void) {
	printf("failed.\n");
	exit(1);
}

int main(void) {
	struct keyval_section * config;
	struct keyval_pair * pair;
	struct keyval_section * watch, * watch_child, * watch_child_child;
	
	if (!(config = keyval_parse(CFGFILE))) {
		fprintf(stderr, "There was an error parsing %s. Does it exist?\n", CFGFILE);
		return 1;
	}
	
	TEST("delay_time exists");
	if ((pair = keyval_pair_find(config->keyvals, "delay_time"))) TEST_PASS;
	else TEST_FAIL;

	TEST("delay_time value");	
	if (keyval_pair_get_value_int(pair) == 300) TEST_PASS;
	else TEST_FAIL;
	
	TEST("delay_repeats exists");
	if ((pair = keyval_pair_find(config->keyvals, "delay_repeats"))) TEST_PASS;
	else TEST_FAIL;
	
	TEST("delay_repeats value");
	if (keyval_pair_get_value_int(pair) == 5) TEST_PASS;
	else TEST_FAIL;

	TEST("watch exists");
	if ((watch = keyval_section_find(config->children, "watch"))) TEST_PASS;
	else TEST_FAIL;

	TEST("~/tmp exists");
	if ((watch_child = keyval_section_find(watch->children, "~/tmp"))) TEST_PASS;
	else TEST_FAIL;
	
	TEST("/.*yats.*/ exists");
	if ((pair = keyval_pair_find(watch_child->keyvals, "/.*yats.*/"))) TEST_PASS;
	else TEST_FAIL;

	TEST("/.*yats.*/ value");	
	if (!strcmp(keyval_pair_get_value_string(pair), "rm %%")) TEST_PASS;
	else TEST_FAIL;

	TEST("*.txt exists");
	if ((pair = keyval_pair_find(watch_child->keyvals, "*.txt"))) TEST_PASS;
	else TEST_FAIL;

	TEST("*.txt value");
	if (!strcmp(keyval_pair_get_value_string(pair), "converttoutf8.rb %%")) TEST_PASS;
	else TEST_FAIL;
	
	TEST("*.py exists");
	if ((pair = keyval_pair_find(watch_child->keyvals, "*.py"))) TEST_PASS;
	else TEST_FAIL;
	
	TEST("*.py value");
	if (!strcmp(keyval_pair_get_value_string(pair), "tar cfvz lolpylon.tar.gz %%"))
		TEST_PASS;
	else TEST_FAIL;
	
	TEST("image/* exists");
	if ((watch_child_child = keyval_section_find(watch_child->children, "image/*"))) TEST_PASS;
	else TEST_FAIL;
	
	TEST("image/* handler 1 exists");
	if ((pair = keyval_pair_find(watch_child_child->keyvals, "handler"))) TEST_PASS;
	else TEST_FAIL;
	
	TEST("image/* handler 1 value");
	if (!strcmp(keyval_pair_get_value_string(pair), "movetobackgrounds.rb %%"))
		TEST_PASS;
	else TEST_FAIL;
	
		TEST("image/* handler 2 exists");
	if ((pair = pair->next)) TEST_PASS;
	else TEST_FAIL;
	
	TEST("image/* handler 2 value");
	if (!strcmp(keyval_pair_get_value_string(pair), "mv %% ~/media/Pictures/"))
		TEST_PASS;
	else TEST_FAIL;
	
	TEST("~/download exists");
	if ((watch_child = keyval_section_find(watch->children, "~/download"))) TEST_PASS;
	else TEST_FAIL;
	
	TEST("* exists");
	if ((pair = keyval_pair_find(watch_child->keyvals, "*"))) TEST_PASS;
	else TEST_FAIL;
	
	TEST("* value");
	if (!strcmp(keyval_pair_get_value_string(pair),
	            "mvtodl.pl %% \"moved via sniper\"")) TEST_PASS;
	else TEST_FAIL;
	
	return 0;
}
