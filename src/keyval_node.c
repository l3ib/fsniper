#include "keyval_node.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

void keyval_node_free_all(struct keyval_node * node) {
    /* free will do nothing if these are null */
    free(node->name);
    free(node->value);
    free(node->comment);
	
    if (node->children) keyval_node_free_all(node->children);
    if (node->next) keyval_node_free_all(node->next);
	
    free(node);
}

char * keyval_node_get_name(struct keyval_node * node) {
    if (!node->name) return NULL;
    return strdup(node->name);
}

char * keyval_node_get_comment(struct keyval_node * node) {
    if (!node->comment) return NULL;
    return strdup(node->comment);
}

struct keyval_node * keyval_node_get_children(struct keyval_node * node) {
    return node->children;
}

struct keyval_node * keyval_node_get_next(struct keyval_node * node) {
    return node->next;
}

struct keyval_node * keyval_node_find(struct keyval_node * head, char * name) {
    return keyval_node_find_next(head->children, name);
}

struct keyval_node * keyval_node_find_next(struct keyval_node * node,
                                           char * name) {
    for (; node; node = node->next) {
        if (!node->name) continue;
        if (!strcmp(node->name, name)) return node;
    }
	
    return NULL;
}

static unsigned char keyval_node_has_list_value(struct keyval_node * node) {
    struct keyval_node * child;
	
    /* a node is a list if:
     * it has no value,
     * it has children,
     * its children do not have children,
     * its children do not have names.
     */
	
    if (node->value) {
        if (node->value && (*node->value == '[')) {
            size_t len = strlen(node->value);
            if (node->value[len - 1] == ']') return 1;
        }
    } else {
        unsigned result = 1;
        if (!node->children) return 0;

        for (child = node->children; child; child = child->next) {
            if (child->name || child->children) result = 0;
        }

        return result;
    }
	
    return 0;
}

enum keyval_value_type keyval_node_get_value_type(struct keyval_node * node) {
    enum keyval_value_type type = KEYVAL_TYPE_INT;
    char * s;
	
    if (keyval_node_has_list_value(node)) return KEYVAL_TYPE_LIST;
    if (!node->value) return KEYVAL_TYPE_NONE;
	
    if (strlen(node->value) == 1) {
        switch (*node->value) {
        case 't':
        case 'T':
        case 'y':
        case 'Y':
        case 'f':
        case 'F':
        case 'n':
        case 'N':
            return KEYVAL_TYPE_BOOL;
        default: break;
        }
    }
	
    if (strcasecmp(node->value, "true") == 0) return KEYVAL_TYPE_BOOL;
    if (strcasecmp(node->value, "false") == 0) return KEYVAL_TYPE_BOOL;
    if (strcasecmp(node->value, "yes") == 0) return KEYVAL_TYPE_BOOL;
    if (strcasecmp(node->value, "no") == 0) return KEYVAL_TYPE_BOOL;

    for (s = node->value; *s; s++) {
        if (!isdigit(*s)) {
            /* hmm, a non-digit. if it's a '.', we could still be a decimal number. */
            if (*s == '.') {
                /* two dots encountered */
                if (type == KEYVAL_TYPE_DOUBLE) return KEYVAL_TYPE_STRING;
                type = KEYVAL_TYPE_DOUBLE; /* first dot */
            } else return KEYVAL_TYPE_STRING;
        }
    }
	
    return type;
}

unsigned char keyval_node_get_value_bool(struct keyval_node * node) {
    if (!node->value) return 0;

    switch (*node->value) {
    case 't':
    case 'T':
    case 'y':
    case 'Y':
    case '1':
        return 1;
    default:
        return 0;
    }
}

char * keyval_node_get_value_string(struct keyval_node * node) {
    return node->value ? strdup(node->value) : NULL;
}

int keyval_node_get_value_int(struct keyval_node * node) {
    return node->value ? atoi(node->value) : 0;
}

double keyval_node_get_value_double(struct keyval_node * node) {
    return node->value ? strtod(node->value, NULL) : 0.0;
}
