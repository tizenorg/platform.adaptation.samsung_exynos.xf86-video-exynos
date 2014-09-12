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

#ifndef _BOOL_EXP_BINTREE_H_
#define _BOOL_EXP_BINTREE_H_

typedef struct _BINARY_TREE_NODE * BINARY_TREE_NODE;
typedef struct _BINARY_TREE * BINARY_TREE;
typedef int (*BINTREE_TRAVERSE_FUNC) (BINARY_TREE tree, BINARY_TREE_NODE node, BINARY_TREE_NODE parent, void * arg);

BINARY_TREE bintree_create_tree (int data_size);

BINARY_TREE_NODE bintree_create_node (BINARY_TREE tree);

BINARY_TREE_NODE bintree_get_head (BINARY_TREE tree);

void bintree_set_head (BINARY_TREE tree, BINARY_TREE_NODE head);

void bintree_set_left_child (BINARY_TREE_NODE node, BINARY_TREE_NODE child);

void bintree_set_right_child (BINARY_TREE_NODE node, BINARY_TREE_NODE child);

BINARY_TREE_NODE bintree_get_left_child (BINARY_TREE_NODE node);

BINARY_TREE_NODE bintree_get_right_child (BINARY_TREE_NODE node);

void * bintree_get_node_data (BINARY_TREE_NODE node);

void bintree_remove_node (BINARY_TREE_NODE node);

void bintree_remove_node_recursive (BINARY_TREE_NODE node);

void bintree_destroy_tree (BINARY_TREE tree);

void bintree_inorder_traverse (BINARY_TREE tree, BINTREE_TRAVERSE_FUNC func, void * arg);

void bintree_postorder_traverse (BINARY_TREE tree, BINTREE_TRAVERSE_FUNC func, void * arg);

#endif /* _BOOL_EXP_BINTREE_H_ */

