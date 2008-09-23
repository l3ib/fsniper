/*
 * fsniper - daemon to run scripts based on changes in files monitored by inotify
 * Copyright (C) 2006, 2007, 2008  Andrew Yates and David A. Foster
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <unistd.h>
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

    if (!logtostdout)
    {
        configdir = get_config_dir();
        logfile = malloc(strlen(configdir) + strlen("/log") + 1);
        sprintf(logfile, "%s/log", configdir);
        free(configdir);	

        _logfd = fopen(logfile, "a");

        free(logfile);
    }
    else
        _logfd = stdout;

    if (_logfd)
    {
    	int pid = getpid();
    	
    	int i = pid;
    	int pidlen = 1;
    	while (i/=10) pidlen++;
    	
    	char *version = PACKAGE_VERSION;
    	char *openstr = malloc(strlen("Log opened: fsniper version ") + strlen(version) + \
    		strlen(" (pid: ") + pidlen + strlen(")\n") + 1);
    		
    	sprintf(openstr, "Log opened: fsniper version %s (pid: %d)\n", version, pid);
        log_write(openstr);
        free(openstr);
    }

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

    va_start(va, str);
    len = vfprintf(_logfd, str, va);
    va_end(va);

    fflush(_logfd);
    return len;
}

int log_close()
{
    if (_logfd && _logfd != stdout)
        fclose(_logfd);

    return 1;
}
