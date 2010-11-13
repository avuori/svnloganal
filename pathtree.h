/*
 * svnloganal - Reads Subversion logs and writes statistics in xml format.
 * Copyright (C) 2007  Arto Vuori
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#define PATH_SEPARATOR "/"
#include <stdbool.h>

typedef struct node {
    char *path;
    struct node *parent;
    struct node *sibling;
    struct node *children;
    void *custom; // place anything you want here.
} Node;

Node *node_create(char *path, Node *parent, int *created);
int node_add(Node **n, char *path, Node **far_end);
Node *find_spot(Node *n, char **path, bool force_get_tail);
void node_delete(Node **n);
void test_add(Node **root, char *path, int expected_result);
int node_pathncpy(char *buf, Node *n, size_t count);
