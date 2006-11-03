struct watchnode
{
	int wd;
	char* path;
	struct keyval_section* section;
	struct watchnode* next;
};
