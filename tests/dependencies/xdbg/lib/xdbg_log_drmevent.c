/**************************************************************************

xdbg

Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved

Contact: SooChan Lim <sc1.lim@samsung.com>
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

#include "xdbg.h"

#include <list.h>

#ifndef API
#define API __attribute__ ((visibility("default")))
#endif

typedef struct _xdbgVblankInfo
{
    int crtc_pipe;
    unsigned int client_idx;
    unsigned int draw_id;
    int flag;
    unsigned long time;
    struct xorg_list link;
} XDbgVblankInfo, *XDbgVblankInfoPtr;

typedef struct _xdbgPageFlipInfo
{
    int crtc_pipe;
    unsigned int client_idx;
    unsigned int draw_id;
    unsigned long time;
    struct xorg_list link;
} XDbgPageFlipInfo, *XDbgPageFlipInfoPtr;

static struct xorg_list vblank_event_list;
static struct xorg_list pageflip_event_list;

API void
xDbgLogDrmEventInit ()
{
    xorg_list_init (&vblank_event_list);
    xorg_list_init (&pageflip_event_list);
}

API void
xDbgLogDrmEventDeInit ()
{
    /* TODO: delete all link(xorg_list_del) in the two list */
}

API void *
xDbgLogDrmEventAddVblank ( int crtc_pipe, unsigned int client_idx, unsigned int draw_id, int flag)
{
    XDbgVblankInfoPtr pVblankInfo = NULL;

    pVblankInfo = calloc (1, sizeof (XDbgPageFlipInfo));
    if (!pVblankInfo)
        return NULL;

    pVblankInfo->crtc_pipe = crtc_pipe;
    pVblankInfo->client_idx = client_idx;
    pVblankInfo->draw_id = draw_id;
    pVblankInfo->flag = flag;
    pVblankInfo->time =GetTimeInMillis();

    xorg_list_init(&pVblankInfo->link);
    xorg_list_add(&pVblankInfo->link, &vblank_event_list);

    return (void *)pVblankInfo;
}

API void
xDbgLogDrmEventRemoveVblank (void *vblank_data)
{
    XDbgVblankInfoPtr pVblankInfo = (XDbgVblankInfoPtr) vblank_data;

    xorg_list_del (&pVblankInfo->link);
    free (pVblankInfo);
    pVblankInfo = NULL;
}

API void *
xDbgLogDrmEventAddPageflip (int crtc_pipe, unsigned int client_idx, unsigned int draw_id)
{
    XDbgPageFlipInfoPtr pPageFlipInfo = NULL;

    pPageFlipInfo = calloc (1, sizeof (XDbgPageFlipInfo));
    if (!pPageFlipInfo)
        return NULL;

    pPageFlipInfo->crtc_pipe = crtc_pipe;
    pPageFlipInfo->client_idx = client_idx;
    pPageFlipInfo->draw_id = draw_id;
    pPageFlipInfo->time =GetTimeInMillis();

    xorg_list_init(&pPageFlipInfo->link);
    xorg_list_add(&pPageFlipInfo->link, &pageflip_event_list);

    return (void *)pPageFlipInfo;
}

API void
xDbgLogDrmEventRemovePageflip (void *pageflip_data)
{
    XDbgPageFlipInfoPtr pPageFlipInfo = (XDbgPageFlipInfoPtr) pageflip_data;

    xorg_list_del (&pPageFlipInfo->link);
    free (pPageFlipInfo);
    pPageFlipInfo = NULL;
}

API void
xDbgLogDrmEventPendingLists ( char *reply, int *remain)
{
    XDbgVblankInfoPtr vblank_ref = NULL;
    XDbgVblankInfoPtr vblank_next = NULL;
    XDbgPageFlipInfoPtr flip_ref = NULL;
    XDbgPageFlipInfoPtr flip_next = NULL;
    Bool check_flip = FALSE;
    Bool check_vblank = FALSE;
    char *p = reply;
    int len;

    len = snprintf (p, *remain, "[vblank event pending]\n");
    p += len;
    *remain -= len;
    xorg_list_for_each_entry_safe (vblank_ref, vblank_next, &vblank_event_list, link)
    {
        check_vblank = TRUE;

        if (vblank_ref->flag > -1)
        {
            len = snprintf (p, *remain, "req_time        client_id       draw_id       crtc_pipe     vblank_type\n");
            p += len;
           *remain -= len;
           len = snprintf (p, *remain, "[%10.3f]    %5d          0x%x     %5d          %5d\n",
                       (double)vblank_ref->time/1000.0,
                       (unsigned int)vblank_ref->client_idx,
                       (unsigned int)vblank_ref->draw_id, vblank_ref->crtc_pipe, vblank_ref->flag);
            p += len;
           *remain -= len;
        }
        else
        {
            len = snprintf (p, *remain, "req_time        vblank_type\n");
            p += len;
           *remain -= len;
            len = snprintf (p, *remain, "[%10.3f]             %d\n", (double)vblank_ref->time/1000.0, vblank_ref->flag);
            p += len;
           *remain -= len;
        }
    }
    if (!check_vblank)
    {
        len = snprintf (p, *remain, "\t no pending events\n");
        p += len;
        *remain -= len;
    }

    len = snprintf (p, *remain, "[flip event pending]\n");
    p += len;
    *remain -= len;
    xorg_list_for_each_entry_safe (flip_ref, flip_next, &pageflip_event_list, link)
    {
        check_flip = TRUE;

        len = snprintf (p, *remain, "req_time        client_id       draw_id       crtc_pipe\n");
        p += len;
        *remain -= len;
        len = snprintf (p, *remain, "[%10.3f]    %5d           0x%x      %4d\n",
                   (double)flip_ref->time/1000.0, (unsigned int)flip_ref->client_idx,
                   (unsigned int)flip_ref->draw_id, flip_ref->crtc_pipe);
        p += len;
        *remain -= len;
    }
    if (!check_flip)
    {
        len = snprintf (p, *remain, "\t no pending events\n");
        p += len;
        *remain -= len;
    }
}



