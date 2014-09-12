/**************************************************************************

xdbg

Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved

Contact: Boram Park <boram1288.park@samsung.com>
         Sangjin LEE <lsj119@samsung.com>

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xdbg.h>

#include "xdbg_module_types.h"

/* Supported options */
typedef enum
{
    OPTION_LOG_PATH,
    OPTION_EVLOG_PATH,
} ModuleOption;

static const OptionInfoRec module_options[] =
{
    { OPTION_LOG_PATH,		"log_path",			OPTV_STRING,	{0},	FALSE },
    { OPTION_EVLOG_PATH,	"evlog_path",		OPTV_STRING,	{0},	FALSE },
    { -1,				NULL,				OPTV_NONE,		{0},	FALSE }
};

void
xDbgModuleParseOptions (XDbgModule *pMod, XF86OptionPtr pOpt)
{
    OptionInfoPtr options = xnfalloc (sizeof (module_options));
    char *log_path;

    memcpy (options, module_options, sizeof(module_options));

    xf86ProcessOptions (-1, pOpt, options);

    /* log_path */
    log_path = xf86GetOptValString (options, OPTION_LOG_PATH);
    if (log_path)
    {
        XDBG_SLOG (MXDBG, "drv's log path: \"%s\"\n", log_path);
        pMod->log_path = strdup (log_path);
    }
    else
        XDBG_SLOG (MXDBG, "drv's log path: none\n");

    /* evlog_path */
    log_path = xf86GetOptValString (options, OPTION_EVLOG_PATH);
    if (log_path)
    {
        XDBG_SLOG (MXDBG, "evlog path: \"%s\"\n", log_path);
        pMod->evlog_path = strdup (log_path);
    }
    else
        XDBG_SLOG (MXDBG, "evlog path: none\n");

    free (options);
}
