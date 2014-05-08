/**************************************************************************

xserver-xorg-video-exynos

Copyright 2001 VA Linux Systems Inc., Fremont, California.
Copyright © 2002 by David Dawes
Copyright 2010 - 2011 Samsung Electronics co., Ltd. All Rights Reserved.

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
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <poll.h>
#include <xf86Crtc.h>

#include "X11/Xatom.h"
#include "xorg-server.h"
#include "xf86.h"
#include "dri2.h"
#include "damage.h"
#include "windowstr.h"
#include "exynos_driver.h"
#include "exynos_accel.h"
#include "exynos_accel_int.h"
#include "exynos_display.h"
#include "exynos_util.h"
#include "exynos_mem_pool.h"

#include "_trace.h"

#define DRI2_BUFFER_TYPE_WINDOW 0x0
#define DRI2_BUFFER_TYPE_PIXMAP 0x1
#define DRI2_BUFFER_TYPE_FB     0x2

/* temp variable for _trace.h. Remove it */
int _temp=6;


typedef union{
    unsigned int flags;
    struct {
        unsigned int type:1;
        unsigned int is_framebuffer:1;
        unsigned int is_viewable:1;
        unsigned int is_reused:1;
        unsigned int reuse_idx:3;
    }data;
}DRI2BufferFlags;

#define NUM_BACKBUF_PIXMAP 2

#define DRI2_GET_NEXT_IDX(idx, max) (((idx+1) % (max)))

/* if a window is mapped and realized (viewable) */
#define IS_VIEWABLE(pDraw) \
    ((pDraw->type == DRAWABLE_PIXMAP)?TRUE:(Bool)(((WindowPtr) pDraw)->viewable))

/* dri2 buffer private infomation */
typedef struct _dri2BufferPriv
{
    Bool      can_flip;
    int       refcnt; /* refcnt */
    int       attachment;
    int       pipe;
    PixmapPtr pPixmap; /* current pixmap of the buffer private */
    ScreenPtr pScreen;
    
    /* for TB */
    int num_buf;
    int avail_idx; /* next available index of the back pixmap, -1 means not to be available */
    int cur_idx;   /* current index of the back pixmap, -1 means not to be available */
    int free_idx;  /* free index of the back pixmap, -1 means not to be available */
    PixmapPtr *pBackPixmaps;

    /* flip buffers */
    ClientPtr    pClient;
    DRI2FrameEventPtr pFlipEvent;
} DRI2BufferPrivRec, *DRI2BufferPrivPtr;

/* prototypes */
static void EYXNOSDri2CopyRegion (DrawablePtr pDraw, RegionPtr pRegion,
                               DRI2BufferPtr pDstBuf, DRI2BufferPtr pSrcBuf);
static void EYXNOSDri2DestroyBuffer (DrawablePtr pDraw, DRI2BufferPtr pBuf);

static unsigned int
_getName (PixmapPtr pPix)
{

    EXYNOSExaPixPrivPtr pExaPixPriv = NULL;


    if (pPix == NULL)
        {

        return 0;
        }

    pExaPixPriv = exaGetPixmapDriverPrivate (pPix);
#if 1
    if (pExaPixPriv->tbo == NULL)
    {
        if(pExaPixPriv->isFrameBuffer)
            {

            return (unsigned int)ROOT_FB_ADDR;
            }
        else
            {

            return 0;
            }
    }
#endif


    return tbm_bo_export (pExaPixPriv->tbo);
}
/** @todo Need crtc.h */
void exynosCrtcExemptAllFlipPixmap (ScrnInfoPtr pScrn, int crtc_pipe);
/* deinitialize the pixmap of the backbuffer */
static void
_deinitBackBufPixmap (DRI2BufferPtr pBackBuf, DrawablePtr pDraw, Bool canFlip)
{
DRI2_BEGIN;
    DRI2BufferPrivPtr pBackBufPriv = pBackBuf->driverPrivate;
    ScreenPtr pScreen = pBackBufPriv->pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    int i;
    int pipe = -1;

    for (i = 0; i < pBackBufPriv->num_buf; i++)
    {
        if (pBackBufPriv->pBackPixmaps)
        {
            if(pBackBufPriv->pBackPixmaps[i])
            {
                if (canFlip)
                {
                    /* have to release the flip pixmap */
                    pipe = pBackBufPriv->pipe;
                    if (pipe != -1)
                    {
                        exynosCrtcExemptAllFlipPixmap (pScrn, pipe);
                        XDBG_ERROR(MDRI2, "This rly used %d\n",__LINE__);
                    }
                    else
                    {
                        XDBG_WARNING(MDRI2, "pipe is -1\n");
                    }
                }
                else
                {
#ifdef USE_XDBG_EXTERNAL
                    xDbgLogPListDrawRemoveRefPixmap (pDraw, pBackBufPriv->pBackPixmaps[i]);
#endif
                    (*pScreen->DestroyPixmap) (pBackBufPriv->pBackPixmaps[i]);
                }
                pBackBufPriv->pBackPixmaps[i] = NULL;
                pBackBufPriv->pPixmap = NULL;
            }
            ctrl_free(pBackBufPriv->pBackPixmaps);
            pBackBufPriv->pBackPixmaps = NULL;
        }
    }
DRI2_END;
}

// This is DOG-NAIL for remove implicit declaration. It must be moved to header (maybe crtc.h needs?)
PixmapPtr exynosCrtcGetFlipPixmap (ScrnInfoPtr pScrn, int crtc_pipe, DrawablePtr pDraw, unsigned int usage_hint);

/* initialize the pixmap of the backbuffer */
static PixmapPtr
_initBackBufPixmap (DRI2BufferPtr pBackBuf, DrawablePtr pDraw, Bool canFlip)
{

    ScreenPtr pScreen = pDraw->pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    DRI2BufferPrivPtr pBackBufPriv = pBackBuf->driverPrivate;
    unsigned int usage_hint = CREATE_PIXMAP_USAGE_DRI2_BACK;
    PixmapPtr pPixmap = NULL;
    int pipe = -1;

    /* if a drawable can be flip, check whether the flip buffer is available */
    if (canFlip)
    {
        usage_hint = CREATE_PIXMAP_USAGE_SCANOUT_DRI2_BACK;
        pipe = 0;// secDisplayDrawablePipe (pDraw);
        if (pipe != -1)
        {
            /* get the flip pixmap from crtc */
            
            pPixmap = exynosCrtcGetFlipPixmap (pScrn, pipe, pDraw, usage_hint);
            if (!pPixmap)
            {
                /* fail to get a  flip pixmap from crtc */
                canFlip = FALSE;
                XDBG_WARNING(MDRI2, "fail to get a  flip pixmap from crtc\n");
            }
        }
        else
        {
            /* pipe is -1 */
            canFlip = FALSE;
            XDBG_WARNING(MDRI2, "pipe is -1");
        }
    }

    /* if canflip is false, get the dri2_back pixmap */
    if (!canFlip)
    {
        pPixmap = (*pScreen->CreatePixmap) (pScreen,
                                            pDraw->width,
                                            pDraw->height,
                                            pDraw->depth,
                                            usage_hint);
        XDBG_RETURN_VAL_IF_FAIL(pPixmap != NULL, NULL);
#ifdef USE_XDBG_EXTERNAL
        xDbgLogPListDrawAddRefPixmap (pDraw, pPixmap);
#endif
    }

    if (canFlip)
    {
        pBackBufPriv->num_buf = 2; 
        pBackBufPriv->pBackPixmaps = ctrl_calloc (pBackBufPriv->num_buf, sizeof (void*));
    }
    else
    {
        pBackBufPriv->num_buf = 1; /* num of backbuffer for swap/blit */
        pBackBufPriv->pBackPixmaps = ctrl_calloc (pBackBufPriv->num_buf, sizeof (void*));
    }

    pBackBufPriv->pBackPixmaps[0] = pPixmap;
    pBackBufPriv->can_flip = canFlip;
    pBackBufPriv->avail_idx = 0;
    pBackBufPriv->free_idx = 0;
    pBackBufPriv->cur_idx = 0;
    pBackBufPriv->pipe = pipe;


    return pPixmap;
}

/* return the next available pixmap of the backbuffer */
static PixmapPtr
_reuseBackBufPixmap (DRI2BufferPtr pBackBuf, DrawablePtr pDraw, Bool can_flip)
{

    ScreenPtr pScreen = pDraw->pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    DRI2BufferPrivPtr pBackBufPriv = pBackBuf->driverPrivate;
    PixmapPtr pPixmap = NULL;
    int avail_idx = pBackBufPriv->avail_idx;
    unsigned int usage_hint = CREATE_PIXMAP_USAGE_DRI2_BACK;
    int pipe = -1;

    if (pBackBufPriv->can_flip != can_flip)
    {
        /* flip buffer -> swap buffer : create dri2_back pixmap */
        if (pBackBufPriv->can_flip && !can_flip)
            XDBG_DEBUG (MDRI2, "flip -> swap\n");

        /* swap buffer -> flip buffer : create flip_dri2_back */
        if (!pBackBufPriv->can_flip && can_flip)
            XDBG_DEBUG (MDRI2, "swap -> flip\n");

        _deinitBackBufPixmap(pBackBuf, pDraw, pBackBufPriv->can_flip);

        pPixmap = _initBackBufPixmap(pBackBuf, pDraw, can_flip);
        XDBG_RETURN_VAL_IF_FAIL(pPixmap != NULL, NULL);
    }
    /*else
        pPixmap = pBackBufPriv->pPixmap;*/

    avail_idx = pBackBufPriv->avail_idx;

#if 1
    /* set the next available pixmap */
    /* if pBackPixmap is available, reuse it */
    if (pBackBufPriv->pBackPixmaps[avail_idx])
    {
        if (can_flip)
        {
            usage_hint = CREATE_PIXMAP_USAGE_SCANOUT_DRI2_BACK;
            pipe = 0;//secDisplayDrawablePipe (pDraw);
            if (pipe != -1)
            {
                if (avail_idx != pBackBufPriv->cur_idx)
                {
                    /* get the flip pixmap from crtc */
                    pBackBufPriv->pBackPixmaps[avail_idx] = exynosCrtcGetFlipPixmap (pScrn, pipe, pDraw, usage_hint);
                    if (!pBackBufPriv->pBackPixmaps[avail_idx])
                    {
                        /* fail to get a  flip pixmap from crtc */
                        XDBG_WARNING(MDRI2, "@@[reuse]: draw(0x%x) fail to get a flip pixmap from crtc to reset the index of pixmap\n",
                                     (unsigned int)pDraw->id);

                        _deinitBackBufPixmap (pBackBuf, pDraw, pBackBufPriv->can_flip);
                        pPixmap = _initBackBufPixmap(pBackBuf, pDraw, FALSE);
                        XDBG_RETURN_VAL_IF_FAIL(pPixmap != NULL, NULL);
                        //*reues = 0;
                        DRI2_END;return pPixmap;
                    }
                    pBackBufPriv->cur_idx = avail_idx;
                }
             }
             else
             {
                 XDBG_WARNING (MDRI2, "pipe is -1(%d)\n", pipe);
                 DRI2_END;return NULL;
             }
        }
        else
        {
            if (avail_idx != pBackBufPriv->cur_idx)
            {
                pBackBufPriv->cur_idx = avail_idx;
            }
        }

       // *reues = 1;
    }
    else
    {
        if (can_flip)
        {
            usage_hint = CREATE_PIXMAP_USAGE_SCANOUT_DRI2_BACK;
            pipe = 0;//secDisplayDrawablePipe (pDraw);
            if (pipe != -1)
            {
                if (avail_idx != pBackBufPriv->cur_idx)
                {
                     /* get the flip pixmap from crtc */
                    pBackBufPriv->pBackPixmaps[avail_idx] = exynosCrtcGetFlipPixmap (pScrn, pipe, pDraw, usage_hint);
                    if (!pBackBufPriv->pBackPixmaps[avail_idx])
                    {
                        /* fail to get a  flip pixmap from crtc */
                        XDBG_WARNING(MDRI2, "@@[initial set]: draw(0x%x) fail to get a flip pixmap from crtc to generate and to set the next available pixmap.\n",
                                     (unsigned int)pDraw->id);

                        _deinitBackBufPixmap (pBackBuf, pDraw, TRUE);
                        pPixmap = _initBackBufPixmap(pBackBuf, pDraw, FALSE);
                        XDBG_RETURN_VAL_IF_FAIL(pPixmap != NULL, NULL);
         //               *reues = 0;
                        DRI2_END;return pPixmap;
                    }
                    pBackBufPriv->cur_idx = avail_idx;
                }
            }
        }
        else
        {
            if (avail_idx != pBackBufPriv->cur_idx)
            {

                pBackBufPriv->pBackPixmaps[avail_idx] = (*pScreen->CreatePixmap) (pScreen,
                        pDraw->width,
                        pDraw->height,
                        pDraw->depth,
                        usage_hint);
                XDBG_RETURN_VAL_IF_FAIL(pBackBufPriv->pBackPixmaps[avail_idx] != NULL, NULL);
                pBackBufPriv->cur_idx = avail_idx;
#ifdef USE_XDBG_EXTERNAL
                xDbgLogPListDrawAddRefPixmap (pDraw, pPixmap);
#endif
            }
        }

        //*reues = 0;
    }
    pPixmap = pBackBufPriv->pBackPixmaps[avail_idx];

    pBackBufPriv->can_flip = can_flip;

#endif


    return pPixmap;
}

/* TODO: HACK */
static void
_setDri2Property (DrawablePtr pDraw)
{

    if(pDraw->type == DRAWABLE_WINDOW)
    {
        static Atom atom_use_dri2= 0;
        static int use = 1;

        if(!atom_use_dri2)
        {
            atom_use_dri2 = MakeAtom ("X_WIN_USE_DRI2", 14, TRUE);
        }

        dixChangeWindowProperty (serverClient,
                                 (WindowPtr)pDraw, atom_use_dri2, XA_CARDINAL, 32,
                                 PropModeReplace, 1, &use, TRUE);
    }

}

static unsigned int
_getBufferFlag (DrawablePtr pDraw, Bool can_flip)
{

    DRI2BufferFlags flag;
    flag.flags = 0;

    switch (pDraw->type)
    {
    case DRAWABLE_WINDOW:
        flag.data.type = DRI2_BUFFER_TYPE_WINDOW;
        break;
    case DRAWABLE_PIXMAP:
        flag.data.type = DRI2_BUFFER_TYPE_PIXMAP;
        break;
    }

    if (IS_VIEWABLE(pDraw))
        flag.data.is_viewable = 1;
    else
    	flag.data.is_viewable = 0;

    if (can_flip)
        flag.data.is_framebuffer = 1;


    return flag.flags;

}

static inline PixmapPtr
_getPixmapFromDrawable (DrawablePtr pDraw)
{

    ScreenPtr pScreen = pDraw->pScreen;
    PixmapPtr pPix;

    if (pDraw->type == DRAWABLE_WINDOW)
        pPix = (*pScreen->GetWindowPixmap) ((WindowPtr)pDraw);
    else
        pPix = (PixmapPtr)pDraw;


    return pPix;

}

#undef COMPOSITE

/* Can this drawable be page flipped? */
static Bool
_canFlip (DrawablePtr pDraw)
{

    ScreenPtr pScreen = pDraw->pScreen;
//    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    WindowPtr pWin, pRoot;
    PixmapPtr pWinPixmap, pRootPixmap;

    if (pDraw->type == DRAWABLE_PIXMAP)
        {

        return FALSE;
        }

    pRoot = pScreen->root;
    pRootPixmap = pScreen->GetWindowPixmap (pRoot);
    pWin = (WindowPtr) pDraw;
    pWinPixmap = pScreen->GetWindowPixmap (pWin);
    if (pRootPixmap != pWinPixmap)
        {

        return FALSE;
        }

    if (!IS_VIEWABLE(pDraw))
        {

        return FALSE;
        }

    /* TODO: check the size of a drawable */
    /* Does the window match the pixmap exactly? */
    if (pDraw->x != 0 || pDraw->y != 0 ||
#ifdef COMPOSITE
        pDraw->x != pWinPixmap->screen_x || pDraw->y != pWinPixmap->screen_y ||
#endif
        pDraw->width != pWinPixmap->drawable.width ||
        pDraw->height != pWinPixmap->drawable.height)
        {

            return FALSE;
        }

    return TRUE;
}

static DRI2FrameEventType
_getSwapType (DrawablePtr pDraw, DRI2BufferPtr pFrontBuf,
              DRI2BufferPtr pBackBuf)
{

    DRI2BufferPrivPtr pFrontBufPriv = pFrontBuf->driverPrivate;
    DRI2BufferPrivPtr pBackBufPriv = pBackBuf->driverPrivate;
    PixmapPtr pFrontPix = pFrontBufPriv->pPixmap;
    PixmapPtr pBackPix = pBackBufPriv->pPixmap;
    DRI2FrameEventType frame_type = DRI2_NONE;

    /* if a drawable is not viewable at DRI2GetBuffers, return none */
    if (!IS_VIEWABLE(pDraw))
    {
        //XDBG_WARNING (MDRI2, "DRI2_NONE: window is not viewable.(%d,%d)\n",
        //                            pDraw->width, pDraw->height);

        return DRI2_NONE;
    }

    /* Check Exchange */
    if (pFrontBufPriv->can_flip == 1)
    {
        if(pBackBufPriv->can_flip == 1)
        {
            frame_type = DRI2_FLIP;
        }
        else
        {
            XDBG_WARNING (MDRI2, "DRI2_FB_BLIT: Front(%d) Back(%d) \n",
                          pFrontBufPriv->can_flip, pBackBufPriv->can_flip);
            frame_type = DRI2_FB_BLIT;
        }
    }
    else
    {
        /* TODO : the case where can_flip=0 and pixmap==framebuffer */
#if 1
        EXYNOSExaPixPrivPtr pFrontExaPixPriv = NULL;
        EXYNOSExaPixPrivPtr pBackExaPixPriv = NULL;

        pFrontExaPixPriv = exaGetPixmapDriverPrivate (pFrontBufPriv->pPixmap);
        pBackExaPixPriv = exaGetPixmapDriverPrivate (pBackBufPriv->pPixmap);
        if (!pFrontExaPixPriv || !pBackExaPixPriv)
        {
            XDBG_WARNING(MDRI2, "Warning: pFrontPixPriv or pBackPixPriv is null.(DRI2_NONE)\n");

            return DRI2_NONE;
        }

        if (pFrontExaPixPriv->isFrameBuffer == 1)
        {
            XDBG_WARNING (MDRI2, "DRI2_FB_BLIT: Front(%d) Back(%d) : front is framebuffer \n",
                          pFrontBufPriv->can_flip, pBackBufPriv->can_flip);
            frame_type = DRI2_FB_BLIT;
        }
        else

#endif
        {
            if (pFrontPix->drawable.width == pBackPix->drawable.width &&
                pFrontPix->drawable.height == pBackPix->drawable.height &&
                pFrontPix->drawable.bitsPerPixel == pBackPix->drawable.bitsPerPixel)
            {
                frame_type = DRI2_SWAP;
              }
            else
            {
                frame_type = DRI2_BLIT;
            }
        }
    }


    return frame_type;

}

static void
_referenceBufferPriv (DRI2BufferPtr pBuf)
{

    if (pBuf)
    {
        DRI2BufferPrivPtr pBufPriv = pBuf->driverPrivate;
        pBufPriv->refcnt++;
    }

}

static void
_unreferenceBufferPriv (DRI2BufferPtr pBuf)
{

    if (pBuf)
    {
        DRI2BufferPrivPtr pBufPriv = pBuf->driverPrivate;
        pBufPriv->refcnt--;
    }

}

static void
_generateDamage (DrawablePtr pDraw, DRI2FrameEventPtr pFrameEvent)
{

    BoxRec box;
    RegionRec region;

    if (pFrameEvent->pRegion)
    {
        /* translate the regions with drawable */
        BoxPtr pBox = RegionRects(pFrameEvent->pRegion);
        int nBox = RegionNumRects(pFrameEvent->pRegion);

        while (nBox--)
        {
            box.x1 = pBox->x1;
            box.y1 = pBox->y1;
            box.x2 = pBox->x2;
            box.y2 = pBox->y2;
           XDBG_DEBUG(MDRI2,"Damage Region[%d]: (x1, y1, x2, y2) = (%d,%d,%d,%d) \n ",
                   nBox, box.x1, box.x2, box.y1, box.y2);
            RegionInit (&region, &box, 0);
            DamageDamageRegion (pDraw, &region);
            pBox++;
        }
    }
    else
    {

        box.x1 = pDraw->x;
        box.y1 = pDraw->y;
        box.x2 = box.x1 + pDraw->width;
        box.y2 = box.y1 + pDraw->height;
        RegionInit (&region, &box, 0);
        DamageDamageRegion (pDraw, &region);
    }

}

static void
_blitBuffers (DrawablePtr pDraw, DRI2BufferPtr pFrontBuf, DRI2BufferPtr pBackBuf)
{

    BoxRec box;
    RegionRec region;

    box.x1 = 0;
    box.y1 = 0;
    box.x2 = pDraw->width;
    box.y2 = pDraw->height;
    REGION_INIT (pScreen, &region, &box, 0);

    EYXNOSDri2CopyRegion (pDraw, &region, pFrontBuf, pBackBuf);

}

static void
_exchangeBuffers (DrawablePtr pDraw, DRI2FrameEventType type,
                  DRI2BufferPtr pFrontBuf, DRI2BufferPtr pBackBuf)
{

#if 0
    DRI2BufferPrivPtr pFrontBufPriv = pFrontBuf->driverPrivate;
    DRI2BufferPrivPtr pBackBufPriv = pBackBuf->driverPrivate;

    XDBG_RETURN_IF_FAIL (pFrontBufPriv->can_flip == pBackBufPriv->can_flip);

    exynosPixmapSwap (pFrontBufPriv->pPixmap, pBackBufPriv->pPixmap);
    pBackBufPriv->avail_idx = DRI2_GET_NEXT_IDX(pBackBufPriv->avail_idx, pBackBufPriv->num_buf);
#else

    ScreenPtr pScreen = pDraw->pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    DRI2BufferPrivPtr pFrontBufPriv = pFrontBuf->driverPrivate;
    DRI2BufferPrivPtr pBackBufPriv = pBackBuf->driverPrivate;
    EXYNOSExaPixPrivRec *pFrontExaPixPriv = exaGetPixmapDriverPrivate (pFrontBufPriv->pPixmap);
    EXYNOSExaPixPrivRec *pBackExaPixPriv = exaGetPixmapDriverPrivate (pBackBufPriv->pPixmap);

    if(pFrontBufPriv->can_flip != pBackBufPriv->can_flip)
    {
        XDBG_WARNING (MDRI2, "Cannot exchange buffer(0x%x): Front(%d, can_flip:%d), Back(%d, can_flip:%d)\n",
                      (unsigned int)pDraw->id, pFrontBuf->name, pFrontBufPriv->can_flip,
                      pBackBuf->name, pBackBufPriv->can_flip);

        DRI2_END;return;
    }

    /* exchange the buffers
     * 1. exchange the bo of the exa pixmap private
     * 2. get the name of the front buffer (the name of the back buffer will get next DRI2GetBuffers.)
     */
    if (pFrontBufPriv->can_flip)
    {
        if(tbm_bo_swap(pFrontExaPixPriv->tbo, pBackExaPixPriv->tbo))
        {
            EXYNOS_FbBoDataPtr bo_data = NULL;
            EXYNOS_FbBoDataPtr back_bo_data = NULL;
            EXYNOS_FbBoDataRec tmp_bo_data;

            tbm_bo_get_user_data(pFrontExaPixPriv->tbo, TBM_BO_DATA_FB, (void**)&bo_data);
            tbm_bo_get_user_data(pBackExaPixPriv->tbo, TBM_BO_DATA_FB, (void**)&back_bo_data);

            if (back_bo_data && bo_data)
            {
                memcpy(&tmp_bo_data,    bo_data,        sizeof(EXYNOS_FbBoDataRec));
                memcpy(bo_data,         back_bo_data,   sizeof(EXYNOS_FbBoDataRec));
                memcpy(back_bo_data,    &tmp_bo_data,   sizeof(EXYNOS_FbBoDataRec));
            }
        }

        int old_name =pFrontBuf->name;
        pFrontBuf->name = _getName (pFrontBufPriv->pPixmap);
        XDBG_ERROR(MDRI2,"Rename from %d to %d at line %d\n",old_name,pFrontBuf->name,__LINE__);
    }
    else
    {
        tbm_bo_swap(pFrontExaPixPriv->tbo, pBackExaPixPriv->tbo);
        int old_name =pFrontBuf->name;
        pFrontBuf->name = _getName (pFrontBufPriv->pPixmap);
        XDBG_ERROR(MDRI2,"Rename from %d to %d at line %d\n",old_name,pFrontBuf->name,__LINE__);
    }

    /*Exchange pixmap owner and sbc*/
    {
        XID owner;
        CARD64 sbc;

        owner = pFrontExaPixPriv->owner;
        sbc = pFrontExaPixPriv->sbc;

        pFrontExaPixPriv->owner = pBackExaPixPriv->owner;
        pFrontExaPixPriv->sbc = pBackExaPixPriv->sbc;

        pBackExaPixPriv->owner = owner;
        pBackExaPixPriv->sbc = sbc;
    }


    pBackBufPriv->avail_idx = DRI2_GET_NEXT_IDX(pBackBufPriv->avail_idx, pBackBufPriv->num_buf);
#endif

}

static DRI2FrameEventPtr
_newFrameEvent (ClientPtr pClient, DrawablePtr pDraw,
           DRI2BufferPtr pFrontBuf, DRI2BufferPtr pBackBuf,
           DRI2SwapEventPtr swap_func, void *data, RegionPtr pRegion)
{

    DRI2FrameEventPtr pFrameEvent = NULL;
    DRI2FrameEventType frame_type = DRI2_NONE;
    DRI2BufferPrivPtr pFrontBufPriv = pFrontBuf->driverPrivate;
    DRI2BufferPrivPtr pBackBufPriv = pBackBuf->driverPrivate;
    CARD64 sbc;
    unsigned int pending;

    /* check and get the frame_type */
    frame_type = _getSwapType (pDraw, pFrontBuf, pBackBuf);
    if (frame_type == DRI2_NONE)
        {

        return NULL;
        }

    pFrameEvent = ctrl_calloc (1, sizeof (DRI2FrameEventRec));
    if (!pFrameEvent)
        {

        return NULL;
        }

    pFrameEvent->type = frame_type;
    pFrameEvent->drawable_id = pDraw->id;
    pFrameEvent->client_idx = pClient->index;
    pFrameEvent->pClient = pClient;
    pFrameEvent->event_complete = swap_func;
    pFrameEvent->event_data = data;
    pFrameEvent->pFrontBuf = pFrontBuf;
    pFrameEvent->pBackBuf = pBackBuf;
    pFrameEvent->front_bo = tbm_bo_ref (exynosExaPixmapGetBo (pFrontBufPriv->pPixmap));
    pFrameEvent->back_bo = tbm_bo_ref (exynosExaPixmapGetBo (pBackBufPriv->pPixmap));
    pFrameEvent->pRegion = NULL;

    if (pRegion)
    {
        pFrameEvent->pRegion = RegionCreate(RegionExtents(pRegion),
                                            RegionNumRects(pRegion));
        if (!RegionCopy(pFrameEvent->pRegion, pRegion))
        {
            RegionDestroy(pFrameEvent->pRegion);
            pFrameEvent->pRegion = NULL;
        }
    }

    /* Set frame count to back*/
    DRI2GetSBC(pDraw, &sbc, &pending);
    sbc = sbc + pending;
    exynosPixmapSetFrameCount (pFrontBufPriv->pPixmap, pDraw->id, sbc);
    exynosPixmapSetFrameCount (pBackBufPriv->pPixmap, pDraw->id, sbc);

    _referenceBufferPriv (pFrontBuf);
    _referenceBufferPriv (pBackBuf);


    return pFrameEvent;

}

static void
_swapFrame (DrawablePtr pDraw, DRI2FrameEventPtr pFrameEvent)
{

    BoxPtr pBox = RegionRects(pFrameEvent->pRegion);
    if(pBox->x1<0){
        XDBG_WARNING (MDRI2, "strange datas\n");
        DRI2_END;
        return;
    }
    switch (pFrameEvent->type)
    {
    case DRI2_FLIP:
//        _generateDamage (pDraw, pFrameEvent);
        break;
    case DRI2_SWAP:
        _exchangeBuffers (pDraw, pFrameEvent->type,
                          pFrameEvent->pFrontBuf, pFrameEvent->pBackBuf);
        _generateDamage (pDraw, pFrameEvent);
        break;
    case DRI2_BLIT:
    case DRI2_FB_BLIT:
        /* copy the region from back buffer to front buffer */
        _blitBuffers (pDraw, pFrameEvent->pFrontBuf, pFrameEvent->pBackBuf);
        break;
    default:
        /* Unknown type */
        XDBG_WARNING (MDRI2, "%s: unknown frame_type received\n", __func__);
        _generateDamage (pDraw, pFrameEvent);
        break;
    }

}

// This is DOG-NAIL for remove implicit declaration. It must be moved to header (maybe crtc.h needs?)
void exynosCrtcExemptFlipPixmap (ScrnInfoPtr pScrn, int crtc_pipe, PixmapPtr pPixmap); 

static void
_disuseBackBufPixmap (DRI2BufferPtr pBackBuf, DRI2FrameEventPtr pEvent)
{
DRI2_BEGIN;
    DRI2BufferPrivPtr pBackBufPriv = pBackBuf->driverPrivate;
    ScreenPtr pScreen = pBackBufPriv->pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];

    if (pEvent->type == DRI2_FLIP)
    {
        exynosCrtcExemptFlipPixmap (pScrn, pEvent->crtc_pipe,
                pBackBufPriv->pBackPixmaps[pBackBufPriv->free_idx]);

        /* increase free_idx when buffers destory or when frame is deleted */
        pBackBufPriv->free_idx = DRI2_GET_NEXT_IDX(pBackBufPriv->free_idx, pBackBufPriv->num_buf);
    }
DRI2_END;
}

static void
_deleteFrameEvent (DrawablePtr pDraw, DRI2FrameEventPtr pEvent)
{

    if (pEvent->pBackBuf)
    {
        XDBG_DEBUG(MDRI2,"pBackBuf\n");
        _disuseBackBufPixmap(pEvent->pBackBuf, pEvent);
        EYXNOSDri2DestroyBuffer (pDraw, pEvent->pBackBuf);
    }

    if (pEvent->pFrontBuf){
        XDBG_DEBUG(MDRI2,"pFrontBuf\n");
        EYXNOSDri2DestroyBuffer (pDraw, pEvent->pFrontBuf);
    }

    if (pEvent->pRegion)
        RegionDestroy (pEvent->pRegion);

    tbm_bo_unref (pEvent->back_bo);
    tbm_bo_unref (pEvent->front_bo);

    ctrl_free (pEvent);
    pEvent = NULL;

}

static void
_asyncSwapBuffers (ClientPtr pClient, DrawablePtr pDraw, DRI2FrameEventPtr pFrameEvent)
{

    XDBG_DEBUG(MDRI2,"id:0x%x(%d) Client:%d Front(attach:%d, name:%d, flag:0x%x), Back(attach:%d, name:%d, flag:0x%x)\n",
               (unsigned int)pDraw->id, pDraw->type,
               pClient->index,
               pFrameEvent->pFrontBuf->attachment, pFrameEvent->pFrontBuf->name, pFrameEvent->pFrontBuf->flags,
               pFrameEvent->pBackBuf->attachment, pFrameEvent->pBackBuf->name, pFrameEvent->pBackBuf->flags);

    _swapFrame (pDraw, pFrameEvent);

    switch (pFrameEvent->type)
    {
    case DRI2_SWAP:
        DRI2SwapComplete (pClient, pDraw, 0, 0, 0,
                          DRI2_EXCHANGE_COMPLETE,
                          pFrameEvent->event_complete, pFrameEvent->event_data);
        break;
    case DRI2_FLIP:
        _exchangeBuffers (pDraw, pFrameEvent->type, pFrameEvent->pFrontBuf, pFrameEvent->pBackBuf);
        DRI2SwapComplete (pClient, pDraw, 0, 0, 0,
                          DRI2_FLIP_COMPLETE,
                          pFrameEvent->event_complete, pFrameEvent->event_data);
        break;
    case DRI2_BLIT:
    case DRI2_FB_BLIT:
        DRI2SwapComplete (pClient, pDraw, 0, 0, 0,
                          DRI2_BLIT_COMPLETE,
                          pFrameEvent->event_complete, pFrameEvent->event_data);
        break;
    default:
        DRI2SwapComplete (pClient, pDraw, 0, 0, 0,
                          0,
                          pFrameEvent->event_complete, pFrameEvent->event_data);
        break;
    }

}

//Move to header
void exynosCrtcAddPendingFlip (xf86CrtcPtr pCrtc, DRI2FrameEventPtr pEvent);

static Bool
_scheduleFlip (DrawablePtr pDraw, DRI2FrameEventPtr pEvent, Bool calledFromProcessPending)
{

    ScreenPtr pScreen = pDraw->pScreen;
    ScrnInfoPtr pScrn =  xf86Screens[pScreen->myNum];
    DRI2BufferPrivPtr pBufPriv = pEvent->pBackBuf->driverPrivate;
    int pipe = -1;
    xf86CrtcPtr pCrtc=NULL;

    /* main crtc for this drawable shall finally deliver pageflip event */
    pipe = exynosDisplayGetPipe (pScrn, pDraw);
    pCrtc = exynosDisplayGetCrtc(pDraw);

    DRI2BufferPrivPtr pBackBufPriv = pEvent->pBackBuf->driverPrivate;
    
    if (pBackBufPriv->pFlipEvent && !calledFromProcessPending)
    {
        if (pBackBufPriv->pFlipEvent->pPendingEvent == NULL)
        {
            pBackBufPriv->pFlipEvent->pPendingEvent = pEvent;
            XDBG_DEBUG(MDRI2,"add pending event id:0x%x(%d) Client:%d pipe:%d Front(attach:%d, name:%d, flag:0x%x), "
                        "Back(attach:%d, name:%d, flag:0x%x )\n",
                        (unsigned int)pDraw->id, pDraw->type,
                        pEvent->pClient->index, pEvent->pipe,
                        pEvent->pFrontBuf->attachment, pEvent->pFrontBuf->name, pEvent->pFrontBuf->flags,
                        pEvent->pBackBuf->attachment, pEvent->pBackBuf->name, pEvent->pBackBuf->flags);
            DRI2_END;
            return TRUE;
        }
        else
        {
            DRI2_END;
            return FALSE;
        }
    }
    else if (!exynosDisplayPageFlip (pScrn, pEvent->pipe, pBufPriv->pPixmap, pEvent))
    {
        XDBG_WARNING (MDRI2, "fail to exynosDisplayPageFlip.\n");

        return FALSE;
    }

    XDBG_DEBUG(MDRI2,"doPageFlip id:0x%x(%d) Client:%d pipe:%d Front(attach:%d, name:%d, flag:0x%x), "
                "Back(attach:%d, name:%d, flag:0x%x )\n",
                (unsigned int)pDraw->id, pDraw->type,
                pEvent->pClient->index, pEvent->pipe,
                pEvent->pFrontBuf->attachment, pEvent->pFrontBuf->name, pEvent->pFrontBuf->flags,
                pEvent->pBackBuf->attachment, pEvent->pBackBuf->name, pEvent->pBackBuf->flags);


    DRI2SwapComplete (pEvent->pClient, pDraw, 0, 0, 0, DRI2_FLIP_COMPLETE,
                              pEvent->event_complete, pEvent->event_data);

    pBackBufPriv->pFlipEvent = pEvent;
    _exchangeBuffers (pDraw, pEvent->type, pEvent->pFrontBuf, pEvent->pBackBuf);


    return TRUE;

}

static DRI2BufferPtr
EYXNOSDri2CreateBuffer (DrawablePtr pDraw, unsigned int attachment, unsigned int format)
{

    ScreenPtr pScreen = pDraw->pScreen;
    DRI2BufferPtr pBuf = NULL;
    DRI2BufferPrivPtr pBufPriv = NULL;
    PixmapPtr pPix = NULL;
    Bool can_flip = FALSE;

    /* create dri2 buffer */
    pBuf = ctrl_calloc (1, sizeof (DRI2BufferRec));
    if (pBuf == NULL)
        goto fail;

    /* create dri2 buffer private */
    pBufPriv = ctrl_calloc (1, sizeof (DRI2BufferPrivRec));
    if (pBufPriv == NULL)
        goto fail;

    /* check can_flip */
    can_flip = _canFlip (pDraw);

    pBuf->driverPrivate = pBufPriv;
    pBuf->format = format;
    pBuf->flags = _getBufferFlag (pDraw, can_flip);

    /* check the attachments */
    if (attachment == DRI2BufferFrontLeft)
    {
        pPix = _getPixmapFromDrawable (pDraw);
        pPix->refcnt++;
    }
    else
    {
        switch (attachment)
        {
        case DRI2BufferDepth:
        case DRI2BufferDepthStencil:
        case DRI2BufferFakeFrontLeft:
        case DRI2BufferFakeFrontRight:
        case DRI2BufferBackRight:
        case DRI2BufferBackLeft:
            pPix = _initBackBufPixmap (pBuf, pDraw, can_flip);
            XDBG_GOTO_IF_FAIL (pPix != NULL, fail);
            break;
        default:
            XDBG_ERROR (MDRI2, "Unsupported attachmemt:%d\n", attachment);
            goto fail;
            break;
        }

        /* TODO: [HACK] Set DRI2 property for selective-composite mode */
        _setDri2Property (pDraw);
    }

    pBuf->cpp = pPix->drawable.bitsPerPixel / 8;
    pBuf->attachment = attachment;
    pBuf->pitch = pPix->devKind;

    //pBuf->name = _getName (pPix);
    RENAME(pBuf->name,pPix);

    XDBG_GOTO_IF_FAIL (pBuf->name != 0, fail);

    pBufPriv->can_flip = can_flip;
    pBufPriv->refcnt = 1;
    pBufPriv->attachment = attachment;
    pBufPriv->pPixmap = pPix;
    pBufPriv->pScreen = pScreen;

/*    pBufPriv->num_buf=1+(can_flip==TRUE);
    pBufPriv->pBackPixmaps = ctrl_calloc (pBufPriv->num_buf, sizeof (void*));
    pBufPriv->avail_idx = 0;
    pBufPriv->free_idx = 0;
    pBufPriv->cur_idx = 0;*/
    pBufPriv->pipe = 0;

    XDBG_DEBUG (MDRI2, "id:0x%x(%d) attach:%d, name:%d, flags:0x%x, flip:%d, geo(%dx%d+%d+%d)\n",
                pDraw->id, pDraw->type, pBuf->attachment, pBuf->name, pBuf->flags,
                pBufPriv->can_flip, pDraw->width, pDraw->height, pDraw->x, pDraw->y);


    return pBuf;
fail:
    XDBG_WARNING (MDRI2, "Failed: id:0x%x(%d) attach:%d, geo(%dx%d+%d+%d)\n",
                  pDraw->id, pDraw->type, attachment,
                  pDraw->width, pDraw->height, pDraw->x, pDraw->y);
    if (pPix)
    {
//        xDbgLogPListDrawRemoveRefPixmap (pDraw, pPix);
		(*pScreen->DestroyPixmap) (pPix);
//        _deinitBackBufPixmap(NULL, pPix, can_flip);
    }
    if (pBufPriv)
        ctrl_free (pBufPriv);
    if (pBuf)
        ctrl_free (pBuf);

    return NULL;
}

static void
EYXNOSDri2DestroyBuffer (DrawablePtr pDraw, DRI2BufferPtr pBuf)
{

    ScreenPtr pScreen = NULL;
    DRI2BufferPrivPtr pBufPriv = NULL;

    if (pBuf == NULL)
        {

        return;
        }

    pBufPriv = pBuf->driverPrivate;
    XDBG_DEBUG(MDRI2, "%p %i\n",pBuf,pBufPriv->refcnt);
    pScreen = pBufPriv->pScreen;

    _unreferenceBufferPriv(pBuf);

    if (pBufPriv->refcnt == 0)
    {
        XDBG_DEBUG(MDRI2, "DestroyBuffer(%d:0x%x) name:%d flip:%d\n",
                   pDraw?pDraw->type:0,
                   pDraw?(unsigned int)pDraw->id:0,
                   pBuf->name,
                   pBufPriv->can_flip);

        if (pBuf->attachment == DRI2BufferFrontLeft)
        {
            (*pScreen->DestroyPixmap) (pBufPriv->pPixmap);
        }
        else
        {
        	_deinitBackBufPixmap(pBuf, pDraw, pBufPriv->can_flip);
        }

        pBufPriv->pPixmap = NULL;
        if (pBufPriv != NULL)	ctrl_free (pBufPriv);
        if (pBuf != NULL)		ctrl_free (pBuf);
    }

}

static void
EYXNOSDri2CopyRegion (DrawablePtr pDraw, RegionPtr pRegion,
                   DRI2BufferPtr pDstBuf, DRI2BufferPtr pSrcBuf)
{

    DRI2BufferPrivPtr pSrcBufPriv = pSrcBuf->driverPrivate;
    DRI2BufferPrivPtr pDstBufPriv = pDstBuf->driverPrivate;
    ScreenPtr pScreen = pDraw->pScreen;
    RegionPtr pCopyClip;
    GCPtr pGc;

    DrawablePtr pSrcDraw = (pSrcBufPriv->attachment == DRI2BufferFrontLeft)
                           ? pDraw : &pSrcBufPriv->pPixmap->drawable;
    DrawablePtr pDstDraw = (pDstBufPriv->attachment == DRI2BufferFrontLeft)
                           ? pDraw : &pDstBufPriv->pPixmap->drawable;

    pGc = GetScratchGC (pDstDraw->depth, pScreen);
    if (!pGc)
        {

        return;
        }

    XDBG_DEBUG(MDRI2,"CopyRegion(%d,0x%x) Dst(attach:%d, name:%d, flag:0x%x), Src(attach:%d, name:%d, flag:0x%x)\n",
               pDraw->type, (unsigned int)pDraw->id,
               pDstBuf->attachment, pDstBuf->name, pDstBuf->flags,
               pSrcBuf->attachment, pSrcBuf->name, pSrcBuf->flags);

    pCopyClip = REGION_CREATE (pScreen, NULL, 0);
    REGION_COPY (pScreen, pCopyClip, pRegion);
    (*pGc->funcs->ChangeClip) (pGc, CT_REGION, pCopyClip, 0);
    ValidateGC (pDstDraw, pGc);

    /* Wait for the scanline to be outside the region to be copied */
    /* [TODO] Something Do ??? */

    /* It's important that this copy gets submitted before the
    * direct rendering client submits rendering for the next
    * frame, but we don't actually need to submit right now.  The
    * client will wait for the DRI2CopyRegion reply or the swap
    * buffer event before rendering, and we'll hit the flush
    * callback chain before those messages are sent.  We submit
    * our batch buffers from the flush callback chain so we know
    * that will happen before the client tries to render
    * again. */

    (*pGc->ops->CopyArea) (pSrcDraw, pDstDraw,
                           pGc,
                           0, 0,
                           pDraw->width, pDraw->height,
                           0, 0);
    (*pGc->funcs->DestroyClip) (pGc);
    FreeScratchGC (pGc);

}


/*
 * ScheduleSwap is responsible for requesting a DRM vblank event for the
 * appropriate frame.
 *
 * In the case of a blit (e.g. for a windowed swap) or buffer exchange,
 * the vblank requested can simply be the last queued swap frame + the swap
 * interval for the drawable.
 *
 * In the case of a page flip, we request an event for the last queued swap
 * frame + swap interval - 1, since we'll need to queue the flip for the frame
 * immediately following the received event.
 *
 * The client will be blocked if it tries to perform further GL commands
 * after queueing a swap, though in the Intel case after queueing a flip, the
 * client is free to queue more commands; they'll block in the kernel if
 * they access buffers busy with the flip.
 *
 * When the swap is complete, the driver should call into the server so it
 * can send any swap complete events that have been requested.
 */
static int
EYXNOSDri2ScheduleSwapWithRegion (ClientPtr pClient, DrawablePtr pDraw,
                     DRI2BufferPtr pFrontBuf, DRI2BufferPtr pBackBuf,
                     CARD64 *target_msc, CARD64 divisor,    CARD64 remainder,
                     DRI2SwapEventPtr swap_func, void *data, RegionPtr pRegion)
{

    ScreenPtr pScreen = pDraw->pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    int pipe = 0; /* default */
    int flip = 0;
    DRI2FrameEventPtr pFrameEvent = NULL;
    DRI2FrameEventType frame_type = DRI2_SWAP;
    CARD64 current_msc;
    CARD64 ust, msc;

    pFrameEvent = _newFrameEvent (pClient, pDraw, pFrontBuf, pBackBuf, swap_func, data, pRegion);
    if (!pFrameEvent)
    {
        DRI2SwapComplete (pClient, pDraw, 0, 0, 0, 0, swap_func, data);

        return TRUE;
    }

    /* Set frame count to back*/
    {
        PixmapPtr pPix;
        EXYNOSExaPixPrivPtr pExaPixPriv = NULL;
        DRI2BufferPrivPtr pBufPriv = pBackBuf->driverPrivate;
        DRI2BufferPrivPtr pfBufPriv = pBackBuf->driverPrivate;
        
        //XDBG_ERROR(MDRI2,"Num is %d %d \n",pBufPriv->num_buf,pfBufPriv->num_buf);
        
        CARD64 sbc;
        unsigned int pending;

        pPix = pBufPriv->pBackPixmaps[pBufPriv->cur_idx];
        pExaPixPriv = exaGetPixmapDriverPrivate (pPix);
        DRI2GetSBC(pDraw, &sbc, &pending);
        pExaPixPriv->owner = pDraw->id;
        pExaPixPriv->sbc = sbc+pending;
    }
    frame_type = pFrameEvent->type;

    pipe = exynosDisplayGetPipe (pScrn, pDraw);

    /* TODO: async swapbuffers and do not wait for the vblank sync,
                       WHEN the CRTC associated with a drawable is off.
         */
    /* check if a crtc associated with the drawable is off. */
    if (!exynosDisplayPipeIsOn (pScrn, pipe))
    {
        _asyncSwapBuffers (pClient, pDraw, pFrameEvent);
        _deleteFrameEvent (pDraw, pFrameEvent);


        return TRUE;
    }

    /* TODO: if the pipe is -1 and DRI2_FLIP, skip it now.
                       blit is reasonable ???
         */
    //if (pipe == -1 && frame_type == DRI2_FLIP)
    if (pipe == -1)
    {
        XDBG_WARNING(MDRI2, "Warning: flip pipe is -1 \n");
        _asyncSwapBuffers (pClient, pDraw, pFrameEvent);
        _deleteFrameEvent (pDraw, pFrameEvent);

        return TRUE;
    }

    /* Truncate to match kernel interfaces; means occasional overflow
     * misses, but that's generally not a big deal */
    *target_msc &= 0xffffffff;
    divisor &= 0xffffffff;
    remainder &= 0xffffffff;

    /* Get current count */
    if (!exynosDisplayGetCurMSC (pScrn, pipe, &ust, &msc))
    {
        xf86DrvMsg (pScrn->scrnIndex, X_WARNING,
                    "fail to get current_msc\n");
        goto blit_fallback;
    }
    current_msc = msc;

    //XDBG_ERROR(MDRI2,"MSC - %d\n",msc);

    /* Flips need to be submitted one frame before */
    if (frame_type == DRI2_FLIP)
    {
        flip = 1;
    }

    /* Correct target_msc by 'flip' if frame_type == DRI2_FLIP.
     * Do it early, so handling of different timing constraints
     * for divisor, remainder and msc vs. target_msc works.
     */
    if (*target_msc > 0)
        *target_msc -= flip;

    /*
     * If divisor is zero, or current_msc is smaller than target_msc
     * we just need to make sure target_msc passes before initiating
     * the swap.
     */
    if (divisor == 0 || current_msc < *target_msc)
    {
        /* If target_msc already reached or passed, set it to
         * current_msc to ensure we return a reasonable value back
         * to the caller. This makes swap_interval logic more robust.
         */
        if (current_msc >= *target_msc)
            *target_msc = current_msc;

        if (!exynosDisplayVBlank (pScrn, pipe, target_msc, flip, VBLANK_INFO_SWAP, pFrameEvent))
        {
            xf86DrvMsg (pScrn->scrnIndex, X_WARNING, "fail to Vblank\n");
            goto blit_fallback;
        }

        pFrameEvent->frame = (unsigned int )*target_msc;

        XDBG_DEBUG(MDRI2,"id:0x%x(%d) SwapType:%d Client:%d pipe:%d Front(attach:%d, name:%d, flag:0x%x), "
                   "Back(attach:%d, name:%d, flag:0x%x )\n",
                   (unsigned int)pDraw->id, pDraw->type,
                   frame_type, pClient->index, pipe,
                   pFrontBuf->attachment, pFrontBuf->name, pFrontBuf->flags,
                   pBackBuf->attachment, pBackBuf->name, pBackBuf->flags);

        if (pFrameEvent->pRegion)
        {
            BoxPtr pBox = RegionRects(pFrameEvent->pRegion);
            int nBox = RegionNumRects(pFrameEvent->pRegion);

            while (nBox--)
            {
                   XDBG_DEBUG(MDRI2," Region[%d]: (x1, y1, x2, y2) = (%d,%d,%d,%d) \n ",
                           nBox, pBox->x1, pBox->y1, pBox->x2, pBox->y2);

                   pBox++;
            }
        }

        _swapFrame (pDraw, pFrameEvent);


        return TRUE;
    }

    /*
     * If we get here, target_msc has already passed or we don't have one,
     * and we need to queue an event that will satisfy the divisor/remainder
     * equation.
     */
    *target_msc = current_msc - (current_msc % divisor) +
                           remainder;

    /*
     * If the calculated deadline vbl.request.sequence is smaller than
     * or equal to current_msc, it means we've passed the last point
     * when effective onset frame seq could satisfy
     * seq % divisor == remainder, so we need to wait for the next time
     * this will happen.

     * This comparison takes the 1 frame swap delay in pageflipping mode
     * into account, as well as a potential DRM_VBLANK_NEXTONMISS delay
     * if we are blitting/exchanging instead of flipping.
     */
    if (*target_msc <= current_msc)
        *target_msc += divisor;

    /* Account for 1 frame extra pageflip delay if flip > 0 */
    *target_msc -= flip;

    if (!exynosDisplayVBlank (pScrn, pipe, target_msc, flip, VBLANK_INFO_SWAP, pFrameEvent))
    {
        xf86DrvMsg (pScrn->scrnIndex, X_WARNING, "fail to Vblank\n");
        goto blit_fallback;
    }

    pFrameEvent->frame = *target_msc;

    XDBG_DEBUG(MDRI2,"ScaduleSwap_ex(%d,0x%x) SwapType:%d Client:%d pipe:%d Front(attach:%d, name:%d, flag:0x%x), Back(attach:%d, name:%d, flag:0x%x)\n",
               pDraw->type, (unsigned int)pDraw->id,
               frame_type, pClient->index, pipe,
               pFrontBuf->attachment, pFrontBuf->name, pFrontBuf->flags,
               pBackBuf->attachment, pBackBuf->name, pBackBuf->flags);

    _swapFrame (pDraw, pFrameEvent);


    return TRUE;

blit_fallback:
    XDBG_WARNING(MDRI2,"blit_fallback(%d,0x%x) SwapType:%d Client:%d pipe:%d Front(attach:%d, name:%d, flag:0x%x), Back(attach:%d, name:%d, flag:0x%x)\n",
                 pDraw->type, (unsigned int)pDraw->id,
                 frame_type, pClient->index, pipe,
                 pFrontBuf->attachment, pFrontBuf->name, pFrontBuf->flags,
                 pBackBuf->attachment, pBackBuf->name, pBackBuf->flags);

    _blitBuffers (pDraw, pFrontBuf, pBackBuf);

    DRI2SwapComplete (pClient, pDraw, 0, 0, 0, DRI2_BLIT_COMPLETE, swap_func, data);
    if (pFrameEvent)
    {
        _deleteFrameEvent (pDraw, pFrameEvent);
    }
    *target_msc = 0; /* offscreen, so zero out target vblank count */

    return TRUE;
}


/*
 * Get current frame count and frame count timestamp, based on drawable's
 * crtc.
 */
static int
EYXNOSDri2GetMSC (DrawablePtr pDraw, CARD64 *ust, CARD64 *msc)
{

    ScreenPtr pScreen = pDraw->pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    int pipe = exynosDisplayGetPipe (pScrn, pDraw);

    /* Get current count */
    if (!exynosDisplayGetCurMSC (pScrn, pipe, ust, msc))
    {
        xf86DrvMsg (pScrn->scrnIndex, X_WARNING,
                    "fail to get current_msc\n");

        return FALSE;
    }


    return TRUE;

}

/*
 * Request a DRM event when the requested conditions will be satisfied.
 *
 * We need to handle the event and ask the server to wake up the client when
 * we receive it.
 */
static int
EYXNOSDri2ScheduleWaitMSC (ClientPtr pClient, DrawablePtr pDraw,
                        CARD64 target_msc, CARD64 divisor, CARD64 remainder)
{

    ScreenPtr pScreen = pDraw->pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    DRI2FrameEventPtr wait_info = NULL;
    CARD64 current_msc;
    CARD64 ust, msc;

    int pipe = 0;


    /* Truncate to match kernel interfaces; means occasional overflow
     * misses, but that's generally not a big deal */
    target_msc &= 0xffffffff;
    divisor &= 0xffffffff;
    remainder &= 0xffffffff;

    /* Drawable not visible, return immediately */
    pipe = exynosDisplayGetPipe (pScrn, pDraw);

    XDBG_WARNING(MDRI2,"target_msc(%i) divisor(%i) remainder(%i) pipe(%i)\n",target_msc,divisor,remainder,pipe);

    if (pipe == -1)
        goto out_complete;

    wait_info = ctrl_calloc (1, sizeof (DRI2FrameEventRec));
    if (!wait_info)
        goto out_complete;

    wait_info->drawable_id = pDraw->id;
    wait_info->pClient = pClient;
    wait_info->type = DRI2_WAITMSC;

    /* Get current count */
    if (!exynosDisplayGetCurMSC (pScrn, pipe, &ust, &msc))
    {
        xf86DrvMsg (pScrn->scrnIndex, X_WARNING,
                    "fail to get current_msc\n");
        goto out_complete;
    }
    current_msc = msc;

    /*
     * If divisor is zero, or current_msc is smaller than target_msc,
     * we just need to make sure target_msc passes  before waking up the
     * client.
     */
    if (divisor == 0 || current_msc < target_msc)
    {
        /* If target_msc already reached or passed, set it to
         * current_msc to ensure we return a reasonable value back
         * to the caller. This keeps the client from continually
         * sending us MSC targets from the past by forcibly updating
         * their count on this call.
         */
        if (current_msc >= target_msc)
            target_msc = current_msc;

        /* flip is 1 to avoid to set DRM_VBLANK_NEXTONMISS */
        if (!exynosDisplayVBlank (pScrn, pipe, &target_msc, 1, VBLANK_INFO_SWAP, wait_info))
        {
            xf86DrvMsg (pScrn->scrnIndex, X_WARNING, "fail to Vblank\n");
            goto out_complete;
        }

        wait_info->frame = target_msc - 1; /* reply qeuenct is +1 in exynosDisplayVBlank */
        DRI2BlockClient (pClient, pDraw);

        return TRUE;
    }

    /*
     * If we get here, target_msc has already passed or we don't have one,
     * so we queue an event that will satisfy the divisor/remainder equation.
     */
    target_msc = current_msc - (current_msc % divisor) +
                           remainder;

    /*
     * If calculated remainder is larger than requested remainder,
     * it means we've passed the last point where
     * seq % divisor == remainder, so we need to wait for the next time
     * that will happen.
     */
    if ((current_msc % divisor) >= remainder)
        target_msc += divisor;

    /* flip is 1 to avoid to set DRM_VBLANK_NEXTONMISS */
    if (!exynosDisplayVBlank (pScrn, pipe, &target_msc, 1, VBLANK_INFO_SWAP, wait_info))
    {
        xf86DrvMsg (pScrn->scrnIndex, X_WARNING, "fail to Vblank\n");
        goto out_complete;
    }

    wait_info->frame = target_msc - 1; /* reply qeuenct is +1 in exynosDisplayVBlank */
    DRI2BlockClient (pClient, pDraw);


    return TRUE;

out_complete:
    ctrl_free(wait_info);
    DRI2WaitMSCComplete (pClient, pDraw, target_msc, 0, 0);

    return TRUE;

}

static int
EYXNOSDri2AuthMagic (int fd, uint32_t magic)
{

    int ret;
    ret = drmAuthMagic (fd, (drm_magic_t) magic);

    XDBG_TRACE(MDRI2, "AuthMagic: %d\n", ret);


    return ret;
}

static void
EYXNOSDri2ReuseBufferNotify (DrawablePtr pDraw, DRI2BufferPtr pBuf)
{

    ScreenPtr pScreen = pDraw->pScreen;
    DRI2BufferPrivPtr pBufPriv = pBuf->driverPrivate;
    Bool can_flip = _canFlip (pDraw);
    PixmapPtr pPix = NULL, pOldPix=NULL;
    DRI2BufferFlags *flags = (DRI2BufferFlags*)&pBuf->flags;
    int reuse_pix = 1;
    CARD64 pix_sbc;
    unsigned int pix_owner = 0;

    /* check reuse the buffer(pixmap) or not */
    if (pBuf->attachment == DRI2BufferFrontLeft)
    {
        pOldPix = pBufPriv->pPixmap;

        pPix = _getPixmapFromDrawable (pDraw);
        if (pPix == pOldPix || flags->data.is_viewable == IS_VIEWABLE(pDraw))
        {
            reuse_pix = 0;

            pBufPriv->pPixmap = pPix;
            pBufPriv->pPixmap->refcnt++;
            pBufPriv->can_flip = can_flip;

            (*pScreen->DestroyPixmap) (pOldPix);
        }
    }
    else
    {
        pOldPix = pBufPriv->pPixmap;

        pBufPriv->pPixmap = _reuseBackBufPixmap(pBuf, pDraw, can_flip);
        if (pBufPriv->pPixmap == NULL)
        {
            XDBG_WARNING (MDRI2, "Error pixmap is null\n");
            pBufPriv->pPixmap = pOldPix;
        }

        if (pBufPriv->pPixmap != pOldPix)
            reuse_pix = 0;
    }
    pBufPriv->can_flip = can_flip;

    /* set the reuse flags */
    
    //pBuf->name = _getName(pBufPriv->pPixmap);
    RENAME(pBuf->name,pBufPriv->pPixmap);

    flags->flags = _getBufferFlag(pDraw, pBufPriv->can_flip);
    if (reuse_pix)
    {
        if (pBuf->attachment == DRI2BufferFrontLeft)
        {
            flags->data.is_reused = 1;
            flags->data.reuse_idx = 0;
        }
        else
        {
            DRI2BufferPrivPtr pBufPriv = pBuf->driverPrivate;
            PixmapPtr pPix;
            EXYNOSExaPixPrivPtr pExaPixPriv = NULL;
            CARD64 sbc;
            unsigned int pending;

            flags->data.is_reused = 1;
            pPix = pBufPriv->pBackPixmaps[pBufPriv->cur_idx];
            pExaPixPriv = exaGetPixmapDriverPrivate (pPix);

            /* get swap buffer count and pending */
            DRI2GetSBC(pDraw, &sbc, &pending);

            /* get sbc info from pixmap */
            exynosPixmapGetFrameCount (pBufPriv->pPixmap, &pix_owner, &pix_sbc);

            /* calculate the reuse_idx  */
            if(pix_owner == pDraw->id)
            {
                unsigned int reuse_idx = (sbc + pending) - (pix_sbc + 1);
                if(reuse_idx > NUM_BACKBUF_PIXMAP + 1)
                {
                   flags->data.reuse_idx = 0;
                }
                else
                {
                   flags->data.reuse_idx = reuse_idx;
                }
            }
            else
            {
                flags->data.reuse_idx = 0;
            }
        }
    }

    XDBG_DEBUG(MDRI2, "id:0x%x(%d) attach:%d, name:%d, flags:0x%x, flip:%d, geo(%dx%d+%d+%d)\n",
               pDraw->id, pDraw->type,
               pBuf->attachment, pBuf->name, pBuf->flags, pBufPriv->can_flip,
               pDraw->width, pDraw->height, pDraw->x, pDraw->y);


}

// This is DOG-NAIL for remove implicit declaration. It must be moved to header (maybe crtc.h needs?)
DRI2FrameEventPtr exynosCrtcGetFirstPendingFlip (xf86CrtcPtr pCrtc);
void exynosCrtcRemovePendingFlip (xf86CrtcPtr pCrtc, DRI2FrameEventPtr pEvent);

void _exynosDri2ProcessPending (DRI2FrameEventPtr pPendingFlip, ScreenPtr pScreen,
                         unsigned int frame, unsigned int tv_sec, unsigned int tv_usec)
{
DRI2_BEGIN;
    DRI2BufferPrivPtr pBackBufPriv = NULL;
    DrawablePtr pPendingDraw = NULL;

    if (pPendingFlip)
    {
        ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
        EXYNOSPtr pExynos = EXYNOSPTR (pScrn);

        if (pPendingFlip->drawable_id)
            dixLookupDrawable (&pPendingDraw, pPendingFlip->drawable_id,
                           serverClient, M_ANY, DixWriteAccess);

        if (!pPendingDraw)
        {
            XDBG_WARNING (MDRI2, "pPendingDraw is null.\n");
            _deleteFrameEvent (pPendingDraw, pPendingFlip);
            DRI2_END;return;
        }
        else
        {
            if(pExynos->lcdstatus)
            {
                XDBG_WARNING (MDRI2, "LCD OFF : Request a pageflip pending even if the lcd is off.\n");

                _exchangeBuffers(pPendingDraw, DRI2_FLIP, pPendingFlip->pFrontBuf, pPendingFlip->pBackBuf);

                DRI2SwapComplete (pPendingFlip->pClient, pPendingDraw,
                                  frame, tv_sec, tv_usec,
                                  0, pPendingFlip->event_complete,
                                  pPendingFlip->event_data);

                pBackBufPriv = pPendingFlip->pBackBuf->driverPrivate;
                pBackBufPriv->pFlipEvent = NULL;
                _deleteFrameEvent (pPendingDraw, pPendingFlip);
            }
            else
            {
                if(!_scheduleFlip (pPendingDraw, pPendingFlip,TRUE))
                {
                    XDBG_WARNING (MDRI2, "fail to _scheduleFlip in secDri2FlipEventHandler\n");
                }
            }
        }

    }
DRI2_END;
}

void
exynosDri2FlipEventHandler (unsigned int frame, unsigned int tv_exynos,
                         unsigned int tv_uexynos, void *event_data)
{

    DRI2FrameEventPtr pEvent = event_data;
    DrawablePtr pDraw = NULL;
    ClientPtr pClient = pEvent->pClient;
    DRI2BufferPrivPtr pBackBufPriv = pEvent->pBackBuf->driverPrivate;
    ScreenPtr pScreen = pBackBufPriv->pScreen;


    if (pEvent->drawable_id)
        dixLookupDrawable (&pDraw, pEvent->drawable_id, serverClient, M_ANY, DixWriteAccess);

    if (!pDraw)
    {
        XDBG_WARNING (MDRI2,"pDraw is null... Client:%d pipe:%d "
                   "Front(attach:%d, name:%d, flag:0x%x), Back(attach:%d, name:%d, flag:0x%x)\n",
                   pClient->index, pBackBufPriv->pipe,
                   pEvent->pFrontBuf->attachment, pEvent->pFrontBuf->name, pEvent->pFrontBuf->flags,
                   pEvent->pBackBuf->attachment, pEvent->pBackBuf->name, pEvent->pBackBuf->flags);
    }
    else
    {
        XDBG_TRACE(MDRI2,"FlipEvent(%d,0x%x) Client:%d pipe:%d "
               "Front(attach:%d, name:%d, flag:0x%x), Back(attach:%d, name:%d, flag:0x%x)\n",
               pDraw->type, (unsigned int)pDraw->id, pClient->index, pEvent->pipe,
               pEvent->pFrontBuf->attachment, pEvent->pFrontBuf->name, pEvent->pFrontBuf->flags,
               pEvent->pBackBuf->attachment, pEvent->pBackBuf->name, pEvent->pBackBuf->flags);
    }

    assert (pBackBufPriv->pFlipEvent == pEvent);
    pBackBufPriv->pFlipEvent = NULL;
    DRI2FrameEventPtr pPendingEvent = pEvent->pPendingEvent;

    _deleteFrameEvent (pDraw, pEvent);

    /* get the next pending flip event */
    _exynosDri2ProcessPending (pPendingEvent, pScreen, frame, tv_exynos, tv_uexynos);

}

void
exynosDri2VblankEventHandler (unsigned int frame, unsigned int tv_exynos,
                          unsigned int tv_uexynos, void *event_data)
{

    DRI2FrameEventPtr pEvent = event_data;
    DrawablePtr pDraw = NULL;
    int status;

    status = dixLookupDrawable (&pDraw, pEvent->drawable_id, serverClient,
                                M_ANY, DixWriteAccess);
    if (status != Success)
    {
        XDBG_WARNING (MDRI2, "drawable is not found\n");
        _deleteFrameEvent (NULL, pEvent);

        return;
    }

    XDBG_RETURN_IF_FAIL(pEvent->pFrontBuf != NULL);
    XDBG_RETURN_IF_FAIL(pEvent->pBackBuf != NULL);

    switch (pEvent->type)
    {
    case DRI2_FLIP:
        if(!_scheduleFlip (pDraw, pEvent,FALSE))
        {
            XDBG_WARNING(MDRI2, "pageflip fails.\n");
            DRI2SwapComplete (pEvent->pClient, pDraw, 0, 0, 0, 0,
                              pEvent->event_complete, pEvent->event_data);
            _deleteFrameEvent (pDraw, pEvent);
        }

        return;
        break;
    case DRI2_SWAP:
        DRI2SwapComplete (pEvent->pClient, pDraw, frame, tv_exynos, tv_uexynos,
                          DRI2_EXCHANGE_COMPLETE, pEvent->event_complete, pEvent->event_data);
        break;
    case DRI2_BLIT:
    case DRI2_FB_BLIT:
        DRI2SwapComplete (pEvent->pClient, pDraw, frame, tv_exynos, tv_uexynos,
                          DRI2_BLIT_COMPLETE, pEvent->event_complete, pEvent->event_data);
        break;
    case DRI2_NONE:
        DRI2SwapComplete (pEvent->pClient, pDraw, frame, tv_exynos, tv_uexynos,
                          0, pEvent->event_complete, pEvent->event_data);
        break;
    case DRI2_WAITMSC:
        DRI2WaitMSCComplete (pEvent->pClient, pDraw, frame, tv_exynos, tv_uexynos);
        break;
    default:
        /* Unknown type */
        break;
    }

    XDBG_DEBUG (MDRI2,"FrameEvent(%d,0x%x) SwapType:%d Front(attach:%d, name:%d, flag:0x%x), Back(attach:%d, name:%d, flag:0x%x)\n",
                pDraw->type, (unsigned int)pDraw->id, pEvent->type,
                pEvent->pFrontBuf->attachment, pEvent->pFrontBuf->name, pEvent->pFrontBuf->flags,
                pEvent->pBackBuf->attachment, pEvent->pBackBuf->name, pEvent->pBackBuf->flags);
    _deleteFrameEvent (pDraw, pEvent);

}


Bool exynosDri2Init (ScreenPtr pScreen)
{


    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    DRI2InfoRec info;
    int ret;
    const char *driverNames[1];
    memset (&info, 0, sizeof(info));
    info.driverName = "exynos-drm";
    info.deviceName = pExynos->drm_device_name;
    info.version = 106;
    info.fd = pExynos->drm_fd;
    info.CreateBuffer = EYXNOSDri2CreateBuffer;
    info.DestroyBuffer = EYXNOSDri2DestroyBuffer;
    info.CopyRegion = EYXNOSDri2CopyRegion;
    info.ScheduleSwap = NULL;
    info.GetMSC = EYXNOSDri2GetMSC;
    info.ScheduleWaitMSC = EYXNOSDri2ScheduleWaitMSC;
    info.AuthMagic = EYXNOSDri2AuthMagic;
    info.ReuseBufferNotify = EYXNOSDri2ReuseBufferNotify;
    info.SwapLimitValidate = NULL;
    /* added in version 7 */
    info.GetParam = NULL;

    /* added in version 8 */
    /* AuthMagic callback which passes extra context */
    /* If this is NULL the AuthMagic callback is used */
    /* If this is non-NULL the AuthMagic callback is ignored */
    info.AuthMagic2 = NULL;

    /* added in version 9 */
    info.CreateBuffer2 = NULL;
    info.DestroyBuffer2 = NULL;
    info.CopyRegion2 = NULL;

    /* add in for Tizen extension */
    info.ScheduleSwapWithRegion = EYXNOSDri2ScheduleSwapWithRegion;

    info.Wait = NULL;
    info.numDrivers = 1;
    info.driverNames = driverNames;
    driverNames[0] = info.driverName;

    ret = DRI2ScreenInit (pScreen, &info);
    if (ret == FALSE)
    {

        return FALSE;
    }

    return ret;

}

void exynosDri2Deinit (ScreenPtr pScreen)
{

    DRI2CloseScreen (pScreen);

}
