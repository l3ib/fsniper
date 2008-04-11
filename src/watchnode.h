struct watchnode
{
    int wd;
    char* path;
    struct keyval_node* section;
    struct watchnode* next;
};
