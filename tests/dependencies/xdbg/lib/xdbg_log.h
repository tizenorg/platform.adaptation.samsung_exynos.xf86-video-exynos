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

#ifndef __XDBG_LOG_H__
#define __XDBG_LOG_H__

#include <errno.h>
#include <stdlib.h>
#include <os.h>

// Masks
#define XLOG_MASK_LOGLEVEL   0x000000FF
#define XLOG_MASK_OPTIONS    0xFFFFFF00

// LogLevels
enum
{
    XLOG_LEVEL_0,
    XLOG_LEVEL_1,
    XLOG_LEVEL_2,
    XLOG_LEVEL_3,
    XLOG_LEVEL_4,
    XLOG_LEVEL_MAX,
    XLOG_LEVEL_DEFAULT = XLOG_LEVEL_3
};

#define XLOG_LEVEL_DEBUG    XLOG_LEVEL_0
#define XLOG_LEVEL_TRACE    XLOG_LEVEL_1
#define XLOG_LEVEL_INFO     XLOG_LEVEL_2
#define XLOG_LEVEL_WARNING  XLOG_LEVEL_3
#define XLOG_LEVEL_ERROR    XLOG_LEVEL_4

// Log Options
#define XLOG_OPTION_KLOG        (1 << 8)
#define XLOG_OPTION_SLOG        (1 << 9)    /* print to stderr always */

typedef enum
{
    MODE_NAME_ONLY,
    MODE_WITH_STATUS
} LOG_ENUM_MODE;

int   xDbgLogEnumModules (LOG_ENUM_MODE mode, char *buf, int *remain);
int   xDbgLogSetLevel    (unsigned int module, int level);
void* xDbgLog            (unsigned int module, int logoption, const char *file, int line, const char *f, ...);

// defines
#define XLOG_DEBUG(mod, ...)    xDbgLog(mod, XLOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define XLOG_TRACE(mod, ...)    xDbgLog(mod, XLOG_LEVEL_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define XLOG_INFO(mod, ...)     xDbgLog(mod, XLOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define XLOG_WARNING(mod, ...)  xDbgLog(mod, XLOG_LEVEL_WARNING, __FILE__, __LINE__, __VA_ARGS__)
#define XLOG_ERROR(mod, ...)    xDbgLog(mod, XLOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define XLOG_KLOG(mod, ...)     xDbgLog(mod, XLOG_LEVEL_INFO|XLOG_OPTION_KLOG, __FILE__, __LINE__, __VA_ARGS__)
#define XLOG_SLOG(mod, ...)     xDbgLog(mod, XLOG_LEVEL_INFO|XLOG_OPTION_SLOG, __FILE__, __LINE__, __VA_ARGS__)

#define XDBG_DEBUG(mod, fmt, ARG...)      XLOG_DEBUG(mod, "[%s] "fmt, __FUNCTION__, ##ARG)
#define XDBG_TRACE(mod, fmt, ARG...)      XLOG_TRACE(mod, "[%s] "fmt, __FUNCTION__, ##ARG)
#define XDBG_INFO(mod, fmt, ARG...)       XLOG_INFO(mod, "[%s] "fmt, __FUNCTION__, ##ARG)
#define XDBG_WARNING(mod, fmt, ARG...)    XLOG_WARNING(mod, "[%s] "fmt, __FUNCTION__, ##ARG)
#define XDBG_ERROR(mod, fmt, ARG...)      XLOG_ERROR(mod, "[%s] "fmt, __FUNCTION__, ##ARG)
#define XDBG_ERRNO(mod, fmt, ARG...)      XLOG_ERROR(mod, "[%s](err=%s(%d)) "fmt, __FUNCTION__, strerror(errno), errno, ##ARG)
#define XDBG_KLOG(mod, fmt, ARG...)       XLOG_KLOG(mod, "[%s] "fmt, __FUNCTION__, ##ARG)
#define XDBG_SLOG(mod, fmt, ARG...)       XLOG_SLOG(mod, "[%s] "fmt, __FUNCTION__, ##ARG)

#define XDBG_NEVER_GET_HERE(mod)          XLOG_ERROR(mod, "[%s] ** NEVER GET HERE **\n", __FUNCTION__)

#define XDBG_WARNING_IF_FAIL(cond)         {if (!(cond)) { ErrorF ("[%s] '%s' failed.\n", __FUNCTION__, #cond);}}
#define XDBG_RETURN_IF_FAIL(cond)          {if (!(cond)) { ErrorF ("[%s] '%s' failed.\n", __FUNCTION__, #cond); return; }}
#define XDBG_RETURN_VAL_IF_FAIL(cond, val) {if (!(cond)) { ErrorF ("[%s] '%s' failed.\n", __FUNCTION__, #cond); return val; }}
#define XDBG_RETURN_VAL_IF_ERRNO(cond, val, errno)       {if (!(cond)) { ErrorF ("[%s] '%s' failed. (err=%s(%d))\n", __FUNCTION__, #cond, strerror(errno), errno); return val; }}
#define XDBG_GOTO_IF_FAIL(cond, dst)       {if (!(cond)) { ErrorF ("[%s] '%s' failed.\n", __FUNCTION__, #cond); goto dst; }}
#define XDBG_GOTO_IF_ERRNO(cond, dst, errno)       {if (!(cond)) { ErrorF ("[%s] '%s' failed. (err=%s(%d))\n", __FUNCTION__, #cond, strerror(errno), errno); goto dst; }}

#define XDBG_REPLY(fmt, ARG...)  \
    do { \
        if (reply && len && *len > 0) \
        { \
            int s = snprintf (reply, *len, fmt, ##ARG); \
            reply += s; \
            *len -= s; \
        } \
    } while (0)

unsigned int xDbgLogGetModule (char *name);

#define _C(b,s)             (((b) >> (s)) & 0xFF)
#define _B(c,s)             ((((unsigned int)(c)) & 0xff) << (s))
#define XDBG_M(a,b,c,d)     (_B(d,24)|_B(c,16)|_B(b,8)|_B(a,0))

/* debug module for XDBG */
#define MXDBG    XDBG_M('X','D','B','G')

#endif  /* __XDBG_LOG_H__ */
