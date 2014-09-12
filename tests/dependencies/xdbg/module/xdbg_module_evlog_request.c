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

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>

#include <dix.h>
#define XREGISTRY
#include <registry.h>
#include <xace.h>
#include <xacestr.h>
#include <X11/Xatom.h>
#include <X11/extensions/XI2proto.h>
#include <windowstr.h>

#include <xdbg.h>

#include "xdbg_module_types.h"
#include "xdbg_module_evlog_request.h"

#define UNKNOWN_EVENT "<unknown>"

static ExtensionEntry *xext;

static Bool
_EvlogRequestGetExtentionEntry (void)
{
    static int init = 0;
    static Bool success = FALSE;

    if (init)
        return success;

    init = 1;

    xext = CheckExtension (SHMNAME); 
    XDBG_RETURN_VAL_IF_FAIL (xext != NULL, FALSE);

    success = TRUE;

    return success;
}

static Bool
_EvlogRequestCore (xReq *req, char *buf, int remain)
{
    xReq *stuff = req;

    switch (stuff->reqType)
    {
    case X_PutImage:
        {
            xPutImageReq *stuff = (xPutImageReq *)req;
            snprintf (buf, remain, ": XID(%lx) size(%dx%d) dst(%d,%d)",
                stuff->drawable,
                stuff->width,
                stuff->height,
                stuff->dstX,
                stuff->dstY);
            return TRUE;
        }
    default:
            break;
    }

    return FALSE;
}

static Bool
_EvlogRequestShm (xReq *req, char *buf, int remain)
{
    xReq *stuff = req;

    switch (stuff->data)
    {
    case X_ShmPutImage:
        {
            xShmPutImageReq *stuff = (xShmPutImageReq *)req;
            snprintf (buf, remain, ": XID(%lx) size(%dx%d) src(%d,%d %dx%d) dst(%d,%d)",
                stuff->drawable,
                stuff->totalWidth,
                stuff->totalHeight,
                stuff->srcX,
                stuff->srcY,
                stuff->srcWidth,
                stuff->srcHeight,
                stuff->dstX,
                stuff->dstY);
            return TRUE;
        }
    default:
            break;
    }

    return FALSE;
}

Bool
xDbgModuleEvlogReqeust (EvlogClientInfo *evinfo, xReq *req, char *buf, int remain)
{
    const char *req_name;
    int len;

    if (!_EvlogRequestGetExtentionEntry () || !req)
        return FALSE;

    if (req->reqType < EXTENSION_BASE)
    {
        req_name = LookupRequestName (req->reqType, 0);
        len = snprintf (buf, remain, "%s", req_name);
        buf += len;
        remain -= len;

        return _EvlogRequestCore (req, buf, remain);
    }
    else
    {
        req_name = LookupRequestName (req->reqType, req->data);
        len = snprintf (buf, remain, "%s", req_name);
        buf += len;
        remain -= len;

        if (req->reqType == xext->base)
           return _EvlogRequestShm (req, buf, remain);
    }

    return FALSE;
}
