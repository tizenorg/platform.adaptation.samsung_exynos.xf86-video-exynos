/*
 * Copyright © 2007 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Dave Airlie <airlied@redhat.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <dirent.h>
#include <exynos_drm.h>

#include "exynos_driver.h"
#include "exynos_display.h"
#include "exynos_display_int.h"
#include "exynos_accel.h"
#include "exynos_util.h"
#include "exynos_video_image.h"
#include "exynos_video_overlay.h"

#include "exynos_accel_int.h"
#include "_trace.h"

static Bool EXYNOSCrtcConfigResize(ScrnInfoPtr pScrn, int width, int height);
static void EXYNOSDispInfoVblankHandler(int fd, unsigned int frame, unsigned int tv_sec,
                                 unsigned int tv_usec, void *event_data);
static void EXYNOSDispInfoPageFlipHandler(int fd, unsigned int frame, unsigned int tv_sec,
                                   unsigned int tv_usec, void *event_data);
static void EXYNOSDispInfoIppHandler(int fd, unsigned int prop_id, unsigned int *buf_idx,
                  unsigned int tv_sec, unsigned int tv_usec, void *event_data);
static void EXYNOSDispG2dHandler(int fd, unsigned int cmdlist_no, unsigned int tv_sec,
                                   unsigned int tv_usec, void *event_data);

static const xf86CrtcConfigFuncsRec exynos_xf86crtc_config_funcs =
{
    EXYNOSCrtcConfigResize
};

static EXYNOSDrmEventCtxPtr
_exynosDisplayInitDrmEventCtx (void)
{
    EXYNOSDrmEventCtxPtr pEventCxt = NULL;

    XDBG_DEBUG (MDRM, " _exynosDisplayInitDrmEventCtx\n");
    pEventCxt = ctrl_calloc (1, sizeof (EXYNOSDrmEventCtxRec));
    if (!pEventCxt)
        return NULL;

    pEventCxt->vblank_handler =     EXYNOSDispInfoVblankHandler; 
    pEventCxt->page_flip_handler =  EXYNOSDispInfoPageFlipHandler;
    pEventCxt->ipp_handler =        EXYNOSDispInfoIppHandler;
    pEventCxt->g2d_handler = EXYNOSDispG2dHandler;

    return pEventCxt;
}

static void
_exynosDisplayDeInitDrmEventCtx(EXYNOSDrmEventCtxPtr pEventCtx)
{
    if (!pEventCtx)
        return;

    ctrl_free(pEventCtx);
    pEventCtx = NULL;
}

static int
_exynosDisplayHandleDrmEvent (int fd, EXYNOSDrmEventCtxPtr pEventCtx)
{
#define MAX_BUF_SIZE    1024

    char buffer[MAX_BUF_SIZE];
    unsigned int len, i;
    struct drm_event *e;
    /* The DRM read semantics guarantees that we always get only
     * complete events. */
    len = read (fd, buffer, sizeof buffer);
    if (len == 0)
    {
        XDBG_WARNING (MDISP, "warning: the size of the drm_event is 0.\n");
        return 0;
    }
    if (len < sizeof *e)
    {
        XDBG_WARNING (MDISP, "warning: the size of the drm_event is less than drm_event structure.\n");
        return -1;
    }
    if (len > MAX_BUF_SIZE - sizeof (struct drm_exynos_ipp_event))
    {
        XDBG_WARNING (MDISP, "warning: the size of the drm_event can be over the maximum size.\n");
        return -1;
    }

    i = 0;
    while (i < len)
    {
        e = (struct drm_event *) &buffer[i];
        switch (e->type)
        {
            case DRM_EVENT_VBLANK:
                {
                    struct drm_event_vblank *vblank;

                    if (pEventCtx->vblank_handler == NULL)
                        break;

                    vblank = (struct drm_event_vblank *) e;
                    pEventCtx->vblank_handler (fd,
                            vblank->sequence,
                            vblank->tv_sec,
                            vblank->tv_usec,
                            (void *)((unsigned long)vblank->user_data));
                }
                break;
            case DRM_EVENT_FLIP_COMPLETE:
                {
                    struct drm_event_vblank *vblank;

                    if (pEventCtx->page_flip_handler == NULL)
                        break;

                    vblank = (struct drm_event_vblank *) e;
                    pEventCtx->page_flip_handler (fd,
                            vblank->sequence,
                            vblank->tv_sec,
                            vblank->tv_usec,
                            (void *)((unsigned long)vblank->user_data));
                }
                break;
            case DRM_EXYNOS_G2D_EVENT:
                {
                    struct drm_exynos_g2d_event *g2d;
                    XDBG_DEBUG (MG2D, "****  DRM_EXYNOS_G2D_EVENT ********.. \n");
                    if (pEventCtx->g2d_handler == NULL)
                        break;

                    g2d = (struct drm_exynos_g2d_event *) e;
                    pEventCtx->g2d_handler(fd, g2d->cmdlist_no, g2d->tv_sec,
                            g2d->tv_usec, (void *) ((unsigned long) g2d->user_data));
                }
                break;

            case DRM_EXYNOS_IPP_EVENT:
                            {
                                struct drm_exynos_ipp_event *ipp;

                                if (pEventCtx->ipp_handler == NULL)
                                    break;

                                ipp = (struct drm_exynos_ipp_event *) e;
                                pEventCtx->ipp_handler (fd,
                                        ipp->prop_id,
                                        ipp->buf_id,
                                        ipp->tv_sec,
                                        ipp->tv_usec,
                                        (void *)((unsigned long)ipp->user_data));
                            }
                            break;
            default:
                break;
        }
        i += e->length;
    }

    return 0;
}

static void
_exynosDisplayRemoveFlipPixmaps (ScrnInfoPtr pScrn)
{
    xf86CrtcConfigPtr pCrtcConfig = XF86_CRTC_CONFIG_PTR (pScrn);
    int c;

    for (c = 0; c < pCrtcConfig->num_crtc; c++)
    {
        xf86CrtcPtr pCrtc = pCrtcConfig->crtc[c];
        int conn_type = exynosCrtcGetConnectType (pCrtc);
        if (conn_type == DRM_MODE_CONNECTOR_Unknown)
            exynosCrtcRemoveFlipPixmap (pCrtc);
    }
}

static Bool
EXYNOSCrtcConfigResize(ScrnInfoPtr pScrn, int width, int height)
{
    XDBG_RETURN_VAL_IF_FAIL (pScrn != NULL, FALSE);
    ScreenPtr pScreen = pScrn->pScreen;
    XDBG_DEBUG (MDISP, "Resize cur(%dx%d) new(%d,%d)\n",
                pScrn->virtualX, pScrn->virtualY,
                width, height);
    xf86DrvMsg (pScrn->scrnIndex, X_INFO,"Resize cur(%dx%d) new(%d,%d)\n",
                pScrn->virtualX, pScrn->virtualY,
                width, height);

    if (pScrn->virtualX == width &&
            pScrn->virtualY == height)
    {
        return TRUE;
    }
    /* exynosFbResize(pSec->pFb, width, height); */

    /* set  the new size of pScrn */
    pScrn->virtualX = width;
    pScrn->virtualY = height;

#if 1
    /* Function used in rotate - tested now */
    /* set the new pixmap associated with a logical screen */
    exynosExaScreenSetScrnPixmap (pScreen);
    outputDrmUpdate (pScrn);
    //_exynosDisplayRemoveFlipPixmaps (pScrn);
#endif
    return TRUE;
}




static void
EXYNOSDispInfoVblankHandler(int fd, unsigned int frame, unsigned int tv_sec,
                     unsigned int tv_usec, void *event_data)
{
    EXYNOSVBlankInfoPtr pVblankInfo = event_data;
    EXYNOSVBlankType vblank_type;
    void *data; 

    XDBG_RETURN_IF_FAIL (pVblankInfo != NULL);

    XDBG_DEBUG (MEVN, "vblank\n");
    vblank_type = pVblankInfo->type;
    data = pVblankInfo->data;

    if (vblank_type == VBLANK_INFO_SWAP)
        exynosDri2VblankEventHandler (frame, tv_sec, tv_usec, data);
#if 0
    xDbgLogDrmEventRemoveVblank (pVblankInfo->xdbg_log_vblank);

    else
        ErrorF("unknown the vblank type\n");
#endif

    ctrl_free (pVblankInfo);
    pVblankInfo = NULL;
}

#if 0
static void
EXYNOSDispInfoIppHandler(int fd, unsigned int prop_id, unsigned int *buf_idx,
        unsigned int tv_sec, unsigned int tv_usec, void *event_data)
{
    XDBG_DEBUG(MDISP, "prop_id(%d), buf_idx(%d, %d) \n", prop_id, buf_idx[0],
            buf_idx[1]);

    exynosVideoImageIppEventHandler(fd, prop_id, buf_idx, event_data);

}
#else

static void EXYNOSDispInfoIppHandler(int fd, unsigned int prop_id,
        unsigned int *buf_idx, unsigned int tv_sec, unsigned int tv_usec,
        void *event_data)
{
    XDBG_RETURN_IF_FAIL(buf_idx != NULL);
    XDBG_RETURN_IF_FAIL(event_data != NULL);
    XDBG_DEBUG(MDRM, "wb_prop_id(%d) prop_id(%d), buf_idx(%d, %d) \n",
            exynosCloneGetPropID (), prop_id, buf_idx[0], buf_idx[1]);

    if (exynosCloneGetPropID () == prop_id)
        exynosCloneHandleIppEvent (fd, buf_idx, event_data);

    else
        exynosVideoImageIppEventHandler (fd, prop_id, buf_idx, event_data);
}
#endif

static void
EXYNOSDispG2dHandler(int fd, unsigned int cmdlist_no, unsigned int tv_sec,
                  unsigned int tv_usec, void *event_data)
{
    XDBG_DEBUG (MEVN, "g2d\n");
}

static void
EXYNOSDispInfoPageFlipHandler(int fd, unsigned int frame, unsigned int tv_sec,
                       unsigned int tv_usec, void *event_data)
{
    EXYNOSPageFlipPtr pPageFlip = event_data;
    xf86CrtcPtr pCrtc = NULL;
    EXYNOSCrtcPrivPtr pCrtcPriv = NULL;

    XDBG_RETURN_IF_FAIL (pPageFlip != NULL);
    pCrtc = pPageFlip->pCrtc;
    XDBG_RETURN_IF_FAIL (pCrtc != NULL);

    pCrtcPriv = pCrtc->driver_private;
    XDBG_RETURN_IF_FAIL (pCrtcPriv != NULL);


    XDBG_DEBUG (MDISP, "ModePageFlipHandler ctrc_id:%d dispatch_me:%d, frame:%d\n",
                pCrtcPriv->crtc_id, pPageFlip->dispatch_me, frame);
    tbm_bo_unref(pPageFlip->back_bo);

    pCrtcPriv->flip_count--;

    XDBG_RETURN_IF_FAIL (pCrtcPriv->pDispInfo!=NULL);

    /* Last crtc completed flip? */
    if (pPageFlip->dispatch_me)
    {
        /* Deliver cached msc, ust from reference crtc to flip event handler */
        exynosDri2FlipEventHandler (pCrtcPriv->fe_frame, pCrtcPriv->fe_tv_sec,
                                 pCrtcPriv->fe_tv_usec, pCrtcPriv->event_data);
    }
    ctrl_free (pPageFlip);
    pPageFlip = NULL;

}

static void
EXYNOSDispInfoWakeupHanlder(pointer data, int err, pointer p)
{
    EXYNOSDispInfoPtr pDispInfo;
    fd_set *read_mask;
    if (data == NULL || err < 0)
        return;
    pDispInfo = data;
    read_mask = p;
    if (FD_ISSET (pDispInfo->drm_fd, read_mask)){
        _exynosDisplayHandleDrmEvent (pDispInfo->drm_fd, pDispInfo->pEventCtx);
    }
}


/* load palette per a crtc */
void
exynosDisplayLoadPalette (ScrnInfoPtr pScrn, int numColors, int* indices,
                    LOCO* colors, VisualPtr pVisual)
{
    xf86CrtcConfigPtr pCrtcConfig = XF86_CRTC_CONFIG_PTR (pScrn);
    int i, j, index;
    int p;
    uint16_t lut_r[256], lut_g[256], lut_b[256];

    ErrorF ("EXYNOSLoadPalette: numColors: %d\n", numColors);

    for (p = 0; p < pCrtcConfig->num_crtc; p++)
    {
        xf86CrtcPtr pCrtc = pCrtcConfig->crtc[p];

        switch (pScrn->depth)
        {
        case 16:
            for (i = 0; i < numColors; i++)
            {
                index = indices[i];
                if (index <= 31)
                {
                    for (j = 0; j < 8; j++)
                    {
                        lut_r[index * 8 + j] = colors[index].red << 8;
                        lut_b[index * 8 + j] = colors[index].blue << 8;
                    }
                }
                for (j = 0; j < 4; j++)
                {
                    lut_g[index * 4 + j] = colors[index].green << 8;
                }
            }
            break;
        default:
            for (i = 0; i < numColors; i++)
            {
                index = indices[i];
                lut_r[index] = colors[index].red << 8;
                lut_g[index] = colors[index].green << 8;
                lut_b[index] = colors[index].blue << 8;

            }
            break;
        }

        /* make the change through RandR */
        RRCrtcGammaSet (pCrtc->randr_crtc, lut_r, lut_g, lut_b);
    }
}

void
exynosDisplayModeFromKmode(ScrnInfoPtr pScrn,
                        drmModeModeInfoPtr kmode,
                        DisplayModePtr    pMode)
{
    memset (pMode, 0, sizeof (DisplayModeRec));
    pMode->status = MODE_OK;

    pMode->Clock = kmode->clock;

    pMode->HDisplay = kmode->hdisplay;
    pMode->HSyncStart = kmode->hsync_start;
    pMode->HSyncEnd = kmode->hsync_end;
    pMode->HTotal = kmode->htotal;
    pMode->HSkew = kmode->hskew;

    pMode->VDisplay = kmode->vdisplay;
    pMode->VSyncStart = kmode->vsync_start;
    pMode->VSyncEnd = kmode->vsync_end;
    pMode->VTotal = kmode->vtotal;
    pMode->VScan = kmode->vscan;

    pMode->Flags = kmode->flags; //& FLAG_BITS;
    pMode->name = ctrl_strdup (kmode->name);

    if (kmode->type & DRM_MODE_TYPE_DRIVER)
        pMode->type = M_T_DRIVER;
    if (kmode->type & DRM_MODE_TYPE_PREFERRED)
        pMode->type |= M_T_PREFERRED;

    xf86SetModeCrtc (pMode, pScrn->adjustFlags);
}

void
exynosDisplayModeToKmode(ScrnInfoPtr pScrn,
                      drmModeModeInfoPtr kmode,
                      DisplayModePtr pMode)
{
    memset (kmode, 0, sizeof (*kmode));

    kmode->clock = pMode->Clock;
    kmode->hdisplay = pMode->HDisplay;
    kmode->hsync_start = pMode->HSyncStart;
    kmode->hsync_end = pMode->HSyncEnd;
    kmode->htotal = pMode->HTotal;
    kmode->hskew = pMode->HSkew;

    kmode->vdisplay = pMode->VDisplay;
    kmode->vsync_start = pMode->VSyncStart;
    kmode->vsync_end = pMode->VSyncEnd;
    kmode->vtotal = pMode->VTotal;
    kmode->vscan = pMode->VScan;
    kmode->vrefresh = xf86ModeVRefresh (pMode);

    kmode->flags = pMode->Flags; //& FLAG_BITS;
    if (pMode->name)
        strncpy (kmode->name, pMode->name, DRM_DISPLAY_MODE_LEN);

    kmode->name[DRM_DISPLAY_MODE_LEN-1] = 0;
}

/* check the crtc is on */
Bool
exynosCrtcOn(xf86CrtcPtr pCrtc)
{
    ScrnInfoPtr pScrn = pCrtc->scrn;
    xf86CrtcConfigPtr pCrtcConfig = XF86_CRTC_CONFIG_PTR (pScrn);
    int i;

    if (!pCrtc->enabled)
        return FALSE;

    /* Kernel manage CRTC status based out output config */
    for (i = 0; i < pCrtcConfig->num_output; i++)
    {
        xf86OutputPtr pOutput = pCrtcConfig->output[i];
        if (pOutput->crtc == pCrtc 
 //               && exynosOutputDpmsStatus(pOutput) == DPMSModeOn
                )
            return TRUE;
    }

    return FALSE;/*was TRUE. should be tested*/
}

/*
  * Return the crtc covering 'box'. If two crtcs cover a portion of
  * 'box', then prefer 'desired'. If 'desired' is NULL, then prefer the crtc
  * with greater coverage
  */
xf86CrtcPtr
exynosDisplayCoveringCrtc (ScrnInfoPtr pScrn, BoxPtr pBox, xf86CrtcPtr pDesiredCrtc, BoxPtr pBoxCrtc)
{
    xf86CrtcConfigPtr pCrtcConfig = XF86_CRTC_CONFIG_PTR (pScrn);
    xf86CrtcPtr pCrtc, pBestCrtc;
    int coverage, best_coverage;
    int c;
    BoxRec crtc_box, cover_box;

    XDBG_RETURN_VAL_IF_FAIL (pBox != NULL, NULL);

    pBestCrtc = NULL;
    best_coverage = 0;

    if (pBoxCrtc)
    {
        pBoxCrtc->x1 = 0;
        pBoxCrtc->y1 = 0;
        pBoxCrtc->x2 = 0;
        pBoxCrtc->y2 = 0;
    }

    for (c = 0; c < pCrtcConfig->num_crtc; c++)
    {
        pCrtc = pCrtcConfig->crtc[c];

        /* If the CRTC is off, treat it as not covering */
        if(!exynosCrtcOn(pCrtc))
            continue;

        crtc_box.x1 = pCrtc->x;
        crtc_box.x2 = pCrtc->x + xf86ModeWidth (&pCrtc->mode, pCrtc->rotation);
        crtc_box.y1 = pCrtc->y;
        crtc_box.y2 = pCrtc->y + xf86ModeHeight (&pCrtc->mode, pCrtc->rotation);

        exynosUtilBoxIntersect(&cover_box, &crtc_box, pBox);
        coverage = exynosUtilBoxArea(&cover_box);

        if (coverage && pCrtc == pDesiredCrtc)
        {
            if (pBoxCrtc)
                *pBoxCrtc = crtc_box;
            return pCrtc;
        }

        if (coverage > best_coverage)
        {
            if (pBoxCrtc)
                *pBoxCrtc = crtc_box;
            pBestCrtc = pCrtc;
            best_coverage = coverage;
        }
    }

    return pBestCrtc;
}

int exynosDisplayGetCrtcPipe (xf86CrtcPtr pCrtc)
{
    EXYNOSCrtcPrivPtr pCrtcPriv = pCrtc->driver_private;
    return pCrtcPriv->pipe;
}


xf86CrtcPtr 
exynosDisplayGetCrtc (DrawablePtr pDraw)
{
    ScreenPtr pScreen = pDraw->pScreen;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    BoxRec box, crtc_box;
    xf86CrtcPtr pCrtc;

    box.x1 = pDraw->x;
    box.y1 = pDraw->y;
    box.x2 = box.x1 + pDraw->width;
    box.y2 = box.y1 + pDraw->height;
    
    pCrtc = exynosDisplayCoveringCrtc (pScrn, &box, NULL, &crtc_box);
    return pCrtc;
}

Bool
displaySetDispConnMode (ScrnInfoPtr pScrn, EXYNOS_DisplayConnMode conn_mode)
{
    XDBG_RETURN_VAL_IF_FAIL(pScrn != NULL,FALSE);

    EXYNOSDispInfoPtr pDispInfo = EXYNOSDISPINFOPTR(pScrn);
    XDBG_RETURN_VAL_IF_FAIL(pDispInfo != NULL,FALSE);

    if (pDispInfo->conn_mode == conn_mode)
    {
        XDBG_DEBUG (MPROP, "conn_mode(%d) is already set\n", conn_mode);
        return TRUE;
    }

    switch (conn_mode)
    {
    case DISPLAY_CONN_MODE_NONE:
       break;
    case DISPLAY_CONN_MODE_HDMI:
       break;
    case DISPLAY_CONN_MODE_VIRTUAL:
       break;
    default:
       break;
    }

    pDispInfo->conn_mode = conn_mode;

    return TRUE;
}




static int
_displayGetFreeCrtcID (ScrnInfoPtr pScrn)
{
    EXYNOSDispInfoPtr pExynosMode = (EXYNOSDispInfoPtr) EXYNOSPTR (pScrn)->pDispInfo;
    int i;
    int crtc_id = 0;

    for (i = 0; i < pExynosMode->mode_res->count_crtcs; i++)
    {
        drmModeCrtcPtr kcrtc = NULL;
        kcrtc = drmModeGetCrtc (pExynosMode->drm_fd, pExynosMode->mode_res->crtcs[i]);
        if (!kcrtc)
        {
            XDBG_ERROR (MTVO, "fail to get kcrtc. \n");
            return 0;
        }

        if (kcrtc->buffer_id > 0)
        {
            drmModeFreeCrtc (kcrtc);
            continue;
        }

        crtc_id = kcrtc->crtc_id;
        drmModeFreeCrtc (kcrtc);

        return crtc_id;
    }

    return 0;
}



static EXYNOSOutputPrivPtr
_lookForOutputByConnType (ScrnInfoPtr pScrn, int connect_type)
{

    int i;
    EXYNOSDispInfoPtr pExynosMode = (EXYNOSDispInfoPtr) EXYNOSPTR (pScrn)->pDispInfo;
    for (i = 0; i < pExynosMode->mode_res->count_connectors; i++)
    {
        EXYNOSOutputPrivPtr pCur, pNext;

        ctrl_xorg_list_for_each_entry_safe(pCur, pNext, &pExynosMode->outputs, link)
        {
            drmModeConnectorPtr koutput = pCur->mode_output;

            if (koutput && koutput->connector_type == connect_type)
                return pCur;
        }
    }

    XDBG_ERROR (MTVO, "no output for connect_type(%d) \n", connect_type);

    return NULL;
}

static int
_fbUnrefBo(tbm_bo bo)
{
    tbm_bo_unref(bo);
    bo = NULL;

    return TRUE;
}


void
_fbFreeBoData(void* data)
{
    XDBG_RETURN_IF_FAIL(data != NULL);

    ScrnInfoPtr pScrn = NULL;
    EXYNOSPtr pExynos = NULL;
    EXYNOS_FbBoDataPtr  bo_data = (EXYNOS_FbBoDataPtr)data;

    pScrn = bo_data->pScrn;
    pExynos = EXYNOSPTR (pScrn);

    XDBG_DEBUG (MTVO, "FreeRender bo_data gem:%d, fb_id:%d, %dx%d+%d+%d\n",
                bo_data->gem_handle, bo_data->fb_id,
                bo_data->pos.x2-bo_data->pos.x1, bo_data->pos.y2-bo_data->pos.y1,
                bo_data->pos.x1, bo_data->pos.y1);

    if (bo_data->fb_id)
    {
        drmModeRmFB (pExynos->drm_fd, bo_data->fb_id);
        bo_data->fb_id = 0;
    }

//    if (bo_data->pPixmap)
//    {
//        pScrn->pScreen->DestroyPixmap (bo_data->pPixmap);
//        bo_data->pPixmap = NULL;
//    }

    ctrl_free (bo_data);
    bo_data = NULL;
}

#define BORDER 100


tbm_bo
exynosDisplayRenderBoCreate (ScrnInfoPtr pScrn, int x, int y, int width, int height)
{
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    XDBG_RETURN_VAL_IF_FAIL ((width > 0), NULL);
    XDBG_RETURN_VAL_IF_FAIL ((height > 0), NULL);

    tbm_bo bo = NULL;
    tbm_bo_handle bo_handle1, bo_handle2;
    EXYNOS_FbBoDataPtr bo_data=NULL;
    EXYNOS_FbPtr pFb=pExynos->pFb;
    unsigned int pitch;
    unsigned int fb_id = 0;
    int ret;
    int flag;

    pitch = width * 4;


    if (!pExynos->cachable)
        flag = TBM_BO_WC;
    else
        flag = TBM_BO_DEFAULT;


    bo = tbm_bo_alloc (pExynos->bufmgr, pitch*height, flag);
    XDBG_GOTO_IF_FAIL (bo != NULL, fail);

    /* memset 0x0 */
    bo_handle1 = tbm_bo_map (bo, TBM_DEVICE_CPU, TBM_OPTION_WRITE);
    XDBG_RETURN_VAL_IF_FAIL (bo_handle1.ptr != NULL, NULL);
    //clear
    memset (bo_handle1.ptr, 0x0, pitch*height);
    //fill some data
    tbm_bo_unmap (bo);

    bo_handle2 = tbm_bo_get_handle(bo, TBM_DEVICE_DEFAULT);

    /* Create drm fb */
    ret = drmModeAddFB(pExynos->drm_fd
                       , width, height
                       , pScrn->depth
                       , pScrn->bitsPerPixel
                       , pitch
                       , bo_handle2.u32
                       , &fb_id);
    XDBG_GOTO_IF_ERRNO(ret == Success, fail, -ret);

    if(x == -1) x = 0;
    if(y == -1) y = 0;

    /* Set bo user data */
    bo_data = ctrl_calloc(1, sizeof(EXYNOS_FbBoDataRec));
    bo_data->pFb = pFb;
    bo_data->gem_handle = bo_handle2.u32;
    bo_data->pitch = pitch;
    bo_data->fb_id = fb_id;
    bo_data->pos.x1 = x;
    bo_data->pos.y1 = y;
    bo_data->pos.x2 = x+width;
    bo_data->pos.y2 = y+height;
    bo_data->size = tbm_bo_size(bo);
    bo_data->pScrn = pScrn;
    XDBG_GOTO_IF_FAIL(tbm_bo_add_user_data(bo, TBM_BO_DATA_FB, _fbFreeBoData), fail);
    XDBG_GOTO_IF_FAIL(tbm_bo_set_user_data(bo, TBM_BO_DATA_FB, (void *)bo_data), fail);

    XDBG_DEBUG (MTVO, "CreateRender bo(name:%d, gem:%d, fb_id:%d, %dx%d+%d+%d\n",
                tbm_bo_export (bo), bo_data->gem_handle, bo_data->fb_id,
                bo_data->pos.x2-bo_data->pos.x1, bo_data->pos.y2-bo_data->pos.y1,
                bo_data->pos.x1, bo_data->pos.y1);

    return bo;
fail:
    if (bo)
    {
        _fbUnrefBo(bo);
    }

    if (fb_id)
    {
        drmModeRmFB(pExynos->drm_fd, fb_id);
    }

    if (bo_data)
    {
        ctrl_free (bo_data);
        bo_data = NULL;
    }

    return NULL;
}



Bool
displayInitDispMode (ScrnInfoPtr pScrn,EXYNOSOutputPrivPtr ext_output)
{
    EXYNOSDispInfoPtr pExynosMode = (EXYNOSDispInfoPtr) EXYNOSPTR (pScrn)->pDispInfo;
    Bool ret = FALSE;
    uint32_t *output_ids = NULL;
    int output_cnt = 1;
    uint32_t fb_id;
    drmModeModeInfoPtr pKmode = NULL;
    drmModeCrtcPtr kcrtc_hdmi = NULL;
    EXYNOS_FbBoDataPtr bo_data = NULL;
    int connector_type = -1;
    int width, height;

    if (pExynosMode->crtc_already_connected)
        return TRUE;

    /* get output ids */
    output_ids = ctrl_calloc (output_cnt, sizeof (uint32_t));
    XDBG_RETURN_VAL_IF_FAIL (output_ids != NULL, FALSE);

    XDBG_GOTO_IF_FAIL (((pExynosMode->conn_mode == DISPLAY_CONN_MODE_HDMI)||
                        (pExynosMode->conn_mode == DISPLAY_CONN_MODE_VIRTUAL)), fail_to_init);

    output_ids[0] = ext_output->mode_output->connector_id;
    pKmode = &pExynosMode->ext_connector_mode;
    connector_type = ext_output->mode_output->connector_type;

    XDBG_GOTO_IF_FAIL (output_ids[0] > 0, fail_to_init);
    XDBG_GOTO_IF_FAIL (pKmode != NULL, fail_to_init);

    XDBG_INFO (MPROP, "pKmode->name - %s connector_type-%i\n", pKmode->name,connector_type);

    width = pKmode->hdisplay;
    height = pKmode->vdisplay;

    pExynosMode->hdmi_crtc_id = _displayGetFreeCrtcID (pScrn);

    XDBG_GOTO_IF_FAIL (pExynosMode->hdmi_crtc_id > 0, fail_to_init);

    /* get crtc_id */
    kcrtc_hdmi = drmModeGetCrtc (pExynosMode->drm_fd, pExynosMode->hdmi_crtc_id);

    ext_output->pOutput->crtc= exynosFindCrtc(pScrn,pExynosMode->hdmi_crtc_id);

    XDBG_GOTO_IF_FAIL (kcrtc_hdmi != NULL, fail_to_init);

    if (kcrtc_hdmi->buffer_id > 0)
    {
        XDBG_ERROR (MTVO, "crtc(%d) already has buffer(%d) \n",
                pExynosMode->hdmi_crtc_id, kcrtc_hdmi->buffer_id);
        goto fail_to_init;
    }

    /* get fb_id */
    pExynosMode->hdmi_bo = exynosDisplayRenderBoCreate (pScrn,-1,-1, width, height);
    //hdmi_bo=0;
    XDBG_GOTO_IF_FAIL (pExynosMode->hdmi_bo != NULL, fail_to_init);

    tbm_bo_get_user_data(pExynosMode->hdmi_bo, TBM_BO_DATA_FB, (void * *)&bo_data);

    XDBG_INFO (MPROP, "bo_data - %x\n", bo_data);

    XDBG_GOTO_IF_FAIL (bo_data != NULL, fail_to_init);

    fb_id = bo_data->fb_id;

    /* set crtc */
    if (drmModeSetCrtc (pExynosMode->drm_fd, pExynosMode->hdmi_crtc_id, fb_id, 0, 0, output_ids, output_cnt, pKmode))
    {
        XDBG_ERRNO (MTVO, "drmModeSetCrtc failed. \n");
        goto fail_to_init;
    }
    else
    {
        ret = TRUE;
    }

    exynosUtilSetDrmProperty(pExynosMode->drm_fd,pExynosMode->hdmi_crtc_id,DRM_MODE_OBJECT_CRTC, "mode", 1);

    outputDrmUpdate (pScrn);

    XDBG_INFO (MTVO, "** ModeSet : (%dx%d) %dHz !!\n", pKmode->hdisplay, pKmode->vdisplay, pKmode->vrefresh);

    pExynosMode->crtc_already_connected = TRUE;

fail_to_init:
    ctrl_free (output_ids);
    if (kcrtc_hdmi)
        drmModeFreeCrtc (kcrtc_hdmi);

    return ret;
}


static void
_exynosDisplayCloneCloseFunc (EXYNOS_CloneObject* clone, EXYNOS_CLONE_NOTIFY noti, void *noti_data, void *user_data)
{
    ScrnInfoPtr pScrn = (ScrnInfoPtr)user_data;


    if (!pScrn)
        return;

    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);

    pExynos->clone = NULL;
}


void
displayDeinitDispMode (ScrnInfoPtr pScrn,EXYNOSOutputPrivPtr ext_output)
{
    EXYNOSDispInfoPtr pExynosMode = (EXYNOSDispInfoPtr) EXYNOSPTR (pScrn)->pDispInfo;

    if (!pExynosMode->crtc_already_connected )
        return;

    XDBG_INFO (MDISP, "** ModeUnset. !!\n");

    //utilSetDrmProperty (pSecMode, crtc_id, DRM_MODE_OBJECT_CRTC, "mode", 0);

    if (pExynosMode->hdmi_bo)
    {
        _fbUnrefBo (pExynosMode->hdmi_bo);
        pExynosMode->hdmi_bo = NULL;
    }

    outputDrmUpdate (pScrn);

    pExynosMode->hdmi_crtc_id = 0;
    pExynosMode->crtc_already_connected = FALSE;
    ext_output->pOutput->crtc=0;
}


Bool
connectExternalCrtc (ScrnInfoPtr scrn)
{
    EXYNOSDispInfoPtr pExynosMode = (EXYNOSDispInfoPtr) EXYNOSPTR (scrn)->pDispInfo;
    EXYNOSOutputPrivPtr pOutputPriv = NULL;

    XDBG_RETURN_VAL_IF_FAIL (scrn != NULL, FALSE);

    if (pExynosMode->conn_mode == DISPLAY_CONN_MODE_HDMI)
    {
        pOutputPriv = _lookForOutputByConnType (scrn, DRM_MODE_CONNECTOR_HDMIA);
        if (!pOutputPriv)
            pOutputPriv = _lookForOutputByConnType (scrn, DRM_MODE_CONNECTOR_HDMIB);
    }
    else
        pOutputPriv = _lookForOutputByConnType (scrn, DRM_MODE_CONNECTOR_VIRTUAL);

    XDBG_RETURN_VAL_IF_FAIL (pOutputPriv != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (pOutputPriv->mode_encoder != NULL, FALSE);

    if (pOutputPriv->mode_encoder->crtc_id > 0)
        return TRUE;

    displayDeinitDispMode (scrn,pOutputPriv);
    return displayInitDispMode (scrn, pOutputPriv);
}

void set_output_crtc(int out,int crtc);

Bool displaySetDispSetMode(xf86OutputPtr pOutput, EXYNOS_DisplaySetMode set_mode)
{
    Bool scanout = TRUE;
    int wb_hz = 60;
    EXYNOSPtr pExynos = EXYNOSPTR (pOutput->scrn);
    EXYNOSDispInfoPtr pDispInfo = EXYNOSDISPINFOPTR(pOutput->scrn);
    XDBG_DEBUG(MPROP, "pDispInfo->conn_mode (%i) set_mode (%i)\n",
            pDispInfo->conn_mode, set_mode);
    if (pDispInfo->set_mode == set_mode) {
        XDBG_INFO(MPROP, "set_mode(%d) is already set\n", set_mode);
        return TRUE;
    }

    if (pDispInfo->conn_mode == DISPLAY_CONN_MODE_NONE) {
        XDBG_WARNING(MPROP,
                "set_mode(%d) is failed : output is not connected yet\n",
                set_mode);
        return FALSE;
    }
    switch (set_mode) {
    case DISPLAY_SET_MODE_OFF:
        XDBG_INFO(MTVO, "DISPLAY_SET_MODE_OFF\n");
        if (exynosCloneIsOpened())
        {

            exynosCloneClose(pExynos->clone);
        }

        displayDeinitDispMode(pOutput->scrn,pOutput->driver_private);
        pExynos->clone = NULL;
        break;
    case DISPLAY_SET_MODE_CLONE:
        /* In case of DISPLAY_CONN_MODE_VIRTUAL, we will open writeback
         * on GetStill.
         */
        XDBG_INFO(MTVO, "DISPLAY_SET_MODE_CLONE\n");
        if (pDispInfo->conn_mode != DISPLAY_CONN_MODE_VIRTUAL)
        {
            if (pExynos->clone)
            {
                XDBG_ERROR(MCLO, "Fail : clone(%p) already exists.\n",
                           pExynos->clone);
                break;
            }
            pExynos->clone = exynosCloneInit(pOutput->scrn, FOURCC_SN12, 0, 0, scanout,wb_hz, TRUE);
            if (pExynos->clone)
            {
                exynosCloneAddNotifyFunc(pExynos->clone, CLONE_NOTI_CLOSED,
                                         _exynosDisplayCloneCloseFunc, pOutput->scrn);
                if (!exynosCloneStart(pExynos->clone)) {
                    XDBG_INFO(MTVO, "!exynosCloneStart\n");
                    exynosCloneClose(pExynos->clone);
                    return FALSE;
                } else
                    XDBG_INFO(MTVO, "exynosCloneStart\n");
            }
            connectExternalCrtc(pOutput->scrn);
        }
        break;

    case DISPLAY_SET_MODE_EXT:
        XDBG_INFO(MTVO, "DISPLAY_SET_MODE_EXT\n");
        if (exynosCloneIsOpened())
        {

            exynosCloneClose(pExynos->clone);
        }

        displayDeinitDispMode(pOutput->scrn,pOutput->driver_private);
        break;

    case DISPLAY_SET_MODE_VIRTUAL:
        XDBG_INFO(MTVO, "DISPLAY_SET_MODE_VIRTUAL\n");
        pDispInfo->conn_mode = DISPLAY_CONN_MODE_VIRTUAL;
        connectExternalCrtc(pOutput->scrn);
        break;
    default:
        break;
    }

    pDispInfo->set_mode = set_mode;
    return TRUE;
}

int
exynosDisplayGetPipe (ScrnInfoPtr pScrn, DrawablePtr pDraw)
{
    int pipe = -1;
	xf86CrtcPtr pCrtc;

	pCrtc = exynosDisplayGetCrtc(pDraw);

    if (pCrtc != NULL && !pCrtc->rotatedData)
    {
        pipe = exynosDisplayGetCrtcPipe (pCrtc);
    }

    return pipe;
}


Bool
exynosDisplayPipeIsOn (ScrnInfoPtr pScrn, int pipe)
{
    EXYNOSDispInfoPtr pDispInfo = EXYNOSDISPINFOPTR(pScrn);

    if (!pDispInfo->pipe0_on)
        return FALSE;

    return TRUE;
}


Bool
exynosDisplayGetCurMSC (ScrnInfoPtr pScrn, int pipe, CARD64 *ust, CARD64 *msc)
{
    EXYNOSDispInfoPtr pDispInfo = EXYNOSDISPINFOPTR(pScrn);
    drmVBlank vbl;
    int ret;

    /* TODO: if a crtc of pipe is off, return true with msc = 0 */
    if (!pDispInfo->pipe0_on)
    {
        *ust = 0;
        *msc = 0;
        return TRUE;
    }

    /* if pipe is -1, return the current msc of the main crtc */
    if (pipe == -1)
        pipe = 0;

    vbl.request.type = DRM_VBLANK_RELATIVE;

    if (pipe == 1)
        vbl.request.type |= DRM_VBLANK_SECONDARY;
    if (pipe == 2)
        vbl.request.type |= _DRM_VBLANK_EXYNOS_VIDI;

    vbl.request.sequence = 0;
    ret = drmWaitVBlank (pDispInfo->drm_fd, &vbl);
    if (ret)
    {
        *ust = 0;
        *msc = 0;
        xf86DrvMsg (pScrn->scrnIndex, X_WARNING,
                    "first get vblank counter failed: %s\n",
                    strerror (errno));
        return FALSE;
    }

    *ust = ((CARD64) vbl.reply.tval_sec * 1000000) + vbl.reply.tval_usec;
    *msc = vbl.reply.sequence;

    return TRUE;
}

Bool
exynosDisplayVBlank (ScrnInfoPtr pScrn, int pipe, CARD64 *target_msc, int flip,
                        EXYNOSVBlankType type, void *data)
{
    EXYNOSVBlankInfoPtr pVblankInfo = NULL;
    EXYNOSDispInfoPtr pDispInfo = EXYNOSDISPINFOPTR(pScrn);
    drmVBlank vbl;
    int ret;

    pVblankInfo = ctrl_calloc (1, sizeof (EXYNOSVBlankInfoRec));
    if (pVblankInfo == NULL)
    {
        xf86DrvMsg (pScrn->scrnIndex, X_WARNING, "data alloc failed\n");
        return FALSE;
    }

    pVblankInfo->type = type;
    pVblankInfo->data = data;

    vbl.request.type =  DRM_VBLANK_ABSOLUTE | DRM_VBLANK_EVENT;

    if (pipe == 1)
        vbl.request.type |= DRM_VBLANK_SECONDARY;
    if (pipe == 2)
        vbl.request.type |= _DRM_VBLANK_EXYNOS_VIDI;

    /* If non-pageflipping, but blitting/exchanging, we need to use
     * DRM_VBLANK_NEXTONMISS to avoid unreliable timestamping later
     * on.
     */
    if (flip == 0)
    {
        if (pipe < 2)
            vbl.request.type |= DRM_VBLANK_NEXTONMISS;
    }

    vbl.request.sequence = *target_msc;
    vbl.request.signal = (unsigned long) pVblankInfo;

    ret = drmWaitVBlank (pDispInfo->drm_fd, &vbl);
    if (ret)
    {
        if (pVblankInfo)
        {
            ctrl_free(pVblankInfo);
            pVblankInfo = NULL;
        }
        xf86DrvMsg (pScrn->scrnIndex, X_WARNING,
                "divisor 0 get vblank counter failed: %s\n",
                strerror (errno));
        return FALSE;
    }

    /* Adjust returned value for 1 fame pageflip offset of flip > 0 */
    *target_msc = vbl.reply.sequence + flip;

    return TRUE;
}

#define USE_PAGE_FLIP

Bool
exynosDisplayPageFlip (ScrnInfoPtr pScrn, int pipe, PixmapPtr pPix, void *data)
{
    EXYNOSPageFlipPtr pPageFlip = NULL;
    EXYNOSCrtcPrivPtr pCrtcPriv;
    EXYNOSDispInfoPtr pDispInfo;
    xf86CrtcConfigPtr pCrtcConfig = XF86_CRTC_CONFIG_PTR (pScrn);
    xf86CrtcPtr pCrtc;
    int ret;
    int found = 0;
    int i;
    tbm_bo tbo = NULL;
    tbm_bo_handle bo_handle;
    int pitch = 4;

#ifdef USE_PAGE_FLIP
    for (i = 0; i < pCrtcConfig->num_crtc; i++)
    {
        pCrtc = pCrtcConfig->crtc[i];
        if (!pCrtc->enabled)
            continue;
        pCrtcPriv =  pCrtc->driver_private;
        pDispInfo =  pCrtcPriv->pDispInfo;

        pPageFlip = ctrl_calloc (1, sizeof (EXYNOSPageFlipRec));
        if (pPageFlip == NULL)
        {
            xf86DrvMsg (pScrn->scrnIndex, X_WARNING, "Page flip alloc failed\n");
            return FALSE;
        }

        pCrtcPriv->old_fb_id = pCrtcPriv->fb_id;

        tbo = exynosExaPixmapGetBo (pPix);

        EXYNOS_FbBoDataPtr bo_data = NULL;
        if(tbm_bo_get_user_data(tbo, TBM_BO_DATA_FB, (void**)&bo_data) == FALSE)
        {
            xf86DrvMsg (pScrn->scrnIndex, X_WARNING, "Page flip failed: %s\n", strerror (errno));
            goto fail;
        }
        else
        {
        	pCrtcPriv->fb_id = bo_data->fb_id;
            XDBG_DEBUG (MDISP, "reuse fb: %d\n", pCrtcPriv->fb_id);
        }

        /* Only the reference crtc will finally deliver its page flip
                * completion event. All other crtc's events will be discarded.
                */
        pPageFlip->dispatch_me = 0;
        pPageFlip->pCrtc = pCrtc;
        pPageFlip->back_bo = tbm_bo_ref(tbo);
        pPageFlip->data = data;

        tbm_bo_map(pPageFlip->back_bo, TBM_DEVICE_2D, TBM_OPTION_READ);
        tbm_bo_unmap(pPageFlip->back_bo);

        /* DRM Page Flip */
        ret  = drmModePageFlip (pDispInfo->drm_fd, pCrtcPriv->crtc_id, pCrtcPriv->fb_id,
                DRM_MODE_PAGE_FLIP_EVENT, pPageFlip);
        if (ret)
        {
            xf86DrvMsg (pScrn->scrnIndex, X_WARNING, "Page flip failed: %s\n", strerror (errno));
            goto fail;
        }
        pCrtcPriv->flip_count++; /* check flipping completed */
        pCrtcPriv->event_data = data;

        found++;
        XDBG_DEBUG(MDISP, "ModePageFlip crtc_id:%d, fb_id:%d, old_fb_id:%d\n",
                   pCrtcPriv->crtc_id, pCrtcPriv->fb_id, pCrtcPriv->old_fb_id);
    }

    if(found==0)
    {
        XDBG_WARNING (MDISP, "Cannot find CRTC\n");
        return FALSE;
    }

    /* Set dispatch_me to last pageflip */
    pPageFlip->dispatch_me = 1;
#else
    XDBG_DEBUG(MDISP, "++++++++++++++++++++++++++++++++++work\n");
#endif

    return TRUE;

fail:
    /* TODO: deal with error condition */
#if 0
    pCrtcPriv->flip_count++; /* check flipping completed */
    pCrtcPriv->event_data = data;
    pPageFlip->dispatch_me = 1;

    EXYNOSDispInfoPageFlipHandler(pDispInfo->drm_fd, 0, 0, 0, pPageFlip);
#endif
    XDBG_ERROR (MDISP, "drmModePageFlip error(crtc:%d, fb_id:%d)\n",
                pCrtcPriv->crtc_id, pCrtcPriv->fb_id);

    return FALSE;
}





static int
_exynosSetMainMode (ScrnInfoPtr pScrn, EXYNOSDispInfoPtr pDispInfo)
{
    xf86CrtcConfigPtr pXf86CrtcConfig;
    pXf86CrtcConfig = XF86_CRTC_CONFIG_PTR (pScrn);
    int i;

    for (i = 0; i < pXf86CrtcConfig->num_output; i++)
    {
        xf86OutputPtr pOutput = pXf86CrtcConfig->output[i];
        EXYNOSOutputPrivPtr pOutputPriv = pOutput->driver_private;
        if (pOutputPriv->mode_output->connector_type == DRM_MODE_CONNECTOR_LVDS ||
            pOutputPriv->mode_output->connector_type == DRM_MODE_CONNECTOR_Unknown)
        {
            memcpy (&pDispInfo->main_lcd_mode, pOutputPriv->mode_output->modes, sizeof(drmModeModeInfo));
            return 1;
        }
    }

    return -1;
}

Bool
exynosDisplayPreInit (ScrnInfoPtr pScrn, int drm_fd)
{
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    EXYNOSDispInfoPtr pDispInfo;
    unsigned int i;

    /* allocate the dispaly info */
    pDispInfo = ctrl_calloc (1, sizeof(EXYNOSDispInfoRec));
    if (!pDispInfo)
        return FALSE;

    pDispInfo->drm_fd = drm_fd;
    xorg_list_init (&pDispInfo->crtcs);
    xorg_list_init (&pDispInfo->outputs);
    xorg_list_init (&pDispInfo->planes);

    pExynos->pDispInfo = pDispInfo;

    /* initialize xf86crtcconfig */
    xf86CrtcConfigInit (pScrn, &exynos_xf86crtc_config_funcs);

    /* get drmmode resources (crtcs, encoders, connectors, and so on) */
    pDispInfo->mode_res = drmModeGetResources (pDispInfo->drm_fd);
    if (!pDispInfo->mode_res)
    {
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
                    "failed to get resources: %s\n", strerror (errno));
        ctrl_free (pDispInfo);
        return FALSE;
    }

    /* get drmmode plane resources */
    pDispInfo->plane_res = drmModeGetPlaneResources (pDispInfo->drm_fd);
    if (!pDispInfo->plane_res)
    {
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
                    "failed to get plane resources: %s\n", strerror (errno));
        drmModeFreeResources (pDispInfo->mode_res);
        ctrl_free (pDispInfo);
        return FALSE;
    }

    xf86CrtcSetSizeRange (pScrn, 320, 200, pDispInfo->mode_res->max_width,
                          pDispInfo->mode_res->max_height);

    for (i = 0; i < pDispInfo->mode_res->count_crtcs; i++)
        exynosCrtcInit (pScrn, pDispInfo, i);
    xf86DrvMsg (pScrn->scrnIndex, X_INFO,
                       "total:%i CRTCs\n",pDispInfo->mode_res->count_crtcs);

    for (i = 0; i < pDispInfo->mode_res->count_connectors; i++)
        exynosOutputInit (pScrn, pDispInfo, i);
    xf86DrvMsg (pScrn->scrnIndex, X_INFO,
                       "total:%i connectors\n",pDispInfo->mode_res->count_connectors);

    for (i = 0; i < pDispInfo->plane_res->count_planes; i++)
        exynosPlaneInit (pScrn, pDispInfo, i);
    xf86DrvMsg (pScrn->scrnIndex, X_INFO,
                        "total:%i planes\n",pDispInfo->plane_res->count_planes);
    
    if (!pDispInfo->using_planes)
        pDispInfo->using_planes = exynosUtilStorageInit();
    if (pDispInfo->using_planes == NULL)
    {
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
                    "failed to allocate memory.\n");
        return FALSE;
    }

    _exynosSetMainMode (pScrn, pDispInfo);

    xf86InitialConfiguration (pScrn, TRUE);

    /* assume that drm module supports the pageflipping and the drm vblank */
    pDispInfo->pEventCtx = _exynosDisplayInitDrmEventCtx ();
    if (!pDispInfo->pEventCtx)
    {
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
                    "failed to set drm event context.\n");
        drmModeFreeResources (pDispInfo->mode_res);
        drmModeFreePlaneResources (pDispInfo->plane_res);
        ctrl_free (pDispInfo);
        return FALSE;

    }

    pDispInfo->crtc_already_connected=FALSE;

#if 0
    /* virtual x and virtual y of the screen is ones from main lcd mode */
    pScrn->virtualX = pDispInfo->main_lcd_mode.hdisplay;
    pScrn->virtualY = pDispInfo->main_lcd_mode.vdisplay;
#endif
#ifdef USE_XDBG_EXTERNAL
    xDbgLogDrmEventInit();
#endif
    return TRUE;
}

void
exynosDisplayInit (ScrnInfoPtr pScrn)
{
    EXYNOSDispInfoPtr pDispInfo = EXYNOSDISPINFOPTR(pScrn);

    /* We need to re-register the mode->fd for the synchronisation
     * feedback on every server generation, so perform the
     * registration within ScreenInit and not PreInit.
     */
    AddGeneralSocket(pDispInfo->drm_fd);
    RegisterBlockAndWakeupHandlers((BlockHandlerProcPtr)NoopDDA,
                                   EXYNOSDispInfoWakeupHanlder, pDispInfo);

}

void
exynosDisplayDeinit (ScrnInfoPtr pScrn)
{
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    EXYNOSDispInfoPtr pDispInfo = (EXYNOSDispInfoPtr) EXYNOSDISPINFOPTR(pScrn);
    xf86CrtcPtr pCrtc = NULL;
    xf86OutputPtr pOutput = NULL;

    _exynosDisplayDeInitDrmEventCtx (pDispInfo->pEventCtx);

    EXYNOSCrtcPrivPtr crtc_ref=NULL, crtc_next=NULL;
    xorg_list_for_each_entry_safe (crtc_ref, crtc_next, &pDispInfo->crtcs, link)
    {
        pCrtc = crtc_ref->pCrtc;
        xf86CrtcDestroy (pCrtc);
    }

    EXYNOSOutputPrivPtr output_ref, output_next;
    xorg_list_for_each_entry_safe (output_ref, output_next, &pDispInfo->outputs, link)
    {
        pOutput = output_ref->pOutput;
        xf86OutputDestroy (pOutput);
    }

    EXYNOSPlanePrivPtr plane_ref, plane_next;
    xorg_list_for_each_entry_safe (plane_ref, plane_next, &pDispInfo->planes, link)
    {
        exynosPlaneDeinit (plane_ref);
    }

    if (pDispInfo->mode_res)
        drmModeFreeResources (pDispInfo->mode_res);

    if (pDispInfo->plane_res)
        drmModeFreePlaneResources (pDispInfo->plane_res);
    
    if (pDispInfo->using_planes)
        exynosUtilStorageFinit(pDispInfo->using_planes);
    
    ctrl_free (pDispInfo);
    pExynos->pDispInfo = NULL;
}
EXYNOS_DisplaySetMode  exynosDisplayGetDispSetMode  (ScrnInfoPtr pScrn){
    EXYNOS_DisplaySetMode set_mode;
    EXYNOSPtr pExPtr = EXYNOSPTR (pScrn);
    EXYNOSDispInfoPtr pSecMode = pExPtr->pDispInfo;

    set_mode = pSecMode->set_mode;

    return set_mode;
}

/*
  * Return the crtc covering 'box'. If two crtcs cover a portion of
  * 'box', then prefer 'desired'. If 'desired' is NULL, then prefer the crtc
  * with greater coverage
  */
/*Not Used 18.09*/
xf86CrtcPtr
exynosModeCoveringCrtc (ScrnInfoPtr pScrn, BoxPtr pBox, xf86CrtcPtr pDesiredCrtc, BoxPtr pBoxCrtc)
{

    xf86CrtcConfigPtr pCrtcConfig = XF86_CRTC_CONFIG_PTR (pScrn);
    xf86CrtcPtr pCrtc, pBestCrtc;
    int coverage, best_coverage;
    int c;
    BoxRec crtc_box, cover_box;

    XDBG_RETURN_VAL_IF_FAIL (pBox != NULL, NULL);

    pBestCrtc = NULL;
    best_coverage = 0;

    if (pBoxCrtc)
    {
        pBoxCrtc->x1 = 0;
        pBoxCrtc->y1 = 0;
        pBoxCrtc->x2 = 0;
        pBoxCrtc->y2 = 0;
    }

    for (c = 0; c < pCrtcConfig->num_crtc; c++)
    {
        pCrtc = pCrtcConfig->crtc[c];

        /* If the CRTC is off, treat it as not covering */

        /*TODO implement
        if(!EXYNOSCrtcOn(pCrtc))
            continue;
        */

        crtc_box.x1 = pCrtc->x;
        crtc_box.x2 = pCrtc->x + xf86ModeWidth (&pCrtc->mode, pCrtc->rotation);
        crtc_box.y1 = pCrtc->y;
        crtc_box.y2 = pCrtc->y + xf86ModeHeight (&pCrtc->mode, pCrtc->rotation);

        exynosUtilBoxIntersect(&cover_box, &crtc_box, pBox);
        coverage = exynosUtilBoxArea(&cover_box);

        if (coverage && pCrtc == pDesiredCrtc)
        {
            if (pBoxCrtc)
                *pBoxCrtc = crtc_box;
            return pCrtc;
        }

        if (coverage > best_coverage)
        {
            if (pBoxCrtc)
                *pBoxCrtc = crtc_box;
            pBestCrtc = pCrtc;
            best_coverage = coverage;
        }
    }

    return pBestCrtc;
}

int exynosCrtcGetConnectType (xf86CrtcPtr pCrtc)
{
    xf86CrtcConfigPtr pCrtcConfig;
    int i;

    XDBG_RETURN_VAL_IF_FAIL (pCrtc != NULL, DRM_MODE_CONNECTOR_Unknown);

    pCrtcConfig = XF86_CRTC_CONFIG_PTR (pCrtc->scrn);
    XDBG_RETURN_VAL_IF_FAIL (pCrtcConfig != NULL, DRM_MODE_CONNECTOR_Unknown);

    for (i = 0; i < pCrtcConfig->num_output; i++)
    {
        xf86OutputPtr pOutput = pCrtcConfig->output[i];
        EXYNOSOutputPrivPtr pOutputPriv = pOutput->driver_private;

        if (pOutput->crtc == pCrtc)
            return pOutputPriv->mode_output->connector_type;
    }

    return DRM_MODE_CONNECTOR_Unknown;

}

