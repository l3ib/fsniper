#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>

int log_open();
int log_write(char *str, ...);
int log_close();

#endif
