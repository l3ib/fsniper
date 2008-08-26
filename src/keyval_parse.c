/*
 * keyval - library to parse a config file with a C-like syntax
 * Copyright (C) 2007, 2008  Javeed Shaikh
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
#include "keyval_parse.h"
#include "keyval_tokenize.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define PICESIZE 512

static char * error = 0;

static void keyval_append_error(char * e) {
    if (error) {
        error = realloc(error, strlen(error) + strlen(e) + 1);
        strcat(error, e);
    } else {
        error = strdup(e);
    }
}

static void keyval_append_error_va(const char * format, ...) {
    char * e = malloc(100 * sizeof(char));
    size_t r;

    va_list ap;
    va_start(ap, format);
  
    r = vsnprintf(e, 100, format, ap);
    e = realloc(e, r + 1);
    if (r > 99) {
        /* we need to do this again */
        r = vsnprintf(e, r + 1, format, ap);
    }
    va_end(ap);
    keyval_append_error(e);
    free(e);
}

char * keyval_get_error(void) {
    char * ret = error;
    error = 0;
  
    return ret;
}

/* take tokens from start to end (including end) and make a string out
 * of them. converts whitespace nodes to single spaces. */
static char * keyval_tokens_to_string(struct keyval_token * start,
                                      struct keyval_token * end) {
    char * s = 0;
    size_t s_alloc = 1;


    /* handle the first one manually, unless it's a space or newline */
    while ((start->flags == KEYVAL_TOKEN_WHITESPACE) || (start->flags == KEYVAL_TOKEN_SEPARATOR && *start->data == '\n')) {
        if (!start->next) return 0;
        start = start->next;
    }

    /* work backwards to find the first non-space token */
    while (end->flags == KEYVAL_TOKEN_WHITESPACE || (end->flags == KEYVAL_TOKEN_SEPARATOR && *end->data == '\n')) {
        end = end->prev;
    }

    if (start->prev == end) return 0;

    s = calloc(start->length + 1, sizeof(char));
    s_alloc = start->length + 1;
    strncpy(s, start->data, start->length);

    start = start->next;
    while (start && (start != end->next)) {
        if (start->flags == KEYVAL_TOKEN_WHITESPACE) {
            /* add a single space character */
            s = realloc(s, s_alloc + 1);
            s[s_alloc - 1] = ' ';
            s[s_alloc] = '\0';
            s_alloc += 1;
        } else {
            s = realloc(s, s_alloc + start->length);
            strncpy(s + s_alloc - 1, start->data, start->length);
            s_alloc += start->length;
            s[s_alloc - 1] = '\0';
        }

        start = start->next;
    }

    return s;
}

struct keyval_node * keyval_parse_comment(struct keyval_token ** token_) {
    struct keyval_token * token = *token_;
    struct keyval_token * first = token;

    while (token) {
        if (token->flags == KEYVAL_TOKEN_SEPARATOR) {
            if (*token->data == '\n') {
                /* the end! */
                struct keyval_node * node = calloc(1, sizeof(struct keyval_node));
                node->comment = keyval_tokens_to_string(first, token->prev);
                *token_ = token;
                return node;
            }
        }

        if (!token->next) {
            /* trailing null means we're at the end of a file... */
            struct keyval_node * node = calloc(1, sizeof(struct keyval_node));
            node->comment = keyval_tokens_to_string(first, token);
            *token_ = token;
            return node;
        }
        token = token->next;
    }

    return 0;
}

struct keyval_node * keyval_parse_list(struct keyval_token ** token_) {
    struct keyval_token * token = *token_;
    struct keyval_token * last = token;
    struct keyval_node * head = 0;

    while (token) {
        if (token->flags == KEYVAL_TOKEN_SEPARATOR) {
            switch (*token->data) {
            case ']':
            {
                /* same as below, except we return */
                struct keyval_node * node = calloc(1, sizeof(struct keyval_node));
                node->value = keyval_tokens_to_string(last, token->prev);
                head = keyval_node_append(head, node);
                *token_ = token;
                return head;
            }
            break;
            case ',':
            {
                struct keyval_node * node = calloc(1, sizeof(struct keyval_node));
                node->value = keyval_tokens_to_string(last, token->prev);
                head = keyval_node_append(head, node);
                last = token = token->next;
            }
            default:
                token = token->next;
                break;
            }
        } else token = token->next;
    }

    /* ran out of tokens... error */

    if (head) keyval_node_free_all(head);

    return 0;
}

struct keyval_node * keyval_parse_value(struct keyval_token ** token_) {
    struct keyval_token * token = *token_;
    struct keyval_token * first = token;

    while (token) {
        if (token->flags == KEYVAL_TOKEN_SEPARATOR) {
            switch (*token->data) {
            case '[':
                if (token->next) {
                    struct keyval_node * node = calloc(1, sizeof(struct keyval_node));
                    token = token->next;
                    node->children = keyval_parse_list(&token);
                    if (!node->children) {
                        free(node);
                        return 0;
                    }
                    *token_ = token;
                    return node;
                }
            case '#':
            {
                /* the value */
                struct keyval_node * node;
                struct keyval_node * comment;
                if (first == token->prev) return 0;

                node = calloc(1, sizeof(struct keyval_node));
                node->value = keyval_tokens_to_string(first, token->prev);
                /* and the comment */
                token = token->next;
                comment = keyval_parse_comment(&token);
                node->comment = comment->comment;
                free(comment);
                *token_ = token;
                return node;
            }
            case '\n':
            case '}':
            {
                /* the end! */
                struct keyval_node * node;
                node = calloc(1, sizeof(struct keyval_node));
                node->value = keyval_tokens_to_string(first, token->prev);
                if (!node->value) {
                    free(node);
                    return 0;
                }
                *token_ = token;
                return node;
            }
            break;
            default: break;
            }
        }

        if (!token->next) {
            /* trailing null means we're at the end of a file... */
            struct keyval_node * node = calloc(1, sizeof(struct keyval_node));
            node->value = keyval_tokens_to_string(first, token);
            *token_ = token;
            return node;
        }
        token = token->next;
    }

    return 0;
}

struct keyval_node * keyval_parse_section(struct keyval_token ** token_) {
    struct keyval_token * token = *token_;
    struct keyval_token * last = token;
    struct keyval_node * head_node = calloc(1, sizeof(struct keyval_node));

    while (token) {
        if (token->flags == KEYVAL_TOKEN_SEPARATOR) {
            switch (*token->data) {
            case '{':
                /* section start */
            {
                struct keyval_node * node;
                struct keyval_token * t = token;
                char * name = 0;
                token = token->next;
                node = keyval_parse_section(&token);

                name = keyval_tokens_to_string(last, t->prev);
                if (!node) {
                    if (name) keyval_append_error_va("keyval: error: (in section `%s')\n", name);
                    keyval_node_free_all(head_node);
                    return 0;
                }

                if (!token || !(token->flags == KEYVAL_TOKEN_SEPARATOR && *token->data == '}')) {
                    /* error! zomg! */
                    keyval_append_error_va("keyval: error: section `%s` never closed\nkeyval: error: (declared on line %d)\n", node->name, t->line);
                    keyval_node_free_all(head_node);
                    keyval_node_free_all(node);
                    return 0;
                }

                if (t->next == token && !node->name) {
                    /* anonymous empty section, an error */
                    keyval_append_error_va("keyval: error: anonymous, empty section\nkeyval: error: (declared on line %d)\n", t->line);
                    keyval_node_free_all(head_node);
                    keyval_node_free_all(node);
                    return 0;
                }

                node->name = name;
                head_node->children = keyval_node_append(head_node->children, node);
                last = token = token->next;
            }
            break;
            case '}':
                /* section end */
                *token_ = token;
                return head_node;
                break;
            case '#':
                /* a comment node */
            {
                struct keyval_node * node;
                if (token->next) {
                    token = token->next;
                    node = keyval_parse_comment(&token);
                    head_node->children = keyval_node_append(head_node->children, node);
                    last = token->next;
                }
            }
            break;
            case '=':
                /* a key-value pair
                 * everything since the last newline is the key */
            {
                struct keyval_node * node;
                struct keyval_token * t = token;
                char * name = keyval_tokens_to_string(last, t->prev);
                if (token->next) {
                    token = token->next;
                    node = keyval_parse_value(&token);
                    if (!node) goto no_value;
                    node->name = name;
                    head_node->children = keyval_node_append(head_node->children, node);
                    last = token->next;
                } else {
                    /* error! */
                no_value:
                    keyval_node_free_all(head_node);
                    keyval_append_error_va("keyval: error: expecting value after `%s =`\nkeyval: error: (on line %d)\n", name, t->line);
                    free(name);
                    return 0;
                }
                break;
            }
            default:
                token = token->next;
                break;
            }
        } else token = token->next;
    }

    *token_ = token;
    return head_node;
}

struct keyval_node * keyval_parse_file(const char * filename) {
    char buf[PICESIZE];
    FILE * file = fopen(filename, "r");
    char * data;

    struct keyval_node * node;

    if (!file) {
        keyval_append_error_va("keyval: error: could not open file `%s'\n", filename);
        return 0;
    }

    data = calloc(PICESIZE, sizeof(char));

    while (fgets(buf, PICESIZE, file)) {
        data = realloc(data, strlen(data) + strlen(buf) + 1);
        data = strcat(data, buf);
    }

    fclose(file);

    node = keyval_parse_string(data);
    free(data);

    return node;
}

struct keyval_node * keyval_parse_string(const char * data) {
    struct keyval_token * token = keyval_tokenize(data, "{}#=\n[],");
    struct keyval_token * token_old = token;
    struct keyval_node * node = keyval_parse_section(&token);

    keyval_token_free_all(token_old);

    return node;
}
