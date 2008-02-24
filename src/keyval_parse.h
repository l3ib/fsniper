#ifndef KEYVAL_PARSE_HEADER_INCLUDED
#define KEYVAL_PARSE_HEADER_INCLUDED

#include "keyval_node.h"

/* parses a config file, the infamous parser func */
struct keyval_node * keyval_parse_file(const char * filename);

/* parses a string */
struct keyval_node * keyval_parse_string(char * data);

#endif
