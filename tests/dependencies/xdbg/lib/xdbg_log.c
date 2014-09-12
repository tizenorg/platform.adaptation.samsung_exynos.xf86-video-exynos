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

#include <string.h>
#include <stdarg.h>

#include "xdbg_log.h"

#ifndef API
#define API __attribute__ ((visibility("default")))
#endif

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#define MAX_MODULE_NAME	4
#define MAX_MODULE_CNT	256
#define BUF_LEN         1024

typedef struct
{
    unsigned int module;
    char *name;
    int   loglevel;
} ModuleInfo;

static ModuleInfo modules[MAX_MODULE_CNT];
static int module_cnt = 0;

extern void dLogWrapper (int loglevel, int is_module, const char * file, int line, const char * f, va_list args);
extern void kLogWrapper (int loglevel, int is_module, const char * file, int line, const char * f, va_list args);
static char* _LogGetName (unsigned int module);

static void*
_LogInitModule (unsigned int module, int loglevel)
{
    if (module_cnt >= MAX_MODULE_CNT)
        return NULL;

    modules[module_cnt].module = module;
    modules[module_cnt].name = _LogGetName (module);
    modules[module_cnt].loglevel = loglevel;
    module_cnt++;

    return &modules[module_cnt-1];
}

static void
_LogModule (void * handle, int logoption, const char * file, int line, const char * f, va_list args)
{
    ModuleInfo *h = (ModuleInfo*)handle;
    char *ostr[XLOG_LEVEL_MAX] = {"DD", "TT", "II", "WW", "EE"};
    char tmpBuf[BUF_LEN];
    int loglevel = logoption & XLOG_MASK_LOGLEVEL;
    const char *name;

    if (!h)
        return;

    name = h->name;

    if (logoption & XLOG_OPTION_KLOG)
    {
        snprintf(tmpBuf, BUF_LEN, "[%s]%s", (name)?name:"", f);
        kLogWrapper (loglevel, logoption, file, line, tmpBuf, args);
    }

    /* write to file */
    if (loglevel >= XLOG_LEVEL_INFO)
    {
        snprintf(tmpBuf, BUF_LEN, "(%s) [%s]%s", ostr[loglevel], (name)?name:"", f);

        LogVWrite (1, tmpBuf, args);
    }

    /* write to terminal */
    if (loglevel >= h->loglevel || logoption & XLOG_OPTION_SLOG)
    {
        char *buf = tmpBuf;
        int   remain = BUF_LEN;
        int   len = 0;

        len = snprintf (buf, remain, "[%s]", name?name:"");
        buf += len;
        remain -= len;

        len += vsnprintf (buf, remain, f, args);

        fwrite(tmpBuf, len, 1, stderr);
    }
}

API int
xDbgLogSetLevel (unsigned int module, int level)
{
    int i;
    ModuleInfo * h;

    if (level < XLOG_LEVEL_0 || level > XLOG_LEVEL_4)
        return FALSE;

    for (i = 0; i < module_cnt; i++)
    {
        if (module == modules[i].module)
        {
            modules[i].loglevel = level;
            return TRUE;
        }
    }

    h = _LogInitModule (module, level);
    if (h == NULL)
        return FALSE;

    return TRUE;
}

API void*
xDbgLog (unsigned int module, int logoption, const char * file, int line, const char * f, ...)
{
    int loglevel = logoption & XLOG_MASK_LOGLEVEL;
    ModuleInfo *h;
    va_list args;
    int i;

    if (module == 0)
        return NULL;

    if (loglevel < XLOG_LEVEL_0 || loglevel > XLOG_LEVEL_4)
        return NULL;

    for (i = 0; i < module_cnt; i++)
    {
        h = &modules[i];
        if (module == h->module)
            goto check_level;
    }

    h= (ModuleInfo *)_LogInitModule (module, XLOG_LEVEL_DEFAULT);
    if(h == NULL)
        return NULL;

check_level:
    if (logoption & (XLOG_OPTION_KLOG | XLOG_OPTION_SLOG))
        goto write_log;

    if (loglevel < XLOG_LEVEL_INFO && loglevel < h->loglevel)
        return h;

write_log:
    va_start (args, f);
    _LogModule (h, logoption, file, line, f, args);
    va_end (args);

    return h;
}

API int
xDbgLogEnumModules (LOG_ENUM_MODE mode, char *buf, int *remain)
{
    int i, len;
    char *p = buf;

    switch (mode)
    {
    case MODE_NAME_ONLY:
        for (i = 0; i < module_cnt && *remain > 0; i++)
        {
            len = snprintf (p, *remain, "%s", modules[i].name);
            p += len;
            *remain -= len;

            if (i != module_cnt - 1 && *remain > 0)
            {
                len = snprintf (p, *remain, "/");
                p += len;
                *remain -= len;
            }
        }
        break;
    case MODE_WITH_STATUS:
        for (i = 0; i < module_cnt && *remain > 0; i++)
        {
            len = snprintf (p, *remain, "   %12s:%d\n", modules[i].name, modules[i].loglevel);
            p += len;
            *remain -= len;
        }
        break;
    default:
        return 0;
    }

    return p - buf;
}

static char*
_LogGetName (unsigned int module)
{
    char *name = malloc (MAX_MODULE_NAME+1);
    char *p = name;
    int i;

    if (!name)
        return NULL;

    name[0] = '\0';

    for (i = 0; i < MAX_MODULE_NAME; i++)
    {
        if (!_C(module, (i<<3)))
            break;

        *p = _C(module, (i<<3));
        p++;
    }

    *p = '\0';

    return name;
}

API unsigned int
xDbgLogGetModule (char *name)
{
    unsigned int module = 0;
    int i, len;

    if (!name)
        return 0;

    len = strlen (name);
    for (i = 0; i < len; i++)
    {
        module |= _B(*name, (i<<3));
        name++;
    }

    return module;
}
