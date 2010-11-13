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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pathtree.h"

static int failedtests = 0;
static int passedtests = 0;

void test_add(Node **root, char *path, int expected_result)
{
    char buf[1024];
    int testresult;
    Node *foo;
    if ((testresult = node_add(root, path, &foo)) != expected_result)
    {
        printf("[ FAIL ] (Expected %d, got %d (node '%s')", expected_result, testresult, path);
        failedtests++;
    } else {
        printf("[ PASS ] (Expected %d, got %d (node '%s')", expected_result, testresult, path);
        passedtests++;
    }
    if (testresult) {
        node_pathncpy(buf, foo, sizeof(buf));
        printf(", Node at: '%s'", buf);
    }
    puts("");
}

int main()
{
    puts("Running tests\n");

    Node *root = NULL;

    char t9[] = ""; //empty root
    test_add(&root, t9, 0);

    char t1[] = "/";
    test_add(&root, t1, 1);
    
    char t2[] = "/foo/bar/baz";
    test_add(&root, t2, 3);
    
    char t3[] = "/foo/bar/baz";
    test_add(&root, t3, 0);

    char t13[] = "/foo////bar/baz"; //extra slashes should be ignored
    test_add(&root, t13, 0);
 
    char t14[] = "/foo/bar/baz///"; //extra slashes should be ignored
    test_add(&root, t14, 0);
   
    char t15[] = "////foo/bar/baz"; //extra slashes should be ignored
    test_add(&root, t15, 0);
    
    char t4[] = "/foo/lol/bar";
    test_add(&root, t4, 2);
 
    char t5[] = "/foo/lol/baz";
    test_add(&root, t5, 1);
 
    char t6[] = "/foo/lol/baz/drool/c/on/kivaa/jos/et/sita/viela/tiennyt/ja/niinhan/se/taitaa/olla";
    test_add(&root, t6, 14);

    char t7[] = "foo"; //duplicate root
    test_add(&root, t7, 0);

    char t10[] = "/foo/"; //duplicate node with an extra slash
    test_add(&root, t10, 0);
 
    char t11[] = "/foo/lol/baz2/"; // extra slash
    test_add(&root, t11, 1);

    Node *root2 = NULL;

    char t12[] = "//"; // extra slashes should be trimmed
    test_add(&root2, t12, 1);

    Node *root3 = NULL;
    char t16[] = "different_root_name";
    test_add(&root3, t16, 1);

    char t17[] = "/"; //root already set
    test_add(&root3, t17, 0);

    char t18[] = "foo"; //root already set
    test_add(&root3, t18, 0);
    
    char t19[] = "different_root_name/next_path";
    test_add(&root3, t19, 1);
    
    Node *root4 = NULL;
    char t20[] = "/root/and/more/at/first";
    test_add(&root4, t20, 6);
    
    puts("\n============================================================");
    printf("Tests completed. Passed %d (%.2f%%), failed %d.\n", 
            passedtests, 
            ((float)passedtests/(passedtests+failedtests))*100, 
            failedtests);
    puts("============================================================");

    node_delete(&root);
    node_delete(&root2);
    node_delete(&root3);
    node_delete(&root4);
    
    return 0;
}
