#include "argparser.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct argument * argument_new(void) {
	struct argument * argument = malloc(sizeof(struct argument));

	argument->list = NULL;
	argument->exclusive = NULL;
	argument->extra = NULL;

	return argument;
}

void argument_register(struct argument * argument, char * key,
	char * description, unsigned char has_value) {

	struct argument_list * list = malloc(sizeof(struct argument_list));
	list->next = NULL;
	list->key = strdup(key);
	list->description = strdup(description);
	list->has_value = has_value;
	list->value = NULL;

	if (argument->list) {
		/* append 'list' to end of existing list */
		struct argument_list * list_ptr = argument->list;
		while (list_ptr->next) list_ptr = list_ptr->next;
		list_ptr->next = list;
		list->first = list_ptr->first;
	} else {
		list->first = list;
		argument->list = list;
	}

}

void argument_set_exclusive(struct argument * argument, char ** keys) {
	struct argument_exclusive * exclusive;
	struct argument_exclusive * exclusive_key_list;

	/* add a new 'exclusive' struct to the existing list */
	if (!argument->exclusive) {
		argument->exclusive = malloc(sizeof(struct argument_exclusive));
		argument->exclusive->next = NULL;
		argument->exclusive->first = argument->exclusive;
		exclusive_key_list = argument->exclusive->exclusive =
			malloc(sizeof(struct argument_exclusive));
	} else {
		/* add a new exclusive thing to the end of the existing linked list */
		exclusive = argument->exclusive;
		while (exclusive->next) exclusive = exclusive->next;

		exclusive->next = malloc(sizeof(struct argument_exclusive));
		exclusive->next->next = NULL;
		exclusive->next->first = exclusive->first;

		exclusive_key_list = exclusive->next->exclusive =
			malloc(sizeof(struct argument_exclusive));
	}
	
	/* exclusive_key_list is now the head of a linked list of keys that are
	 * mutually exclusive to each other. we should populate the list. */

	exclusive_key_list->first = exclusive_key_list;
	exclusive_key_list->next = NULL;
	exclusive_key_list->key = strdup(*(keys++));

	for (; *keys; keys++) {
		/* i re-use 'exclusive' over here to save vars. lol. */
		exclusive = malloc(sizeof(struct argument_exclusive));
		exclusive->next = NULL;
		exclusive->first = exclusive_key_list->first;

		exclusive->key = strdup(*keys);
	
		exclusive_key_list->next = exclusive;
		exclusive_key_list = exclusive_key_list->next;
	}

}

/* returns the argument struct in the linked list with key 'key' */
static struct argument_list * argument_search(struct argument * argument,
	char * key) {

	struct argument_list * list = argument->list;

	for (; list; list = list->next) {
		if (strcmp(list->key, key) == 0) return list;
	}

	return NULL; /* no match */
}

unsigned char argument_exists(struct argument * argument, char * key) {
	struct argument_list * list;

	if ((list = argument_search(argument, key)) && list->specified) {
		return 1;
	}

	return 0;
}

char * argument_get_value(struct argument * argument, char * key) {
	struct argument_list * list;
	if ((list = argument_search(argument, key))) {
		return list->value;
	}

	return NULL;
}

char * argument_get_extra(struct argument * argument) {
	return argument->extra;
}

static void argument_exclusive_free(struct argument_exclusive * exclusive) {
	while (exclusive) {
		struct argument_exclusive * tmp = exclusive->next;

		if (exclusive->key) free(exclusive->key);
		if (exclusive->exclusive) argument_exclusive_free(exclusive->exclusive);

		free(exclusive);

		exclusive = tmp;
	}
}

static void argument_list_free(struct argument_list * list) {
	while (list) {
		struct argument_list * tmp = list->next;

		if (list->key) free(list->key);
		if (list->value) free(list->value);
		if (list->description) free(list->description);

		free(list);
		list = tmp;
	}
}

void argument_free(struct argument * argument) {
	if (argument->list) argument_list_free(argument->list);
	if (argument->exclusive) argument_exclusive_free(argument->exclusive);
	if (argument->extra) free(argument->extra);

	free(argument);
}

/* lets you know if 'key' conflicts with another argument due to
 * exclusivity. */
char * argument_conflicts(struct argument * argument,
	char * key) {

	struct argument_exclusive * exclusive = argument->exclusive;

	/* iterate through the list of possibly mutually exclusive things */
	for (; exclusive; exclusive = exclusive->next) {
		struct argument_exclusive * child;
		/* iterate through mutually exclusive keys */
		for (child = exclusive->exclusive; child; child = child->next) {
			if (strcmp(key, child->key) == 0) {
				/* 'key' is mutually exclusive to something. is that 'something'
				 * specified by the user? */
				struct argument_exclusive * tmp = child->first;
				for (; tmp; tmp = tmp->next) {
					if (argument_exists(argument, tmp->key)) {

						/* a conflicting argument was specified */
					
						return tmp->key;
					}
				}
				return NULL;
			}
		}
	}

	return NULL;
}

/* concatenates src onto the end of dest, allocating memory if necessary.
 * returns the new string (src's pointer may be modified) */
char * strcat_dup(char * dest, const char * src) {
	dest = realloc(dest, (dest ? strlen(dest) : 0) + strlen(src) + 1);
	return strcat(dest, src);
}

char * argument_parse(struct argument * argument, int argc,
	char ** argv) {

	char * error = NULL;
	char * err_str; /* temporary; used to hold allocs and stuff */

	/* skip the program name */
	argc--;

	while (argc--) {
		char * key;
		char * value = NULL;
		struct argument_list * child;
	
		argv++;

		if (!(((*argv)[0] == '-') && ((*argv)[1] == '-'))) {
			/* this should be tacked onto the extra arg string */
			argument->extra = strcat_dup(argument->extra, *argv);
			continue; /* next! */
		}

		/* is this key given a value? */
		if ((value = strchr(*argv, '='))) {

			/* it is given a value. */
			size_t key_len = value - *argv - 2; /* 2' for the -- */

			/* store the key */
			key = malloc(key_len + 1);
			key = strncpy(key, *argv + 2, key_len);
			key[key_len] = '\0';


			value = strdup(value + 1);
		} else {
			key = strdup(*argv + 2);
		}

		child = argument_search(argument, key);
		if (child) {
			char * conflicting_key;
	
			/* does this key conflict with something else? */
			if ((conflicting_key = argument_conflicts(argument, key))) {
				/* yes, error! */
				err_str = malloc(strlen(key)
					+ strlen(" cannot be used in combination with ")
					+ strlen(conflicting_key) + 7);

				sprintf(err_str, "'%s' cannot be used in combination with '%s'.\n",
					key, conflicting_key);

				error = strcat_dup(error, err_str);
				free(err_str);
			}

			child->value = value;
			child->specified = 1;
			if (value && !child->has_value) {
				/* value specified but not supposed to be, error */
				err_str = malloc(strlen(key)
					+ strlen(" does not take a value.\n") + 3);

				sprintf(err_str, "'%s' does not take a value.\n", key);

				error = strcat_dup(error, err_str);
				free(err_str);
			}
		} else {
			/* unknown argument specified, error */
			err_str = malloc(strlen(key) + strlen(" is not a valid argument.\n")
				+ 3);

			sprintf(err_str, "'%s' is not a valid argument.\n", key);

			error = strcat_dup(error, err_str);
			free(err_str);
		}
		free(key);
	}

	return error;
}

/* extremely inefficient memory management follows. */
char * argument_get_help_text(struct argument * argument) {
	struct argument_list * list = argument->list;
	char * ret_str = strdup("Usage:\n");
	
	for (; list; list = list->next) {
		size_t len = strlen(list->key) + strlen(list->description)
			+ (list->has_value ? 6 : 0) + 8;
		char * arg_str = malloc(len);
		char * format_str = list->has_value ? 
			"\t--%s=[arg]\n\t\t%s\n" : "\t--%s\n\t\t%s\n";
		snprintf(arg_str, len, format_str, list->key, list->description);
		ret_str = strcat_dup(ret_str, arg_str);

		free(arg_str);
	}

	return ret_str;
}
