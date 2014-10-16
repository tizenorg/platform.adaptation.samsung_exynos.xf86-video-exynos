/*
 * Copyright © 2013 Keith Packard
 * Copyright 2010 - 2014 Samsung Electronics co., Ltd. All Rights Reserved.
 *
 * Contact: Roman Marchenko <r.marchenko@samsung.com>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include <tbm_bufmgr.h>

#include "xorg-server.h"
#include "xf86.h"

#include "xf86drm.h"
#include "misyncshm.h"

#include "dri3.h"

#include "sec.h"
#include "sec_accel.h"

// -------------------------- Private functions--------------------------------

// -------------------------- Callback functions--------------------------------
static int
SECDRI3Open(ScreenPtr screen, RRProviderPtr provider, int *fdp)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(screen);
    SECPtr pExynos = SECPTR(pScrn);
    drm_magic_t magic;
    int fd;

    /* Open the device for the client */
    fd = open(pExynos->drm_device_name, O_RDWR | O_CLOEXEC);
    if (fd == -1 && errno == EINVAL)
    {
        fd = open(pExynos->drm_device_name, O_RDWR);
        if (fd != -1)
            fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);
    }

    if (fd < 0)
        return BadAlloc;

    /* Go through the auth dance locally */
    if (drmGetMagic(fd, &magic) < 0)
    {
        close(fd);
        return BadMatch;
    }

    /* And we're done */
    *fdp = fd;
    return Success;
}

static PixmapPtr 
SECDRI3PixmapFromFd(ScreenPtr pScreen,
                    int fd,
                    CARD16 width,
                    CARD16 height,
                    CARD16 stride,
                    CARD8 depth,
                    CARD8 bpp)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    SECPtr pSec = SECPTR(pScrn);

    PixmapPtr pPixmap = NULL;

    XDBG_DEBUG(MDRI3, "fd:%d width:%d height:%d stride:%d depth:%d bpp:%d\n",
               fd, width, height, stride, depth, bpp);

    XDBG_RETURN_VAL_IF_FAIL((width <= INT16_MAX && height <= INT16_MAX), NULL);

    XDBG_RETURN_VAL_IF_FAIL(((uint32_t )width * bpp <= (uint32_t )stride * 8), NULL);

    XDBG_RETURN_VAL_IF_FAIL((depth > 8), NULL);

    XDBG_RETURN_VAL_IF_FAIL((bpp == 8 || bpp == 16 || bpp == 24 || bpp == 32), NULL);

    tbm_bo tbo = tbm_bo_import_fd(pSec->tbm_bufmgr, fd);

    XDBG_RETURN_VAL_IF_FAIL(tbo != NULL, NULL);

    uint32_t real_size = tbm_bo_size(tbo);
    uint32_t target_size = (uint32_t) height * stride;

    if (real_size < target_size)
    {
        XDBG_WARNING(MDRI3, "the real size of bo(%p) less then target: %d, %d\n", tbo, real_size, target_size);
        goto free_bo;
    }

    pPixmap = (*pScreen->CreatePixmap)(pScreen, 0, 0, depth, 0);

    XDBG_GOTO_IF_FAIL(pPixmap != NULL, free_bo);

    if (!pScreen->ModifyPixmapHeader(pPixmap, width, height, 0, 0, stride, 0))
        goto free_pix;

    secExaMigratePixmap(pPixmap, tbo);

    XDBG_DEBUG(MDRI3, "pixmap(sn:%ld p:%p ID:0x%x %dx%d stride:%d depth:%d bpp:%d) bo(name:%d p:%p)\n",
            pPixmap->drawable.serialNumber, pPixmap, pPixmap->drawable.id,
            pPixmap->drawable.width, pPixmap->drawable.height,
            pPixmap->devKind, pPixmap->drawable.depth, pPixmap->drawable.bitsPerPixel,
            tbm_bo_export(tbo), tbo);

    return pPixmap;

free_pix:
    (*pScreen->DestroyPixmap)(pPixmap);
free_bo:
    tbm_bo_unref(tbo);

    return NULL;
}

static int
SECDRI3FdFromPixmap(ScreenPtr pScreen,
                    PixmapPtr pPixmap,
                    CARD16 *stride,
                    CARD32 *size)
{
    SECPixmapPriv * priv = NULL;
    int fd;

    XDBG_DEBUG(MDRI3, "pixmap(sn:%ld p:%p ID:0x%x) (%dx%d)\n",
            pPixmap->drawable.serialNumber, pPixmap, pPixmap->drawable.id,
            pPixmap->drawable.width, pPixmap->drawable.height);


    priv = exaGetPixmapDriverPrivate(pPixmap);
    XDBG_RETURN_VAL_IF_FAIL(priv, -1);

    fd = tbm_bo_export_fd(priv->bo);
    XDBG_RETURN_VAL_IF_FAIL(fd > 0, -1);

    *stride = pPixmap->devKind;
    *size = tbm_bo_size(priv->bo);

    XDBG_DEBUG(MDRI3, "fd:%d stride:%d size:%d bo_name:%d\n",
            fd, *stride, *size, tbm_bo_export(priv->bo));
    return fd;
}

static dri3_screen_info_rec sec_dri3_screen_info = {
        .version = DRI3_SCREEN_INFO_VERSION,

        .open = SECDRI3Open,
        .pixmap_from_fd = SECDRI3PixmapFromFd,
        .fd_from_pixmap = SECDRI3FdFromPixmap
};

// -------------------------- Public functions--------------------------------
Bool
secDri3ScreenInit(ScreenPtr screen)
{
    if (!miSyncShmScreenInit(screen))
        return FALSE;

    return dri3_screen_init(screen, &sec_dri3_screen_info);
}

