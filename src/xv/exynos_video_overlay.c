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
 * software for any purpose.  It is provided "as is" without any express
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

#include <pixman-1/pixman.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvproto.h>
#include <xorg/fourcc.h>

#include <xorg/xf86xv.h>

#include "exynos_driver.h"
#include "exynos_util.h"
#include "exynos_mem_pool.h"
#include "exynos_video_fourcc.h"
#include "exynos_video_buffer.h"
#include "exynos_accel_int.h"
#include "exynos_display_int.h"
#include "_trace.h"
#include "fimg2d.h"

#define EXYNOS_MAX_PORT        1
#define LAYER_BUF_CNT       3

static XF86VideoEncodingRec dummy_encoding[] ={
    { 0, "XV_IMAGE", -1, -1,
        { 1, 1}},
    { 1, "XV_IMAGE", 4224, 4224,
        { 1, 1}},
};

static XF86VideoFormatRec formats[] ={
    { 16, TrueColor},
    { 24, TrueColor},
    { 32, TrueColor},
};

static XF86AttributeRec attributes[] ={
    { 0, 0, 1, "_USER_WM_PORT_ATTRIBUTE_OVERLAY"},
};

typedef enum
{
    PAA_MIN,
    PAA_OVERLAY,
    PAA_MAX
} EXYNOSPortAttrAtom;

static struct
{
    EXYNOSPortAttrAtom paa;
    const char *name;
    Atom atom;
} atoms[] ={
    { PAA_OVERLAY, "_USER_WM_PORT_ATTRIBUTE_OVERLAY", None},
};

typedef struct _GetData
{
    int width;
    int height;
    xRectangle src;
    xRectangle dst;
    DrawablePtr pDraw;
} GetData;

/* SEC port information structure */
typedef struct
{
    int index;

    /* attributes */
    Bool overlay;

    ScrnInfoPtr pScrn;
    GetData d;
    GetData old_d;


    PixmapPtr outbuf[LAYER_BUF_CNT];
    int plane_id;
    int crtc_id;
    Bool watch_vblank;
    int watch_pipe;
    PixmapPtr showing_outbuf;
    PixmapPtr waiting_outbuf;
    
    

    int stream_cnt;
    struct xorg_list link;
} OverlayPortPriv, *OverlayPortPrivPtr;

static RESTYPE event_drawable_type;

typedef struct _SECDisplayVideoResource
{
    XID id;
    RESTYPE type;

    OverlayPortPrivPtr pPort;
    ScrnInfoPtr pScrn;
} EXYNOSVideoOverlayResource;

static void EXYNOSVideoOverlayVideoStop (ScrnInfoPtr pScrn, pointer data, Bool exit);

#define NUM_FORMATS       (sizeof(formats) / sizeof(formats[0]))
#define NUM_ATTRIBUTES    (sizeof(attributes) / sizeof(attributes[0]))
#define NUM_ATOMS         (sizeof(atoms) / sizeof(atoms[0]))

static PixmapPtr
_getPixmap (DrawablePtr pDraw)
{
    if (pDraw->type == DRAWABLE_WINDOW)
        return pDraw->pScreen->GetWindowPixmap ((WindowPtr) pDraw);
    else
        return (PixmapPtr) pDraw;
}

static Atom
_portAtom (EXYNOSPortAttrAtom paa)
{
    int i;

    XDBG_RETURN_VAL_IF_FAIL (paa > PAA_MIN && paa < PAA_MAX, None);

    for (i = 0; i < NUM_ATOMS; i++)
    {
        if (paa == atoms[i].paa)
        {
            if (atoms[i].atom == None)
                atoms[i].atom = MakeAtom (atoms[i].name,
                                          strlen (atoms[i].name), TRUE);

            return atoms[i].atom;
        }
    }

    XDBG_ERROR (MOVA, "Error: Unknown Port Attribute Name!\n");

    return None;
}

static void
_copyBuffer (DrawablePtr pDraw, PixmapPtr pPixmap_dst)
{
    XDBG_GOTO_IF_FAIL(pDraw != NULL, done_copy_buf);
    XDBG_GOTO_IF_FAIL(pPixmap_dst != NULL, done_copy_buf);
    
    PixmapPtr pPixmap_src = (PixmapPtr) _getPixmap (pDraw);
    
    EXYNOSExaPixPrivPtr privPixmap_src = exaGetPixmapDriverPrivate (pPixmap_src);
    EXYNOSExaPixPrivPtr privPixmap_dst = exaGetPixmapDriverPrivate (pPixmap_dst);
    Bool need_finish_src = FALSE, need_finish_dst = FALSE;
    
    tbm_bo_handle bo_handle_src, bo_handle_dst;
    G2dImage *src_image = NULL, *dst_image = NULL;
    xRectangle crop_dst = {0,};
    
    if (!privPixmap_src->tbo)
    {
        need_finish_src = TRUE;
        EXYNOSExaPrepareAccess (pPixmap_src, EXA_PREPARE_DEST);
        XDBG_GOTO_IF_FAIL (privPixmap_src->tbo != NULL, done_copy_buf);
    }
    
    if (!privPixmap_dst->tbo)
    {
        need_finish_dst = TRUE;
        EXYNOSExaPrepareAccess (pPixmap_dst, EXA_PREPARE_DEST);
        XDBG_GOTO_IF_FAIL (privPixmap_dst->tbo != NULL, done_copy_buf);
    }
    

    bo_handle_src = tbm_bo_get_handle (privPixmap_src->tbo, TBM_DEVICE_DEFAULT);
    XDBG_GOTO_IF_FAIL (bo_handle_src.u32 > 0, done_copy_buf);

    bo_handle_dst = tbm_bo_get_handle (privPixmap_dst->tbo, TBM_DEVICE_DEFAULT);
    XDBG_GOTO_IF_FAIL (bo_handle_dst.u32 > 0, done_copy_buf);

    src_image = g2d_image_create_bo2 (G2D_COLOR_FMT_ARGB8888 | G2D_ORDER_AXRGB,
                                      (unsigned int) pDraw->width,
                                      (unsigned int) pDraw->height,
                                      (unsigned int) bo_handle_src.u32,
                                      (unsigned int) 0,
                                      (unsigned int) pDraw->width * 4);
    XDBG_GOTO_IF_FAIL (src_image != NULL, done_copy_buf);

    dst_image = g2d_image_create_bo2 (G2D_COLOR_FMT_ARGB8888 | G2D_ORDER_AXRGB,
                                      (unsigned int) pPixmap_dst->drawable.width,
                                      (unsigned int) pPixmap_dst->drawable.height,
                                      (unsigned int) bo_handle_dst.u32,
                                      (unsigned int) 0,
                                      (unsigned int) pPixmap_dst->drawable.width*4);
    XDBG_GOTO_IF_FAIL (dst_image != NULL, done_copy_buf);

    tbm_bo_map (privPixmap_src->tbo, TBM_DEVICE_MM, TBM_OPTION_READ);
    tbm_bo_map (privPixmap_dst->tbo, TBM_DEVICE_MM, TBM_OPTION_READ);
    
    crop_dst = exynosVideoBufferGetCrop (pPixmap_dst);
    
    util_g2d_blend_with_scale (G2D_OP_SRC, src_image, dst_image,
                               (int) crop_dst.x, (int) crop_dst.y,
                               (unsigned int) crop_dst.width,
                               (unsigned int) crop_dst.height,
                               (int) crop_dst.x, (int) crop_dst.y,
                               (unsigned int) crop_dst.width,
                               (unsigned int) crop_dst.height,
                               FALSE);
    g2d_exec ();

    tbm_bo_unmap (privPixmap_src->tbo);
    tbm_bo_unmap (privPixmap_dst->tbo);

done_copy_buf:
    if (src_image)
        g2d_image_free (src_image);
    if (dst_image)
        g2d_image_free (dst_image);
    if (need_finish_src)
        EXYNOSExaFinishAccess (pPixmap_src, EXA_PREPARE_DEST);
    if (need_finish_dst)
        EXYNOSExaFinishAccess (pPixmap_dst, EXA_PREPARE_DEST);
}


static PixmapPtr
_exynosVideoOverlayGetOutBuffer (OverlayPortPrivPtr pPort)
{
    int i;

    for (i = 0; i < LAYER_BUF_CNT; i++)
    {
        if (!pPort->outbuf[i])
        {
            pPort->outbuf[i] = exynosVideoBufferCreateOutbufFB (pPort->pScrn, FOURCC_RGB32,
                                                                pPort->d.pDraw->width,
                                                                pPort->d.pDraw->height);
            XDBG_GOTO_IF_FAIL (pPort->outbuf[i] != NULL, fail_get_buf);
            break;
        }
        else if (!exynosVideoBufferIsShowing(pPort->outbuf[i]))
            break;
    }

    if (i == LAYER_BUF_CNT)
    {
        XDBG_ERROR (MOVA, "now all outbufs in use!\n");
        return NULL;
    }
/*
    XDBG_DEBUG (MOVA, "outbuf: stamp(%ld) index(%d) h(%d,%d,%d)\n",
                pPort->outbuf[i]->stamp, i,
                pPort->outbuf[i]->handles[0],
                pPort->outbuf[i]->handles[1],
                pPort->outbuf[i]->handles[2]);
*/
    XDBG_GOTO_IF_FAIL(
            exynosVideoBufferSetCrop(pPort->outbuf[i], pPort->d.src), fail_get_buf);

    _copyBuffer (pPort->d.pDraw, pPort->outbuf[i]);

    return pPort->outbuf[i];

fail_get_buf:
   // _secDisplayVideoCloseBuffers (pPort);
    return NULL;
}

static Bool
_exynosVideoOverlayAddDrawableEvent (OverlayPortPrivPtr pPort)
{
    EXYNOSVideoOverlayResource *resource;
    void *ptr = NULL;
    int ret;

    ret = dixLookupResourceByType (&ptr, pPort->d.pDraw->id,
                                   event_drawable_type, NULL, DixWriteAccess);
    if (ret == Success)
    {
        return TRUE;
    }

    resource = malloc (sizeof (EXYNOSVideoOverlayResource));
    if (resource == NULL)
        return FALSE;

    if (!AddResource (pPort->d.pDraw->id, event_drawable_type, resource))
    {
        free (resource);
        return FALSE;
    }

    XDBG_TRACE (MOVA, "id(0x%08lx). \n", pPort->d.pDraw->id);

    resource->id = pPort->d.pDraw->id;
    resource->type = event_drawable_type;
    resource->pPort = pPort;
    resource->pScrn = pPort->pScrn;

    return TRUE;
}

static int
_secDisplayVideoRegisterEventDrawableGone (void *data, XID id)
{
    EXYNOSVideoOverlayResource *resource = (EXYNOSVideoOverlayResource*) data;

    XDBG_TRACE (MOVA, "id(0x%08lx). \n", id);

    if (!resource)
        return Success;

    if (!resource->pPort || !resource->pScrn)
        return Success;

    EXYNOSVideoOverlayVideoStop (resource->pScrn, (pointer) resource->pPort, 1);

    free (resource);

    return Success;
}

static Bool
_exynosVideoOverlayRegisterEventResourceTypes (void)
{
    event_drawable_type = CreateNewResourceType (_secDisplayVideoRegisterEventDrawableGone, "Sec Video Drawable");

    if (!event_drawable_type)
        return FALSE;

    return TRUE;
}

static int
EXYNOSVideoOverlayGetPortAttribute (ScrnInfoPtr pScrn,
                                    Atom attribute,
                                    INT32 *value,
                                    pointer data)
{
    OverlayPortPrivPtr pPort = (OverlayPortPrivPtr) data;

    if (attribute == _portAtom (PAA_OVERLAY))
    {
        *value = pPort->overlay;
        return Success;
    }

    return Success;
}

static int
EXYNOSVideoOverlaySetPortAttribute (ScrnInfoPtr pScrn,
                                    Atom attribute,
                                    INT32 value,
                                    pointer data)
{
    OverlayPortPrivPtr pPort = (OverlayPortPrivPtr) data;

    if (attribute == _portAtom (PAA_OVERLAY))
    {
        pPort->overlay = value;
        XDBG_DEBUG (MOVA, "overlay(%d) \n", value);
        return Success;
    }

    return Success;
}

static void
_exynosVideoOverlayShowOutbuf (OverlayPortPrivPtr pPort, PixmapPtr pPixmap)
{
    XDBG_RETURN_IF_FAIL(pPort != NULL);
    XDBG_RETURN_IF_FAIL(pPixmap != NULL);
    unsigned int fb_id = 0;
 //   exynosUtilPlaneDump (pPort->pScrn);
    if (pPort->plane_id == 0)
    {
        pPort->plane_id = exynosPlaneGetAvailable(pPort->pScrn);

        if (pPort->plane_id == 0)
        {
            XDBG_ERROR(MOVA, "port(%d) no avaliable plane\n", pPort->index);
            return;
        }
    }
    if (pPort->crtc_id == 0)
    {
        /** @todo Get proper crtc_id related with window */
        pPort->crtc_id = 4;
    }

    fb_id = exynosVideoBufferGetFBID (pPixmap);
    XDBG_RETURN_IF_FAIL(fb_id > 0);
    
    XDBG_RETURN_IF_FAIL(
            exynosPlaneDraw( pPort->pScrn, pPort->d.src, pPort->d.dst, fb_id, 
                             pPort->plane_id, pPort->crtc_id, LAYER_UPPER));

    XDBG_TRACE(MOVA,
               "port(%d) plane(%d) show: crtc(%d) fb(%d) src(%d,%d %dx%d) dst(%d,%d %dx%d)\n",
               pPort->index, pPort->plane_id, pPort->crtc_id, fb_id, 
               pPort->d.src.x, pPort->d.src.y, pPort->d.src.width, pPort->d.src.height,
               pPort->d.dst.x, pPort->d.dst.y, pPort->d.dst.width, pPort->d.dst.height);

 //   exynosUtilPlaneDump (pPort->pScrn);
}

static int
EXYNOSVideoOverlayGetStill (ScrnInfoPtr pScrn,
                            short vid_x, short vid_y, short drw_x, short drw_y,
                            short vid_w, short vid_h, short drw_w, short drw_h,
                            RegionPtr clipBoxes, pointer data,
                            DrawablePtr pDraw)
{
    OverlayPortPrivPtr pPort = (OverlayPortPrivPtr) data;
    PixmapPtr pPixmap = NULL;
//    XDBG_RETURN_VAL_IF_FAIL (pDraw->type == DRAWABLE_PIXMAP, BadRequest);

    pPort->pScrn = pScrn;

    pPort->d.width = pDraw->width;
    pPort->d.height = pDraw->height;
    pPort->d.src.x = drw_x;
    pPort->d.src.y = drw_y;
    pPort->d.src.width = drw_w;
    pPort->d.src.height = drw_h;
    pPort->d.dst.x = vid_x;
    pPort->d.dst.y = vid_y;
    pPort->d.dst.width = drw_w;
    pPort->d.dst.height = drw_h;
    pPort->d.pDraw = pDraw;
    /*
        if (pPort->old_d.width != pPort->d.width ||
            pPort->old_d.height != pPort->d.height)
        {
            _secDisplayVideoHideLayer (pPort);
            _secDisplayVideoCloseBuffers (pPort);
        }
     */
    if (!_exynosVideoOverlayAddDrawableEvent (pPort))
        return BadRequest;
    /*
        if (pPort->stream_cnt == 0)
        {
            pPort->stream_cnt++;

            XLOG_SECURE (MOVA, "pPort(%d) sz(%dx%d) src(%d,%d %dx%d) dst(%d,%d %dx%d)\n",
                         pPort->index, pDraw->width, pDraw->height,
                         drw_x, drw_y, drw_w, drw_h, vid_x, vid_y, vid_w, vid_h);
        }
     */
    
    XDBG_TRACE (MOVA, "pPort(%d) sz(%dx%d) src(%d,%d %dx%d) dst(%d,%d %dx%d)\n",
                         pPort->index, pDraw->width, pDraw->height,
                         drw_x, drw_y, drw_w, drw_h, vid_x, vid_y, vid_w, vid_h);
     pPixmap = _exynosVideoOverlayGetOutBuffer (pPort);
    XDBG_RETURN_VAL_IF_FAIL (pPixmap != NULL, BadRequest);

    if (pPort->overlay)
    {
        /*
        output = LAYER_OUTPUT_EXT;
        lpos = LAYER_UPPER;
         */
    }
    else
    {
        XDBG_ERROR (MOVA, "pPort(%d) implemented for only overlay\n", pPort->index);
        return BadRequest;
    }

    _exynosVideoOverlayShowOutbuf(pPort, pPixmap);
    
    pPort->old_d = pPort->d;

    return Success;
}

static void
EXYNOSVideoOverlayVideoStop (ScrnInfoPtr pScrn, pointer data, Bool exit)
{
    OverlayPortPrivPtr pPort = (OverlayPortPrivPtr) data;
    int i = 0;

    XDBG_TRACE (MOVA, "exit (%d) \n", exit);

    if (!exit)
        return;

   XDBG_DEBUG(MOVA, "pPort(%d) exit (%d) \n", pPort->index, exit);

   
    if (!exynosPlaneHide (pScrn, pPort->plane_id))
    {
        XDBG_ERRNO(MOVA, "port(%d) hide plane failed. plane(%d)\n",
                   pPort->index, pPort->plane_id);
    }
   
   //exynosUtilPlaneDump (pScrn);

   for (i = 0 ; i < LAYER_BUF_CNT ; i++)
   {
       if (pPort->outbuf[i])
       {
            exynosVideoBufferFree(pPort->outbuf[i]);
            pPort->outbuf[i] = NULL;
       }
   }

    memset (&pPort->old_d, 0, sizeof(GetData));
    memset (&pPort->d, 0, sizeof(GetData));
    pPort->plane_id = 0;
    pPort->crtc_id = 0;
    pPort->overlay = FALSE;
    pPort->showing_outbuf = NULL;
    pPort->waiting_outbuf = NULL;
    pPort->watch_pipe = 0;
}

static void
EXYNOSVideoOverlayQueryBestSize (ScrnInfoPtr pScrn,
                                 Bool motion,
                                 short vid_w, short vid_h,
                                 short dst_w, short dst_h,
                                 unsigned int *p_w, unsigned int *p_h,
                                 pointer data)
{
    if (p_w)
        *p_w = (unsigned int) dst_w & ~1;
    if (p_h)
        *p_h = (unsigned int) dst_h;
}

XF86VideoAdaptorPtr
exynosVideoOverlaySetup (ScreenPtr pScreen)
{
    XF86VideoAdaptorPtr pAdaptor;
    OverlayPortPrivPtr pPort;
    int i;

    pAdaptor = calloc (1, sizeof (XF86VideoAdaptorRec) +
                       (sizeof (DevUnion) + sizeof (OverlayPortPriv)) * EXYNOS_MAX_PORT);
    if (!pAdaptor)
        return NULL;

    dummy_encoding[0].width = pScreen->width;
    dummy_encoding[0].height = pScreen->height;

    pAdaptor->type = XvPixmapMask | XvOutputMask | XvStillMask;
    pAdaptor->flags = VIDEO_OVERLAID_IMAGES;
    pAdaptor->name = "EXYNOS Overlay Video";
    pAdaptor->nEncodings = sizeof (dummy_encoding) / sizeof (XF86VideoEncodingRec);
    pAdaptor->pEncodings = dummy_encoding;
    pAdaptor->nFormats = NUM_FORMATS;
    pAdaptor->pFormats = formats;
    pAdaptor->nPorts = EXYNOS_MAX_PORT;
    pAdaptor->pPortPrivates = (DevUnion*) (&pAdaptor[1]);

    pPort = (OverlayPortPrivPtr) (&pAdaptor->pPortPrivates[EXYNOS_MAX_PORT]);

    for (i = 0; i < EXYNOS_MAX_PORT; i++)
    {
        pAdaptor->pPortPrivates[i].ptr = &pPort[i];
        pPort[i].index = i;
    }

    pAdaptor->nAttributes = NUM_ATTRIBUTES;
    pAdaptor->pAttributes = attributes;

    pAdaptor->GetPortAttribute = EXYNOSVideoOverlayGetPortAttribute;
    pAdaptor->SetPortAttribute = EXYNOSVideoOverlaySetPortAttribute;
    pAdaptor->GetStill = EXYNOSVideoOverlayGetStill;
    pAdaptor->StopVideo = EXYNOSVideoOverlayVideoStop;
    pAdaptor->QueryBestSize = EXYNOSVideoOverlayQueryBestSize;

    if (!_exynosVideoOverlayRegisterEventResourceTypes ())
    {
        ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR, "Failed to register EventResourceTypes. \n");
        return FALSE;
    }

    return pAdaptor;
}
