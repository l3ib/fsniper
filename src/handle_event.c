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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <fnmatch.h>
#include <pcre.h>
#include <magic.h>
#include "keyval_node.h"
#include "watchnode.h"
#include "util.h"
#include "log.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_EFENCE
#include <efence.h>
#endif

extern struct keyval_node *config;
extern struct watchnode *g_watchnode;
extern int verbose;
extern int syncmode;
extern int logtostdout;

extern void free_all_globals();

static int get_delay_time(struct keyval_node* kv);
static int get_delay_repeats(struct keyval_node* kv);
static char* build_exec_line(char* handler, char* path, char *name);
static void write_out(int writefd, char* text);

/* exits the handle_event function, conditional based on
 * whether it needs to simply return (in sync mode) or do
 * cleanup and exit a child process. */
#define EXIT_HANDLER(status)                    \
    if (syncmode) return;                       \
    else                                        \
    {                                           \
        close(writefd);                         \
        free_all_globals();                     \
        exit(status);                           \
    }

void handle_event(struct inotify_event* event, int writefd)
{
    char abort;
    char isglob;
    char foundslash;
    char *filename;
    char *path;
    char *handlerexec;
    char *name;
    char *newpathenv;
    char *configdir;
    char *scriptdir;
    char *temp;
    const char *mimetype;
    magic_t magic;
    int i, j, sysret, attempts;
    int pathlen;
    struct keyval_node *child;
    struct keyval_node *handler;
    pcre *regex;
    char *regex_str;
    const char *pcre_err;
    int pcre_erroffset;
    int pcre_match;
    int delay_repeats;
    int delay_time;
    struct watchnode* node;

    /* find the node that corresponds to the event's descriptor */
    for (node=g_watchnode->next; node; node = node->next)
    {
        if (node->wd == event->wd)
            break;
    }

    if (!node)
        return;

    /* combine the name inotify gives with the full path to the file */
    filename = malloc(strlen(node->path) + strlen("/") + strlen(event->name) + 1);
    sprintf(filename, "%s/%s", node->path, event->name);

    /* does perror work here? should we also call exit()? */
    if ( (magic = magic_open(MAGIC_MIME)) == NULL)
        perror("magic_open");

    if (magic_load(magic, NULL) < 0)
        perror("magic_load");

    mimetype = magic_file(magic, filename);
    if (mimetype == NULL)
        perror("magic_file");

    /* match the config's expression against a glob, regex, or mimetype */
    abort = 0;

    for (child = node->section->children; child; child = child->next)
    {
        abort = 0;
        foundslash = 0;
        isglob = 1;
        for (i=0; child->name[i]; i++)
        {
            if (child->name[i] == '/')
            {
                isglob = 0;
                break;
            }
        }

        /* filename is '/path/foo', but we want to match against 'foo' for globbing and regexs */
        name = strrchr(filename, '/') + 1; /* add one to change /foo -> foo */

        pathlen = strlen(filename) - strlen(name);
        path = malloc(pathlen + 1);
        strncpy(path, filename, pathlen);
        /* remove the trailing / from path */
        if (pathlen > 0)
            if (path[pathlen-1] == '/')
                path[pathlen-1] = '\0';

        if (name == NULL)
            name = filename;

        if (isglob == 1)
        {
            if (fnmatch(child->name, name, 0) != 0)
                abort = 1;

            if (abort == 0)
                break;
        }

        /* regexs are in the format /regex/ */
        if (isglob == 0 && child->name[0] == '/' && child->name[strlen(child->name)-1] == '/')
        {
            /* child->name is "/regex/", we want "regex" */
            regex_str = strndup(child->name+1, strlen(child->name)-2);
            regex = pcre_compile(regex_str, 0, &pcre_err, &pcre_erroffset, NULL);
            pcre_match = pcre_exec(regex, NULL, name, strlen(name), 0, 0, NULL, 0);
            free(regex_str);

            if (pcre_match < 0)
                abort = 1;

            if (abort == 0)
                break;
        }
		
        /* parse the mimetype to see if it matches the one in the config */
        for (i=0, j=0; isglob == 0 && mimetype[i] && child->name[j] && abort == 0; i++, j++)
        {
            if (!mimetype[i])
            {
                abort = 1;
                break;
            }
            else if (!child->name[j])
            {
                abort = 1;
                break;
            }

            if (foundslash == 0 && child->name[j] == '/')
                foundslash = 1;

            if (mimetype[i] != child->name[j] && child->name[j] != '*')
            {
                abort = 1;
                break;
            }
            else if (child->name[j] == '*' && foundslash == 0)
            {
                while (mimetype[i] != '/')
                    i++;
                while (child->name[j] != '/')
                    j++;
                foundslash = 1;
            }
            else if (child->name[j] == '*' && foundslash == 1)
                break;
        }
        if (abort == 0)
            break;

        /* if we make it here we need to free the path 
         * (it's allocated every loop iteration) */
        free(path);
    }

    if (abort == 1)
    {
        free(filename);
        magic_close(magic);
        EXIT_HANDLER(-1);
    }

    /* dup the fds */
    dup2(writefd, fileno(stdout));
    dup2(writefd, fileno(stderr));

/* find the handlers which correspond to the mimetype, and continue executing them
   until we've run them all or one returns 0 */	
    handler = child->children;

    /* delay attempts are limited! */
    attempts = 0;

    /* modify PATH */
    configdir = get_config_dir();
    scriptdir = malloc(strlen(configdir) + strlen("/scripts") + 2 + 1);
    sprintf(scriptdir, ":%s/scripts:", configdir);
    free(configdir);

    newpathenv = malloc(strlen(getenv("PATH")) + strlen(scriptdir) + 1);
    strcpy(newpathenv, getenv("PATH"));
    strcat(newpathenv, scriptdir);
	
    setenv("PATH", newpathenv, 1);
    free(newpathenv);
    free(scriptdir);

    /* get delay info */
    delay_repeats = get_delay_repeats(child);
    delay_time = get_delay_time(child);
	
    while (handler && handler->name && (attempts < delay_repeats || delay_repeats == 0))
    {
        /* if not a handler, skip it */
        if (strcmp(handler->name, "handler") != 0)
        {
            handler = handler->next;
            continue;
        }

        /* create executable statement (subs %%) */
        handlerexec = build_exec_line(handler->value, path, name);

        temp = malloc(11 + strlen(handlerexec) + 1 + 1);
        sprintf(temp, "Executing: %s\n", handlerexec);
        write_out(writefd, temp);
        free(temp);

        sysret = WEXITSTATUS(system(handlerexec));		

        /* ugh, i know.  we have to make calls to write_out becuase we cannot log from this 
         * function.  if it's called from another process (aka, normal operation, opposed to syncmode)
         * you could get funny results. */
        if (verbose)
        {
            temp = malloc(9 + strlen(handler->value) + 21 + 4 + 1 + 1);
            sprintf(temp, "Handler \"%s\" returned exit code %d\n", handler->value, sysret);
            write_out(writefd, temp);

            free(temp);
        }

        /* special message if system() didn't find the handler.  control flow will pass down below
         * to calling the next handler */
        if (sysret == 127)
        {
            temp = malloc(27 + strlen(handler->value) + 20 + 1);
            sprintf("Could not execute handler \"%s\", trying next one.\n", handler->value);
            write_out(writefd, temp);

            free(temp);
        }

        free(handlerexec);

        /* do somethng based on return code! */

        if (sysret == 0)
        {
            /* the handler handled it!  break out of the handler loop and exit this thread */
            break;
        }
        else if (sysret == DELAY_RET_CODE)
        {
            /* go to sleep for a while */
            attempts++;
            write_out(writefd, "Handler indicated delay, sleeping..."); 
            sleep(delay_time);
            write_out(writefd, "Handler process resuming.");
        } 
        else
        {
            /* some other return code, means try the next handler */
            handler = handler->next;
        }
    }

    if (delay_repeats != 0 && attempts >= delay_repeats)
        write_out(writefd, "Handler gave up on retries."); 

    free(filename);
    free(path);
    magic_close(magic);

    EXIT_HANDLER(0);
}

/**
 * Get the delay time (in seconds) from the config file.
 *
 * The kv parameter should be the node->section parameter as it is being 
 * handled inside of the handle_event function.
 *
 * This function checks the node->section to see if a delay_time keypair is
 * set, if not, it tries the root of the config to see if that same keypair is
 * set.  Failing that, it returns 300 (300 seconds, aka 5 minutes).
 */
static int get_delay_time(struct keyval_node* kv)
{
    struct keyval_node* val;

    if ((val = keyval_node_find(kv, "delay_time")))
        return keyval_node_get_value_int(val);

    if ((val = keyval_node_find(config, "delay_time")))
        return keyval_node_get_value_int(val);

    /* 5 minute default */
    return 300; 
}

/**
 * Get the number of repeats from the config file.
 *
 * The kv parameter should be the node->section parameter as it is being 
 * handled inside of the handle_event function.
 *
 * This function checks the node->section to see if a delay_repeats keypair is
 * set, if not, it tries the root of the config to see if that same keypair is
 * set.  Failing that, it returns 0 (infinite).
 *
 * A value of 0 returned from this function means it should keep looping
 * indefinetely.
 */
static int get_delay_repeats(struct keyval_node* kv)
{
    struct keyval_node* val;

    if ((val = keyval_node_find(kv, "delay_repeats")))
        return keyval_node_get_value_int(val);

    if ((val = keyval_node_find(config, "delay_repeats")))
        return keyval_node_get_value_int(val);

    return 0;
}

/**
 * Builds the line to execute by the handler.
 *
 * Takes the handler from the config file and the filename, and mates them
 * together, placing the filename where appropriate.
 *
 * It will replace any instance of %% with the filename.  If no %% exists in
 * the handler parameter, it will add the filename to the end of the handler
 * with a space.  The filename will always be surrounded by double quotes.
 *
 * This returns a newly allocated string that the caller is expected to 
 * free.
 */
static char* build_exec_line(char* handler, char* path, char* name)
{
    char* sanefilename;
    char* handlerline;
    char* replacename;
    char* percentmod = NULL;
    char* postpercent = NULL;
    char* percentloc = NULL;
    int didreplace = 0;
    int percentoffset;
    int reallocsize;

    /* build filename with quotes */
    sanefilename = malloc(strlen(path) + strlen(name) + 4);
    sprintf(sanefilename, "\"%s/%s\"", path, name);

    handlerline = strdup(handler); 

    /* now replace all %% that occur */
    while ((percentloc = strstr(handlerline, "%")))
    {
        percentmod = percentloc+1;

        /* parse the substitution string, and replace it as follows:
           %F is replaced with only the filename being acted upon
           %D is replaced with only the path to the above file
           %% is replaced with %D/%F */
        if (percentmod[0] == '%')
            replacename = sanefilename;
        else if (percentmod[0] == 'F' || percentmod[0] == 'f')
            replacename = name;
        else if (percentmod[0] == 'D' || percentmod[0] == 'd')
            replacename = path;
        else
        {
            log_write("Error: %%%c is not a valid substitution string\n",percentmod[0]);
        }

        percentoffset = percentloc - handlerline;
        didreplace = 1;
        postpercent = strdup(percentloc+2);

        /* size is length of original string minus size of %% plus size of file plus \0 */
        reallocsize = strlen(handlerline) - 2 + strlen(replacename) + 1;
        handlerline = realloc(handlerline, reallocsize);

        /* make sure we get the whole filename including the \0 */
        strncpy(handlerline+percentoffset, replacename, strlen(replacename) + 1);
        strncat(handlerline, postpercent, strlen(postpercent));
        free(postpercent);
    }

    /* if no replacements, we must tack it on the end */
    if (!didreplace)
    {
        handlerline = realloc(handlerline, strlen(handlerline) + 1 + strlen(sanefilename) + 1);
        strcat(handlerline, " ");
        strcat(handlerline, sanefilename);
    }

    free(sanefilename);

    return handlerline;
}

void write_out(int writefd, char* text)
{
    write(writefd, text, strlen(text));
}

