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
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <pixman.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvproto.h>
#include <fourcc.h>

#include <fb.h>
#include <fbdevhw.h>
#include <damage.h>

#include <xf86xv.h>

#include "exynos_driver.h"

#include "exynos_accel.h"
#include "exynos_display.h"
#include "exynos_video.h"
#include "exynos_util.h"
#include "exynos_video_buffer.h"
#include "exynos_video_image.h"
#include "exynos_video_fourcc.h"
#include "exynos_video_converter.h"

#include "xv_types.h"
#include "_trace.h"

#define OUTPUT_LCD   (1 << 0)
#define OUTPUT_EXT   (1 << 1)
#define OUTPUT_FULL  (1 << 8)

#define IMAGE_IMAGES_NUM    (sizeof(image_images) / sizeof(image_images[0]))
#define IMAGE_FORMATS_NUM   (sizeof(image_formats) / sizeof(image_formats[0]))
#define IMAGE_ATTRS_NUM     (sizeof(image_attrs) / sizeof(image_attrs[0]))
#define IMAGE_ATOMS_NUM     (sizeof(image_atoms) / sizeof(image_atoms[0]))

#define ImageClientKey  (&image_client_key)
#define GetImageClientInfo(pDraw) ((IMAGEPortClientInfo*)dixLookupPrivate(&(pDraw)->devPrivates, ImageClientKey))

static Bool _exynosVideoSetOutputExternalProperty (DrawablePtr pDraw, Bool tvout);
static int _exynosVideoGetTvoutMode (IMAGEPortInfoPtr pPort);

static XF86VideoEncodingRec image_encoding[] =
{
    { 0, "XV_IMAGE", -1, -1, { 1, 1 } },
    { 1, "XV_IMAGE", 4224, 4224, { 1, 1 } },
};
/* There are two types of format.
 * The format including with 'S' means zero-copy format.
 * In this zero-copy case, buffer doesn't include raw image data. But it includes
 * 'XV_DATA' structure data(See xv_types.h). To share buffers between multi
 * process, Tizen uses the 'KEY' of buffer. In other words, we say "NAME"
 * instead of "KEY".
 * XV_DATA structure includes the names of buffers.
 * In case of zero-copy, to avoid tearing issue we need to return buffers to
 * client so that client can re-use buffers.(See _exynosVideoImageSendReturnBufferMessage)
 */

/* image formats */
static XF86ImageRec image_images[] =
{
    XVIMAGE_YUY2,
    XVIMAGE_SUYV,
    XVIMAGE_UYVY,
    XVIMAGE_SYVY,
    XVIMAGE_ITLV,
    XVIMAGE_YV12,
    XVIMAGE_I420,
    XVIMAGE_S420,
    XVIMAGE_ST12,
    XVIMAGE_NV12,
    XVIMAGE_SN12,
    XVIMAGE_NV21,
    XVIMAGE_SN21,
    XVIMAGE_RGB32,
    XVIMAGE_SR32,
    XVIMAGE_RGB565,
    XVIMAGE_SR16,
};

/* drawable formats */
static XF86VideoFormatRec image_formats[] =
{
    { 16, TrueColor },
    { 24, TrueColor },
    { 32, TrueColor },
};

/** attributes
 * @todo need to replace "_USER_WM_PORT_ATTRIBUTE_" to "XV_"
 * @todo need to remove "_USER_WM_PORT_ATTRIBUTE_STREAM_OFF"
 */
static XF86AttributeRec image_attrs[] =
{
    { 0, 0, 270, "_USER_WM_PORT_ATTRIBUTE_ROTATION" },
    { 0, 0, 1, "_USER_WM_PORT_ATTRIBUTE_HFLIP" },
    { 0, 0, 1, "_USER_WM_PORT_ATTRIBUTE_VFLIP" },
    { 0, 0, OUTPUT_MODE_EXT_ONLY, "_USER_WM_PORT_ATTRIBUTE_OUTPUT" },
};

typedef enum
{
    PAA_MIN,
    PAA_ROTATION,
    PAA_HFLIP,
    PAA_VFLIP,
    PAA_OUTPUT,
    PAA_MAX
} IMAGEPortAttrAtom;

static struct
{
    IMAGEPortAttrAtom  paa;
    const char        *name;
    Atom               atom;
} image_atoms[] =
{
    { PAA_ROTATION, "_USER_WM_PORT_ATTRIBUTE_ROTATION", None },
    { PAA_HFLIP, "_USER_WM_PORT_ATTRIBUTE_HFLIP", None },
    { PAA_VFLIP, "_USER_WM_PORT_ATTRIBUTE_VFLIP", None },
    { PAA_OUTPUT, "_USER_WM_PORT_ATTRIBUTE_OUTPUT", None },
};

enum
{
    ON_NONE,
    ON_FB,
    ON_WINDOW,
    ON_PIXMAP
};

enum
{
    CHANGED_NONE,
    CHANGED_IN,
    CHANGED_OUT,
    CHANGED_ALL
};

static char *drawing_type[4] = {"NONE", "FB", "WIN", "PIX"};



/* In case drawing type is "pixmap", when pixmap is destroyed,
 * StopVideo isn't used to be called originally. So, to stop video when
 * pixmap is destoryed, we register a callback to pixmap.
 */
typedef struct _IMAGEPortDrawableInfo
{
    ScrnInfoPtr pScrn;
    XID id;
    IMAGEPortInfoPtr pPort;
} IMAGEPortDrawableInfo;

static RESTYPE image_rtype;

/* client information which calls PutImage.
 * we use client information to return buffer's name to client by clientmessage
 * "XV_RETURN_BUFFER". In case of zero-copy, we need to return buffers to client,
 * then client can re-use buffers.
 */
typedef struct _IMAGEPortClientInfo
{
    ClientPtr client;
} IMAGEPortClientInfo;

static int (*ddPutImage) (ClientPtr, DrawablePtr, struct _XvPortRec *, GCPtr,
                          INT16, INT16, CARD16, CARD16,
                          INT16, INT16, CARD16, CARD16,
                          XvImagePtr, unsigned char *, Bool, CARD16, CARD16);

static void EXYNOSVideoImageStop (ScrnInfoPtr pScrn, pointer data, Bool exit);
static void _exynosVideoImageHideOutbuf (IMAGEPortInfoPtr pPort);


static int registered_handler;

static DevPrivateKeyRec image_client_key;

static struct xorg_list watch_vlank_ports;

static Bool
_exynosVideoImageCompareIppId (pointer structure, pointer element)
{

    EXYNOSIppPtr pIpp_data = (EXYNOSIppPtr) structure;
    unsigned int prop_id = * ((unsigned int *) element);
    return (pIpp_data->prop_id == prop_id);
}

static Bool
_exynosVideoImageCompareBufIndex (pointer structure, pointer element)
{
    EXYNOSIppBuf *ippbufinfo = (EXYNOSIppBuf*) structure;
    unsigned int index = * ((unsigned int *) element);
    return (ippbufinfo->index == index);
}
/*
static Bool
_exynosVideoImageComparePixmap (pointer pPixInList, pointer pPixCurrent)
{
    PixmapPtr pPix1 = (PixmapPtr) pPixInList;
    PixmapPtr pPix2 = (PixmapPtr) pPixCurrent;
    return (pPix1 == pPix2);
}
*/
static Bool
_exynosVideoImageCompareStamp (pointer structure, pointer element)
{

    PixmapPtr pPixmap = (PixmapPtr) structure;
    unsigned int serialNumber = * ((unsigned int *) element);

    return (pPixmap->drawable.serialNumber == serialNumber);
}
/*
static Bool
_exynosVideoImageCompareIppBufStamp (pointer structure, pointer element)
{

    EXYNOSIppBufPtr pIpp_buf = (EXYNOSIppBufPtr) structure;
    CARD32 stamp = * ((CARD32 *) element);

    return (pIpp_buf->stamp < stamp);
}
*/
#if 0
static PixmapPtr
_exynosVideoImageGetPixmap (DrawablePtr pDraw)
{
    PixmapPtr pPixmap;

    if (pDraw->type == DRAWABLE_WINDOW)
        pPixmap = pDraw->pScreen->GetWindowPixmap ((WindowPtr) pDraw);
    else
        pPixmap = (PixmapPtr) pDraw;

    if (exynosVideoPixmapIsValid (pPixmap))
        return exynosVideoPixmapRef (pPixmap);
    else
    {
        pPixmap = exynosVideoPixmapFromPixmap (pPixmap);
        return exynosVideoPixmapRef (pPixmap);
    }
}
#endif

XF86ImagePtr
exynosVideoImageGetImageInfo (int id)
{
    XDBG_RETURN_VAL_IF_FAIL (id > 0, NULL);
    int i = 0;

    for (i = 0; i < IMAGE_IMAGES_NUM; i++)
        if (image_images[i].id == id)
            return &image_images[i];
    return NULL;
}

static Bool
_exynosVideoImageIsZeroCopy (int id)
{
    XDBG_RETURN_VAL_IF_FAIL (id > 0, FALSE);

    switch (id)
    {
    case FOURCC_SR16:
    case FOURCC_SR32:
    case FOURCC_S420:
    case FOURCC_SUYV:
    case FOURCC_SN12:
    case FOURCC_SN21:
    case FOURCC_SYVY:
    case FOURCC_ITLV:
    case FOURCC_ST12:
        return TRUE;
    case FOURCC_RGB565:
    case FOURCC_RGB32:
    case FOURCC_YV12:
    case FOURCC_I420:
    case FOURCC_NV12:
    case FOURCC_NV21:
    case FOURCC_YUY2:
    case FOURCC_UYVY:
        return FALSE;
        break;
    default:
        XDBG_NEVER_GET_HERE(MIA);
        return FALSE;
    }
}

static Atom
_exynosVideoImageGetPortAtom (IMAGEPortAttrAtom paa)
{
    int i;

    XDBG_RETURN_VAL_IF_FAIL(paa > PAA_MIN && paa < PAA_MAX, None);

    for (i = 0; i < IMAGE_ATOMS_NUM; i++)
    {
        if (paa == image_atoms[i].paa)
        {
            if (image_atoms[i].atom == None)
                image_atoms[i].atom = MakeAtom (image_atoms[i].name,
                                                strlen (image_atoms[i].name),
                                                TRUE);
            return image_atoms[i].atom;
        }
    }

    ErrorF ("Error: Unknown Port Attribute Name!\n");

    return None;
}
static PixmapPtr
_exynosVideoImageGetDrawPixmap (DrawablePtr pDraw)
{
    XDBG_RETURN_VAL_IF_FAIL(pDraw != NULL, NULL);
    if (pDraw->type == DRAWABLE_WINDOW)
        return pDraw->pScreen->GetWindowPixmap ((WindowPtr) pDraw);
    else
        return (PixmapPtr) pDraw;
}
static int
_exynosVideoImagedrawingOn (IMAGEPortInfoPtr pPort)
{
    if (pPort->d.pDraw != pPort->old_d.pDraw)
        pPort->drawing = ON_NONE;

    if (pPort->drawing != ON_NONE)
        return pPort->drawing;

    if (pPort->d.pDraw->type == DRAWABLE_PIXMAP)
    {
        /* rendering video on pixmap */
        return ON_PIXMAP;
    }
    else if (pPort->d.pDraw->type == DRAWABLE_WINDOW)
    {
        PropertyPtr prop = exynosUtilGetWindowProperty ((WindowPtr) pPort->d.pDraw,
                           "XV_ON_DRAWABLE");
        if (prop && *(int*) prop->data > 0)
        {
            /* rendering video on window */
            return ON_WINDOW;
        }
    }

    /* rendering video on framebuffer */
    return ON_FB;
}
#if 0
EXYNOSVideoBufInfo*
exynosVideoFindBufferIndex(EXYNOSIppPtr pIpp_data, EXYNOSCvtType type, int index)
{
    struct xorg_list *bufs;
    EXYNOSIppBuf *cur = NULL;

    bufs = (type == CVT_TYPE_SRC) ?
           &pIpp_data->list_of_inbuf : &pIpp_data->list_of_outbuf;

    xorg_list_for_each_entry (cur, bufs, entry)
    {
        if (index == cur->index)
            return cur;
    }

    XDBG_ERROR(MVBUF, "cvt(%p), type(%d), index(%d) not found.\n", pIpp_data,
               type, index);

    return NULL;
}
#endif

static PixmapPtr
_exynosVideoImageGetInbuf (IMAGEPortInfoPtr pPort)
{
    PixmapPtr pPixmapIn = NULL;
    if (_exynosVideoImageIsZeroCopy (pPort->d.fourcc))
    {
        ClientPtr client = exynosVideoImageGetClient(pPort->d.pDraw);
        pPixmapIn = exynosVideoBufferCreateInbufZeroCopy (pPort->pScrn, pPort->d.fourcc,
                    pPort->in_width,
                    pPort->in_height, pPort->d.buf, client);
        XDBG_RETURN_VAL_IF_FAIL(pPixmapIn != NULL, NULL);
    }
    else
    {
        pPixmapIn = exynosVideoBufferCreateInbufRAW (pPort->pScrn, pPort->d.fourcc,
                    pPort->in_width, pPort->in_height);
        XDBG_RETURN_VAL_IF_FAIL(pPixmapIn != NULL, NULL);

        XDBG_RETURN_VAL_IF_FAIL (exynosVideoImageRawCopy (pPort, pPixmapIn), NULL);
    }
    exynosUtilStorageAdd(pPort->list_of_pixmap_in, pPixmapIn);
    if (exynosUtilStorageGetSize(pPort->list_of_pixmap_in) > 3)
        return NULL;
    return pPixmapIn;
}

static void
_exynosVideoImageClosePixs (pointer list_of_pixmap, Bool force)
{
    if ( list_of_pixmap == NULL)
        return;
    PixmapPtr pPixmap = NULL;
    
    int i = 0, size = 0;
    size = exynosUtilStorageGetSize (list_of_pixmap);
    XDBG_TRACE(MIA, "Array Size %d\n", size);
    XDBG_RETURN_IF_FAIL (size >= 0);
    for (i = 0; i < size; i++)
    {
        pPixmap = (PixmapPtr) exynosUtilStorageGetFirst(list_of_pixmap);
        if (force || (!exynosVideoBufferIsConverting(pPixmap) 
                  && !exynosVideoBufferIsShowing(pPixmap)))
        {
            exynosUtilStorageEraseData (list_of_pixmap, (pointer) pPixmap);
            exynosVideoBufferFree (pPixmap);
        }
    }
    XDBG_DEBUG(MIA, "All Video Pixmap in List %p close\n", list_of_pixmap);
}

static PixmapPtr
_exynosVideoImageGetOutbuf (IMAGEPortInfoPtr pPort)
{
    PixmapPtr pPixmap = NULL;
    PixmapPtr pDrawPix = NULL;
    int i = 0, size = 0;
    size = exynosUtilStorageGetSize (pPort->list_of_pixmap_out);
    XDBG_TRACE(MIA, "Array Size %d\n", size);
    if (size > 0)
    {
        
        if (pPort->drawing == ON_PIXMAP || pPort->drawing == ON_WINDOW)
        {
            pDrawPix = _exynosVideoImageGetDrawPixmap(pPort->d.pDraw);
            XDBG_RETURN_VAL_IF_FAIL(pDrawPix != NULL, NULL);
            pPixmap = exynosUtilStorageFindData(pPort->list_of_pixmap_out,
                                                &pDrawPix->drawable.serialNumber,
                                                _exynosVideoImageCompareStamp);
        }
        if (pPort->drawing == ON_FB)
            for (i = 0; i < size; i++)
            {
                pPixmap = (PixmapPtr) exynosUtilStorageGetData(pPort->list_of_pixmap_out, i);
                XDBG_DEBUG(MIA, "Pixmap(%p) - stamp(%lu) Converting(%d) Showing(%d)\n",
                           pPixmap, STAMP(pPixmap), exynosVideoBufferIsConverting (pPixmap),
                           exynosVideoBufferIsShowing (pPixmap));

                XDBG_RETURN_VAL_IF_FAIL(pPixmap != NULL, NULL);
                if ( !exynosVideoBufferIsConverting (pPixmap)
                        && !exynosVideoBufferIsShowing (pPixmap))
                    break;
                else
                    pPixmap = NULL;
            }
    }
    if (pPixmap == NULL)
    {
        if (size >= 3)
        { 
            XDBG_DEBUG(MIA,"Pixmap Out Buffer array >= 3\n");
            return NULL;
        }
        if (pPort->drawing == ON_PIXMAP || pPort->drawing == ON_WINDOW)
        {
            pPixmap = exynosVideoBufferModifyOutbufPixmap (pPort->pScrn, FOURCC_RGB32,
                      pPort->out_width,
                      pPort->out_height,
                      _exynosVideoImageGetDrawPixmap(pPort->d.pDraw));
        }
        else if (pPort->drawing == ON_FB)
        {
             /** @todo Need implement check missing events */
            pPixmap = exynosVideoBufferCreateOutbufFB (pPort->pScrn, FOURCC_RGB32,
                      pPort->out_width,
                      pPort->out_height);
        }
        XDBG_RETURN_VAL_IF_FAIL(pPixmap != NULL, NULL);
        exynosUtilStorageAdd (pPort->list_of_pixmap_out, pPixmap);
    }
    return pPixmap;
}

static void
_exynosVideoImageHideOutbuf (IMAGEPortInfoPtr pPort)
{
    
    if (!exynosPlaneHide (pPort->pScrn, pPort->plane_id))
    {
        XDBG_ERRNO(MIA, "port(%d) hide plane failed. plane(%d)\n",
                   pPort->index, pPort->plane_id);
    }

    XDBG_TRACE(MIA, "port(%d) plane(%d) hide\n", pPort->index, pPort->plane_id);

    /* reset vblank information */
    pPort->watch_pipe = 0;
    if (pPort->waiting_outbuf)
    {
        exynosVideoBufferShowing (pPort->waiting_outbuf, FALSE);
        pPort->waiting_outbuf = NULL;
    }
    if (pPort->showing_outbuf)
    {
        exynosVideoBufferShowing (pPort->showing_outbuf, FALSE);
        pPort->showing_outbuf = NULL;
    }
    if (pPort->watch_vblank)
    {
        pPort->watch_vblank = FALSE;
    }
    if (pPort->punched)
    {
        pPort->punched = FALSE;
    }

}

static void
_exynosVideoImageShowOutbuf (IMAGEPortInfoPtr pPort, PixmapPtr pPixmap)
{
    XDBG_RETURN_IF_FAIL(pPort != NULL);
    XDBG_RETURN_IF_FAIL(pPixmap != NULL);
    unsigned int fb_id = 0;
    xRectangle fixed_box, crtc_box;
//    exynosUtilPlaneDump (pPort->pScrn);
    if (pPort->plane_id == 0)
    {
        pPort->plane_id = exynosPlaneGetAvailable(pPort->pScrn);

        if (pPort->plane_id == 0)
        {
            XDBG_ERROR(MIA, "port(%d) no avaliable plane\n", pPort->index);
            return;
        }
    }
    if (pPort->crtc_id == 0)
    {
        /** @todo: Get proper crtc_id related with window */
        pPort->crtc_id = 4;
    }

    fb_id = exynosVideoBufferGetFBID (pPixmap);
    XDBG_RETURN_IF_FAIL(fb_id > 0);
    fixed_box = pPort->out_crop;
    crtc_box.x = pPort->d.dst.x;
    crtc_box.y = pPort->d.dst.y;
    crtc_box.width = pPort->out_crop.width;
    crtc_box.height = pPort->out_crop.height;    
    XDBG_RETURN_IF_FAIL(
            exynosPlaneDraw( pPort->pScrn, fixed_box, crtc_box, fb_id, 
                             pPort->plane_id, pPort->crtc_id, LAYER_LOWER1));
/*
    XDBG_TRACE(MIA,
               "port(%d) plane(%d) show: crtc(%d) fb(%d) src(%d,%d %dx%d) dst(%d,%d %dx%d)\n",
               pPort->index, pPort->plane_id, pPort->crtc_id, fb_id, 
               pPort->d.src.x, pPort->d.src.y, pPort->d.src.width, pPort->d.src.height,
               pPort->d.dst.x, pPort->d.dst.y, pPort->d.dst.width, pPort->d.dst.height);
*/
//    exynosUtilPlaneDump (pPort->pScrn);
}


static Bool
_exynosVideoImageCalculateSize (IMAGEPortInfoPtr pPort)
{
    XDBG_RETURN_VAL_IF_FAIL(pPort != NULL, FALSE);
    EXYNOSIppProp src_prop = { 0, }, dst_prop = { 0, };

    src_prop.id = pPort->d.fourcc;
    src_prop.width = pPort->d.width;
    src_prop.height = pPort->d.height;
    src_prop.crop = pPort->d.src;

    dst_prop.id = FOURCC_RGB32;

    if (pPort->drawing == ON_PIXMAP || pPort->drawing == ON_WINDOW)
    {
        dst_prop.width = pPort->d.pDraw->width;
        dst_prop.height = pPort->d.pDraw->height;
        dst_prop.crop = pPort->d.dst;
        dst_prop.crop.x = pPort->d.pDraw->x;
        dst_prop.crop.y = pPort->d.pDraw->y;
    }
    else
    {
        dst_prop.width = pPort->d.dst.width;
        dst_prop.height = pPort->d.dst.height;
        dst_prop.crop = pPort->d.dst;
        dst_prop.crop.x = 0;
        dst_prop.crop.y = 0;
    }

    XDBG_DEBUG(MIA, "(%dx%d : %d,%d %dx%d) => (%dx%d : %d,%d %dx%d)\n", src_prop.width,
               src_prop.height, src_prop.crop.x, src_prop.crop.y, src_prop.crop.width,
               src_prop.crop.height, dst_prop.width, dst_prop.height, dst_prop.crop.x,
               dst_prop.crop.y, dst_prop.crop.width, dst_prop.crop.height);

    if (!exynosVideoConverterAdaptSize (&src_prop, &dst_prop))
        return FALSE;

    XDBG_DEBUG(MIA, "(%dx%d : %d,%d %dx%d) => (%dx%d : %d,%d %dx%d)\n", src_prop.width,
               src_prop.height, src_prop.crop.x, src_prop.crop.y, src_prop.crop.width,
               src_prop.crop.height, dst_prop.width, dst_prop.height, dst_prop.crop.x,
               dst_prop.crop.y, dst_prop.crop.width, dst_prop.crop.height);

    XDBG_RETURN_VAL_IF_FAIL(src_prop.width > 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL(src_prop.height > 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL(src_prop.crop.width > 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL(src_prop.crop.height > 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL(dst_prop.width > 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL(dst_prop.height > 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL(dst_prop.crop.width > 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL(dst_prop.crop.height > 0, FALSE);

    pPort->in_width = src_prop.width;
    pPort->in_height = src_prop.height;
    pPort->in_crop = src_prop.crop;

    pPort->out_width = dst_prop.width;
    pPort->out_height = dst_prop.height;
    pPort->out_crop = dst_prop.crop;

    return TRUE;
}

static Bool
_exynosVideoImageSetHWPortsProperty (ScreenPtr pScreen, int nums)
{
    WindowPtr pWin = pScreen->root;
    Atom atom_hw_ports;

    /* With "XV_HW_PORTS", an application can know
     * how many fimc devices XV uses.
     */
    if (!pWin || !serverClient)
        return FALSE;

    atom_hw_ports = MakeAtom ("XV_HW_PORTS", strlen ("XV_HW_PORTS"), TRUE);

    /* X clients can check how many HW layers Xserver supports */
    dixChangeWindowProperty (serverClient, pWin, atom_hw_ports, XA_CARDINAL, 32,
                             PropModeReplace,
                             1, (unsigned int*) &nums, FALSE);

    return TRUE;
}

static void
_exynosVideoImageBlockHandler (pointer data, OSTimePtr pTimeout, pointer pRead)
{
    ScreenPtr pScreen = ((ScrnInfoPtr) data)->pScreen;

    if (registered_handler
            && _exynosVideoImageSetHWPortsProperty (pScreen, IMAGE_HW_LAYER))
    {
        RemoveBlockAndWakeupHandlers (_exynosVideoImageBlockHandler,
                                      (WakeupHandlerProcPtr) NoopDDA, data);
        registered_handler = FALSE;
    }
}

static Bool
_exynosVideoImageAddDrawableEvent (IMAGEPortInfoPtr pPort)
{
    IMAGEPortDrawableInfo *resource;
    void *ptr = NULL;
    int ret;

    ret = dixLookupResourceByType (&ptr, pPort->d.pDraw->id, image_rtype, NULL,
                                   DixWriteAccess);
    if (ret == Success)
    {
        return TRUE;
    }

    resource = malloc (sizeof(IMAGEPortDrawableInfo));
    if (resource == NULL)
        return FALSE;

    /* 1. Add image_rtype to drawable
     * 2. When pixmap is destroyed, _exynosVideoImageRegisterEventDrawableGone
     *    will be called and video will be stopped.
     */
    if (!AddResource (pPort->d.pDraw->id, image_rtype, resource))
    {
        ctrl_free (resource);
        return FALSE;
    }

    XDBG_DEBUG(MIA, "port(%d) id(%ld)\n", pPort->index, pPort->d.pDraw->id);

    resource->id = pPort->d.pDraw->id;
    resource->pPort = pPort;
    resource->pScrn = pPort->pScrn;

    return TRUE;
}

static int
_exynosVideoImageRegisterEventDrawableGone (void *data, XID id)
{
    IMAGEPortDrawableInfo *resource = (IMAGEPortDrawableInfo*) data;

    if (!resource)
        return Success;

    if (!resource->pPort || !resource->pScrn)
        return Success;

    XDBG_INFO(MIA, "port(%d) id(%ld)\n", ((IMAGEPortInfoPtr )resource->pPort)->index, id);

    EXYNOSVideoImageStop (resource->pScrn, (pointer) resource->pPort, 1);

    ctrl_free (resource);

    return Success;
}

static Bool
_exynosVideoImageRegisterEventResourceTypes (void)
{
    image_rtype = CreateNewResourceType (_exynosVideoImageRegisterEventDrawableGone,
                                         "Sec Video Drawable");

    if (!image_rtype)
        return FALSE;

    return TRUE;
}

static int
EXYNOSVideoImageGetPortAttribute (ScrnInfoPtr pScrn, Atom attribute, INT32 *value,
                                  pointer data)
{
    XDBG_RETURN_VAL_IF_FAIL (data != NULL, 0);
    IMAGEPortInfoPtr pPort = (IMAGEPortInfoPtr) data;
    if (attribute == _exynosVideoImageGetPortAtom (PAA_ROTATION))
    {
        *value = pPort->rotate;
        XDBG_TRACE(MIA, "Get Attribute ROTATION Value %d\n", *value);
        return Success;
    }
    else if (attribute == _exynosVideoImageGetPortAtom (PAA_HFLIP))
    {
        *value = pPort->hflip;
        XDBG_TRACE(MIA, "Get Attribute HFLIP Value %d\n", *value);
        return Success;
    }
    else if (attribute == _exynosVideoImageGetPortAtom (PAA_VFLIP))
    {
        *value = pPort->vflip;
        XDBG_TRACE(MIA, "Get Attribute VFLIP Value %d\n", *value);
        return Success;
    }
    else if (attribute == _exynosVideoImageGetPortAtom (PAA_OUTPUT))
    {
        *value = pPort->usr_output;
        return Success;
    }

    XDBG_DEBUG (MIA, "Unknown attribute\n");
    return Success;
}

static int
EXYNOSVideoImageSetPortAttribute (ScrnInfoPtr pScrn, Atom attribute, INT32 value,
                                  pointer data)
{
    XDBG_RETURN_VAL_IF_FAIL (data != NULL, 0);

    IMAGEPortInfoPtr pPort = (IMAGEPortInfoPtr) data;
    if (attribute == _exynosVideoImageGetPortAtom (PAA_ROTATION))
    {
        pPort->rotate = value;
        XDBG_DEBUG(MIA, "port(%d) rotate(%d) \n", pPort->index, value);
        return Success;
    }
    else if (attribute == _exynosVideoImageGetPortAtom (PAA_HFLIP))
    {
        pPort->hflip = value;
        XDBG_DEBUG(MIA, "port(%d) hflip(%d) \n", pPort->index, value);
        return Success;
    }
    else if (attribute == _exynosVideoImageGetPortAtom (PAA_VFLIP))
    {
        pPort->vflip = value;
        XDBG_DEBUG(MIA, "port(%d) vflip(%d) \n", pPort->index, value);
        return Success;
    }
    else if (attribute == _exynosVideoImageGetPortAtom (PAA_OUTPUT))
    {
        if (value == OUTPUT_MODE_TVOUT)
            pPort->usr_output = OUTPUT_LCD|OUTPUT_EXT|OUTPUT_FULL;
        else if (value == OUTPUT_MODE_EXT_ONLY)
            pPort->usr_output = OUTPUT_EXT|OUTPUT_FULL;
        else
            pPort->usr_output = OUTPUT_LCD|OUTPUT_EXT;

        XDBG_DEBUG (MIA, "output (%d) \n", value);

        return Success;
    }
    XDBG_DEBUG (MIA, "Unknown attribute\n");

    return Success;
}

static void
EXYNOSVideoImageQueryBestSize (ScrnInfoPtr pScrn, Bool motion, short vid_w,
                               short vid_h, short dst_w, short dst_h,
                               unsigned int *p_w, unsigned int *p_h,
                               pointer data)
{
    EXYNOSIppProp prop = { 0, };

    if (!p_w && !p_h)
    {
        return;
    }
    prop.width = dst_w;
    prop.height = dst_h;
    prop.crop.width = dst_w;
    prop.crop.height = dst_h;

    if (exynosVideoConverterAdaptSize (NULL, &prop))
    {
        if (p_w)
            *p_w = prop.width;
        if (p_h)
            *p_h = prop.height;
    }
    else
    {
        if (p_w)
            *p_w = dst_w;
        if (p_h)
            *p_h = dst_h;
    }
}

/**
 * Give image size and pitches.
 */

static int
EXYNOSVideoImageQueryImageAttributes (ScrnInfoPtr pScrn, int id,
                                      unsigned short *w, unsigned short *h,
                                      int *pitches, int *offsets)
{
    int width, height, size;

    if (!w || !h)
    {
        return 0;
    }
    width = (int) * w;
    height = (int) * h;
    XDBG_TRACE (MIA, "Before id (%c%c%c%c) weight %u height %u\n", FOURCC_STR(id), *w, *h);
    size = exynosVideoImageAttrs (id, &width, &height, pitches, offsets, NULL);

    *w = (unsigned short) width;
    *h = (unsigned short) height;
    XDBG_TRACE (MIA, "After id (%c%c%c%c) weight %u height %u size %d\n", FOURCC_STR(id), *w, *h, size);

    return size;
}

/* coordinates : HW, SCREEN, PORT
 * BadRequest : when video can't be shown or drawn.
 * Success    : A damage event(pixmap) and inbuf should be return.
 *              If can't return a damage event and inbuf, should be return
 *              BadRequest.
 */

static int
_exynosVideoImageCheckChange (IMAGEPortInfoPtr pPort)
{
    int ret = CHANGED_NONE;

    if (pPort->d.fourcc != pPort->old_d.fourcc || pPort->d.width != pPort->old_d.width
            || pPort->d.height != pPort->old_d.height
            || memcmp (&pPort->d.src, &pPort->old_d.src, sizeof(xRectangle))
            || pPort->old_rotate != pPort->rotate || pPort->old_hflip != pPort->hflip
            || pPort->old_vflip != pPort->vflip)

    {
        XDBG_INFO(
            MIA,
            "pPort(%d) old_src(%dx%d %d,%d %dx%d) : new_src(%dx%d %d,%d %dx%d)\n",
            pPort->index, pPort->old_d.width, pPort->old_d.height,
            pPort->old_d.src.x, pPort->old_d.src.y, pPort->old_d.src.width,
            pPort->old_d.src.height, pPort->d.width, pPort->d.height,
            pPort->d.src.x, pPort->d.src.y, pPort->d.src.width,
            pPort->d.src.height);

        ret += CHANGED_IN;
    }

    if (memcmp (&pPort->d.dst, &pPort->old_d.dst, sizeof(xRectangle)))
    {
        XDBG_INFO(MIA, "pPort(%d) old_dst(%d,%d %dx%d) : new_dst(%dx%d %dx%d)\n",
                  pPort->index, pPort->old_d.dst.x, pPort->old_d.dst.y,
                  pPort->old_d.dst.width, pPort->old_d.dst.height, pPort->d.dst.x,
                  pPort->d.dst.y, pPort->d.dst.width, pPort->d.dst.height);
        ret += CHANGED_OUT;
    }

    return ret;
}

static void
_exynosVideoImagePunchDrawable (IMAGEPortInfoPtr pPort)
{
    PixmapPtr pPixmap = _exynosVideoImageGetDrawPixmap (pPort->d.pDraw);

    if (pPort->drawing != ON_FB)
        return;

    if (!pPort->punched)
    {
        EXYNOSExaPrepareAccess (pPixmap, EXA_PREPARE_DEST);
        if (pPixmap->devPrivate.ptr)
            memset (pPixmap->devPrivate.ptr, 0,
                    pPixmap->drawable.width * pPixmap->drawable.height * 4);
        XDBG_TRACE (MIA, "Punched (%dx%d) %p. \n",
                    pPixmap->drawable.width, pPixmap->drawable.height,
                    pPixmap->devPrivate.ptr);
        EXYNOSExaFinishAccess (pPixmap, EXA_PREPARE_DEST);
        pPort->punched = TRUE;
        DamageDamageRegion (pPort->d.pDraw, pPort->d.clip_boxes);
    }
}

unsigned long long get_nanotime(){
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC,&tv);
    return tv.tv_sec*1000000000L+tv.tv_nsec;
}


static int
EXYNOSVideoImagePutImage(ScrnInfoPtr pScrn, short src_x, short src_y,
                         short dst_x, short dst_y, short src_w, short src_h, short dst_w,
                         short dst_h, int id, unsigned char *buf, short width, short height,
                         Bool sync, RegionPtr clip_boxes, pointer data, DrawablePtr pDraw)
{
    IMAGEPortInfoPtr pPort = (IMAGEPortInfoPtr) data;
    PixmapPtr outbuf = NULL;
    PixmapPtr inbuf = NULL;
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    int output;

    /* PutImage's data */
    pPort->pScrn = pScrn;
    pPort->d.fourcc = id;
    pPort->d.width = width;
    pPort->d.height = height;
    pPort->d.src.x = src_x;
    pPort->d.src.y = src_y;
    pPort->d.src.width = src_w;
    pPort->d.src.height = src_h;
    pPort->d.dst.x = dst_x; /* included pDraw'x */
    pPort->d.dst.y = dst_y; /* included pDraw'y */
    pPort->d.dst.width = dst_w;
    pPort->d.dst.height = dst_h;
    pPort->d.buf = buf;
    pPort->d.sync = FALSE; /* Don't support */

    pPort->d.data = data;
    pPort->d.pDraw = pDraw;

    unsigned long long t1=get_nanotime();

    /* on which video is rendered */
    pPort->drawing = _exynosVideoImagedrawingOn (pPort);
    /* calculate proper size to render video
     * it depends on HW or SW.
     */

    unsigned long long t2=get_nanotime();

    XDBG_RETURN_VAL_IF_FAIL(
            _exynosVideoImageCalculateSize (pPort), 
                                                    BadRequest);

    unsigned long long t3=get_nanotime();

    if (pExynos->lcdstatus)
    {
        /**
         * @todo if LCD is off, skip rendering (temporary part)
         */
        EXYNOSVideoImageStop (pScrn, (pointer) pPort, TRUE);
        XDBG_DEBUG(MIA, "port(%d) put image after dpms off.\n", pPort->index);
        return BadRequest;
    }

    output = _exynosVideoGetTvoutMode (pPort);

    unsigned long long t4=get_nanotime();

    if (pPort->last_ipp_data)
    {
        switch (_exynosVideoImageCheckChange (pPort))
        {
        case CHANGED_IN:
            exynosVideoConverterClose (pPort);
            _exynosVideoImageClosePixs (pPort->list_of_pixmap_in,TRUE);

            exynosUtilStorageFinit(pPort->list_of_pixmap_in);
            pPort->list_of_pixmap_in = NULL;
            break;
        case CHANGED_OUT:
            exynosVideoConverterClose (pPort);
            if (pPort->drawing == ON_FB)
            {
                _exynosVideoImageHideOutbuf (pPort);
            }
            _exynosVideoImageClosePixs (pPort->list_of_pixmap_out, TRUE);

            exynosUtilStorageFinit(pPort->list_of_pixmap_out);
            pPort->list_of_pixmap_out = NULL;
            break;
        case CHANGED_ALL:
            exynosVideoConverterClose (pPort);
            _exynosVideoImageClosePixs (pPort->list_of_pixmap_in,TRUE);
            exynosUtilStorageFinit(pPort->list_of_pixmap_in);
            pPort->list_of_pixmap_in = NULL;
            if (pPort->drawing == ON_FB)
            {
                _exynosVideoImageHideOutbuf (pPort);
            }
            _exynosVideoImageClosePixs (pPort->list_of_pixmap_out, TRUE);
            exynosUtilStorageFinit(pPort->list_of_pixmap_out);
            pPort->list_of_pixmap_out = NULL;
            break;
        default:
            break;
        }
    }

    unsigned long long t5=get_nanotime();

    if (!pPort->list_of_ipp)
        pPort->list_of_ipp = exynosUtilStorageInit();
    if (!pPort->list_of_pixmap_in)
        pPort->list_of_pixmap_in = exynosUtilStorageInit();
    if (!pPort->list_of_pixmap_out)
        pPort->list_of_pixmap_out = exynosUtilStorageInit();

    if (clip_boxes)
    {
        if (!pPort->d.clip_boxes)
            pPort->d.clip_boxes = RegionCreate (NullBox, 0);
        RegionCopy (pPort->d.clip_boxes, clip_boxes);
    }

    unsigned long long t6=get_nanotime();


    XDBG_DEBUG(MIA, "pPort(%d) rotate(%d) flip(%d,%d) on(%s)\n", pPort->index,
               pPort->rotate, pPort->hflip, pPort->vflip, drawing_type[pPort->drawing]);
    XDBG_DEBUG(MIA, "port(%d) id(%c%c%c%c) sz(%dx%d) src(%d,%d %dx%d) dst(%d,%d %dx%d)\n",
               pPort->index, FOURCC_STR (id), width, height, src_x, src_y, src_w, src_h,
               dst_x, dst_y, dst_w, dst_h);

    if (pPort->drawing == ON_PIXMAP || pPort->drawing == ON_WINDOW)
        XDBG_RETURN_VAL_IF_FAIL(_exynosVideoImageAddDrawableEvent (pPort), BadAlloc);
    if (pPort->drawing == ON_FB)
    {
        _exynosVideoImagePunchDrawable(pPort);
    }

    unsigned long long t7=get_nanotime();

    outbuf =  _exynosVideoImageGetOutbuf(pPort);
    // XDBG_RETURN_VAL_IF_FAIL(outbuf != NULL, BadRequest);
    inbuf =  _exynosVideoImageGetInbuf(pPort);
    // XDBG_RETURN_VAL_IF_FAIL(inbuf != NULL, BadRequest);

    unsigned long long t8=get_nanotime();

    if (!outbuf || !inbuf)
    {
        _exynosVideoImageClosePixs (pPort->list_of_pixmap_in, FALSE);
        _exynosVideoImageClosePixs (pPort->list_of_pixmap_out, FALSE);
    }
    else if (!exynosVideoConverterOpen (pPort, inbuf, outbuf))
    {
        EXYNOSVideoImageStop (pScrn, (pointer) pPort, TRUE);
        return BadRequest;
    }

    XDBG_DEBUG(MTST, "%i %i %i %i %i %i %i  total: %i\n", t2-t1,t3-t2,t4-t3,t5-t4,t6-t5,t7-t6,t8-t7,t8-t1);

    /*******************   testing part ************************/
        /* HDMI */
//        if (output & OUTPUT_EXT)
//            tvout = _secVideoPutImageTvout (pPort, output, inbuf);
//        else
    //    {
    //        _secVideoUngrabTvout (pPort);

    //        SECWb *wb = secWbGet ();
    //        if (wb)
    //            secWbSetSecure (wb, pPort->secure);
    //    }

    //    /* LCD */
    //    if (output & OUTPUT_LCD)
    //    {
    /**********************************************************/


    /* keep PutImage's data to check if new PutImage's data is changed. */
    pPort->old_rotate = pPort->rotate;
    pPort->old_hflip = pPort->hflip;
    pPort->old_vflip = pPort->vflip;
    pPort->old_d = pPort->d;
    return Success;
}

static int
EXYNOSVideoImageDDPutImage (ClientPtr client, DrawablePtr pDraw, XvPortPtr pPort,
                            GCPtr pGC, INT16 src_x, INT16 src_y, CARD16 src_w,
                            CARD16 src_h, INT16 drw_x, INT16 drw_y, CARD16 drw_w,
                            CARD16 drw_h, XvImagePtr format, unsigned char *data,
                            Bool sync, CARD16 width, CARD16 height)
{

    IMAGEPortClientInfo* info = NULL;

    if (pDraw->type == DRAWABLE_WINDOW)
    {
        info = GetImageClientInfo ((WindowPtr)pDraw);
    }
    else
    {
        info = GetImageClientInfo ((PixmapPtr)pDraw);
    }

    int ret = 0;

    if (info)
        info->client = client;

    ret = ddPutImage (client, pDraw, pPort, pGC, src_x, src_y, src_w, src_h, drw_x,
                      drw_y, drw_w, drw_h, format, data, sync, width, height);

    return ret;
}

static void
EXYNOSVideoImageStop (ScrnInfoPtr pScrn, pointer data, Bool exit)
{
    IMAGEPortInfoPtr pPort = (IMAGEPortInfoPtr) data;

    if (!exit)
        return;

    if (pPort->d.clip_boxes)
    {
        RegionDestroy (pPort->d.clip_boxes);
        pPort->d.clip_boxes = NULL;
    }

    XDBG_DEBUG(MIA, "pPort(%d) exit (%d) \n", pPort->index, exit);

    while (!exynosUtilStorageIsEmpty (pPort->list_of_ipp))
    {
        pPort->last_ipp_data = exynosUtilStorageGetFirst (pPort->list_of_ipp);
        exynosVideoConverterClose (pPort);
    }

    exynosUtilStorageFinit (pPort->list_of_ipp);
    pPort->list_of_ipp = NULL;


    _exynosVideoImageClosePixs (pPort->list_of_pixmap_in, TRUE);
    exynosUtilStorageFinit(pPort->list_of_pixmap_in);
    pPort->list_of_pixmap_in = NULL;
    if (pPort->drawing == ON_FB)
    {
        _exynosVideoImageHideOutbuf (pPort);
    }
    _exynosVideoImageClosePixs (pPort->list_of_pixmap_out, TRUE);
    exynosUtilStorageFinit(pPort->list_of_pixmap_out);
    pPort->list_of_pixmap_out = NULL;

    memset (&pPort->old_d, 0, sizeof(IMAGEPutData));
    memset (&pPort->d, 0, sizeof(IMAGEPutData));

    pPort->in_width = 0;
    pPort->in_height = 0;
    memset (&pPort->in_crop, 0, sizeof(xRectangle));

    pPort->out_width = 0;
    pPort->out_height = 0;
    memset (&pPort->out_crop, 0, sizeof(xRectangle));

    pPort->rotate = 0;
    pPort->hflip = 0;
    pPort->vflip = 0;
    pPort->drawing = 0;

    pPort->plane_id = 0;
    pPort->crtc_id = 0;

}

/**
 * Set up all our internal structures.
 */
XF86VideoAdaptorPtr
exynosVideoImageSetup (ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    XF86VideoAdaptorPtr pAdaptor = NULL;
    IMAGEPortInfoPtr pPort;
    int i = 0;

    pAdaptor = ctrl_calloc (
                   1,
                   sizeof(XF86VideoAdaptorRec)
                   + (sizeof(DevUnion) + sizeof(IMAGEPortInfo)) * IMAGE_MAX_PORT);
    if (!pAdaptor)
        return NULL;

    image_encoding[0].width = pScreen->width;
    image_encoding[0].height = pScreen->height;

    pAdaptor->type = XvWindowMask | XvPixmapMask | XvInputMask | XvImageMask;
    pAdaptor->flags = VIDEO_OVERLAID_IMAGES;
    pAdaptor->name = "EXYNOS Image Video";
    pAdaptor->nEncodings = sizeof (image_encoding) / sizeof(XF86VideoEncodingRec);
    pAdaptor->pEncodings = image_encoding;
    pAdaptor->nFormats = IMAGE_FORMATS_NUM;
    pAdaptor->pFormats = image_formats;
    pAdaptor->nPorts = IMAGE_MAX_PORT;
    pAdaptor->pPortPrivates = (DevUnion*) (&pAdaptor[1]);

    pPort = (IMAGEPortInfoPtr) (&pAdaptor->pPortPrivates[IMAGE_MAX_PORT]);

    for (i = 0; i < IMAGE_MAX_PORT; i++)
    {
        pAdaptor->pPortPrivates[i].ptr = &pPort[i];
        pPort[i].index = i;
        pPort[i].usr_output = OUTPUT_LCD|OUTPUT_EXT;
     }

    pAdaptor->nAttributes = IMAGE_ATTRS_NUM;
    pAdaptor->pAttributes = image_attrs;
    pAdaptor->nImages = IMAGE_IMAGES_NUM;
    pAdaptor->pImages = image_images;

    pAdaptor->GetPortAttribute = EXYNOSVideoImageGetPortAttribute;
    pAdaptor->SetPortAttribute = EXYNOSVideoImageSetPortAttribute;
    pAdaptor->QueryBestSize = EXYNOSVideoImageQueryBestSize;
    pAdaptor->QueryImageAttributes = EXYNOSVideoImageQueryImageAttributes;
    pAdaptor->PutImage = EXYNOSVideoImagePutImage;
    pAdaptor->StopVideo = EXYNOSVideoImageStop;

    if (!_exynosVideoImageRegisterEventResourceTypes ())
    {
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
                    "Failed to register EventResourceTypes. \n");
        return FALSE;
    }

    if (!registered_handler)
    {
        RegisterBlockAndWakeupHandlers (_exynosVideoImageBlockHandler,
                                        (WakeupHandlerProcPtr) NoopDDA, pScrn);
        registered_handler = TRUE;
    }

    xorg_list_init (&watch_vlank_ports);

    return pAdaptor;
}

void
exynosVideoImageReplacePutImageFunc (ScreenPtr pScreen)
{
    int i;

    XvScreenPtr xvsp = dixLookupPrivate (&pScreen->devPrivates, XvGetScreenKey ());
    if (!xvsp)
        return;

    for (i = 0; i < xvsp->nAdaptors; i++)
    {
        XvAdaptorPtr pAdapt = xvsp->pAdaptors + i;
        if (pAdapt->ddPutImage)
        {
            ddPutImage = pAdapt->ddPutImage;
            pAdapt->ddPutImage = EXYNOSVideoImageDDPutImage;
            break;
        }
    }

    if (!dixRegisterPrivateKey (ImageClientKey, PRIVATE_WINDOW, sizeof(ClientPtr)))
        return;
    if (!dixRegisterPrivateKey (ImageClientKey, PRIVATE_PIXMAP, sizeof(ClientPtr)))
        return;
}

int
exynosVideoImageAttrs (int id, int *w, int *h, int *pitches, int *offsets, int *lengths)
{
    int size = 0, tmp = 0;
    *w = (*w + 1) & ~1;
    if (offsets)
        offsets[0] = 0;

    switch (id)
    {
        /* RGB565 */
    case FOURCC_SR16:
    case FOURCC_RGB565:
        size += (*w << 1);
        if (pitches)
            pitches[0] = size;
        size *= *h;
        if (lengths)
            lengths[0] = size;
        break;
        /* RGB32 */
    case FOURCC_SR32:
    case FOURCC_RGB32:
        size += (*w << 2);
        if (pitches)
            pitches[0] = size;
        size *= *h;
        if (lengths)
            lengths[0] = size;
        break;
        /* YUV420, 3 planar */
    case FOURCC_I420:
    case FOURCC_S420:
    case FOURCC_YV12:
        *h = (*h + 1) & ~1;
        size = (*w + 3) & ~3;
        if (pitches)
            pitches[0] = size;

        size *= *h;
        if (offsets)
            offsets[1] = size;
        if (lengths)
            lengths[0] = size;

        tmp = ( (*w >> 1) + 3) & ~3;
        if (pitches)
            pitches[1] = pitches[2] = tmp;

        tmp *= (*h >> 1);
        size += tmp;
        if (offsets)
            offsets[2] = size;
        if (lengths)
            lengths[1] = tmp;

        size += tmp;
        if (lengths)
            lengths[2] = tmp;

        break;
        /* YUV422, packed */
    case FOURCC_UYVY:
    case FOURCC_SYVY:
    case FOURCC_ITLV:
    case FOURCC_SUYV:
    case FOURCC_YUY2:
        size = *w << 1;
        if (pitches)
            pitches[0] = size;

        size *= *h;
        if (lengths)
            lengths[0] = size;
        break;

        /* YUV420, 2 planar */
    case FOURCC_SN12:
    case FOURCC_NV12:
    case FOURCC_SN21:
    case FOURCC_NV21:
        if (pitches)
            pitches[0] = *w;

        size = (*w) * (*h);
        if (offsets)
            offsets[1] = size;
        if (lengths)
            lengths[0] = size;

        if (pitches)
            pitches[1] = *w >> 1;

        tmp = (*w) * (*h >> 1);
        size += tmp;
        if (lengths)
            lengths[1] = tmp;
        break;

        /* YUV420, 2 planar, tiled */
    case FOURCC_ST12:
        if (pitches)
            pitches[0] = *w;

        size = ALIGN_TO_8KB(ALIGN_TO_128B(*w) * ALIGN_TO_32B(*h));
        if (offsets)
            offsets[1] = size;
        if (lengths)
            lengths[0] = size;

        if (pitches)
            pitches[1] = *w >> 1;

        tmp = ALIGN_TO_8KB(ALIGN_TO_128B(*w) * ALIGN_TO_32B(*h >> 1));
        size += tmp;
        if (lengths)
            lengths[1] = tmp;
        break;
    default:
        return 0;
    }

    return size;
}


#if 0
static Bool
_exynosVideoImageClearMissedEvent (IMAGEPortInfoPtr pPort)
{

    XDBG_RETURN_VAL_IF_FAIL(pPort != NULL, FALSE);
    EXYNOSIppBufPtr pIpp_buf_in = NULL, pIpp_buf_out = NULL;
    int i = 0, size = 0;
    size = exynosUtilStorageGetSize (pIpp_data->list_of_inbuf);
    for (i = 0; i < size; i++)
    {

        pIpp_buf_in = exynosUtilStorageFindData(pIpp_data->list_of_inbuf, &stamp, _exynosVideoImageCompareIppBufStamp);
        if (pIpp_buf_in == NULL)
            return FALSE;
//        pIpp_buf_out = exynosUtilStorageFindData(pIpp_data->list_of_outbuf, &pIpp_buf_in->index, _exynosVideoImageCompareBufIndex);


//        XDBG_TRACE(MIA, "OUT Pixmap %u Ipp_index %d Ipp_stamp %lu Delta %d\n", STAMP(pIpp_buf_out->pVideoPixmap), pIpp_buf_out->index, pIpp_buf_out->stamp, (int) cur_stamp - pIpp_buf_out->stamp);

        XDBG_TRACE(MIA, "IN Pixmap %u Ipp_index %d Ipp_stamp %lu Delta %ld\n", STAMP(pIpp_buf_in->pVideoPixmap), pIpp_buf_in->index, pIpp_buf_in->stamp, (long) cur_stamp - pIpp_buf_in->stamp);

//        exynosVideoConverterIppDequeueBuf(pPort, pIpp_data, pIpp_buf_dst);
//        exynosVideoConverterIppDequeueBuf(pPort, pIpp_data, pIpp_buf_src);
//        exynosVideoBufferConverting (pIpp_buf_in->pVideoPixmap, FALSE);
//        exynosVideoBufferConverting (pIpp_buf_out->pVideoPixmap, FALSE);
/*        exynosUtilStorageEraseData (pIpp_data->list_of_outbuf, pIpp_buf_out);
        ctrl_free (pIpp_buf_out);

        exynosUtilStorageEraseData (pPort->list_of_pixmap_in, pIpp_buf_in->pVideoPixmap);
        exynosVideoBufferFree (pIpp_buf_in->pVideoPixmap);

        exynosUtilStorageEraseData (pIpp_data->list_of_inbuf, pIpp_buf_in);
        ctrl_free (pIpp_buf_in);
*/
    }
return TRUE;
}
#endif


/** @todo Temporary here exynosVideoImageIppEventHandler*/
void
exynosVideoImageIppEventHandler (int fd, unsigned int prop_id,
                                 unsigned int * buf_idx, pointer data)
{
    XDBG_DEBUG(MIA, "IPP EVENT %p\n", data);
    XDBG_DEBUG(MIA, "IPP EVENT buf_idx %u %u prop_id %u\n",
               buf_idx[EXYNOS_DRM_OPS_SRC], buf_idx[EXYNOS_DRM_OPS_DST], prop_id);

    IMAGEPortInfoPtr pPort = (IMAGEPortInfoPtr) data;
    DrawablePtr pDrawable = NULL;
//    CARD32 cur_stamp = GetTimeInMillis();
    EXYNOSIppPtr pIpp_data = exynosUtilStorageFindData (pPort->list_of_ipp, &prop_id,
                             _exynosVideoImageCompareIppId);
    XDBG_RETURN_IF_FAIL(pIpp_data != NULL);

    EXYNOSIppBuf *inbuf = NULL, *outbuf = NULL;
    outbuf = exynosUtilStorageFindData (pIpp_data->list_of_outbuf,
                                        &buf_idx[EXYNOS_DRM_OPS_DST],
                                        _exynosVideoImageCompareBufIndex);
    XDBG_RETURN_IF_FAIL(outbuf != NULL);
    XDBG_DEBUG(MIA, "EVENT! OUTPixmap (%p)-(stamp:%lu)\n", outbuf->pVideoPixmap,
                   outbuf->stamp);
    exynosVideoBufferConverting(outbuf->pVideoPixmap, FALSE);

    if (pPort->drawing == ON_PIXMAP || pPort->drawing == ON_WINDOW)
    {
        pDrawable = &outbuf->pVideoPixmap->drawable;
//        pDrawable = pPort->d.pDraw;
        if (!pDrawable)
        {
            XDBG_NEVER_GET_HERE(MIA);
            pDrawable = pPort->d.pDraw;
        }
        DamageDamageRegion (pDrawable, pPort->d.clip_boxes);

    }
    if (pPort->drawing == ON_FB)
        _exynosVideoImageShowOutbuf (pPort, outbuf->pVideoPixmap);


    inbuf = exynosUtilStorageFindData (pIpp_data->list_of_inbuf,
                                       &buf_idx[EXYNOS_DRM_OPS_SRC],
                                       _exynosVideoImageCompareBufIndex);
    XDBG_RETURN_IF_FAIL(inbuf != NULL);
    exynosVideoBufferConverting(inbuf->pVideoPixmap, FALSE);
    XDBG_DEBUG(MIA, "EVENT! INPixmap (%p)-(stamp:%lu)\n", inbuf->pVideoPixmap,
                   inbuf->stamp);
    if (inbuf->stamp < pPort->temp_stamp)
    {
        XDBG_DEBUG(MIA,"Found **********************************************\n");
    }
    pPort->temp_stamp = inbuf->stamp;
   
    /** @todo Correct CVT struct clear */
//    exynosVideoConverterIppDequeueBuf(pPort, pIpp_data, inbuf);
//    exynosVideoConverterIppDequeueBuf(pPort, pIpp_data, outbuf);
    exynosUtilStorageEraseData (pIpp_data->list_of_outbuf, outbuf);
    ctrl_free (outbuf);


    exynosUtilStorageEraseData (pPort->list_of_pixmap_in, inbuf->pVideoPixmap);
    exynosVideoBufferFree (inbuf->pVideoPixmap);
    exynosUtilStorageEraseData (pIpp_data->list_of_inbuf, inbuf);
    ctrl_free (inbuf);
//    _exynosVideoImageClearMissedEvent (pPort, pIpp_data, pPort->temp_stamp);
}

ClientPtr
exynosVideoImageGetClient (DrawablePtr pDraw)
{
    XDBG_RETURN_VAL_IF_FAIL(pDraw != NULL, NULL);

    IMAGEPortClientInfo* client_info = NULL;

    if (pDraw->type == DRAWABLE_WINDOW)
    {
        client_info = GetImageClientInfo ((WindowPtr)pDraw);
        return client_info->client;
    }
    else
    {
        client_info = GetImageClientInfo ((PixmapPtr)pDraw);
        return client_info->client;
    }
}

Bool
exynosVideoImageRawCopy (IMAGEPortInfoPtr pPort, PixmapPtr dst)
{

    XDBG_RETURN_VAL_IF_FAIL(pPort != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL(dst != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL( (exynosVideoBufferGetFourcc (dst) == pPort->d.fourcc), FALSE);
    int dst_size = 0, src_size = 0;
    int dst_pitches[PLANE_CNT] = {0}, src_pitches[PLANE_CNT] = {0};
    int dst_offsets[PLANE_CNT] = {0}, src_offsets[PLANE_CNT] = {0};
    int dst_lengths[PLANE_CNT] = {0}, src_lengths[PLANE_CNT] = {0};
    pointer src_sample[PLANE_CNT] = {0};
    int  src_width = 0, src_height = 0, src_YUV_height[PLANE_CNT] = {0};
    int i = 0;
    src_width = pPort->d.width;
    src_height = pPort->d.height;
    XF86ImagePtr image_info = exynosVideoImageGetImageInfo (pPort->d.fourcc);
    XDBG_RETURN_VAL_IF_FAIL(image_info != NULL, FALSE);
    dst_size = exynosVideoBufferGetSize (dst, dst_pitches, dst_offsets, dst_lengths);
    src_size = exynosVideoImageAttrs (pPort->d.fourcc, &src_width, &src_height,
                                      src_pitches, src_offsets, src_lengths);

    XDBG_TRACE(
        MIA,
        "SRC: F(%c%c%c%c),w(%d),h(%d),p(%d,%d,%d),o(%d,%d,%d),l(%d,%d,%d)\n",
        FOURCC_STR (pPort->d.fourcc), src_width, src_height, src_pitches[0],
        src_pitches[1], src_pitches[2], src_offsets[0], src_offsets[1],
        src_offsets[2], src_lengths[0], src_lengths[1], src_lengths[2]);

    XDBG_TRACE(
        MIA,
        "DST: F(%c%c%c%c),w(%d),h(%d),p(%d,%d,%d),o(%d,%d,%d),l(%d,%d,%d)\n",
        FOURCC_STR (exynosVideoBufferGetFourcc (dst)), dst->drawable.width,
        dst->drawable.height, dst_pitches[0], dst_pitches[1], dst_pitches[2],
        dst_offsets[0], dst_offsets[1], dst_offsets[2], dst_lengths[0],
        dst_lengths[1], dst_lengths[2]);


    if (src_size == dst_size && (memcmp (src_pitches, dst_pitches, PLANE_CNT * sizeof(int)) == 0)
            && (memcmp (src_lengths, dst_lengths, PLANE_CNT * sizeof(int)) == 0))
    {
        for (i = 0; i < image_info->num_planes; i++)
        {
            src_sample[i] = pPort->d.buf + src_offsets[i];
        }

        XDBG_RETURN_VAL_IF_FAIL(
            exynosVideoBufferCopyRawData (dst, src_sample, src_lengths), FALSE);
    }
    else if (src_size < dst_size && (memcmp (src_pitches, dst_pitches, PLANE_CNT * sizeof(int)) < 0))
    {
        if (image_info->vert_y_period)
        src_YUV_height[0] = src_height / image_info->vert_y_period;
        if (image_info->vert_u_period)
        src_YUV_height[1] = src_height / image_info->vert_u_period;
        if (image_info->vert_v_period)
        src_YUV_height[2] = src_height / image_info->vert_v_period;
        
        for (i = 0; i < image_info->num_planes; i++)
        {
            src_sample[i] = pPort->d.buf + src_offsets[i];
        }
        XDBG_RETURN_VAL_IF_FAIL(
            exynosVideoBufferCopyOneChannel (dst, src_sample, src_pitches, src_YUV_height), FALSE);
    }
    else
    {
        XDBG_NEVER_GET_HERE(MIA);
        return FALSE;
    }
    return TRUE;
}

static Bool
_exynosVideoSetOutputExternalProperty (DrawablePtr pDraw, Bool video_only)
{
    WindowPtr pWin;
    Atom atom_external;

    XDBG_RETURN_VAL_IF_FAIL (pDraw != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (pDraw->type == DRAWABLE_WINDOW, FALSE);

    pWin = (WindowPtr)pDraw;

    atom_external = MakeAtom ("XV_OUTPUT_EXTERNAL", strlen ("XV_OUTPUT_EXTERNAL"), TRUE);

    dixChangeWindowProperty (clients[CLIENT_ID(pDraw->id)],
                             pWin, atom_external, XA_CARDINAL, 32,
                             PropModeReplace, 1, (unsigned int*)&video_only, TRUE);

    XDBG_TRACE (MVDO, "pDraw(0x%08x) video-only(%s)\n",
                pDraw->id, (video_only)?"ON":"OFF");

    return TRUE;
}

static int
_exynosVideoGetTvoutMode (IMAGEPortInfoPtr pPort)
{
//    SECModePtr pSecMode = (SECModePtr) SECPTR (pPort->pScrn)->pSecMode;
//    SECVideoPrivPtr pVideo = SECPTR(pPort->pScrn)->pVideoPriv;
//    SECDisplaySetMode disp_mode = secDisplayGetDispSetMode (pPort->pScrn);

    EXYNOSDispInfoPtr pDispInfo = EXYNOSDISPINFOPTR(pPort->pScrn);
    XDBG_RETURN_VAL_IF_FAIL(pDispInfo != NULL,FALSE);

    int output = OUTPUT_LCD;

    EXYNOS_DisplaySetMode disp_mode = pDispInfo->set_mode;
    if (disp_mode == DISPLAY_SET_MODE_CLONE)
    {
    /* temporary hide section*/
#if 0
        if (pPort->preemption > -1)
        {
            if (pVideo->video_output > 0 && streaming_ports == 1)
            {
                int video_output = pVideo->video_output - 1;

                if (video_output == OUTPUT_MODE_DEFAULT)
                    output = OUTPUT_LCD;
                else if (video_output == OUTPUT_MODE_TVOUT)
                    output = OUTPUT_LCD|OUTPUT_EXT|OUTPUT_FULL;
                else
                    output = OUTPUT_EXT|OUTPUT_FULL;
            }
            else if (streaming_ports == 1)
            {
                output = pPort->usr_output;
                if (!(output & OUTPUT_FULL))
                    output &= ~(OUTPUT_EXT);
            }
            else if (streaming_ports > 1)
                output = OUTPUT_LCD;
            else
                XDBG_NEVER_GET_HERE (MIA);
        }
        else
#endif
            output = OUTPUT_LCD;
    }
    else if (disp_mode == DISPLAY_SET_MODE_EXT)
    {
        if (pPort->drawing == ON_PIXMAP)
            output = OUTPUT_LCD;
        else
        {
            xf86CrtcPtr pCrtc = exynosDisplayGetCrtc (pPort->d.pDraw);

            int c = exynosCrtcGetConnectType (pCrtc);

            if (c == DRM_MODE_CONNECTOR_LVDS || c == DRM_MODE_CONNECTOR_Unknown)
                output = OUTPUT_LCD;
            else if (c == DRM_MODE_CONNECTOR_HDMIA || c == DRM_MODE_CONNECTOR_HDMIB)
                output = OUTPUT_EXT;
            else if (c == DRM_MODE_CONNECTOR_VIRTUAL)
                output = OUTPUT_EXT;
            else
                XDBG_NEVER_GET_HERE (MIA);
        }
    }
    else /* DISPLAY_SET_MODE_OFF */
    {
        output = OUTPUT_LCD;
    }

    if (pPort->drawing == ON_PIXMAP)
        output = OUTPUT_LCD;

    XDBG_DEBUG (MIA, "drawing(%d) disp_mode(%d) preemption(%d) streaming_ports(%d) conn_mode(%d) usr_output(%d) video_output(%d) output(%x) skip(%d)\n",
                pPort->drawing, disp_mode, pPort->preemption, streaming_ports, pSecMode->conn_mode,
                pPort->usr_output, pVideo->video_output, output, pPort->skip_tvout);

    return output;
}
