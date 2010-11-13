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
 *
 *
 *
 *
 * This data structure contains filepaths.
 *
 * Every node has a parent (except the root).
 * Node might have siblings.
 *
 * A visual example is shown below
 *
 * <pre>
 *          O----
 *         / \   \  
 *        O---O---O---------+
 *       /\   |    \    |   |
 *      O--O  |     O---O---O
 *            |      \   \
 *          +---+     O   O
 *          |   |     |
 *          O---O     O
 * </pre>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pathtree.h"
#include "stack.h"

Node *node_create(char *path, Node *parent, int *created)
{
    if (!strlen(path))
    {
        return parent;
    }

//    printf("pre: %s\n", path);
    // handle possible extra slashes from root
    if (*path == *PATH_SEPARATOR)
        while (*(path+1) == *PATH_SEPARATOR)
            path++;

    Node *new;
    new = malloc(sizeof(struct node));
    if (new == NULL)
    {
        fprintf(stderr, "Malloc failed\n");
        return NULL;
    }
//    printf("%s\n", path);
    
    char pathcopy[strlen(path)+1];
    char *pathtoken;
    if (*path != *PATH_SEPARATOR
        && strcmp(path, PATH_SEPARATOR))
    {
        strcpy(pathcopy, path);
        pathtoken = strtok(pathcopy, PATH_SEPARATOR);
    } else {
        pathtoken = PATH_SEPARATOR;
    }
    new->path = malloc(strlen(pathtoken)+1);
    if (new->path == NULL)
    {
        fprintf(stderr, "Malloc failed\n");
        free(new);
        return NULL;
    }
    strcpy(new->path, pathtoken);
    new->parent = parent;
    new->children = NULL;
    new->sibling = NULL;
    new->custom = NULL;
    
    if (parent)
    {
        Node *sibling = parent->children;
        if (sibling)
        {
            while (sibling->sibling != NULL)
            {
                sibling = sibling->sibling;
            }
            sibling->sibling = new;
        } else {
            parent->children = new;
        }
    }
    
    (*created)++;

    if (*path == *PATH_SEPARATOR)
    {
        //root part
        path++; //TODO static separator size shouldn't be used.
    } else if (strpbrk(path, PATH_SEPARATOR) == NULL) {
        //file part
        path = "";
    } else {
        // middle part
        path += strlen(pathtoken) + sizeof(PATH_SEPARATOR) -1;
    }

    return node_create(path, new, created);
}

int node_add(Node **n, char *path, Node **far_end)
{
//   printf("[NODE ADD] '%s'\n", path);
    Node *ret = NULL;
    int created = 0;
    if (*n == NULL)
    {
        // attach root node to given pointer
        Node *new = node_create(path, NULL, &created);
        ret = new;
        while (new && new->parent)
            new = new->parent;
        *n = new;
    } else {
        char pathcopy[strlen(path)+1];
        strcpy(pathcopy, path);
        char *p = pathcopy;
        Node *parent = find_spot(*n, &path, false);
        if (parent != NULL)
        {
  //          printf("[CREATE NEED]\n");
            ret = node_create(path, parent, &created);
        } else {
//            printf("[CREATE EJ]\n");
            // node was already in place
            ret = find_spot(*n, &p, true);
//              ret = NULL;
        }
  //      printf("[HANDLED %d] '%s'\n", created, ret->path);
    }
    *far_end = ret;
    return created;
}

Node *find_spot(Node *n, char **path, bool force_get_tail)
{   
//    printf("[FIND SPOT] '%s'\n", *path);
    char *token;
    char pathcopy[strlen(*path)+1];
    strcpy(pathcopy, *path);
    
    token = strtok(pathcopy, PATH_SEPARATOR);
//    printf("[FIND SPOT] IF **path('%s', '%c')==PATH_SEPARATOR\n", *path, **path);
    if (**path == *PATH_SEPARATOR)
    {
        while (**path == *PATH_SEPARATOR)
            (*path)++;
    } else {
        if (strstr(*path, PATH_SEPARATOR)) {
            *path += strlen(token) + sizeof(PATH_SEPARATOR) - 1;
//            printf("[FIND SPOT] WHILE **path('%s')== *PATH_SEPARATOR\n", *path);
             while (**path == *PATH_SEPARATOR)
             {
                (*path)++;
  //              printf("[FIND SPOT] WHILE **path('%s', '%c')== *PATH_SEPARATOR\n", *path, **path);
             }
        }
      token = strtok(NULL, PATH_SEPARATOR);
    }
    
  //  printf("token='%s'\n", token);
   
//   printf("[FIND SPOT] IF token==NULL\n");
   if (token == NULL)
   {
  //     printf("[FIND SPOT]      token == NULL\n");
       // node is already in the tree
    //   printf("[OLD] node %s\n", n->path);
       return force_get_tail ? n : NULL;
   }
//   printf("[FIND SPOT]      token != NULL\n");
   Node *child = n->children;
   while (child != NULL && strcmp(child->path, token))
   {
       child = child->sibling;
   }
   if (child != NULL)
   {
       return find_spot(child, path, force_get_tail);
   }
   return n;
}

void node_delete(Node **n)
{
    if (n == NULL)
    {
        return;
    }
    while ((*n)->children != NULL)
    {
        Node *first_child = (*n)->children;
        Node **c = &((*n)->children);
        bool firstdeleted = true;
        Node *sibling = NULL;
        while ((*c)->sibling != NULL)
        {
            sibling = *c;
            *c = (*c)->sibling;
            firstdeleted = false;
        }
        node_delete(c);
        if (sibling)
        {
            sibling->sibling = NULL;
        }
        
        if (!firstdeleted)
        {
            (*n)->children = first_child;
        }
    }
//    printf("[NODE DELETE] %s\n", (*n)->path);
    free((*n)->path);
    free((*n)->custom);
    free(*n);
    *n = NULL;
}

int node_pathncpy(char *buf, Node *n, size_t count) {

    if (count < 1) {
        return 0;
    }

    char *p;
    size_t length = 0;
    int tokens = 0;
    c_stack s;
    init_c_stk(&s);

    do {
        length += strlen(n->path);
        tokens++;
        if (push_c_stk(n->path, &s) == 1) {
            // memory full
            break;
        }
        n = n->parent;
    } while(n);

    if (length == 0) {
        *buf = '\0';
        return 1;
    }

    char tmp[length + tokens*strlen(PATH_SEPARATOR) + 2];
    char *tmp_p = tmp;
    *tmp_p = '\0';
    while ((p = (char*)pop_c_stk(&s))) {
//        printf("POP: '%s'\n", p);
        int tokenLen = strlen(p);
        strcat(tmp_p, p);
        tmp_p += tokenLen;
        if (*p != *PATH_SEPARATOR) {
            *tmp_p++ = *PATH_SEPARATOR;
        }
        *tmp_p = '\0';
    }
    //remove trailing PATH_SEPARATOR
    *(tmp_p-1) = '\0';

    strncpy(buf, tmp, count);
    return 1;
}
