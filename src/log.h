#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>

#define LOG_NONE    0x0
#define LOG_FILE    0x1
#define LOG_STDOUT  0x2
#define LOG_SYS     0x3

int log_open();
int log_write(char *str, ...);
int log_close();

#endif
