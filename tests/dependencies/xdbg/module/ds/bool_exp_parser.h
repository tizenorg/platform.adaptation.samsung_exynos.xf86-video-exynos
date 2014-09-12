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

#ifndef _BOOL_EXP_PARSER_H_
#define _BOOL_EXP_PARSER_H_

#include "bintree.h"

#define STRING_MAX	64

typedef enum { NONE, AND, OR, DATA, ALL } NODE_TYPE;
typedef enum { EQUAL, LESS, GREATER, LESS_EQ, GREATER_EQ, NOT_EQ } COMPARER;
typedef enum { INTEGER, STRING } DATA_TYPE;

typedef struct _PARSE_DATA * PARSE_DATA;

struct _PARSE_DATA
{
    NODE_TYPE node_type;

    char variable_name[STRING_MAX];
    COMPARER compare;
    DATA_TYPE value_type;
    union
    {
        char string[STRING_MAX];
        int integer;
    } value;

    enum { BEP_UNKNOWN, BEP_TRUE, BEP_FALSE } result;
};

BINARY_TREE bool_exp_parse (const char * string);

#endif /* _BOOL_EXP_PARSER_H_ */

