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
#include <stdint.h>
#include <sys/types.h>
#include <xf86drm.h>
#include "xorg-server.h"
#include "xdbg.h"
#include "xdbg_log_fpsdebug.h"

#ifndef API
#define API __attribute__ ((visibility("default")))
#endif

static Bool g_on = FALSE;

struct _fpsDebug
{
    int nPanCount;
    CARD32 tStart, tCur, tLast;
    OsTimerPtr fpsTimer;
    int  connector_type;
    int  cid;
};

static struct
{
    int type;
    const char *name;
} connector_list[] =
{
       { DRM_MODE_CONNECTOR_Unknown, "unknown" },
       { DRM_MODE_CONNECTOR_VGA, "VGA" },
       { DRM_MODE_CONNECTOR_DVII, "DVI-I" },
       { DRM_MODE_CONNECTOR_DVID, "DVI-D" },
       { DRM_MODE_CONNECTOR_DVIA, "DVI-A" },
       { DRM_MODE_CONNECTOR_Composite, "composite" },
       { DRM_MODE_CONNECTOR_SVIDEO, "s-video" },
       { DRM_MODE_CONNECTOR_LVDS, "LVDS" },
       { DRM_MODE_CONNECTOR_Component, "component" },
       { DRM_MODE_CONNECTOR_9PinDIN, "9-pin DIN" },
       { DRM_MODE_CONNECTOR_DisplayPort, "displayport" },
       { DRM_MODE_CONNECTOR_HDMIA, "HDMI-A" },
       { DRM_MODE_CONNECTOR_HDMIB, "HDMI-B" },
       { DRM_MODE_CONNECTOR_TV, "TV" },
       { DRM_MODE_CONNECTOR_eDP, "embedded displayport" },
       { DRM_MODE_CONNECTOR_VIRTUAL, "Virtual" },
};

static int
_get_connector_id (int connector_type)
{
    int num = sizeof (connector_list) / sizeof (connector_list[0]);
    int i;
    int cid = 0;

    for (i = 0; i < num; i++)
    {
        if (connector_type == connector_list[i].type)
        {
            cid = i;
            break;
        }
    }

    return cid;
}


static int
_fps_print_fps (FpsDebugPtr pFpsDebug, int isTimeOut)
{
    double sec;

    sec = (double) (pFpsDebug->tLast - pFpsDebug->tStart) /1000.0;
    ErrorF ("[Xorg..V][%s][FB:(%3dframes %3.1ffps)] Dur:%3.3f...%s\n",
            connector_list[pFpsDebug->cid].name, pFpsDebug->nPanCount, pFpsDebug->nPanCount / sec,
            sec, isTimeOut ? "[TimeOut]" : "");
    pFpsDebug->nPanCount = 0;

    return 0;
}

static CARD32
_fps_frame_timeout (OsTimerPtr timer, CARD32 now, pointer arg)
{
        _fps_print_fps (arg, TRUE);

    return 0;
}


API FpsDebugPtr
xDbgLogFpsDebugCreate ()
{
    FpsDebugPtr pFpsDebug = NULL;

    pFpsDebug = calloc (1, sizeof (struct _fpsDebug));
    if (!pFpsDebug)
    {
        XDBG_ERROR (MXDBG, "fail to allocate the FpsDebug\n");
        return NULL;
    }

    return pFpsDebug;
}

API void
xDbgLogFpsDebugDestroy (FpsDebugPtr pFpsDebug)
{
    if (!pFpsDebug)
        return;

    free (pFpsDebug);
    pFpsDebug = NULL;
}

API void
xDbgLogFpsDebugCount (FpsDebugPtr pFpsDebug, int connector_type)
{
    /* if fpsdebug is off, do not count fps */
    if (!g_on)
        return;

    if (!pFpsDebug)
        return;

    if (connector_type != pFpsDebug->connector_type)
    {
        pFpsDebug->connector_type = connector_type;
        pFpsDebug->cid = _get_connector_id (connector_type);
    }

    pFpsDebug->tCur = GetTimeInMillis();
    if (pFpsDebug->nPanCount && pFpsDebug->tStart + 1000 <= pFpsDebug->tCur)
        _fps_print_fps (pFpsDebug, FALSE);

    if (pFpsDebug->nPanCount == 0)
        pFpsDebug->tStart = pFpsDebug->tLast;

    pFpsDebug->nPanCount++;
    pFpsDebug->tLast = pFpsDebug->tCur;
    pFpsDebug->fpsTimer = TimerSet (pFpsDebug->fpsTimer
                                    , 0, 1000
                                    , _fps_frame_timeout, pFpsDebug);
}

API void
xDbgLogFpsDebug (int on)
{
    if (g_on != on)
        g_on = on;
}

