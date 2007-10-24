#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "log.h"
#include "util.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_EFENCE
#include <efence.h>
#endif

FILE *_logfd;

extern int logtostdout;

int log_open()
{
	char *configdir, *logfile;
	configdir = get_config_dir();

	logfile = malloc(strlen(configdir) + strlen("/log") + 1);
	sprintf(logfile, "%s/log", configdir);
	free(configdir);	

	_logfd = fopen(logfile, "w");

	free(logfile);

	if (_logfd)
		log_write("Log opened\n");

	return (_logfd != NULL);	
}

int log_write(char *str, ...)
{
	va_list va;
	int len;
	time_t t;
	char readabletime[30];
	t = time(NULL);
	strftime(readabletime, sizeof(readabletime), "%F %T", localtime(&t));
	
	fprintf(_logfd, "%s ", readabletime);
	if (logtostdout) fprintf(stdout, "%s ", readabletime);

	va_start(va, str);
	len = vfprintf(_logfd, str, va);
	if (logtostdout) len = vfprintf(stdout, str, va);
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
