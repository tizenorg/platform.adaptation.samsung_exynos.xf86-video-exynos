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

#ifndef _BOOL_EXP_RULE_CHECKER_H_
#define _BOOL_EXP_RULE_CHECKER_H_

typedef enum { UNDEFINED, ALLOW, DENY } POLICY_TYPE;

typedef enum { RC_OK, RC_ERR_TOO_MANY_RULES, RC_ERR_PARSE_ERROR, RC_ERR_NO_RULE } RC_RESULT_TYPE;

typedef struct _RULE_CHECKER * RULE_CHECKER;

RULE_CHECKER rulechecker_init();

void rulechecker_destroy (RULE_CHECKER rc);

RC_RESULT_TYPE rulechecker_add_rule (RULE_CHECKER rc, POLICY_TYPE policy, const char * rule_string);

RC_RESULT_TYPE rulechecker_remove_rule (RULE_CHECKER rc, int index);

void rulechecker_print_rule (RULE_CHECKER rc, char * rules_buf);

const char * rulechecker_print_usage (void);

int rulechecker_validate_rule (RULE_CHECKER rc, int direct, int reqID, const char * name, int pid, char * cmd);

#endif /* _BOOL_EXP_RULE_CHECKER_H_ */

