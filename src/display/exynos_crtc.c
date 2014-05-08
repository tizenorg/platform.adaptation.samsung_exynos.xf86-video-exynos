/**************************************************************************

xserver-xorg-video-exynos

Copyright 2011-2012 Samsung Electronics co., Ltd. All Rights Reserved.

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

#include "exynos_driver.h"
#include "exynos_display.h"
#include "exynos_display_int.h"
#include "exynos_util.h"
#include "exynos_accel_int.h"

#include <xf86Crtc.h>
#include <xf86DDC.h>
#include <X11/Xatom.h>
#include <X11/extensions/dpmsconst.h>
#include <list.h>

#include "_trace.h"
#include "exynos_mem_pool.h"
/*
#define DRM_MODE_ENCODER_NONE	0
#define DRM_MODE_ENCODER_DAC	1
#define DRM_MODE_ENCODER_TMDS	2
#define DRM_MODE_ENCODER_LVDS	3
#define DRM_MODE_ENCODER_TVDAC	4
#define DRM_MODE_ENCODER_VIRTUAL	5
*/
/* Remove temp variable. Used in _trace.h */
int _free=0;

static void
EXYNOSCrtcDpms(xf86CrtcPtr pCrtc, int pMode)
{
}

#define XF86_CRTC_CONFIG_PTR(p)	((xf86CrtcConfigPtr) ((p)->privates[xf86CrtcConfigPrivateIndex].ptr))

xf86CrtcPtr
_exynosCrtcGetFromPipe (ScrnInfoPtr pScrn, int pipe)
{

    xf86CrtcConfigPtr pXf86CrtcConfig;
    pXf86CrtcConfig = XF86_CRTC_CONFIG_PTR (pScrn);
    xf86CrtcPtr pCrtc = NULL;
    EXYNOSCrtcPrivPtr pCrtcPriv = NULL;
    int i;

    for (i = 0; i < pXf86CrtcConfig->num_output; i++)
    {
        pCrtc = pXf86CrtcConfig->crtc[i];
        pCrtcPriv = pCrtc->driver_private;
        if (pCrtcPriv->pipe == pipe)
        {

            return pCrtc;
        }
    }


    return NULL;
}

xf86CrtcPtr exynosFindCrtc(ScrnInfoPtr pScrn, int crtc_id) {
    EXYNOSDispInfoPtr pDispInfo = (EXYNOSDispInfoPtr) EXYNOSDISPINFOPTR(pScrn);
    EXYNOSCrtcPrivPtr crtc_ref = NULL, crtc_next = NULL;
    xf86CrtcPtr pCrtc = NULL;
    xorg_list_for_each_entry_safe (crtc_ref, crtc_next, &pDispInfo->crtcs, link)
    {
        pCrtc = crtc_ref->pCrtc;
        EXYNOSCrtcPrivPtr pCrtcPriv = pCrtc->driver_private;
        if(pCrtcPriv->crtc_id==crtc_id)break;
    }
    return pCrtc;
}

static void
_flipPixmapInit (xf86CrtcPtr pCrtc)
{
    //ScrnInfoPtr pScrn = pCrtc->scrn;
    //EXYNOSPtr pSec = EXYNOSPTR (pScrn);
    EXYNOSCrtcPrivPtr pCrtcPriv = pCrtc->driver_private;
    int flip_backbufs = 3 - 1;//TODO: Get it from pSec->flip_bufs
    int i;

    pCrtcPriv->flip_backpixs.lub = -1;
    pCrtcPriv->flip_backpixs.num = flip_backbufs;

    pCrtcPriv->flip_backpixs.pix_free = ctrl_calloc (flip_backbufs, sizeof(void*));
    for (i = 0; i < flip_backbufs; i++)
        {
        MAKE_FREE(pCrtcPriv->flip_backpixs.pix_free[i],TRUE);
        }
    pCrtcPriv->flip_backpixs.flip_pixmaps = ctrl_calloc (flip_backbufs, sizeof(void*));
    pCrtcPriv->flip_backpixs.flip_draws = ctrl_calloc (flip_backbufs, sizeof(void*));
}

static void
_flipPixmapDeinit (xf86CrtcPtr pCrtc)
{
    EXYNOSCrtcPrivPtr pCrtcPriv = pCrtc->driver_private;
    ScreenPtr pScreen = pCrtc->scrn->pScreen;
    int i;

    for (i = 0; i < pCrtcPriv->flip_backpixs.num; i++)
    {
        //XDBG_ERROR(MDRI2,"\E[0;42;1mMake it TRUE\n\E[0m");
        MAKE_FREE(pCrtcPriv->flip_backpixs.pix_free[i],TRUE);
        if (pCrtcPriv->flip_backpixs.flip_pixmaps[i])
        {
#ifdef USE_XDBG_EXTERNAL

            if (pCrtcPriv->flip_backpixs.flip_draws[i])
                xDbgLogPListDrawRemoveRefPixmap (pCrtcPriv->flip_backpixs.flip_draws[i],
                                                   pCrtcPriv->flip_backpixs.flip_pixmaps[i]);
#endif

            (*pScreen->DestroyPixmap) (pCrtcPriv->flip_backpixs.flip_pixmaps[i]);
            pCrtcPriv->flip_backpixs.flip_pixmaps[i] = NULL;
            pCrtcPriv->flip_backpixs.flip_draws[i] = NULL;
        }
    }
    pCrtcPriv->flip_backpixs.lub = -1;
}

/* return true if there is no flip pixmap available */
Bool
exynosCrtcAllInUseFlipPixmap (ScrnInfoPtr pScrn, int crtc_pipe)
{
    xf86CrtcPtr pCrtc = NULL;
    EXYNOSCrtcPrivPtr pCrtcPriv = NULL;
    int i;

    pCrtc = _exynosCrtcGetFromPipe (pScrn, crtc_pipe);
    XDBG_RETURN_VAL_IF_FAIL(pCrtc != NULL, FALSE);

    pCrtcPriv = pCrtc->driver_private;
    
    //XDBG_ERROR(MDRI2,"\E[0;43;1mSTATE:\n\E[0m");
    //for (i = 0; i < pCrtcPriv->flip_backpixs.num; i++)
        //XDBG_ERROR(MDRI2,"\E[0;43;1m%d-%d:\n\E[0m",i,pCrtcPriv->flip_backpixs.pix_free[i]);

    /* there is a free flip pixmap, return false */
    for (i = 0; i < pCrtcPriv->flip_backpixs.num; i++)
    {
        if (pCrtcPriv->flip_backpixs.pix_free[i])
        {
            return FALSE;
        }
    }

    XDBG_WARNING (MCRTC, "no free flip pixmap\n");

    return TRUE;
}

void
exynosCrtcExemptAllFlipPixmap (ScrnInfoPtr pScrn, int crtc_pipe)
{
    xf86CrtcPtr pCrtc = NULL;
    EXYNOSCrtcPrivPtr pCrtcPriv = NULL;
    int i;

    pCrtc = _exynosCrtcGetFromPipe (pScrn, crtc_pipe);
    XDBG_RETURN_IF_FAIL(pCrtc != NULL);

    pCrtcPriv = pCrtc->driver_private;

    /* release flip pixmap */
    for (i = 0; i < pCrtcPriv->flip_backpixs.num; i++)
    {
        MAKE_FREE(pCrtcPriv->flip_backpixs.pix_free[i],TRUE);
        /*pCrtcPriv->flip_backpixs.flip_draws[i] = NULL;*/

        XDBG_DEBUG (MCRTC,
                    "the index(%d) of the flip draw in pipe(%d) is unset\n",
                    i, crtc_pipe);
    }
}

#define GET_NEXT_IDX(idx, max) (((idx+1) % (max)))
PixmapPtr
exynosCrtcGetFlipPixmap (ScrnInfoPtr pScrn, int crtc_pipe, DrawablePtr pDraw, unsigned int usage_hint)
{
    xf86CrtcPtr pCrtc = NULL;
    EXYNOSCrtcPrivPtr pCrtcPriv = NULL;
    PixmapPtr pPixmap = NULL;
    ScreenPtr pScreen = pScrn->pScreen;
    int i;
    int check_release = 0;

    pCrtc = _exynosCrtcGetFromPipe (pScrn, crtc_pipe);
    XDBG_RETURN_VAL_IF_FAIL(pCrtc != NULL, FALSE);

    pCrtcPriv = pCrtc->driver_private;

    /* check if there is free flip pixmaps */
    if (exynosCrtcAllInUseFlipPixmap(pScrn, crtc_pipe))
    {
        /* case : flip pixmap is never release
           if flip_count is 0 where there is no uncompleted pageflipping,
           release the flip_pixmap which occupied by a drawable. */
        if (pCrtcPriv->flip_count == 0)
        {
            exynosCrtcExemptAllFlipPixmap (pScrn, crtc_pipe);
            check_release = 1;
            XDBG_WARNING (MCRTC, "@@ release the drawable pre-occuiped the flip_pixmap\n");
        }

        /* return null, if there is no flip_backpixmap which can release */
        if (!check_release)
            return NULL;
    }

    /* return flip pixmap */
    for (i = GET_NEXT_IDX(pCrtcPriv->flip_backpixs.lub, pCrtcPriv->flip_backpixs.num)
            ; i < pCrtcPriv->flip_backpixs.num
            ; i = GET_NEXT_IDX(i, pCrtcPriv->flip_backpixs.num))
    {
        if (pCrtcPriv->flip_backpixs.pix_free[i])
        {
            if (pCrtcPriv->flip_backpixs.flip_pixmaps[i])
            {
                pPixmap = pCrtcPriv->flip_backpixs.flip_pixmaps[i];
                //XDBG_DEBUG (MFLIP, "the index(%d, %d) of the flip pixmap in pipe(%d) is set\n",
                //            i, tbm_bo_export(secExaPixmapGetBo(pPixmap)), crtc_pipe);
            }
            else
            {
                pPixmap = (*pScreen->CreatePixmap) (pScreen,
                                            pDraw->width,
                                            pDraw->height,
                                            pDraw->depth,
                                            usage_hint);
                XDBG_RETURN_VAL_IF_FAIL(pPixmap != NULL, NULL);


                pCrtcPriv->flip_backpixs.flip_pixmaps[i] = pPixmap;

                /*XDBG_DEBUG (MFLIP,
                            "the index(%d, %d) of the flip pixmap in pipe(%d) is created\n",
                            i, tbm_bo_export(secExaPixmapGetBo(pPixmap)), crtc_pipe);*/
            }

#ifdef USE_XDBG_EXTERNAL
            if (pCrtcPriv->flip_backpixs.flip_draws[i] &&
                (pCrtcPriv->flip_backpixs.flip_draws[i] != pDraw))
            {
                xDbgLogPListDrawRemoveRefPixmap (pCrtcPriv->flip_backpixs.flip_draws[i],
                                                   pCrtcPriv->flip_backpixs.flip_pixmaps[i]);
            }
#endif

            MAKE_FREE(pCrtcPriv->flip_backpixs.pix_free[i],FALSE);
            pCrtcPriv->flip_backpixs.flip_draws[i] = pDraw;
            pCrtcPriv->flip_backpixs.lub = i;

#ifdef USE_XDBG_EXTERNAL
            xDbgLogPListDrawAddRefPixmap (pDraw, pPixmap);
#endif
            break;
        }
    }

    return pPixmap;
}

void
exynosCrtcExemptFlipPixmap (ScrnInfoPtr pScrn, int crtc_pipe, PixmapPtr pPixmap)
{

    xf86CrtcPtr pCrtc = NULL;
    EXYNOSCrtcPrivPtr  pCrtcPriv = NULL;
    int i;

    pCrtc = _exynosCrtcGetFromPipe (pScrn, crtc_pipe);
    XDBG_RETURN_IF_FAIL(pCrtc != NULL);


    pCrtcPriv = pCrtc->driver_private;

    /* release flip pixmap */
    for (i = 0; i < pCrtcPriv->flip_backpixs.num; i++)
    {
        if (pPixmap == pCrtcPriv->flip_backpixs.flip_pixmaps[i])
        {
            MAKE_FREE(pCrtcPriv->flip_backpixs.pix_free[i],TRUE);
            /*pCrtcPriv->flip_backpixs.flip_draws[i] = NULL;*/

          /*  XDBG_DEBUG (MCRTC, "the index(%d, %d) of the flip pixmap in pipe(%d) is unset\n",
                        i, tbm_bo_export(secExaPixmapGetBo(pPixmap)), crtc_pipe);*/
            break;
        }
    }

}

extern void _fbFreeBoData(void* data);


static Bool
EXYNOSCrtcSetModeMajor(xf86CrtcPtr pCrtc, DisplayModePtr pMode,
                    Rotation rotation, int x, int y)
{

    ScrnInfoPtr pScrn = pCrtc->scrn;
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    EXYNOSCrtcPrivPtr pCrtcPriv = pCrtc->driver_private;
    EXYNOSDispInfoPtr pDispInfo = pCrtcPriv->pDispInfo;
    int saved_x = 0, saved_y = 0;
    Rotation saved_rotation = 0;
    DisplayModeRec saved_mode;
    Bool ret = FALSE;
    int pitch = 4;
    tbm_bo_handle bo_handle;
    xf86DrvMsg(pScrn->scrnIndex,X_INFO,
               "SetModeMajor id:%d cur(%dx%d+%d+%d),rot:%d new(%dx%d+%d+%d),rot:%d\n",
               pCrtcPriv->crtc_id,
               pCrtc->x, pCrtc->y, pCrtc->mode.HDisplay, pCrtc->mode.VDisplay,
               pCrtc->rotation,
               x,y,pMode->HDisplay,pMode->VDisplay,
               rotation);

    if (pCrtcPriv->fb_id == 0)
    {
        EXYNOS_FbBoDataPtr bo_data = NULL;
        XDBG_GOTO_IF_FAIL(tbm_bo_get_user_data(pExynos->default_bo, TBM_BO_DATA_FB, (void**)&bo_data) == TRUE, fail);
        pCrtcPriv->fb_id = bo_data->fb_id;
        XDBG_DEBUG (MDISP, "reuse fb: %d\n", pCrtcPriv->fb_id);
    }

    memcpy (&saved_mode, &pCrtc->mode, sizeof(DisplayModeRec));
    saved_x = pCrtc->x;
    saved_y = pCrtc->y;
    saved_rotation = pCrtc->rotation;

    memcpy (&pCrtc->mode, pMode, sizeof(DisplayModeRec));
    pCrtc->x = x;
    pCrtc->y = y;
    pCrtc->rotation = rotation;

    exynosDisplayModeToKmode(pCrtc->scrn, &pCrtcPriv->kmode, pMode);

    ret = exynosCrtcApply(pCrtc);
    XDBG_GOTO_IF_FAIL (ret == TRUE, fail);


    return ret;
fail:
    xf86DrvMsg(pScrn->scrnIndex,X_INFO, "Fail crtc apply(crtc_id:%d, rotate:%d, %dx%d+%d+%d\n",
               pCrtcPriv->crtc_id, rotation, x, y, pCrtc->mode.HDisplay, pCrtc->mode.VDisplay);

    exynosDisplayModeToKmode(pCrtc->scrn, &pCrtcPriv->kmode, &saved_mode);

    memcpy (&pCrtc->mode, &saved_mode, sizeof(DisplayModeRec));
    pCrtc->x = saved_x;
    pCrtc->y = saved_y;
    pCrtc->rotation = saved_rotation;


    return ret;

}

static void
EXYNOSCrtcSetCursorColors(xf86CrtcPtr pCrtc, int bg, int fg)
{

    /* TODO: not implemented yet */
XDBG_DEBUG(MCRTC, "not impelented yet.\n");

}

static void
EXYNOSCrtcSetCursorPosition (xf86CrtcPtr pCrtc, int x, int y)
{

    /* TODO: not implemented yet */
XDBG_DEBUG(MCRTC, "not impelented yet.\n");

}

static void
EXYNOSCrtcShowCursor (xf86CrtcPtr pCrtc)
{

    /* TODO: not implemented yet */
XDBG_DEBUG(MCRTC, "not impelented yet.\n");

}

static void
EXYNOSCrtcHideCursor (xf86CrtcPtr pCrtc)
{

    /* TODO: not implemented yet */
XDBG_DEBUG(MCRTC, "not impelented yet.\n");

}

static void
EXYNOSCrtcLoadCursorArgb(xf86CrtcPtr pCrtc, CARD32 *image)
{

    /* TODO: not implemented yet */
XDBG_DEBUG(MCRTC, "not impelented yet.\n");

}


static void *
EXYNOSCrtcShadowAllocate(xf86CrtcPtr pCrtc, int width, int height)
{
    XDBG_RETURN_VAL_IF_FAIL(pCrtc != NULL,0);
    EXYNOSCrtcPrivPtr pCrtcPriv = pCrtc->driver_private;
    ScrnInfoPtr pScrn = pCrtc->scrn;
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    int flag;
    if (!pExynos->cachable)
        flag = TBM_BO_WC;
    else
        flag = TBM_BO_DEFAULT;

    int pitch = width*4;
    int size = height*pitch;
    tbm_bo_handle bo_handle1,bo_handle2;
    pCrtcPriv->rotate_bo = tbm_bo_alloc (pExynos->bufmgr, size, flag);

    bo_handle1 = tbm_bo_map (pCrtcPriv->rotate_bo, TBM_DEVICE_CPU, TBM_OPTION_WRITE);
    XDBG_RETURN_VAL_IF_FAIL (bo_handle1.ptr != NULL, NULL);
    memset (bo_handle1.ptr, 0x0, pitch*height);
    tbm_bo_unmap (pCrtcPriv->rotate_bo);

    bo_handle2 = tbm_bo_get_handle(pCrtcPriv->rotate_bo, TBM_DEVICE_DEFAULT);
    int ret = drmModeAddFB (pExynos->drm_fd, width, height, pCrtc->scrn->depth,
                            pCrtc->scrn->bitsPerPixel, pitch,
                            bo_handle2.u32,
                            &pCrtcPriv->rotate_fb_id);

    if (ret < 0)
        {
            ErrorF ("failed to add rotate fb\n");
            tbm_bo_unref (pCrtcPriv->rotate_bo);
            return NULL;
        }
    pCrtcPriv->rotate_pitch = pitch;
    return pCrtcPriv->rotate_bo;
}

static PixmapPtr EXYNOSCrtcShadowCreate(xf86CrtcPtr pCrtc, void *data,
        int width, int height) {
    XDBG_RETURN_VAL_IF_FAIL(pCrtc != NULL, 0);
    EXYNOSCrtcPrivPtr pCrtcPriv = pCrtc->driver_private;
    ScrnInfoPtr pScrn = pCrtc->scrn;
    PixmapPtr rotate_pixmap;

    XDBG_DEBUG (MCRTC, "<<\n");

    if (!data) {
            data = EXYNOSCrtcShadowAllocate(pCrtc, width, height);
            if (!data) {
                xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                        "Couldn't allocate shadow pixmap for rotated CRTC\n");
                return NULL;
            }
            pCrtcPriv->rotate_bo=data;
    }
    rotate_pixmap = GetScratchPixmapHeader(pScrn->pScreen, width, height,
            pScrn->depth, pScrn->bitsPerPixel, pCrtcPriv->rotate_pitch,
            data);

    if (rotate_pixmap == NULL) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                "Couldn't allocate shadow pixmap for rotated CRTC\n");
        return NULL;
    }

    XDBG_DEBUG (MCRTC, ">>w(%i)h(%i)rotate pixmap-%x\n",width,height,rotate_pixmap);
    return rotate_pixmap;
}

static void EXYNOSCrtcShadowDestroy(xf86CrtcPtr pCrtc, PixmapPtr rotate_pixmap,
        void *data) {
    EXYNOSCrtcPrivPtr pCrtcPriv = pCrtc->driver_private;
    ScrnInfoPtr pScrn = pCrtc->scrn;
    int ret;
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    XDBG_DEBUG(MCRTC,"\n");
    if (rotate_pixmap) {
        FreeScratchPixmapHeader(rotate_pixmap);
        XDBG_DEBUG(MCRTC,"FreeScratchPixmapHeader\n");
    }

    if (data) {
        ret=drmModeRmFB(pExynos->drm_fd, pCrtcPriv->rotate_fb_id);
        XDBG_DEBUG(MCRTC,"ret-%i\n",ret);
        pCrtcPriv->rotate_fb_id = 0;

        tbm_bo_unref(pCrtcPriv->rotate_bo);
        pCrtcPriv->rotate_bo = NULL;
    }
//shadow_present=FALSE; //TODO
}


void debug_out() {
    ScrnInfoPtr pScrn = xf86Screens[0];
    EXYNOSDispInfoPtr pDispInfo = (EXYNOSDispInfoPtr) EXYNOSDISPINFOPTR(pScrn);
    EXYNOSCrtcPrivPtr crtc_ref = NULL, crtc_next = NULL;
    xf86CrtcPtr pCrtc = NULL;
    XDBG_INFO(MDRV, "CRTC:\n");
    xorg_list_for_each_entry_safe (crtc_ref, crtc_next, &pDispInfo->crtcs, link)
    {
        pCrtc = crtc_ref->pCrtc;
#if 0
        EXYNOSCrtcPrivPtr pCrtcPriv = pCrtc->driver_private;
        XDBG_INFO(MDRV, "crtc_id (%i) %p in_use (%i)\n", pCrtcPriv->crtc_id,pCrtc,
                xf86CrtcInUse(pCrtc));
#endif
    }
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(pScrn);
    int o;
    XDBG_INFO(MDRV, "outputs:\n");
    for (o = 0; o < xf86_config->num_output; o++)
        XDBG_INFO(MDRV, "out %i %s %p\n", o, xf86_config->output[o]->name,
                xf86_config->output[o]->crtc);

}

void set_output_crtc(int out,int crtc){
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR( xf86Screens[0]);
    xf86_config->output[out]->crtc = crtc;
}


static void
EXYNOSCrtcGammaSet(xf86CrtcPtr pCrtc,
                CARD16 *red, CARD16 *green, CARD16 *blue, int size)
{
    xf86DrvMsg(pCrtc->scrn->scrnIndex, X_ERROR,"Thanx for using our driver, but drmModeCrtcSetGamma is not realized in kernel now.\n");
}

static void
EXYNOSCrtcDestroy(xf86CrtcPtr pCrtc)
{

    EXYNOSCrtcPrivPtr pCrtcPriv = pCrtc->driver_private;

    _flipPixmapDeinit (pCrtc);
#ifdef USE_XDBG_EXTERNAL
    if (pCrtcPriv->xdbg_fpsdebug)
    {
        xDbgLogFpsDebugDestroy (pCrtcPriv->xdbg_fpsdebug);
        pCrtcPriv->xdbg_fpsdebug = NULL;
    }
#endif

    if (pCrtcPriv->mode_crtc)
        drmModeFreeCrtc (pCrtcPriv->mode_crtc);

    xorg_list_del (&pCrtcPriv->link);
    ctrl_free (pCrtcPriv);

    pCrtc->driver_private = NULL;

}

/*
(*mode_set) (xf86CrtcPtr crtc,
             DisplayModePtr mode,
             DisplayModePtr adjusted_mode, int x, int y);*/
static void
EXYNOSCrtcModeSet(xf86CrtcPtr pCrtc, DisplayModePtr pMode,
                    DisplayModePtr adjusted_mode, int x, int y)
{
    EXYNOSCrtcSetModeMajor (pCrtc, adjusted_mode, 0,  x, y);

}


/*   *
     * Callback for panning. Doesn't change the mode.
     * Added in ABI version 2
      void
     (*set_origin) (xf86CrtcPtr crtc, int x, int y);*/
static void
EXYNOSCrtcSetOrigin(xf86CrtcPtr pCrtc, int x, int y)
{

    EXYNOSCrtcPrivPtr pCrtcPriv = pCrtc->driver_private;
    ScrnInfoPtr pScrn = pCrtc->scrn;
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    EXYNOSDispInfoPtr pDispInfo = EXYNOSDISPINFOPTR(pScrn);
    int width = pDispInfo->main_lcd_mode.hdisplay;
    int height = pDispInfo->main_lcd_mode.vdisplay;

    ScreenPtr pScreen = pScrn->pScreen;
    PixmapPtr pPixmap = pScreen->GetScreenPixmap(pScreen);

    pPixmap->devKind = width*4;//pitch
    exynosCrtcApply(pCrtc);
    XDBG_INFO (MCRTC, "CrtcSetOrigin!  x=%i y=%i \n",x,y);

}

/**
    Bool
    (*set_scanout_pixmap)(xf86CrtcPtr crtc, PixmapPtr pixmap);*/

static Bool
EXYNOSCrtcSetScanoutPixmap(xf86CrtcPtr pCrtc, int x, int y)
{

return FALSE;

}

Bool
exynosCrtcApply(xf86CrtcPtr pCrtc)
{

    ScrnInfoPtr pScrn = pCrtc->scrn;
    EXYNOSCrtcPrivPtr pCrtcPriv = pCrtc->driver_private;
    EXYNOSDispInfoPtr pDispInfo = pCrtcPriv->pDispInfo;
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    xf86CrtcConfigPtr   xf86_config = XF86_CRTC_CONFIG_PTR (pCrtc->scrn);
    uint32_t *output_ids;
    int output_count = 0;
    int fb_id = 0, x = pCrtc->x, y = pCrtc->y;
    int i;
    Bool ret = FALSE;
    EXYNOS_FbBoDataPtr bo_data = NULL;

    xf86OutputPtr output = NULL;
    for (i = 0; i < xf86_config->num_output; output = NULL, i++) {
        output = xf86_config->output[i];

        if (output->crtc == pCrtc)
            break;
        }

        if (!output) {
        xf86DrvMsg(pCrtc->scrn->scrnIndex,X_ERROR, "No output for this crtc.\n");
        return FALSE;
        }

    output_ids = ctrl_calloc (xf86_config->num_output,sizeof (uint32_t));
    XDBG_RETURN_VAL_IF_FAIL (output_ids != 0, FALSE);

    for (i = 0; i < xf86_config->num_output; i++)
    {
        xf86OutputPtr pOutput = xf86_config->output[i];
        EXYNOSOutputPrivPtr pOutputPriv;
        if (pOutput->crtc != pCrtc)
            continue;

        pOutputPriv = pOutput->driver_private;

        /* modify the physical size of monitor */
        if (!strcmp(pOutput->name, "LVDS1"))
        {
            pOutput->mm_width = pOutputPriv->mode_output->mmWidth;
            pOutput->mm_height = pOutputPriv->mode_output->mmHeight;
            pOutput->conf_monitor->mon_width = pOutputPriv->mode_output->mmWidth;
            pOutput->conf_monitor->mon_height = pOutputPriv->mode_output->mmHeight;
            XDBG_INFO (MCRTC, "%i %i %i %i\n",pOutput->mm_width,pOutput->mm_height,pOutput->conf_monitor->mon_width,pOutput->conf_monitor->mon_height);

        }

        output_ids[output_count] = pOutputPriv->mode_output->connector_id;
        output_count++;
    }

    /*  * Check if we need to scanout from something else than the root
        * pixmap. In that case, xf86CrtcRotate will take care of allocating
        * new opaque scanout buffer data "crtc->rotatedData".
        * However, it will not wrap
        * that data into pixmaps until the first rotated damage composite.
        **/
    if (!xf86CrtcRotate (pCrtc)){
        XDBG_INFO (MCRTC, "!xf86CrtcRotate \n");
        goto done;
        }

    pCrtc->funcs->gamma_set (pCrtc, pCrtc->gamma_red, pCrtc->gamma_green,
                             pCrtc->gamma_blue, pCrtc->gamma_size);


    XDBG_INFO (MCRTC, "gamma_set\n");

    fb_id = pCrtcPriv->fb_id;

    if (pCrtcPriv->rotate_fb_id)
    {
        fb_id = pCrtcPriv->rotate_fb_id;
        x = 0;
        y = 0;
    }

    XDBG_INFO (MCRTC, "fb_id,%d name,%s width,%d height,%d\n",
               fb_id, pCrtcPriv->kmode.name, pCrtcPriv->kmode.hdisplay,
               pCrtcPriv->kmode.vdisplay);

    XDBG_INFO (MCRTC, "pDispInfo->drm_fd-%i, pCrtcPriv->crtc_id-%i,fb_id-%i, x-%i, y-%i, output_ids-%i, output_count-%i,pCrtcPriv->kmode-%i\n",
                   pDispInfo->drm_fd, pCrtcPriv->crtc_id,
                   fb_id, x, y, output_ids, output_count,
                   pCrtcPriv->kmode);


    ret = drmModeSetCrtc(pDispInfo->drm_fd, pCrtcPriv->crtc_id,
                         fb_id, x, y, output_ids, output_count,
                         &pCrtcPriv->kmode);
    if (ret)
    {
        XDBG_ERRNO (MCRTC, "failed to set crtc. error(%s)\n",strerror(-ret));
        ret = FALSE;
    }
    else
    {
        XDBG_INFO (MCRTC, "set crtc ok.\n");
        ret = TRUE;

        /* Force DPMS to On for all outputs, which the kernel will have done
               * with the mode set. Also, restore the backlight level
               */
        for (i = 0; i < xf86_config->num_output; i++)
        {
            xf86OutputPtr pOutput = xf86_config->output[i];
            EXYNOSOutputPrivPtr pOutputPriv;

            if (pOutput->crtc != pCrtc)
                continue;

            pOutputPriv = pOutput->driver_private;

            /* update mode_encoder */
            XDBG_ERRNO (MCRTC, "freeEnc %x\n",pOutputPriv->mode_encoder);
            drmModeFreeEncoder (pOutputPriv->mode_encoder);
            pOutputPriv->mode_encoder =
                drmModeGetEncoder (pDispInfo->drm_fd, pOutputPriv->mode_output->encoders[0]);

            /* set display connector and display set mode */
            if (pOutputPriv->mode_output->connector_type == DRM_MODE_CONNECTOR_HDMIA ||
                pOutputPriv->mode_output->connector_type == DRM_MODE_CONNECTOR_HDMIB)
            {
                displaySetDispConnMode (pScrn, DISPLAY_CONN_MODE_HDMI);
                /* TODO : find the display mode */
                displaySetDispSetMode (pOutput, DISPLAY_SET_MODE_EXT);

            }
            else if (pOutputPriv->mode_output->connector_type == DRM_MODE_CONNECTOR_VIRTUAL)
            {
                displaySetDispConnMode (pScrn, DISPLAY_CONN_MODE_VIRTUAL);
                /* TODO : find the display mode */
               //  DisplaySetDispSetMode (pScrn, DISPLAY_SET_MODE_EXT);

            }
            else if (pOutputPriv->mode_output->connector_type == DRM_MODE_CONNECTOR_LVDS)
            {
                /* should be shown again when crtc on. */
                 int crtc_id = 0;

            }
            else
                XDBG_NEVER_GET_HERE (MDISP);



        }
    }

    outputDrmUpdate (pScrn);
    if (pScrn->pScreen)
        xf86_reload_cursors (pScrn->pScreen);

done:
    ctrl_free (output_ids);

    return ret;
}



static const xf86CrtcFuncsRec exynos_crtc_funcs =
{
    .dpms = EXYNOSCrtcDpms,
    .set_mode_major = EXYNOSCrtcSetModeMajor,
    .set_cursor_colors = EXYNOSCrtcSetCursorColors,
    .set_cursor_position = EXYNOSCrtcSetCursorPosition,
    .show_cursor = EXYNOSCrtcShowCursor,
    .hide_cursor = EXYNOSCrtcHideCursor,
    .load_cursor_argb = EXYNOSCrtcLoadCursorArgb,
    .shadow_create = EXYNOSCrtcShadowCreate,
    .shadow_allocate = EXYNOSCrtcShadowAllocate,
    .shadow_destroy = EXYNOSCrtcShadowDestroy,
    .gamma_set = EXYNOSCrtcGammaSet,
    .destroy = EXYNOSCrtcDestroy,
    /*********** For Panning  ************/
    .mode_set = EXYNOSCrtcModeSet,
    .set_origin = EXYNOSCrtcSetOrigin,
    //.set_scanout_pixmap = EXYNOSCrtcSetScanoutPixmap,

    /*****************************************/
};

void
exynosCrtcInit (ScrnInfoPtr pScrn, EXYNOSDispInfoPtr pDispInfo, int num)
{


    xf86CrtcPtr pCrtc;
    EXYNOSCrtcPrivPtr pCrtcPriv;
    drmModeCrtcPtr kcrtc;

    pCrtcPriv = ctrl_calloc (1, sizeof (EXYNOSCrtcPrivRec));
    if (pCrtcPriv == NULL)
    {

        return;
    }

    xorg_list_init (&pCrtcPriv->pending_flips);

    pCrtc = xf86CrtcCreate (pScrn, &exynos_crtc_funcs);
    if (pCrtc == NULL)
    {
        ctrl_free (pCrtcPriv);
        {

            return;
        }
    }
    pCrtc->driver_private = pCrtcPriv;

    kcrtc = drmModeGetCrtc (pDispInfo->drm_fd, pDispInfo->mode_res->crtcs[num]);
    if (!kcrtc)
    {
        ctrl_free (pCrtcPriv);
        xf86CrtcDestroy (pCrtc);

        return;
    }

    pCrtcPriv->pCrtc = pCrtc;
    pCrtcPriv->pDispInfo = pDispInfo;
    pCrtcPriv->crtc_id = pDispInfo->mode_res->crtcs[num];
    pCrtcPriv->mode_crtc = kcrtc;
    pCrtcPriv->rotate_bo=0;

    /* TODO: numbering primary display or secondary display */
    pCrtcPriv->pipe = num;
#ifdef USE_XDBG_EXTERNAL
    pCrtcPriv->xdbg_fpsdebug = xDbgLogFpsDebugCreate ();

    if (pCrtcPriv->xdbg_fpsdebug == NULL)
    {
        ctrl_free (pCrtcPriv);
        xf86CrtcDestroy (pCrtc);

        return;
    }
#endif
    _flipPixmapInit (pCrtc);

    xorg_list_add(&pCrtcPriv->link, &pDispInfo->crtcs);

}

// --- for pending --- //

DRI2FrameEventPtr 
exynosCrtcGetFirstPendingFlip (xf86CrtcPtr pCrtc)
{
    EXYNOSCrtcPrivPtr pCrtcPriv;
    DRI2FrameEventPtr item=NULL, tmp=NULL;

    XDBG_RETURN_VAL_IF_FAIL (pCrtc != NULL, NULL);
    pCrtcPriv = pCrtc->driver_private;

    if (xorg_list_is_empty (&pCrtcPriv->pending_flips))
        return NULL;

    /* get the last item in the circular list ( last item is at last_item.next==head)*/
    xorg_list_for_each_entry_safe(item, tmp, &pCrtcPriv->pending_flips, crtc_pending_link)
    {
       if (item->crtc_pending_link.next == &pCrtcPriv->pending_flips)
       {
           return item;
       }
    }

    /* never fall to this line, "return" only for remove warnings */
    return NULL;
}

void
exynosCrtcRemovePendingFlip (xf86CrtcPtr pCrtc, DRI2FrameEventPtr pEvent)
{
    EXYNOSCrtcPrivPtr pCrtcPriv;
    DRI2FrameEventPtr item=NULL, tmp=NULL;

    XDBG_RETURN_IF_FAIL (pCrtc != NULL);

    pCrtcPriv = pCrtc->driver_private;

    if (xorg_list_is_empty (&pCrtcPriv->pending_flips))
        return;

    xorg_list_for_each_entry_safe(item, tmp, &pCrtcPriv->pending_flips, crtc_pending_link)
    {
        if (item == pEvent)
        {
            xorg_list_del(&item->crtc_pending_link);
        }
    }
}

void
exynosCrtcAddPendingFlip (xf86CrtcPtr pCrtc, DRI2FrameEventPtr pEvent)
{
    EXYNOSCrtcPrivPtr pCrtcPriv;

    XDBG_RETURN_IF_FAIL (pCrtc != NULL);

    pCrtcPriv = pCrtc->driver_private;

    xorg_list_add(&(pEvent->crtc_pending_link), &(pCrtcPriv->pending_flips));
}


void
exynosCrtcRemoveFlipPixmap (xf86CrtcPtr pCrtc)
{
    _flipPixmapDeinit (pCrtc);
}
