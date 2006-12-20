#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "log.h"
#include "util.h"

FILE *_logfd;

int log_open()
{
	char *configdir, *logfile;
	configdir = get_config_dir();

	logfile = malloc(strlen(configdir) + strlen("/log") + 1);
	sprintf(logfile, "%s/log", configdir);
	free(configdir);	

	_logfd = fopen(logfile, "w");

	if (_logfd)
		log_write("Log opened\n");

	return (_logfd != NULL);	
}

int log_write(char *str, ...)
{
	va_list va;
	int len;

	
	fprintf(_logfd, "%d ", time(NULL));

	va_start(va, str);
	len = vfprintf(_logfd, str, va);
	va_end(va);

	fflush(_logfd);
	return len;
}

int log_close()
{
	if (_logfd)
		fclose(_logfd);

	return 1;
}
