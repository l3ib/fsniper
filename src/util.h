#include "keyval_node.h"

#ifndef _UTIL_H_
#define _UTIL_H_

char* get_config_dir();
void validate_config(struct keyval_node *config);

#define DELAY_RET_CODE 2

#endif
