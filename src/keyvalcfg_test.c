#include "keyvalcfg_test.h"

struct test_case {
	char * input;
	char * output;
};

unsigned char test_strip_comments(void) {
	struct test_case one = {"# comment", ""};
	struct test_case two = {"hello # comment", "hello "};
	struct test_case three = {"hello # comment\n# hi\nthere # heh", "hello \nthere"};
}
