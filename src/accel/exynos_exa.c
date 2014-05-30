/**************************************************************************

xserver-xorg-video-exynos

Copyright 2013 Samsung Electronics co., Ltd. All Rights Reserved.

Contact: SooChan Lim <sc1.lim@samsung.com>

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

#include "exynos_driver.h"
#include "exynos_accel.h"
#include "exynos_accel_int.h"
#include "exynos_display.h"
#include "exynos_util.h"
#include "_trace.h"
#include "exynos_mem_pool.h"
#include <X11/Xatom.h>

static struct
{
    int usage_hint;
    char *string;
} hint_strs[] =
{
    {CREATE_PIXMAP_USAGE_NORMAL, "NORMAL"},
    {CREATE_PIXMAP_USAGE_SCRATCH, "SCRATCH"},
    {CREATE_PIXMAP_USAGE_BACKING_PIXMAP, "BACKING_PIXMAP"},
    {CREATE_PIXMAP_USAGE_GLYPH_PICTURE, "GLYPH_PICTURE"},
    {CREATE_PIXMAP_USAGE_SHARED, "SHARED"},
    {CREATE_PIXMAP_USAGE_DRI2_BACK, "DRI2_BACK"},
    {CREATE_PIXMAP_USAGE_SCANOUT, "SCANOUT"},
    {CREATE_PIXMAP_USAGE_SCANOUT_DRI2_BACK, "SCANOUT_DRI2_BACK"},
    {CREATE_PIXMAP_USAGE_XV, "XV"},
    { -1, 0}
};


char *
_getStrFromHint (int usage_hint)
{
    int i;
    int num = sizeof (hint_strs) / sizeof (hint_strs[0]);

    for (i = 0; i < num; i++)
    {
        if (hint_strs[i].usage_hint == usage_hint)
            return hint_strs[i].string;
    }

    return NULL;
}

static void
EXYNOSExaWaitMarker (ScreenPtr pScreen, int marker)
{
}

Bool
EXYNOSExaPrepareAccess (PixmapPtr pPix, int index)
{
    EXYNOSExaPixPrivPtr pExaPixPriv = exaGetPixmapDriverPrivate (pPix);
    int opt = TBM_OPTION_READ;
    tbm_bo_handle bo_handle;

    XDBG_RETURN_VAL_IF_FAIL((pExaPixPriv != NULL), FALSE);

    if (pExaPixPriv->tbo)
    {
        if (index == EXA_PREPARE_DEST || index == EXA_PREPARE_AUX_DEST)
            opt |= TBM_OPTION_WRITE;

        bo_handle = tbm_bo_map(pExaPixPriv->tbo, TBM_DEVICE_CPU, opt);
        pPix->devPrivate.ptr = bo_handle.ptr;
    }
    else
    {
        if (pExaPixPriv->pPixData)
        {
            pPix->devPrivate.ptr = pExaPixPriv->pPixData;
        }
    }

    XDBG_DEBUG (MEXA, "pix:%p index:%d hint:%s ptr:%p opt %s\n",
                pPix, index, _getStrFromHint(pPix->usage_hint), pPix->devPrivate.ptr,opt&TBM_OPTION_WRITE?"TBM_OPTION_WRITE|TBM_OPTION_READ":"TBM_OPTION_READ");

    return TRUE;
}

void
EXYNOSExaFinishAccess (PixmapPtr pPix, int index)
{
    XDBG_TRACE(MEXAS, "\n");

    XDBG_RETURN_IF_FAIL((pPix != NULL));

    EXYNOSExaPixPrivPtr pExaPixPriv = exaGetPixmapDriverPrivate (pPix);

    XDBG_RETURN_IF_FAIL((pExaPixPriv != NULL));

    if (pExaPixPriv->tbo)
        tbm_bo_unmap (pExaPixPriv->tbo);

    pPix->devPrivate.ptr = NULL;

    XDBG_DEBUG (MEXA, "pix:%p index:%d hint:%s ptr:%p\n",
                pPix, index,  _getStrFromHint(pPix->usage_hint), pPix->devPrivate.ptr);
}

static void *
EXYNOSExaCreatePixmap (ScreenPtr pScreen, int size, int align)
{
    EXYNOSExaPixPrivPtr pExaPixPriv = ctrl_calloc (1, sizeof (EXYNOSExaPixPrivRec));
    XDBG_RETURN_VAL_IF_FAIL (pExaPixPriv != NULL, NULL);

    return pExaPixPriv;
}

static void
EXYNOSExaDestroyPixmap (ScreenPtr pScreen, void *driverPriv)
{
    XDBG_RETURN_IF_FAIL (driverPriv != NULL);

    EXYNOSExaPixPrivPtr pExaPixPriv = driverPriv;

    int hint = pExaPixPriv->usage_hint;

    XDBG_TRACE (MEXA, "DESTROY_PIXMAP : usage_hint:%s tbo-%p\n", _getStrFromHint(hint),pExaPixPriv->tbo);

    switch (hint)
    {
    case CREATE_PIXMAP_USAGE_NORMAL: /* this is 0. */
    case CREATE_PIXMAP_USAGE_SCRATCH:
    case CREATE_PIXMAP_USAGE_BACKING_PIXMAP:
    case CREATE_PIXMAP_USAGE_GLYPH_PICTURE:
    case CREATE_PIXMAP_USAGE_SHARED:
    case CREATE_PIXMAP_USAGE_DRI2_BACK:
    case CREATE_PIXMAP_USAGE_SCANOUT:
    case CREATE_PIXMAP_USAGE_SCANOUT_DRI2_BACK:
        tbm_bo_unref (pExaPixPriv->tbo);
        pExaPixPriv->tbo = NULL;
        break;
    case CREATE_PIXMAP_USAGE_XV:
        break;

    default:
        XDBG_ERROR (MEXA, "wrong usage_hint.(%d)\n", hint);
    }

    ctrl_free (pExaPixPriv);
}

static Bool
EXYNOSExaModifyPixmapHeader (PixmapPtr pPixmap, int width, int height,
                             int depth, int bitsPerPixel, int devKind, pointer pPixData)
{
    EXA_BEGIN;
    int flag=TBM_BO_DEFAULT;
    XDBG_RETURN_VAL_IF_FAIL (pPixmap != NULL, FALSE);

    ScrnInfoPtr pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    EXYNOSExaPixPrivPtr pExaPixPriv = exaGetPixmapDriverPrivate (pPixmap);
    int size = 0;

    XDBG_DEBUG (MEXA, "w %i h%i d%i bpp%i dk%d pPixData%p\n",width,height,depth, bitsPerPixel, devKind, pPixData);

    /* set the default headers of the pixmap */
    miModifyPixmapHeader (pPixmap, width, height, depth, bitsPerPixel,
                          devKind, pPixData);

    if (!pExynos->cachable){
                flag = TBM_BO_WC;
                }
            else{
                flag = TBM_BO_DEFAULT;
            };

    pExaPixPriv->isFrameBuffer = FALSE;
    /* screen pixmap : set a framebuffer pixmap */
    if (pPixData == (void*)ROOT_FB_ADDR)
    {
        pPixmap->usage_hint = CREATE_PIXMAP_USAGE_SCANOUT;

        size = pPixmap->drawable.height * pPixmap->devKind;

        pExaPixPriv->usage_hint = pPixmap->usage_hint;
        /*if(pExynos->default_bo)
            while(pExynos->default_bo)
                tbm_bo_unref(pExynos->default_bo);
         pExynos->default_bo = tbm_bo_alloc (pExynos->bufmgr, size, flag);
                */
        pExaPixPriv->tbo = tbm_bo_ref(pExynos->default_bo);
        pExaPixPriv->isFrameBuffer = TRUE;

        XDBG_DEBUG (MEXA, "Create a pixmap for a main screen.(%p): (x,y,w,h)=(%d,%d,%d,%d)\n",
                pPixmap, pPixmap->drawable.x, pPixmap->drawable.y, width, height);
        return TRUE;
    }

    if (pExaPixPriv->tbo != NULL)
    {
        XDBG_WARNING (MEXA, "pExaPixPriv->tbo is not null (%p).(had set before)\n",pExaPixPriv->tbo);
    }

    /* pPixData is also set for text glyphs or SHM-PutImage */
    if (pPixData)
    {
        size = pPixmap->drawable.height * pPixmap->devKind;

        pExaPixPriv->usage_hint = pPixmap->usage_hint;
        pExaPixPriv->pPixData = pPixData;
        pExaPixPriv->tbo=pPixData;
        XDBG_WARNING (MEXA, "pPixData is not null.\n");
    }
    else
    {
        switch (pPixmap->usage_hint)
        {
        case CREATE_PIXMAP_USAGE_NORMAL: /* this is 0. */
        case CREATE_PIXMAP_USAGE_SCRATCH:
        case CREATE_PIXMAP_USAGE_BACKING_PIXMAP:
        case CREATE_PIXMAP_USAGE_GLYPH_PICTURE:
        case CREATE_PIXMAP_USAGE_SHARED:
        case CREATE_PIXMAP_USAGE_DRI2_BACK:
            pExaPixPriv->usage_hint = pPixmap->usage_hint;

            size = pPixmap->drawable.height * pPixmap->devKind;

            /* TODO : there is a case where the size is 0, when the server creates a screen pixmap.
                                       how can we can deal with it?
                       */
            if (size > 0)
            {
                pExaPixPriv->tbo = tbm_bo_alloc (pExynos->bufmgr, size, flag);
                if (pExaPixPriv->tbo == NULL)
                {
                    XDBG_ERROR (MEXA, "fail to allocate a bo.(size:%d, hint:%s)\n",
                                size, _getStrFromHint(pPixmap->usage_hint));
                    return FALSE;
                }
            }
            XDBG_DEBUG (MEXA, "creating bo. new one is %p\n",pExaPixPriv->tbo);
            break;
        case CREATE_PIXMAP_USAGE_SCANOUT:
        case CREATE_PIXMAP_USAGE_SCANOUT_DRI2_BACK:
            /** @todo: allocate the scanout memory */
            pExaPixPriv->usage_hint = pPixmap->usage_hint;
            size = pPixmap->drawable.height * pPixmap->devKind;
            pExaPixPriv->tbo = exynosDisplayRenderBoCreate (pScrn, -1, -1, pPixmap->drawable.width, pPixmap->drawable.height);
            if (pExaPixPriv->tbo == NULL)
            {
                XDBG_ERROR (MEXA, "fail to allocate a bo.(size:%d, hint:%s)\n",
                            size, _getStrFromHint(pPixmap->usage_hint));
                return FALSE;
            }
            break;
        case CREATE_PIXMAP_USAGE_XV:
            /** @todo: Implement ModifyPixmap for XV extension */
            pExaPixPriv->usage_hint = pPixmap->usage_hint;
            break;
        default:
            size = pPixmap->drawable.height * pPixmap->devKind;
            XDBG_ERROR (MEXA, "wrong usage_hint.(size:%d, hint:%s)\n",
                        size, _getStrFromHint(pPixmap->usage_hint));
            return FALSE;
        }
    }

    XDBG_TRACE (MEXA, "%s(%p) : bo:%p, pPixData:%p (%dx%d+%d+%d)\n",
                _getStrFromHint(pPixmap->usage_hint),
                pPixmap, pExaPixPriv->tbo, pPixData,
                width, height,
                pPixmap->drawable.x, pPixmap->drawable.y);

    return TRUE;
}

static Bool
EXYNOSExaPixmapIsOffscreen (PixmapPtr pPix)
{
    return TRUE;
}

/* set sbc (swap buffer count) of pixmap */
void
exynosPixmapSetFrameCount (PixmapPtr pPix, unsigned int xid, CARD64 sbc)
{
    EXYNOSExaPixPrivPtr pExaPixPriv = exaGetPixmapDriverPrivate (pPix);

    pExaPixPriv->owner = xid;
    pExaPixPriv->sbc = sbc;
}

/* get sbc (swap buffer count) of pixmap */
void
exynosPixmapGetFrameCount (PixmapPtr pPix, unsigned int *xid, CARD64 *sbc)
{
    EXYNOSExaPixPrivPtr pExaPixPriv = exaGetPixmapDriverPrivate (pPix);

    *xid = pExaPixPriv->owner;
    *sbc = pExaPixPriv->sbc;
}

void
exynosPixmapSwap (PixmapPtr pFrontPix, PixmapPtr pBackPix)
{
    EXYNOSExaPixPrivPtr pFrontExaPixPriv = exaGetPixmapDriverPrivate (pFrontPix);
    EXYNOSExaPixPrivPtr pBackExaPixPriv = exaGetPixmapDriverPrivate (pBackPix);

    tbm_bo_swap(pFrontExaPixPriv->tbo, pBackExaPixPriv->tbo);
//    tbm_bo tmp = NULL;
//
//    tmp = pFrontExaPixPriv->tbo;
//    pFrontExaPixPriv->tbo = pBackExaPixPriv->tbo;
//    pBackExaPixPriv->tbo = tmp;
}

tbm_bo
exynosExaPixmapGetBo (PixmapPtr pPix)
{
    XDBG_RETURN_VAL_IF_FAIL (pPix != NULL, NULL);

    EXYNOSExaPixPrivPtr pExaPixPriv = exaGetPixmapDriverPrivate (pPix);

    return pExaPixPriv->tbo;
}

void
exynosExaPixmapSetBo (PixmapPtr pPix, tbm_bo tbo)
{
    XDBG_RETURN_IF_FAIL (pPix != NULL);

    EXYNOSExaPixPrivPtr pExaPixPriv = exaGetPixmapDriverPrivate (pPix);

    XDBG_RETURN_IF_FAIL (pExaPixPriv->tbo == NULL);

    pExaPixPriv->tbo = tbo;
}

int
exynosExaScreenSetScrnPixmap (ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    PixmapPtr pPix = (*pScreen->GetScreenPixmap) (pScreen);
    unsigned int pitch = pScrn->virtualX * 4;
    (*pScreen->ModifyPixmapHeader) (pPix, pScrn->virtualX, pScrn->virtualY,
                                    -1, -1, pitch, (void*)ROOT_FB_ADDR);
    pScrn->displayWidth = pitch / 4;
    return 1;
}

static void
_setScreenRotationProperty (ScrnInfoPtr pScrn)
{
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    ScreenPtr pScreen = pScrn->pScreen;
    Atom atom_screen_rotation;
    WindowPtr pWin = pScreen->root;
    int rc;
    atom_screen_rotation = MakeAtom ("X_SCREEN_ROTATION", 17, TRUE);
    unsigned int rotation = (unsigned int) pExynos->rotate;

    rc = dixChangeWindowProperty (serverClient,
                                  pWin, atom_screen_rotation, XA_CARDINAL, 32,
                                  PropModeReplace, 1, &rotation, FALSE);
    if (rc != Success)
        XDBG_ERROR (MEXA, "failed : set X_SCREEN_ROTATION to %d\n", rotation);
}

static void
_exynosExaBlockHandler (pointer blockData, OSTimePtr pTimeout,
                     pointer pReadmask)
{
    ScreenPtr pScreen = screenInfo.screens[0];
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];

    /* add screen rotation property to the root window */
    _setScreenRotationProperty (pScrn);

    RemoveBlockAndWakeupHandlers (_exynosExaBlockHandler /*blockHandler*/,
                                  (void*)NULL /*wakeupHandler*/,
                                  (void*)NULL /*blockData*/);
}

Bool
exynosExaInit (ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    EXYNOSAccelInfoPtr pAccelInfo = pExynos->pAccelInfo;
    ExaDriverPtr pExaDriver = NULL;
    unsigned int cpp = 4;

    /* allocate the EXA driver private */
    pExaDriver = exaDriverAlloc();
    XDBG_RETURN_VAL_IF_FAIL (pExaDriver != NULL, FALSE);

    /* version of exa */
    pExaDriver->exa_major = EXA_VERSION_MAJOR;
    pExaDriver->exa_minor = EXA_VERSION_MINOR;

    /* setting the memory stuffs */
    pExaDriver->memoryBase = (void*)ROOT_FB_ADDR;
    pExaDriver->memorySize = pScrn->videoRam * 1024;
    pExaDriver->offScreenBase = pScrn->displayWidth * cpp * pScrn->virtualY;

    pExaDriver->maxX = 1 << 16;
    pExaDriver->maxY = 1 << 16;
    pExaDriver->pixmapOffsetAlign = 0;
    pExaDriver->pixmapPitchAlign = 8;
    pExaDriver->flags = (EXA_OFFSCREEN_PIXMAPS | EXA_HANDLES_PIXMAPS
                         | EXA_SUPPORTS_OFFSCREEN_OVERLAPS
                         | EXA_SUPPORTS_PREPARE_AUX);

    pExaDriver->WaitMarker = EXYNOSExaWaitMarker;
    pExaDriver->PrepareAccess = EXYNOSExaPrepareAccess;
    pExaDriver->FinishAccess = EXYNOSExaFinishAccess;

    pExaDriver->CreatePixmap = EXYNOSExaCreatePixmap;
    pExaDriver->DestroyPixmap = EXYNOSExaDestroyPixmap;
    pExaDriver->ModifyPixmapHeader = EXYNOSExaModifyPixmapHeader;
    pExaDriver->PixmapIsOffscreen = EXYNOSExaPixmapIsOffscreen;

    /* call init function */
    if (pExynos->is_sw_exa)
    {
        if (!exynosExaSwInit (pScreen, pExaDriver))
        {
            XDBG_ERROR (MEXA, "fail to initalize EXA_SW.\n");
            ctrl_free (pExaDriver);
            return FALSE;
        }
        else
        {
            XDBG_INFO (MEXA, "Initialized EXYNOS EXA SW acceleration OK !\n");
        }
    }

    /* exa driver init with exa driver private */
    if (!exaDriverInit (pScreen, pExaDriver))
    {
        XDBG_ERROR (MEXA, "fail to initalize EXA.\n");
        ctrl_free (pExaDriver);
        return FALSE;
    }

    pAccelInfo->pExaDriver = pExaDriver;

    /* block handler */
    RegisterBlockAndWakeupHandlers (_exynosExaBlockHandler /*blockHandler*/,
                                    NULL /*wakeupHandler*/,
                                    NULL /*blockData*/);

    XDBG_INFO (MEXA, "EXA driver is Loaded successfully\n");

    return TRUE;
}

void
exynosExaDeinit (ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);

    if (pExynos->is_sw_exa)
    {
        XDBG_INFO (MEXA, "Finish SW EXA acceleration.\n");
        exynosExaSwDeinit (pScreen);
    }

    XDBG_INFO (MEXA, "EXA driver is UnLoaded successfully\n");
}

