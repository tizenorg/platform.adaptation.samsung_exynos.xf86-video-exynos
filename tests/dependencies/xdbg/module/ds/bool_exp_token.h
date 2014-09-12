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

#ifndef _BOOL_EXP_TOKEN_H_
#define _BOOL_EXP_TOKEN_H_

typedef enum
{
    BET_UNKNOWN = 0,
    BET_L_BR = 1,
    BET_R_BR = 2,
    BET_NOT_EQ = 3,
    BET_EQUAL = 4,
    BET_LSS_THAN = 5,
    BET_LSS_EQ = 6,
    BET_GRT_THAN = 7,
    BET_GRT_EQ = 8,
    BET_AND = 9,
    BET_OR = 10,
    BET_SPACE = 11,
    BET_SYMBOL = 12,
    BET_NUMBER = 13,
    BET_EOS = 14,
} TOKEN;

#endif /* _BOOL_EXP_TOKEN_H_ */

