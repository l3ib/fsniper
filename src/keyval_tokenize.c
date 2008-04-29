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
#include "keyval_tokenize.h"
#include <string.h>
#include <stdlib.h>

static unsigned char is_space(char c) {
    return (c == ' ') || (c == '\t');
}

static struct keyval_token * keyval_token_append(struct keyval_token * head,
                                                 struct keyval_token * token) {

    if (head) {
        while (head->next) head = head->next;
        token->prev = head;
        token->head = head->head;
        head->next = token;
    } else {
        token->next = token->prev = 0;
        token->head = token;
        head = token;
    }

    return head->head;
}

struct keyval_token * keyval_tokenize(const char * s, char * separators) {
    const char * last = s; /* last non-separating non-space character */
    struct keyval_token * token = 0;
    size_t line = 1;
    while (*s) {
        enum keyval_token_flags flags = KEYVAL_TOKEN_NORMAL;
        const char * split_point = 0;
        size_t length = 0;

        if (*s == '\n') line++;

        if (*s == '\\') {
            /* take stuff to the left of this character */
            struct keyval_token * a = calloc(1, sizeof(struct keyval_token));
            a->data = last;
            a->length = s - last;
            a->flags = KEYVAL_TOKEN_NORMAL;
            a->line = line;

            if (token) keyval_token_append(token, a);
            else token = keyval_token_append(token, a);

            s += 2;
            last = s - 1;
            if (*last == '\n') line++;
            continue;
        }

        if (strchr(separators, *s)) {
            split_point = s;
            flags = KEYVAL_TOKEN_SEPARATOR;
            length = 1;
        } else if (is_space(*s)) {
            split_point = s;
            flags = KEYVAL_TOKEN_WHITESPACE;
            /* how much whitespace is there? */
            while (is_space(s[length])) length++;
        }

        /* all the stuff from last up to (but not including) *s should be in its
         * own token. the character *s should be in its own token. */

        if (split_point) {
            struct keyval_token * a = 0;
            struct keyval_token * b = calloc(1, sizeof(struct keyval_token));

            if (s - last) {
                /* only if this is not the first character */
                a = calloc(1, sizeof(struct keyval_token));
                a->data = last;
                a->length = s - last;
                a->flags = KEYVAL_TOKEN_NORMAL;
                a->line = line;
            }

            b->data = s;
            b->flags = flags;
            b->length = length;
            b->line = line;

            last = s + length;

            /* trivial linked list manipulation */

            if (a) {
                if (token) keyval_token_append(token, a);
                else token = keyval_token_append(token, a);
                token = a;
            }

            if (token) keyval_token_append(token, b);
            else token = keyval_token_append(token, b);
            token = b;

            if (flags == KEYVAL_TOKEN_WHITESPACE) s += length - 1;
        }
        s++;
    }

    if (s - last) {
        /* some stuff at the end */
        struct keyval_token * a = calloc(1, sizeof(struct keyval_token));
        a->data = last;
        a->length = s - last;
        a->flags = KEYVAL_TOKEN_NORMAL;
        if (token) {
            a->head = token->head;
            token->next = a;
        } else a->head = a;
    }

    return token->head;
}

void keyval_token_free_all(struct keyval_token * token) {
    if (token->next) keyval_token_free_all(token->next);
    free(token);
}
