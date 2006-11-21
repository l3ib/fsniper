/* a command-line option parser for C. */

#ifndef ARG_PARSER_HEADER_INCLUDED
#define ARG_PARSER_HEADER_INCLUDED

/* a linked list of arguments */
struct argument_list {
	struct argument_list * first;
	struct argument_list * next;

	char * key;
	char * value;
	char * description; /* for help text */

	unsigned char has_value; /* should this key have a value? */
	unsigned char specified; /* did the user specify this key? */
};

struct argument_exclusive {
	struct argument_exclusive * first;
	struct argument_exclusive * next;

	/* horray (or boo) for struct re-use. this struct is meant to be used as
	 * a linked list containing child structs (the 'exclusive' var that
	 * follows. this is because you can have infinitely many exclusive
	 * arguments. etc. you probably don't need to fuck with this. */

	char * key;
	struct argument_exclusive * exclusive;
};

struct argument {
	struct argument_list * list;
	struct argument_exclusive * exclusive;

	/* extra stuff not preceded by a --, this can be useful for paths etc */
	char * extra;
};

/* returns a new argument structure */
struct argument * argument_new(void);

/* register a new argument in 'argument'. key and
 * description are malloc'd inside this function, there's no need to
 * malloc them yourself if you don't have to. has_value is either 0 or 1. */
void argument_register(struct argument * argument, char * key,
	char * description, unsigned char has_value);

/* makes arguments mutually exclusive (only one or the other can be used.) 
 * 'keys' should be a NULL terminated array of strings. */
void argument_set_exclusive(struct argument * argument, char ** keys);

/* parses argv into 'argument'. returns an error string if an error
 * occurred. returns NULL on success. */
char * argument_parse(struct argument * argument, int argc,
	char ** argv);

/* returns 1 if 'key' was specified by the user (as a command-line
 * option.) returns 0 otherwise. */
unsigned char argument_exists(struct argument * argument, char * key);

/* returns the value given to 'key' (by the user.) returns NULL on
 * failure. */
char * argument_get_value(struct argument * argument, char * key);

/* returns any extra arguments (useful for lists of files, paths, etc.) */
char * argument_get_extra(struct argument * argument);

/* frees everything in the argument struct */
void argument_free(struct argument * argument);

/* returns help text. ensures that line length < 80 chars. */
char * argument_get_help_text(struct argument * argument);

#endif
