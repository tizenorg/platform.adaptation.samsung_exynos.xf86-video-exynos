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

#ifndef __XDBG_MODULE_TYPES_H__
#define __XDBG_MODULE_TYPES_H__

#include <xf86.h>
#include <X11/Xdefs.h>	/* for Bool */

#define XDBG_PATH_MAX        1024

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef SWAP
#define SWAP(a, b)  ({int t; t = a; a = b; b = t;})
#endif

typedef struct _ModuleClientInfo
{
    int    index;
    int    pid;
    int    gid;
    int    uid;
    int    conn_fd;
    char   command[PATH_MAX+1];
} ModuleClientInfo;

typedef struct _XDbgModule
{
    char *log_path;
    char *real_log_path;

    char *evlog_path;
} XDbgModule;

typedef struct _EvlogClientInfo
{
    int    index;
    int    pid;
    int    gid;
    int    uid;
    char   command[PATH_MAX+1];
    pointer requestBuffer;
    CARD32 req_len;
} EvlogClientInfo;

extern DevPrivateKeyRec debug_client_key;
#define DebugClientKey (&debug_client_key)
#define GetClientInfo(pClient) ((ModuleClientInfo*)dixLookupPrivate(&(pClient)->devPrivates, DebugClientKey))

#endif  /* __XDBG_MODULE_TYPES_H__ */
