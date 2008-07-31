#ifndef _ADD_WATCHES_H_ 
#define _ADD_WATCHES_H_ 

struct watchnode* add_watches(int fd);

struct watchnode* watch_dir(struct watchnode* node, int ifd, char *dir, struct keyval_node* kv_section);
int unwatch_dir(char *dir, int ifd);

#endif 
