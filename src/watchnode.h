#ifndef _WATCHNODE_H_
#define _WATCHNODE_H_

struct watchnode
{
    int wd;
    char* path;
    struct keyval_node* section;
    struct watchnode* next;
};

struct watchnode* watchnode_create(struct watchnode* node, int wd, char* path, struct keyval_node* section);
 
#endif
