#include "keyval_node.h"

struct keyval_node * keyval_node_append(struct keyval_node * head,
		struct keyval_node * node) {
	node->next = 0;
	if (head) {
		node->head = head->head;
		while (head->next) head = head->next;
		head->next = node;
	} else {
		node->head = node;
		head = node;
	}

	return head->head;
}
