#include "keyvalcfg.h"
#include "keyvalcfg_test.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct test_case {
	char * input;
	char * output;
};

struct test_case_sanitize {
	char * input;
	size_t n;
	char * output;
};

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

char * skip_leading_whitespace(char * string);
unsigned char test_skip_leading_whitespace(void) {
	size_t i;
	struct test_case cases[] = {
		{"\thello", "hello"},
		{" hello", "hello"},
		{"\nhello", "hello"},
		{"  hello", "hello"},
		{"\n\t     \t\n hello", "hello"}
	};

	printf("skip_leading_whitespace\n");

	for (i = 0; i < (sizeof(cases) / sizeof(cases[0])); i++) {
		char * input = strdup(cases[i].input);
		char * output;
		
		printf("\ttest case %d: ", i);
		
		output = skip_leading_whitespace(input);
		
		if (strcmp(output, cases[i].output) == 0) {
			printf("pass\n");
		} else {
			printf("fail\n");
			printf("\t\texpected: \"%s\"\n\t\tgot: \"%s\"\n", cases[i].output, output);
		}
		
		/* we don't free output since skip_leading_whitespace doesn't allocate any
		 * new memory */
		free(input);
	}

	return 1;
}

size_t skip_trailing_whitespace(char * string);
unsigned char test_skip_trailing_whitespace(void) {
	size_t i;
	struct test_case cases[] = {
		{"hello\t", "hello"},
		{"hello" , "hello"},
		{"hello\n", "hello"},
		{"hello  ", "hello"},
		{"   hello\n\t     \t\n ", "   hello"}
	};

	printf("skip_trailing_whitespace\n");

	for (i = 0; i < (sizeof(cases) / sizeof(cases[0])); i++) {
		size_t output;
		
		printf("\ttest case %d: ", i);
		
		output = skip_trailing_whitespace(cases[i].input);
		
		if (strlen(cases[i].output) == output) {
			printf("pass\n");
		} else {
			printf("fail\n");
			printf("\t\texpected: \"%d\"\n\t\tgot: \"%d\"\n",
				strlen(cases[i].output), output);
		}
	}

	return 1;
}

char * sanitize_str(char * string, size_t n);
unsigned char test_sanitize_str(void) {
	size_t i;
	struct test_case_sanitize cases[] = {
		{"hello\t", 6, "hello"},
		{"hello" , 5, "hello"},
		{"hello\n", 6, "hello"},
		{"hello  ", 6, "hello"},
		{"   hello\n\t     \t\n ", 12, "   hello"},
		{"some craziness       occurred", 21, "some craziness"}
	};

	printf("sanitize_str\n");

	for (i = 0; i < (sizeof(cases) / sizeof(cases[0])); i++) {
		char * output;
		
		printf("\ttest case %d: ", i);
		
		output = sanitize_str(cases[i].input, cases[i].n);
		
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

struct keyval_node * keyval_parse_node(char ** data, size_t indents);
void keyval_node_write(struct keyval_node * node, size_t depth, FILE * file);
int main(void) {
	char * data = "lol { pice { x = 0\nno = u} } ham { burgled = true }";
	char * s;
	struct keyval_node * node;
/*
	test_collapse();
	putchar('\n');
	test_strip_multiple_spaces();
	putchar('\n');
	test_skip_leading_whitespace();
	putchar('\n');
	test_skip_trailing_whitespace();
	putchar('\n');
	test_sanitize_str();*/

	node = keyval_parse_file("test/keyval_test3.cfg");
	s = keyval_get_error();
	if (s) {
		printf("%s", s);
		free(s);
	}
	/*
	free(node->children->children->next->value);
	node->children->children->next->value = strdup("haha\nlol\nhaha fhiuhef . . .   ");
	keyval_node_write(node, 0, stdout);
	
	s = keyval_list_to_string(node->children->children->next);
	printf("string version: %s\n", s);
	free(s);*/

	keyval_node_free_all(node);
	
	/*node = keyval_parse_string(data);
	keyval_node_write(node, 0, stdout);
	keyval_node_free_all(node);*/
	return 0;
}
