#ifndef KEYVAL_NODE_HEADER_INCLUDED
#define KEYVAL_NODE_HEADER_INCLUDED

struct keyval_node {
	/* the name of the key or section. can be NULL. */
	char * name;
	
	/* if this is a simple key-value pair, this contains the value. if this is
	 * a section or just a comment, the value is NULL. */
	char * value;
	
	/* any comment associated with this node. the whole node could be just
	 * a comment!*/
	char * comment;

	/* self explanatory, can be null if appropriate */
	struct keyval_node * children;
	struct keyval_node * head;
	struct keyval_node * next;
};

/* appends a node to the end of a linked list. returns the new head of the
 * list. */
struct keyval_node * keyval_node_append(struct keyval_node * head,
		struct keyval_node * node);

/* traverses the linked list pointed to by 'head' and frees it and all
 * children. */
void keyval_node_free_all(struct keyval_node * head);

/* writes the stuff out to a file. returns 1 on success, 0 on
 * failure. if filename is NULL, writes to stdout. */
unsigned char keyval_write(struct keyval_node * head, const char * filename);

/* gets the name of a node. */
char * keyval_node_get_name(struct keyval_node * node);

/* gets any comment associated with a node. */
char * keyval_node_get_comment(struct keyval_node * node);

/* gets the children of a node. */
struct keyval_node * keyval_node_get_children(struct keyval_node * node);

/* gets the next node in a linked list. */
struct keyval_node * keyval_node_get_next(struct keyval_node * node);

/* finds the first keyval_node with name 'name' in the CHILDREN of head.
 * returns NULL on failure. */
struct keyval_node * keyval_node_find(struct keyval_node * head, char * name);

struct keyval_node * keyval_node_find_next(struct keyval_node * node,
                                           char * name);

enum keyval_value_type {
	KEYVAL_TYPE_NONE,
	KEYVAL_TYPE_BOOL,
	KEYVAL_TYPE_STRING,
	KEYVAL_TYPE_INT,
	KEYVAL_TYPE_DOUBLE,
	KEYVAL_TYPE_LIST
};

/* determines the value type of a node. */
enum keyval_value_type keyval_node_get_value_type(struct keyval_node * node);

/* convenience functions for getting values. figure it out. */
unsigned char keyval_node_get_value_bool(struct keyval_node * node);
char * keyval_node_get_value_string(struct keyval_node * node);
int keyval_node_get_value_int(struct keyval_node * node);
double keyval_node_get_value_double(struct keyval_node * node);

#endif
