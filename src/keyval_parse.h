#ifndef KEYVAL_PARSE_HEADER_INCLUDED
#define KEYVAL_PARSE_HEADER_INCLUDED

#include "keyval_node.h"

struct keyval_node * keyval_parse_file(const char * filename);
struct keyval_node * keyval_parse_string(const char * data);
char * keyval_get_error(void);

#endif
