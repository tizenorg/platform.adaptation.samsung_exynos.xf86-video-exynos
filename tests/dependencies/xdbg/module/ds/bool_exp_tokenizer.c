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

#include <string.h>
#include <ctype.h>

#include "bool_exp_tokenizer.h"

//#define debugf printf
#define debugf(...)

// should be increasing order for binary search
static struct
{
    const char * token_char;
    const int token_length;
    TOKEN token_name;
} token_table[] =
{
    { "\0",	1, BET_EOS }, // 0
    { "\t",	1, BET_SPACE }, // 9
    { " ",	1, BET_SPACE }, // 32
    { "!=",	2, BET_NOT_EQ }, // 33 61
    { "&",	1, BET_AND }, // 38
    { "&&",	2, BET_AND }, // 38 38
    { "(",	1, BET_L_BR }, // 40
    { ")",	1, BET_R_BR }, // 41
    { "<",	1, BET_LSS_THAN }, // 60
    { "<=",	2, BET_LSS_EQ }, // 60 61
    { "<>",	2, BET_NOT_EQ }, // 60 62
    { "=",	1, BET_EQUAL }, // 61
    { "==",	2, BET_EQUAL }, // 61 61
    { ">",	1, BET_GRT_THAN }, // 62
    { ">=",	2, BET_GRT_EQ }, // 62 61
    { "and",3, BET_AND }, // 97 110
    { "or",	2, BET_OR }, // 111 114
    { "|",	1, BET_OR }, // 124
    { "||",	2, BET_OR }, // 124 124
};

TOKEN get_next_token (const char ** string)
{
    static int token_cnt = sizeof (token_table) / sizeof (token_table[0]);
    int i;
    int first = 0;
    int last = token_cnt - 1;
    int compare_res;
    int found = 0;

    i = (first + last) / 2;
    while (1)
    {
        compare_res = strncmp (*string, token_table[i].token_char, token_table[i].token_length);
        debugf ("string [%s] token[%s] res = %d i=%d, first=%d, last=%d\n", *string, token_table[i].token_char, compare_res, i, first, last);
        while (compare_res == 0)
        {
            found = 1;
            i++;
            if (i == token_cnt)
                break;
            compare_res = strncmp (*string, token_table[i].token_char, token_table[i].token_length);
            debugf ("string [%s] token[%s] res = %d i=%d, first=%d, last=%d\n", *string, token_table[i].token_char, compare_res, i, first, last);
        }
        if (found)
        {
            i--;
            *string += token_table[i].token_length;

            return token_table[i].token_name;
        }

        if (first >= last)
            break;

        if (compare_res > 0)
        {
            first = i + 1;
        }
        else
        {
            last = i - 1;
        }
        i = (first + last) / 2;
    }
    if (isalpha (**string))
    {
        (*string) ++;
        while (isalpha (**string) || isdigit (**string) || **string == '_' || **string == '-')
            (*string) ++;
        return BET_SYMBOL;
    }
    if (isdigit (**string))
    {
        (*string) ++;
        while (isdigit (**string))
            (*string) ++;
        return BET_NUMBER;

    }

    return BET_UNKNOWN;
}
