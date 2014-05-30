/*
 * xserver-xorg-video-exynos
 *
 * Copyright 2004 Keith Packard
 * Copyright 2005 Eric Anholt
 * Copyright 2006 Nokia Corporation
 * Copyright 2010 - 2011 Samsung Electronics co., Ltd. All Rights Reserved.
 *
 * Contact: Boram Park <boram1288.park@samsung.com>
 *
 * Permission to use, copy, modify, distribute and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of the authors and/or copyright holders
 * not be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  The authors and
 * copyright holders make no representations about the suitability of this
 * software for any purpose.  It is provided "pas is" without any express
 * or implied warranty.
 *
 * THE AUTHORS AND COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvproto.h>

#include <xorg/xf86xv.h>

#include "exynos_driver.h"
#include "exynos_video.h"
#include "exynos_video_image.h"
#include "exynos_video_overlay.h"
#include "exynos_mem_pool.h"
#include "_trace.h"

#ifdef XV

/**
 * Set up everything we need for Xv.
 */
Bool
exynosVideoInit (ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    EXYNOSPtr pExynos = (EXYNOSPtr) pScrn->driverPrivate;
    EXYNOSXvInfoPtr pXvInfo;
    pXvInfo = (EXYNOSXvInfoPtr) ctrl_calloc (sizeof (EXYNOSXvInfoRec), 1);
    if (!pXvInfo)
        return FALSE;

    /* setup image adaptor */
    pXvInfo->pAdaptor[0] = exynosVideoImageSetup (pScreen);
    if (!pXvInfo->pAdaptor[0])
    {
        ctrl_free (pXvInfo);
        return FALSE;
    }

//    /* setup capture adaptor */
//    pXvInfo->pAdaptor[1] = exynosVideoCaptureSetup (pScreen);
//    if (!pXvInfo->pAdaptor[1])
//    {
//        ctrl_free (pXvInfo->pAdaptor[0]);
//        ctrl_free (pXvInfo);
//        return FALSE;
//    }
//    /* setup overlay adaptor*/
//    pXvInfo->pAdaptor[2] = exynosVideoOverlaySetup (pScreen);
//    if (!pXvInfo->pAdaptor[2])
//    {
//        ctrl_free (pXvInfo->pAdaptor[0]);
//        ctrl_free (pXvInfo->pAdaptor[1]);
//        ctrl_free (pXvInfo);
//        return FALSE;
//    }

    /* register two adaptor to xserver */
    xf86XVScreenInit (pScreen, pXvInfo->pAdaptor, ADAPTOR_NUM);

    pExynos->pXvInfo = pXvInfo;

    exynosVideoImageReplacePutImageFunc (pScreen);

//    exynosVideoImageReplacePutStillFunc (pScreen);
    return TRUE;
}

/**
 * Shut down Xv, used on regeneration.
 *
 */
void
exynosVideoFini (ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    EXYNOSPtr pExynos = (EXYNOSPtr) pScrn->driverPrivate;
    EXYNOSXvInfoPtr pXvInfo = pExynos->pXvInfo;
    int i;

    for (i = 0; i < ADAPTOR_NUM; i++)
        if (pXvInfo->pAdaptor[i])
            ctrl_free (pXvInfo->pAdaptor[i]);

    ctrl_free (pXvInfo);
    pExynos->pXvInfo = NULL;
}
#endif
