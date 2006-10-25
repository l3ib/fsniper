#include <stdio.h>
#include <string.h>
#include <sys/inotify.h>
#include <wordexp.h>
#include "keyvalcfg.h"

/* parses the config and adds inotify watches to the fd */
void add_watches(int fd, char* configfile)
{
  struct keyval_section *config;
  struct keyval_pair *pair;
  struct keyval_section *child;
  wordexp_t wexp;
  char** expdir;
  int wd;

  config = keyval_parse(configfile);

  for (child = config->children; child; child = child->next)
  {
    wordexp(child->name, &wexp, 0);
    expdir = wexp.we_wordv;
    printf("watching dir: %s\n", expdir[0]);
    wd = inotify_add_watch(fd, expdir[0], IN_CREATE);
    if (keyval_pair_find(child->keyvals, "recurse")->value != NULL && strcmp(keyval_pair_find(child->keyvals, "recurse")->value, "true") == 0)
    {
      printf("Hi. I should currently be recursing the dir and adding watches for subdirs. But I'm not.\n");
    }
    wordfree(&wexp);
  }
}
