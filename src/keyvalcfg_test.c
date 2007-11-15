#include "keyvalcfg_test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct test_case {
	char * input;
	char * output;
};

void strip_comments(char * string);
unsigned char test_strip_comments(void) {
	size_t i;
	struct test_case cases[] = {
		{"# comment", "         "},
		{"hello # comment", "hello          "},
		{"hello # comment\n# hi\nthere # heh", "hello          \n    \nthere      "}
	};
	
	printf("strip_comments\n");
	
	for (i = 0; i < (sizeof(cases) / sizeof(cases[0])); i++) {
		char * input;
		
		printf("\ttest case %d: ", i);
		
		input = strdup(cases[i].input);
		strip_comments(input);
		
		if (strcmp(input, cases[i].output) == 0) {
			printf("pass\n");
		} else {
			printf("fail\n");
			printf("\t\texpected: \"%s\"\n\t\tgot: \"%s\"\n", cases[i].output, input);
		}
		
		free(input);
	}

	return 1;
}


void collapse(char * string);
unsigned char test_collapse(void) {
	size_t i;
	struct test_case cases[] = {
		{"hi \\ \nthere", "hi    there"},
		{"insert \\ here", "insert \\ here"}
	};
	
	printf("collapse\n");

	for (i = 0; i < (sizeof(cases) / sizeof(cases[0])); i++) {
		char * input;
		
		printf("\ttest case %d: ", i);
		
		input = strdup(cases[i].input);
		collapse(input);
		
		if (strcmp(input, cases[i].output) == 0) {
			printf("pass\n");
		} else {
			printf("fail\n");
			printf("\t\texpected: \"%s\"\n\t\tgot: \"%s\"\n", cases[i].output, input);
		}
		
		free(input);
	}

	return 1;
}

char * strip_multiple_spaces(char * string);
unsigned char test_strip_multiple_spaces(void) {
	size_t i;
	struct test_case cases[] = {
		{"hi             there", "hi there"},
		{"insert\t\t         \ncrap", "insert crap"},
		{"no multiple spaces here\n", "no multiple spaces here\n"}
	};
	
	printf("strip_multiple_spaces\n");

	for (i = 0; i < (sizeof(cases) / sizeof(cases[0])); i++) {
		char * output;
		
		printf("\ttest case %d: ", i);
		
		output = strip_multiple_spaces(cases[i].input);
		
		if (strcmp(output, cases[i].output) == 0) {
			printf("pass\n");
		} else {
			printf("fail\n");
			printf("\t\texpected: \"%s\"\n\t\tgot: \"%s\"\n", cases[i].output, output);
		}
		
		free(output);
	}

	return 1;
}

int main(void) {
	test_strip_comments();
	putchar('\n');
	test_collapse();
	putchar('\n');
	test_strip_multiple_spaces();
	return 0;
}
