/**************************************************************************

xserver-xorg-video-exynos

Copyright 2010 - 2011 Samsung Electronics co., Ltd. All Rights Reserved.

Contact: Boram Park <boram1288.park@samsung.com>

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "bintree.h"

struct _BINARY_TREE_NODE
{
    BINARY_TREE_NODE left;
    BINARY_TREE_NODE right;
};

struct _BINARY_TREE
{
    int size;
    BINARY_TREE_NODE head;
};

BINARY_TREE bintree_create_tree (int size)
{
    BINARY_TREE tree = calloc (sizeof (struct _BINARY_TREE) + size, 1);

    tree->size = size;
    tree->head = NULL;

    return tree;
}

BINARY_TREE_NODE bintree_create_node (BINARY_TREE tree)
{
    BINARY_TREE_NODE node = calloc (sizeof (struct _BINARY_TREE_NODE) + tree->size, 1);

    node->left = NULL;
    node->right = NULL;

    return node;
}

BINARY_TREE_NODE bintree_get_head (BINARY_TREE tree)
{
    return tree->head;
}

void bintree_set_head (BINARY_TREE tree, BINARY_TREE_NODE head)
{
    tree->head = head;
}

void bintree_set_left_child (BINARY_TREE_NODE node, BINARY_TREE_NODE child)
{
    node->left = child;
}

void bintree_set_right_child (BINARY_TREE_NODE node, BINARY_TREE_NODE child)
{
    node->right = child;
}

BINARY_TREE_NODE bintree_get_left_child (BINARY_TREE_NODE node)
{
    return node->left;
}

BINARY_TREE_NODE bintree_get_right_child (BINARY_TREE_NODE node)
{
    return node->right;
}

void * bintree_get_node_data (BINARY_TREE_NODE node)
{
    return (void*)(node + 1);
}

void bintree_remove_node (BINARY_TREE_NODE node)
{
    free (node);
}

void bintree_remove_node_recursive (BINARY_TREE_NODE node)
{
    if (node->left)
        bintree_remove_node_recursive (node->left);
    if (node->right)
        bintree_remove_node_recursive (node->right);

    bintree_remove_node (node);
}

void bintree_destroy_tree (BINARY_TREE tree)
{
    if (tree->head)
        bintree_remove_node_recursive (tree->head);

    free (tree);
}

static int bintree_inorder_traverse_recursive (BINARY_TREE tree, BINARY_TREE_NODE node, BINARY_TREE_NODE parent, BINTREE_TRAVERSE_FUNC func, void * arg)
{
    if (node->left)
        if (bintree_inorder_traverse_recursive (tree, node->left, node, func, arg) != 0)
            return 1;

    if (func (tree, node, parent, arg))
        return 1;

    if (node->right)
        if (bintree_inorder_traverse_recursive (tree, node->right, node, func, arg) != 0)
            return 1;

    return 0;
}

void bintree_inorder_traverse (BINARY_TREE tree, BINTREE_TRAVERSE_FUNC func, void * arg)
{
    if (tree->head)
        bintree_inorder_traverse_recursive (tree, tree->head, tree->head, func, arg);
}

static int bintree_postorder_traverse_recursive (BINARY_TREE tree, BINARY_TREE_NODE node, BINARY_TREE_NODE parent, BINTREE_TRAVERSE_FUNC func, void * arg)
{
    if (node->left)
        if (bintree_postorder_traverse_recursive (tree, node->left, node, func, arg) != 0)
            return 1;
    if (node->right)
        if (bintree_postorder_traverse_recursive (tree, node->right, node, func, arg) != 0)
            return 1;

    return func (tree, node, parent, arg);
}

void bintree_postorder_traverse (BINARY_TREE tree, BINTREE_TRAVERSE_FUNC func, void * arg)
{
    if (tree->head)
        bintree_postorder_traverse_recursive (tree, tree->head, tree->head, func, arg);
}

