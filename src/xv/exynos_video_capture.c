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


#include <pixman.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvproto.h>
#include <fourcc.h>

#include "exynos_driver.h"
#include "exynos_util.h"
#include "exynos_video.h"
#include "exynos_video_converter.h"
#include "exynos_video_fourcc.h"
#include "exynos_video_capture.h"
#include "exynos_video_buffer.h"
#include "exynos_accel_int.h"
#include "fimg2d.h"
#include "_trace.h"
#include <xf86Crtc.h>
#include <xf86xv.h>
#include "xv_types.h"
#include <damage.h>
#include <fb.h>
#include <fbdevhw.h>
#include "exynos_display_int.h"
#include "exynos_clone.h"
#include "exynos_mem_pool.h"

/*Uncomment DEBUG_CAPTURE define for verbose debuging information*/
//#define DEBUG_CAPTURE           1

#define CAPTURE_MAX_PORT        1

/* key of tbo's user_data */
#define TBM_BODATA_VPIXMAP 0x1000
typedef void (*NotifyFunc) (EXYNOSBufInfo* vbuf, int type, void *type_data, void *user_data);

typedef struct _NotifyFuncData
{
    NotifyFunc  func;
    void       *user_data;

    struct xorg_list link;
} NotifyFuncData;

typedef struct _RetBufInfo
{
    EXYNOSBufInfo *vbuf;
    int type;
    struct xorg_list link;
} RetBufInfo;

typedef struct
{
    ScrnInfoPtr pScrn;
    int index;
    RegionPtr   clipBoxes;

    /* capture_attrs */
    Bool rotate_off;
    Bool swcomp_on;
    int     capture;
    unsigned int     id;
    Bool    data_type;

    /* writeback */
    EXYNOS_CloneObject* clone;
    int wb_fps;

    EXYNOSBufInfo *_inbuf;
    EXYNOSBufInfo *_outbuf;
    EXYNOSBufInfo **outbuf;
    int          outbuf_num;
    int          outbuf_index;

    /* put data */
    DrawablePtr pDraw;

    struct xorg_list retbuf_info;
    Bool         need_damage;

    OsTimerPtr retire_timer;
    Bool putstill_on;

    unsigned int status;
    CARD32       retire_time;

    struct xorg_list link;

    struct xorg_list watch_link;
    /* Structure for */
    struct xorg_list notilink;
    tbm_bo bo[3]; 
} CAPTUREPortInfo, *CAPTUREPortInfoPtr;

struct xorg_list noti_data;


int  _CaptureVideoPutStill (CAPTUREPortInfoPtr pPort);
int  _CaptureVideoPutWB (CAPTUREPortInfoPtr pPort);
int  _CapturePutVideoOnly (CAPTUREPortInfoPtr pPort);
static Bool _CaptureVideoCompositeHW ( EXYNOSBufInfo *src,  EXYNOSBufInfo *dst,
                                int src_x, int src_y, int src_w, int src_h,
                                int dst_x, int dst_y, int dst_w, int dst_h,
                                int comp, int rotate);
static Bool _CaptureVideoCompositeSW (EXYNOSBufInfo *src, EXYNOSBufInfo *dst, int src_w, int src_h,
                          int dst_w, int dst_h, int comp,int rotate);


static EXYNOSBufInfo* _CaptureVideoGetBlackBuffer (CAPTUREPortInfoPtr pPort);

typedef struct _CaptureVideoPortInfo
{
    ClientPtr client;
    XvPortPtr pp;
} CaptureVideoPortInfo;

static CaptureVideoPortInfo* _exynosVideoStillGetClient (DrawablePtr pDraw);


int (*ddPutStill) (ClientPtr, DrawablePtr, struct _XvPortRec *, GCPtr,
                   INT16, INT16, CARD16, CARD16,
                   INT16, INT16, CARD16, CARD16);

static RESTYPE still_event_type;

static DevPrivateKeyRec still_client_key;
#define StillClientKey  (&still_client_key)
#define GetStillClientInfo(pDraw) ((CaptureVideoPortInfo*)dixLookupPrivate(&(pDraw)->devPrivates, StillClientKey))
#define DEV_INDEX   2
#define BUF_NUM_VIRTUAL 5
#define BUF_NUM_STREAM  3

enum
{
    CAPTURE_MODE_NONE,
    CAPTURE_MODE_STILL,
    CAPTURE_MODE_STREAM,
    CAPTURE_MODE_MAX,
};

enum
{
    COMP_SRC,
    COMP_OVER,
    COMP_MAX,
};

enum
{
    DATA_TYPE_UI,
    DATA_TYPE_VIDEO,
    DATA_TYPE_MAX,
};

typedef struct _VideoResource
{
    XID            id;
    RESTYPE        type;
    CAPTUREPortInfoPtr pPort;
    ScrnInfoPtr    pScrn;
} VideoResource;

static XF86ImageRec images[] =
{
    XVIMAGE_RGB32,
    XVIMAGE_SN12,
    XVIMAGE_SR16,
    XVIMAGE_ST12,
};

static XF86VideoEncodingRec capture_encoding[] =
{
    { 0, "XV_IMAGE", -1, -1, { 1, 1 } },
    { 1, "XV_IMAGE", 2560, 2560, { 1, 1 } },
};

static XF86VideoFormatRec capture_formats[] =
{
    { 16, TrueColor },
    { 24, TrueColor },
    { 32, TrueColor },
};

static XF86AttributeRec capture_attrs[] =
{
    { 0, 0, 0x7fffffff, "_USER_WM_PORT_ATTRIBUTE_FORMAT" },
    { 0, 0, CAPTURE_MODE_MAX, "_USER_WM_PORT_ATTRIBUTE_CAPTURE" },
    { 0, 0, 1, "_USER_WM_PORT_ATTRIBUTE_DISPLAY" },
    { 0, 0, 1, "_USER_WM_PORT_ATTRIBUTE_ROTATE_OFF" },
    { 0, 0, DATA_TYPE_MAX, "_USER_WM_PORT_ATTRIBUTE_DATA_TYPE" },
    { 0, 0, 1, "_USER_WM_PORT_ATTRIBUTE_SWCOMPOSITE" },
    { 0, 0, 0x7fffffff, "_USER_WM_PORT_ATTRIBUTE_RETURN_BUFFER" },
    { 0, 0, 1, "_USER_WM_PORT_ATTRIBUTE_STREAM_OFF" },
    { 0, 0, 1, "_USER_WM_PORT_ATTRIBUTE_FPS" },
    
};


typedef enum
{
    PAA_MIN,
    PAA_FORMAT,
    PAA_CAPTURE,
    PAA_DISPLAY,
    PAA_ROTATE_OFF,
    PAA_DATA_TYPE,
    PAA_SOFTWARECOMPOSITE_ON,
    PAA_RETBUF,
    PAA_STREAM_OFF,
    PAA_FPS,        
    PAA_MAX
} CAPTUREPortAttrAtom;

static struct
{
    CAPTUREPortAttrAtom  paa;
    const char          *name;
    Atom                 atom;
} capture_atoms[] =
{
    { PAA_FORMAT, "_USER_WM_PORT_ATTRIBUTE_FORMAT", None },
    { PAA_CAPTURE, "_USER_WM_PORT_ATTRIBUTE_CAPTURE", None },
    { PAA_DISPLAY, "_USER_WM_PORT_ATTRIBUTE_DISPLAY", None },
    { PAA_ROTATE_OFF, "_USER_WM_PORT_ATTRIBUTE_ROTATE_OFF", None },
    { PAA_DATA_TYPE, "_USER_WM_PORT_ATTRIBUTE_DATA_TYPE", None },
    { PAA_SOFTWARECOMPOSITE_ON, "_USER_WM_PORT_ATTRIBUTE_SWCOMPOSITE", None },
    { PAA_RETBUF, "_USER_WM_PORT_ATTRIBUTE_RETURN_BUFFER", None },
    { PAA_STREAM_OFF, "_USER_WM_PORT_ATTRIBUTE_STREAM_OFF", None },
    { PAA_FPS, "_USER_WM_PORT_ATTRIBUTE_FPS", None },
};



#define NUM_IMAGES        (sizeof(images) / sizeof(images[0]))

#define CAPTURE_FORMATS_NUM (sizeof(capture_formats) / sizeof(capture_formats[0]))
#define CAPTURE_ATTRS_NUM   (sizeof(capture_attrs) / sizeof(capture_attrs[0]))
#define CAPTURE_ATOMS_NUM   (sizeof(capture_atoms) / sizeof(capture_atoms[0]))

static EXYNOSBufInfoPtr
_CaptureBufferGetInfo (PixmapPtr pPixmap)
{
    EXYNOSBufInfoPtr bufinfo = NULL;
    tbm_bo tbo = NULL;

    tbo = exynosExaPixmapGetBo (pPixmap);
    XDBG_RETURN_VAL_IF_FAIL(tbo != NULL, NULL);

    tbm_bo_get_user_data (tbo, TBM_BODATA_VPIXMAP, (void**) &bufinfo);

    return bufinfo;
}

static Atom
_exynosVideoCaptureGetPortAtom (CAPTUREPortAttrAtom paa)
{
    int i;

    XDBG_RETURN_VAL_IF_FAIL (paa > PAA_MIN && paa < PAA_MAX, None);

    for (i = 0; i < CAPTURE_ATOMS_NUM; i++)
    {
        if (paa == capture_atoms[i].paa)
        {
            if (capture_atoms[i].atom == None)
                capture_atoms[i].atom = MakeAtom (capture_atoms[i].name,
                                                  strlen (capture_atoms[i].name), TRUE);

            return capture_atoms[i].atom;
        }
    }

    ErrorF ("Error: Unknown Port Attribute Name!\n");

    return None;
}

static RetBufInfo*
_CaptureVideoFindReturnBuf (CAPTUREPortInfoPtr pPort, unsigned int name)
{
    RetBufInfo *cur = NULL, *next = NULL;

    XDBG_DEBUG (MVA, "_CaptureVideoFindReturnBuf\n");

    XDBG_RETURN_VAL_IF_FAIL (pPort->capture != CAPTURE_MODE_STILL, NULL);

    f_xorg_list_for_each_entry_safe (cur, next, &pPort->retbuf_info, link)
    {
        if (cur->vbuf && cur->vbuf->names[0] == name)
            return cur;
    }

    return NULL;
}

static Bool
_CaptureVideoAddReturnBuf (CAPTUREPortInfoPtr pPort, EXYNOSBufInfo *vbuf)
{
    RetBufInfo *info;
    XDBG_DEBUG (MVA, "_CaptureVideoAddReturnBuf\n");
    XDBG_RETURN_VAL_IF_FAIL (pPort->capture != CAPTURE_MODE_STILL, FALSE);

    info = _CaptureVideoFindReturnBuf (pPort, vbuf->names[0]);
    XDBG_RETURN_VAL_IF_FAIL (info == NULL, FALSE);

    info = ctrl_calloc (1, sizeof (RetBufInfo));
    XDBG_RETURN_VAL_IF_FAIL (info != NULL, FALSE);

    vbuf->ref_cnt++;
    info->vbuf = vbuf;
    info->vbuf->showing = TRUE;

    XDBG_DEBUG (MVA, "retbuf (%ld,%d,%d,%d) added.\n", vbuf->stamp,
                vbuf->names[0], vbuf->names[1], vbuf->names[2]);

    f_xorg_list_add (&info->link, &pPort->retbuf_info);

    return TRUE;
}



static void
_exynosCaptureVideoRemoveReturnBuf (CAPTUREPortInfoPtr pPort, RetBufInfo *info)
{
    XDBG_RETURN_IF_FAIL (pPort->capture != CAPTURE_MODE_STILL);
    XDBG_RETURN_IF_FAIL (info != NULL);
    XDBG_RETURN_IF_FAIL (info->vbuf != NULL);
    XDBG_DEBUG (MVA, "_exynosCaptureVideoRemoveReturnBuf\n");
    info->vbuf->showing = FALSE;

    XDBG_DEBUG (MVA, "retbuf (%ld,%d,%d,%d) removed.\n", info->vbuf->stamp,
                info->vbuf->names[0], info->vbuf->names[1], info->vbuf->names[2]);

    if (pPort->clone)
        CloneQueue (pPort->clone, info->vbuf);

    /* decrease refcnt and ctrl_free video buffer*/
    info->vbuf->ref_cnt--;
    /*if (info->vbuf->ref_cnt == 0)
        FreeVideoBuffer (vbuf, func);*/
    /* ---------------------------------    */
    f_xorg_list_del (&info->link);
    ctrl_free (info);
}

static int
EXYNOSCaptureGetPortAttribute (ScrnInfoPtr pScrn,
                               Atom        attribute,
                               INT32      *value,
                               pointer     data)
{
    CAPTUREPortInfoPtr pPort = (CAPTUREPortInfoPtr) data;
    XDBG_DEBUG (MVA, "GetAttribute:  \n" );
    if (attribute == _exynosVideoCaptureGetPortAtom (PAA_ROTATE_OFF))
    {
        *value = pPort->rotate_off;
        return Success;
    }
    else if (attribute == _exynosVideoCaptureGetPortAtom (PAA_FORMAT))
    {
        *value = pPort->id;
        return Success;
    }
    else if (attribute == _exynosVideoCaptureGetPortAtom (PAA_CAPTURE))
    {
        *value = pPort->capture;
        return Success;
    }
     else if (attribute == _exynosVideoCaptureGetPortAtom (PAA_DATA_TYPE))
    {
        *value = pPort->data_type;
        return Success;
    }
     else if (attribute == _exynosVideoCaptureGetPortAtom (PAA_SOFTWARECOMPOSITE_ON))
    {
        *value = pPort->swcomp_on;
        return Success;
    }
    else if (attribute == _exynosVideoCaptureGetPortAtom (PAA_DISPLAY))
    {
        return Success;
    }
    else if (attribute == _exynosVideoCaptureGetPortAtom (PAA_STREAM_OFF))
    {
        return Success;
    }
    else if (attribute == _exynosVideoCaptureGetPortAtom (PAA_FPS))
    {
        return Success;
    }


    return BadMatch;
}

static int
EXYNOSCaptureSetPortAttribute (ScrnInfoPtr pScrn,
                               Atom        attribute,
                               INT32       value,
                               pointer     data)
{
    CAPTUREPortInfoPtr pPort = (CAPTUREPortInfoPtr) data;
    if (attribute == _exynosVideoCaptureGetPortAtom(PAA_FORMAT))
    {
        XDBG_INFO (MVA, "**** SetPortAttribute PAA_FORMAT****.. \n");

        pPort->id = (unsigned int)FOURCC_RGB32;//value;
        XDBG_DEBUG (MVA, "id(%d) \n", value);
        return Success;
    }
    else if (attribute == _exynosVideoCaptureGetPortAtom(PAA_CAPTURE))
    {
        XDBG_INFO (MVA, "**** SetPortAttribute PAA_CAPTURE ****.. \n");
        if (value < CAPTURE_MODE_NONE || value >= CAPTURE_MODE_MAX)
        {
            XDBG_ERROR (MVA, "capture value(%d) is out of range\n", value);
            return BadRequest;
        }

        pPort->capture = value;
        XDBG_DEBUG (MVA, "capture(%d) \n", pPort->capture);
        return Success;
    }
    else if (attribute == _exynosVideoCaptureGetPortAtom(PAA_ROTATE_OFF))
    {
        pPort->rotate_off = value;
        return Success;

    }
    else if (attribute == _exynosVideoCaptureGetPortAtom(PAA_SOFTWARECOMPOSITE_ON))
    {
        pPort->swcomp_on = value;
        return Success;

    }
    else if (attribute == _exynosVideoCaptureGetPortAtom (PAA_RETBUF))
    {
        RetBufInfo *info;

        if (!pPort->pDraw)
            return Success;
        XDBG_INFO (MVA, "SET ATTRIBUTE: RETURN BUFFER! \n");
        info = _CaptureVideoFindReturnBuf (pPort, value);
        if (!info)
        {
            XDBG_WARNING (MVA, "wrong gem name(%d) returned\n", value);
            return Success;
        }

        if (info->vbuf && info->vbuf->need_reset)
            exynosUtilClearNormalVideoBuffer (info->vbuf);

        _exynosCaptureVideoRemoveReturnBuf (pPort, info);

        return Success;
    } /*Dummies for PAA_STREAM_OFF and PAA_FPS atoms */
     else if (attribute == _exynosVideoCaptureGetPortAtom(PAA_STREAM_OFF))
    {
        return Success;
    }
     else if (attribute == _exynosVideoCaptureGetPortAtom(PAA_FPS))
    {
        return Success;
    }


    return BadMatch;
}
static void
EXYNOSCaptureQueryBestSize (ScrnInfoPtr pScrn,
                            Bool motion,
                            short vid_w, short vid_h,
                            short dst_w, short dst_h,
                            unsigned int *p_w, unsigned int *p_h,
                            pointer data)
{
    CAPTUREPortInfoPtr pPort = (CAPTUREPortInfoPtr) data;
    XDBG_DEBUG (MVA, "****Capture Adaptor VideoQueryBestSize ***** \n");
    if (!pPort->capture)
    {
        if (p_w)
            *p_w = pScrn->virtualX;
        if (p_h)
            *p_h = pScrn->virtualY;
    }
    else
    {
        if (p_w)
            *p_w = (unsigned int)dst_w;
        if (p_h)
            *p_h = (unsigned int)dst_h;
    }
}

static CARD32
_CaptureVideoRetireTimeout (OsTimerPtr timer, CARD32 now, pointer arg)
{
    CAPTUREPortInfoPtr pPort = (CAPTUREPortInfoPtr) arg;
    int diff;

    if (!pPort)
        return 0;

    if (pPort->retire_timer)
    {
        TimerFree (pPort->retire_timer);
        pPort->retire_timer = NULL;
    }

    XDBG_ERROR (MVA, "capture(%d)  type(%d) status(%x). \n",
                pPort->capture, pPort->data_type, pPort->status);

    diff = GetTimeInMillis () - pPort->retire_time;
    XDBG_ERROR (MVA, "failed : +++ Retire Timeout!! diff(%d)\n", diff);

    return 0;
}

static int
_CaptureVideoPreProcess (ScrnInfoPtr pScrn, CAPTUREPortInfoPtr pPort,
                         RegionPtr clipBoxes, DrawablePtr pDraw)
{
    if (pPort->pScrn == pScrn && pPort->pDraw == pDraw &&
            pPort->clipBoxes && clipBoxes && RegionEqual (pPort->clipBoxes, clipBoxes))
        return Success;

    pPort->pScrn = pScrn;
    pPort->pDraw = pDraw;

    if (clipBoxes)
    {
        if (!pPort->clipBoxes)
            pPort->clipBoxes = RegionCreate (NULL, 1);

        XDBG_RETURN_VAL_IF_FAIL (pPort->clipBoxes != NULL, BadAlloc);

        RegionCopy (pPort->clipBoxes, clipBoxes);
    }

    XDBG_TRACE (MVA, "pDraw(0x%x, %dx%d). \n", pDraw->id, pDraw->width, pDraw->height);

    return Success;
}

static int
_CaptureVideoAddDrawableEvent (CAPTUREPortInfoPtr pPort)
{
    VideoResource *resource;
    void *ptr;
    int ret = 0;

    XDBG_RETURN_VAL_IF_FAIL (pPort->pScrn != NULL, BadImplementation);
    XDBG_RETURN_VAL_IF_FAIL (pPort->pDraw != NULL, BadImplementation);

    ptr = NULL;
    ret = dixLookupResourceByType (&ptr, pPort->pDraw->id, still_event_type, NULL, DixWriteAccess);
    if (ret == Success)
        return Success;

    resource = ctrl_malloc (sizeof (VideoResource));
    if (resource == NULL)
        return BadAlloc;

    if (!AddResource (pPort->pDraw->id, still_event_type, resource))
    {
        ctrl_free (resource);
        return BadAlloc;
    }

    XDBG_TRACE (MVA, "id(0x%08lx). \n", pPort->pDraw->id);

    resource->id = pPort->pDraw->id;
    resource->type = still_event_type;
    resource->pPort = pPort;
    resource->pScrn = pPort->pScrn;

    return Success;
}

static PixmapPtr
_CaptureVideoGetPixmap (DrawablePtr pDraw)
{
    if (pDraw->type == DRAWABLE_WINDOW)
        return pDraw->pScreen->GetWindowPixmap ((WindowPtr) pDraw);
    else
        return (PixmapPtr) pDraw;
}


/* vid_x, vid_y, vid_w, vid_h : no meaning for us. not using.
 * drw_x, drw_y, dst_w, dst_h : no meaning for us. not using.
 * Only pDraw's size is used.
 */
static
int
EXYNOSCapturePutStill (ScrnInfoPtr pScrn,
                       short vid_x, short vid_y, short drw_x, short drw_y,
                       short vid_w, short vid_h, short drw_w, short drw_h,
                       RegionPtr clipBoxes, pointer data, DrawablePtr pDraw )
{
#ifdef DEBUG_CAPTURE  
    XDBG_INFO (MVA, " pScrn(%p) vid_x(%d) vid_y(%d) drw_x(%d)  drw_y(%d) vid_w(%d), vid_h(%d),/"
               "drw_w(%d),drw_h(%d),clipBoxes(%p), pointer data(%x), DrawablePtr pDraw(%x)\n",
               pScrn, vid_x, vid_y, drw_x, drw_y, vid_w, vid_h, drw_w, drw_h, clipBoxes, data, pDraw);
#endif
    CAPTUREPortInfoPtr pPort = (CAPTUREPortInfoPtr) data;
    pPort->pScrn = pScrn;
    /*_exynosScrnPriv -> have pointer to  display info  EXYNOSDispInfoPtr pDispInfo;*/
    int ret = BadRequest;

    /* we support only RGB32 for capture */
    pPort->id = FOURCC_RGB32;
    XDBG_GOTO_IF_FAIL (pPort->need_damage == FALSE, put_still_fail);

    /*if retire timer is running = free him */
    if (pPort->retire_timer)
    {
        TimerFree (pPort->retire_timer);
        pPort->retire_timer = NULL;
    }
    pPort->retire_timer = TimerSet (pPort->retire_timer, 0, 4000,
                                    _CaptureVideoRetireTimeout,
                                    pPort);

    XDBG_GOTO_IF_FAIL (pPort->id > 0, put_still_fail);
    pPort->status = 0;
    pPort->retire_time = GetTimeInMillis ();

    ret = _CaptureVideoPreProcess (pScrn, pPort, clipBoxes, pDraw);
    XDBG_GOTO_IF_FAIL (ret == Success, put_still_fail);

    ret = _CaptureVideoAddDrawableEvent (pPort);

    XDBG_GOTO_IF_FAIL (ret == Success, put_still_fail);

    /* check drawable */
    XDBG_RETURN_VAL_IF_FAIL (pDraw->type == DRAWABLE_PIXMAP, BadPixmap);

    if (!pPort->putstill_on)
    {
        pPort->putstill_on = TRUE;
#ifdef DEBUG_CAPTURE
        XDBG_INFO (MVA, "pPort(%d) putstill on., capture(%d), format(%c%c%c%c), fps(%d)\n",
                   pPort->index, pPort->capture, FOURCC_STR (pPort->id), pPort->wb_fps);
#endif
    }

    pPort->need_damage = TRUE;

    if (pPort->capture == CAPTURE_MODE_STILL)
    {
        XDBG_DEBUG(MVA, "still mode.\n");
        ret = _CaptureVideoPutStill(pPort);
        XDBG_GOTO_IF_FAIL(ret == Success, put_still_fail);
        XDBG_DEBUG(MVA, "*** SUCCESS EXIT EXYNOSVideoCapturePutStill *** \n");
        return Success;
    }

    else if (pPort->capture == CAPTURE_MODE_STREAM)
    {
        /* in case of capture mode, we don't support video-only */
        if (pPort->capture)
            pPort->data_type = DATA_TYPE_UI;

        if (pPort->data_type == DATA_TYPE_UI)
        {
            XDBG_DEBUG(MVA, "clone mode.  DATA_TYPE_UI\n");

            ret = _CaptureVideoPutWB(pPort);
            if (ret != Success)
                goto put_still_fail;
        }
        else
        {
            XDBG_DEBUG(MVA, "video only mode.\n");
            ret = _CapturePutVideoOnly (pPort);
            if (ret != Success)
                goto put_still_fail;
        }


    }
put_still_fail:

    if (pPort->retire_timer)
    {
        TimerFree (pPort->retire_timer);
        pPort->retire_timer = NULL;
    }

    XDBG_DEBUG (MVA, "* EXIT EXYNOSVideoCapturePutStill * \n");

    return ret;


}

static void
_CaptureCloneVideoDraw (CAPTUREPortInfoPtr pPort, EXYNOSBufInfo *buf)
{
    XDBG_RETURN_IF_FAIL (pPort->capture != CAPTURE_MODE_STILL);
    XDBG_DEBUG (MVA, " _CaptureCloneVideoDraw buf(%p)  \n", buf);
    if (pPort->retire_timer)
    {
        TimerFree (pPort->retire_timer);
        pPort->retire_timer = NULL;
    }

    if (!pPort->pDraw)
    {
        XDBG_TRACE (MVA, "drawable gone!\n");
        return;
    }

    XDBG_RETURN_IF_FAIL (pPort->need_damage == TRUE);
    XDBG_GOTO_IF_FAIL (buf?TRUE:FALSE, draw_done);
#ifdef TRACE_CAPTURE
    XDBG_TRACE (MVA, "__FUNCTION__   %c%c%c%c, %dx%d. \n",
                FOURCC_STR (buf->fourcc), buf->width, buf->height);
#endif
    if (pPort->id == FOURCC_RGB32)
    {
        PixmapPtr pPixmap = _CaptureVideoGetPixmap (pPort->pDraw);
        tbm_bo_handle bo_handle;

        if (pPort->pDraw->width != buf->width || pPort->pDraw->height != buf->height)
        {
            XDBG_ERROR (MVA, "not matched. (%dx%d != %dx%d) \n",
                        pPort->pDraw->width, pPort->pDraw->height,
                        buf->width, buf->height);
            goto draw_done;
        }

        bo_handle = tbm_bo_map (buf->tbos[0], TBM_DEVICE_CPU, TBM_OPTION_READ);
        XDBG_GOTO_IF_FAIL (bo_handle.ptr != NULL, draw_done);
        XDBG_GOTO_IF_FAIL (buf->size > 0, draw_done);

        EXYNOSExaPrepareAccess (pPixmap, EXA_PREPARE_DEST);

        if (pPixmap->devPrivate.ptr)
        {
            XDBG_DEBUG (MVA, "vir(%p) size(%d) => pixmap(%p)\n",
                        bo_handle.ptr, buf->size, pPixmap->devPrivate.ptr);

            memcpy (pPixmap->devPrivate.ptr, bo_handle.ptr, buf->size);
        }

        EXYNOSExaFinishAccess (pPixmap, EXA_PREPARE_DEST);

        tbm_bo_unmap (buf->tbos[0]);
    }
    else /* FOURCC_ST12 */
    {
        PixmapPtr pPixmap = _CaptureVideoGetPixmap (pPort->pDraw);
        XV_DATA putstill_data = {0,};

        XV_INIT_DATA (&putstill_data);

        if (buf->names[0] > 0)
        {
            putstill_data.YBuf =  buf->names[0]; // buf->phy_addrs[0];
            putstill_data.CbBuf = buf->names[1]; // buf->phy_addrs[1];
            putstill_data.CrBuf = buf->names[2]; //buf->phy_addrs[2];

            putstill_data.BufType = XV_BUF_TYPE_LEGACY;
        }
        else
        {
            putstill_data.YBuf =  buf->handles[0].u32;
            putstill_data.CbBuf = buf->handles[1].u32;
            putstill_data.CrBuf = buf->handles[2].u32;

            putstill_data.BufType = XV_BUF_TYPE_DMABUF;
        }

#if 0
        _buffers (pPort);
        ErrorF ("[Xorg] send : %d (%s)\n", putstill_data.YBuf, buffers);
#endif
#ifdef DEBUG_CAPTURE
        XDBG_DEBUG (MVA, "__FUNCTION__ still_data(%d,%d,%d) type(%d) index(%d) \n",
                    putstill_data.YBuf, putstill_data.CbBuf, putstill_data.CrBuf,
                    putstill_data.BufType, putstill_data.BufType);
#endif
        
        EXYNOSExaPrepareAccess (pPixmap, EXA_PREPARE_DEST);

        if (pPixmap->devPrivate.ptr)
            memcpy (pPixmap->devPrivate.ptr, &putstill_data, sizeof (XV_DATA));

        EXYNOSExaFinishAccess (pPixmap, EXA_PREPARE_DEST);

        if (pPort->capture == CAPTURE_MODE_NONE)
            _CaptureVideoAddReturnBuf (pPort, buf);
    }

draw_done:
    DamageDamageRegion (pPort->pDraw, pPort->clipBoxes);
    pPort->need_damage = FALSE;
}

static void
_CaptureVideoWbStopFunc (EXYNOS_CloneObject* clone, EXYNOS_CLONE_NOTIFY noti, void *noti_data, void *user_data)
{
    CAPTUREPortInfoPtr pPort = (CAPTUREPortInfoPtr)user_data;
    XDBG_INFO (MVA, " Notify WB_NOTI_STOP   _CaptureVideoWbStopFunc \n");
    if (!pPort)
        return;

    if (pPort->need_damage)
    {
        EXYNOSBufInfo *black = _CaptureVideoGetBlackBuffer (pPort);
        XDBG_TRACE (MVA, "black buffer(%d) return: wb stop\n", (black) ? black->names[0] : 0);
        _CaptureCloneVideoDraw (pPort, black);
    }
}

void
_CaptureVideoWbDumpFunc (EXYNOS_CloneObject* clone, EXYNOS_CLONE_NOTIFY noti, void *noti_data, void *user_data)
{
    CAPTUREPortInfoPtr pPort = (CAPTUREPortInfoPtr)user_data;
    EXYNOSBufInfo *vbuf = (EXYNOSBufInfo*) noti_data;
    XDBG_INFO (MVA, " Notify WB_NOTI_STOP   _CaptureVideoWbDumpFunc \n");

    XDBG_RETURN_IF_FAIL (pPort != NULL);
    XDBG_RETURN_IF_FAIL (pPort->capture != CAPTURE_MODE_STILL);
    XDBG_RETURN_IF_FAIL (vbuf->showing == FALSE);

    if (pPort->need_damage)
        _CaptureCloneVideoDraw (pPort, vbuf);
    return;
}


static void
_CaptureVideoWbCloseFunc (EXYNOS_CloneObject* clone, EXYNOS_CLONE_NOTIFY noti, void *noti_data, void *user_data)
{
    CAPTUREPortInfoPtr pPort = (CAPTUREPortInfoPtr)user_data;
    XDBG_INFO (MVA, " Notify WB_NOTI_STOP   _CaptureVideoWbCloseFunc \n");
    if (!pPort)
        return;
    pPort->clone = NULL;
}

void
_CaptureVideoWbDumpDoneFunc (EXYNOS_CloneObject* clone, EXYNOS_CLONE_NOTIFY noti, void *noti_data, void *user_data)
{
    CAPTUREPortInfoPtr pPort = (CAPTUREPortInfoPtr)user_data;
    XDBG_INFO (MVA, " Notify WB_NOTI_STOP   _CaptureVideoWbDumpDoneFunc \n");
    if (!pPort)
        return;

    XDBG_DEBUG (MVA, "close Clone after ipp event done\n");

    XDBG_RETURN_IF_FAIL (pPort->clone != NULL);

    exynosCloneRemoveNotifyFunc (pPort->clone, _CaptureVideoWbStopFunc);
    exynosCloneRemoveNotifyFunc (pPort->clone, _CaptureVideoWbDumpFunc);
    exynosCloneRemoveNotifyFunc (pPort->clone, _CaptureVideoWbDumpDoneFunc);
    exynosCloneRemoveNotifyFunc (pPort->clone, _CaptureVideoWbCloseFunc);

    exynosCloneClose (pPort->clone);
    pPort->clone = NULL;
}

static void
EXYNOSCaptureStop (ScrnInfoPtr pScrn, pointer data, Bool exit)
{
    CAPTUREPortInfoPtr pPort = (CAPTUREPortInfoPtr) data;
    XDBG_DEBUG (MVA, "**** Capture Adaptor EXYNOSVideoCaptureStop **** \n");
    /* stream off and clear buffer*/
    if (pPort->retire_timer)
    {
        TimerFree (pPort->retire_timer);
        pPort->retire_timer = NULL;
    }
    if (pPort->clone)
    {
        exynosCloneRemoveNotifyFunc (pPort->clone, _CaptureVideoWbStopFunc);
        exynosCloneRemoveNotifyFunc (pPort->clone, _CaptureVideoWbDumpFunc);
        exynosCloneRemoveNotifyFunc (pPort->clone, _CaptureVideoWbDumpDoneFunc);
        exynosCloneRemoveNotifyFunc (pPort->clone, _CaptureVideoWbCloseFunc);

        exynosCloneClose (pPort->clone);
        pPort->clone = NULL;
    }
    if (pPort->need_damage)
    {
        /* all callbacks has been removed from clone. We can't expect
         * any event. So we send black image at the end.
         */
        EXYNOSBufInfo *black = _CaptureVideoGetBlackBuffer (pPort);
        //XDBG_TRACE (MVA, "black buffer(%d) return: stream off\n", (black)?black->keys[0]:0);
        _CaptureCloneVideoDraw (pPort, black);
    }
    if (pPort->clipBoxes)
    {
        RegionDestroy (pPort->clipBoxes);
        pPort->clipBoxes = NULL;
    }
    pPort->pDraw = NULL;
    pPort->capture = CAPTURE_MODE_NONE;
    pPort->id = FOURCC_RGB32;
    pPort->data_type = DATA_TYPE_UI;
    pPort->swcomp_on = FALSE;
    if (pPort->putstill_on)
    {
        pPort->putstill_on = FALSE;
        XDBG_INFO (MVA, "pPort(%d) putstill off. \n", pPort->index);
    }

}

static int
_CaptureVideoRegisterStillEventGone (void *data, XID id)
{
    VideoResource *resource = (VideoResource*)data;

    XDBG_TRACE (MVA, "id(0x%08lx). \n", id);

    if (!resource)
        return Success;

    if (!resource->pPort || !resource->pScrn)
        return Success;

    resource->pPort->pDraw = NULL;

    EXYNOSCaptureStop (resource->pScrn, (pointer)resource->pPort, 1);

    ctrl_free(resource);

    return Success;
}




XF86VideoAdaptorPtr
exynosVideoCaptureSetup (ScreenPtr pScreen)
{
    XF86VideoAdaptorPtr pAdaptor;
    CAPTUREPortInfoPtr pPort;
    int i = 0;
    TRACE;
    pAdaptor = calloc (1, sizeof (XF86VideoAdaptorRec) +
                       (sizeof (DevUnion) + sizeof (CAPTUREPortInfo)) * CAPTURE_MAX_PORT);
    if (!pAdaptor)
        return NULL;

    capture_encoding[0].width = pScreen->width;
    capture_encoding[0].height = pScreen->height;

    pAdaptor->type = XvWindowMask | XvPixmapMask | XvInputMask | XvStillMask;
    pAdaptor->flags = 0;
    pAdaptor->name = "EXYNOS Capture Video";
    pAdaptor->nEncodings = sizeof (capture_encoding) / sizeof (XF86VideoEncodingRec);
    pAdaptor->pEncodings = capture_encoding;
    pAdaptor->nFormats = CAPTURE_FORMATS_NUM;
    pAdaptor->pFormats = capture_formats;
    pAdaptor->nPorts = CAPTURE_MAX_PORT;
    pAdaptor->pPortPrivates = (DevUnion*)(&pAdaptor[1]);

    pPort = (CAPTUREPortInfoPtr) (&pAdaptor->pPortPrivates[CAPTURE_MAX_PORT]);

    for (i = 0; i < CAPTURE_MAX_PORT; i++)
    {
        pAdaptor->pPortPrivates[i].ptr = &pPort[i];
        pPort[i].index = i;
        pPort[i].id = FOURCC_RGB32;

        xorg_list_init (&pPort[i].retbuf_info);
        xorg_list_init (&noti_data);
        pPort->swcomp_on = FALSE;
    }

    pAdaptor->nAttributes = CAPTURE_ATTRS_NUM;
    pAdaptor->pAttributes = capture_attrs;
    pAdaptor->nImages = NUM_IMAGES;
    pAdaptor->pImages = images;

    pAdaptor->GetPortAttribute     = EXYNOSCaptureGetPortAttribute;
    pAdaptor->SetPortAttribute     = EXYNOSCaptureSetPortAttribute;
    pAdaptor->QueryBestSize        = EXYNOSCaptureQueryBestSize;
    pAdaptor->PutStill             = EXYNOSCapturePutStill;
    pAdaptor->StopVideo            = EXYNOSCaptureStop;

    still_event_type = CreateNewResourceType (_CaptureVideoRegisterStillEventGone,
                       "Capture Video Drawable");
    if (!still_event_type)
    {
        ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "Failed to register EventResourceTypes. \n");
        return NULL;
    }

    return pAdaptor;
}




static CaptureVideoPortInfo*
_exynosVideoStillGetClient (DrawablePtr pDraw)
{
    if (!pDraw)
        return NULL;

    if (pDraw->type == DRAWABLE_WINDOW)
        return GetStillClientInfo ((WindowPtr)pDraw);
    else
        return GetStillClientInfo ((PixmapPtr)pDraw);
}

static int
EXYNOSVideoCaptureDDPutStill (ClientPtr client,
                              DrawablePtr pDraw,
                              XvPortPtr pPort,
                              GCPtr pGC,
                              INT16 vid_x, INT16 vid_y,
                              CARD16 vid_w, CARD16 vid_h,
                              INT16 drw_x, INT16 drw_y, CARD16 drw_w, CARD16 drw_h)
{
    CaptureVideoPortInfo *info = _exynosVideoStillGetClient (pDraw);
    int ret;
    XDBG_INFO (MVA, "EXYNOSVideoCaptureDDPutStill\n");
    if (info)
    {
        info->client = client;
        info->pp = pPort;
    }

    ret = ddPutStill (client, pDraw, pPort, pGC,
                      vid_x, vid_y, vid_w, vid_h,
                      drw_x, drw_y, drw_w, drw_h);
    return ret;
}

void
exynosVideoImageReplacePutStillFunc (ScreenPtr pScreen)
{
    int i;

    XvScreenPtr xvsp = dixLookupPrivate (&pScreen->devPrivates,
                                         XvGetScreenKey());
    if (!xvsp)
        return;

    for (i = 0; i < xvsp->nAdaptors; i++)
    {
        XvAdaptorPtr pAdapt = xvsp->pAdaptors + i;
        if (pAdapt->ddPutStill)
        {
            ddPutStill = pAdapt->ddPutStill;
            pAdapt->ddPutStill = EXYNOSVideoCaptureDDPutStill;
            break;
        }
    }

    if (!dixRegisterPrivateKey (StillClientKey, PRIVATE_WINDOW, sizeof (ClientPtr)))
        return;
    if (!dixRegisterPrivateKey (StillClientKey, PRIVATE_PIXMAP, sizeof (ClientPtr)))
        return;
}

static void
_exynosVideoBufferDestroyOutbuf (pointer data)
{
  XDBG_DEBUG (MVA, "***** Destroy OUtbuf Ok!*****\n");
 
}
  
static tbm_bo
_CaptureVideoGetFrontBo2 (CAPTUREPortInfoPtr pPort, int connector_type, tbm_bo_handle* outbo_handle)
{
    ScrnInfoPtr pScrn = pPort->pScrn;
    tbm_bo_handle bo_handle;
    
    drmModeFBPtr CurFBPTR=NULL;
    struct drm_gem_flink arg_flink = {0,};

    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    XDBG_RETURN_VAL_IF_FAIL(pExynos != NULL, NULL);
    
    xf86CrtcConfigPtr pCrtcConfig = XF86_CRTC_CONFIG_PTR (pScrn);
    xf86CrtcPtr pCrtc = pCrtcConfig->crtc[0];
    EXYNOSCrtcPrivPtr pCrtcPriv = (EXYNOSCrtcPrivPtr)pCrtc->driver_private;
     
    CurFBPTR = drmModeGetFB (pExynos->drm_fd, pCrtcPriv->fb_id);
    XDBG_RETURN_VAL_IF_FAIL(CurFBPTR != NULL, NULL);
        if (!CurFBPTR->fb_id)
        {
            XDBG_DEBUG (MVA, "fail to get fb \n" );
            return NULL;
        }
    
    arg_flink.handle = CurFBPTR->handle;

    if (pExynos->drm_fd)
        if (drmIoctl (pExynos->drm_fd, DRM_IOCTL_GEM_FLINK, &arg_flink) < 0)
        {
            XDBG_ERRNO(MMPL, "DRM_IOCTL_GEM_FLINK failed. %U\n", arg_flink.handle);
            return FALSE;
        }
    
#ifdef DEBUG_CAPTURE
   XDBG_DEBUG (MVA, "pScrn=(%p), pCrtc->mode.HDisplay=(%d) pCrtc->mode.VDisplay=(%d) pScrn->bitsPerPixel=[%d] \n",
                pScrn, pCrtc->mode.HDisplay, pCrtc->mode.VDisplay, pScrn->bitsPerPixel);
#endif
    
    bo_handle.u32 = CurFBPTR->handle;     
    *outbo_handle = bo_handle;
#ifdef DEBUG_CAPTURE
     XDBG_DEBUG (MVA, "_CaptureVideoGetFrontBo2 exit  pExynos->default_bo=[%p], pCrtcPriv->fb_id=(%d) bo_handle(%p) \n", pExynos->default_bo, pCrtcPriv->fb_id, bo_handle);
#endif
    return tbm_bo_import(pExynos->bufmgr, arg_flink.name);

}

#if 0
tbm_bo
_CaptureVideoGetFrontBo (CAPTUREPortInfoPtr pPort, int connector_type, tbm_bo_handle* outbo_handle)
{
    ScrnInfoPtr pScrn = pPort->pScrn;
    tbm_bo_handle bo_handle;

    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    XDBG_RETURN_VAL_IF_FAIL(pExynos->default_bo != NULL, NULL);
    
    xf86CrtcConfigPtr pCrtcConfig = XF86_CRTC_CONFIG_PTR (pScrn);
    xf86CrtcPtr pCrtc = pCrtcConfig->crtc[0];
    EXYNOSCrtcPrivPtr pCrtcPriv = (EXYNOSCrtcPrivPtr)pCrtc->driver_private;
    
  
#ifdef DEBUG_CAPTURE
   XDBG_DEBUG (MVA, "pScrn=(%p), pCrtc->mode.HDisplay=(%d) pCrtc->mode.VDisplay=(%d) pScrn->bitsPerPixel=[%d] \n",
                pScrn, pCrtc->mode.HDisplay, pCrtc->mode.VDisplay, pScrn->bitsPerPixel);
#endif
    
    bo_handle = tbm_bo_get_handle(pExynos->default_bo, TBM_DEVICE_DEFAULT);
    *outbo_handle = bo_handle;

    XDBG_DEBUG (MVA, "_CaptureVideoGetFrontBo exit  pExynos->default_bo=[%p], pCrtcPriv->fb_id=(%d) bo_handle(%p) \n", pExynos->default_bo, pCrtcPriv->fb_id, bo_handle);

    return pExynos->default_bo;

}
#endif

static PixmapPtr
_CaptureVideoGetDrawableBuffer (CAPTUREPortInfoPtr pPort)
{
    ScrnInfoPtr pScrn;
    EXYNOSBufInfo *bufinfo = NULL;
    PixmapPtr pPixmap = NULL;
    tbm_bo_handle bo_handle;
    EXYNOSExaPixPrivRec *privPixmap;
    Bool need_finish = FALSE;
  
    CARD32 stamp;
    int proper_width, proper_height;

    XDBG_DEBUG (MVA, "_CaptureVideoGetDrawableBuffer  \n");

    pPixmap = _CaptureVideoGetPixmap (pPort->pDraw);
    XDBG_RETURN_VAL_IF_FAIL  (pPixmap != NULL, NULL);
    pScrn = xf86Screens[pPixmap->drawable.pScreen->myNum];
    XDBG_RETURN_VAL_IF_FAIL  (pScrn != NULL, NULL);
    
    /** @todo Create Valid_pixmap structure */
    if (_CaptureBufferGetInfo(pPixmap))
    {
        return exynosVideoBufferRef (pPixmap);
    }

    privPixmap = exaGetPixmapDriverPrivate (pPixmap);
    XDBG_GOTO_IF_FAIL (privPixmap != NULL, fail_get);

    bufinfo = calloc(1, sizeof(EXYNOSBufInfo));
    XDBG_GOTO_IF_FAIL(bufinfo != NULL, alloc_fail);
    bufinfo->pScrn = pScrn;
    bufinfo->fourcc = FOURCC_RGB32;
    bufinfo->width = pPixmap->drawable.width;
    bufinfo->height = pPixmap->drawable.height;
    bufinfo->crop.x = 0;
    bufinfo->crop.y = 0;
    bufinfo->crop.width = bufinfo->width;
    bufinfo->crop.height = bufinfo->height;
    proper_width = bufinfo->width;
    proper_height = bufinfo->height;

    bufinfo->size = exynosVideoImageAttrs(bufinfo->fourcc, &proper_width,
                                          &proper_height, bufinfo->pitches, bufinfo->offsets,
                                          bufinfo->lengths);
    XDBG_GOTO_IF_FAIL(bufinfo->size > 0, alloc_fail);
    XDBG_GOTO_IF_FAIL(bufinfo->width == proper_width, alloc_fail);
    XDBG_GOTO_IF_FAIL(bufinfo->height == proper_height, alloc_fail);

    bufinfo->tbos[0] = tbm_bo_ref(exynosExaPixmapGetBo(pPixmap));
    XDBG_GOTO_IF_FAIL(bufinfo->tbos[0] != NULL, alloc_fail);
    bufinfo->names[0] = tbm_bo_export(bufinfo->tbos[0]);
    XDBG_GOTO_IF_FAIL(bufinfo->names[0] > 0, alloc_fail);
    bo_handle = tbm_bo_get_handle(bufinfo->tbos[0], TBM_DEVICE_DEFAULT);
    XDBG_GOTO_IF_FAIL(bo_handle.u32 > 0, alloc_fail);

    bufinfo->handles[0].u32 = bo_handle.u32;

    EXYNOSExaPrepareAccess (pPixmap, EXA_PREPARE_DEST);
    memset (pPixmap->devPrivate.ptr, 0x0, bufinfo->size);
    EXYNOSExaFinishAccess (pPixmap, EXA_PREPARE_DEST);
    exynosMemPoolCacheFlush (pPort->pScrn);

    stamp = pPixmap->drawable.serialNumber;//GetTimeInMillis();

    bufinfo->stamp =  (CARD32) stamp;
    bufinfo->ref_cnt = 1;

    XDBG_GOTO_IF_FAIL (
        tbm_bo_add_user_data (bufinfo->tbos[0], TBM_BODATA_VPIXMAP,
                             _exynosVideoBufferDestroyOutbuf),
        alloc_fail); 
    XDBG_GOTO_IF_FAIL (
        tbm_bo_set_user_data (bufinfo->tbos[0], TBM_BODATA_VPIXMAP, (void*) bufinfo),
        alloc_fail);
#ifdef DEBUG_CAPTURE
    XDBG_DEBUG(MVBUF, "pVideoPixmap(%ld) create(%d %d %d, [%d,%d %d %d], h[%x,%x,%x], name[%d,%d,%d])\n",
               bufinfo->stamp, bufinfo->fourcc,
               bufinfo->width, bufinfo->height, bufinfo->crop.x,
               bufinfo->crop.y, bufinfo->crop.width,
               bufinfo->crop.height, bufinfo->handles[0].u32, bufinfo->handles[1].u32,
               bufinfo->handles[2].u32, bufinfo->names[0],
               bufinfo->names[1], bufinfo->names[2]);
#endif
    
    return exynosVideoBufferRef (pPixmap);

fail_get:
    if (pPixmap && need_finish)
        EXYNOSExaFinishAccess(pPixmap, EXA_PREPARE_DEST);

alloc_fail:
    XDBG_DEBUG (MVA, " alloc fail VideoGetDrawableBuffer  \n");
    if (bufinfo)
    {
         tbm_bo_delete_user_data(bufinfo->tbos[0], TBM_BODATA_VPIXMAP);

        if (bufinfo->tbos[0])
                tbm_bo_unref(bufinfo->tbos[0]);
         
        ctrl_free(bufinfo);
    }

    return NULL;
}


static
PixmapPtr
_CaptureVideoGetUIBuffer (CAPTUREPortInfoPtr pPort, int connector_type)
{
    EXYNOSBufInfo *uibuf = NULL; 
    PixmapPtr pPixmap;
  
    tbm_bo_handle bo_handle;
    int ret = 0;
    CARD32 stamp;
    ScreenPtr pScreen = pPort->pScrn->pScreen;
    XDBG_RETURN_VAL_IF_FAIL (pScreen != NULL, NULL);
    
    pPort->bo[0]=_CaptureVideoGetFrontBo2 (pPort, connector_type, &bo_handle);
    XDBG_RETURN_VAL_IF_FAIL (pPort->bo[0] != NULL, NULL);
   

    uibuf = calloc(1, sizeof(EXYNOSBufInfo));
    XDBG_RETURN_VAL_IF_FAIL(uibuf != NULL, NULL);

    uibuf->pScrn = pPort->pScrn;
    uibuf->fourcc = (unsigned int)pPort->id;
    uibuf->width = pPort->pScrn->virtualX;
    uibuf->height = pPort->pScrn->virtualY;
    uibuf->crop.width = uibuf->width;
    uibuf->crop.height = uibuf->height;

    uibuf->size = exynosVideoImageAttrs(FOURCC_RGB32, &uibuf->width,
                                        &uibuf->height, uibuf->pitches, uibuf->offsets,
                                        uibuf->lengths);
    XDBG_GOTO_IF_FAIL(uibuf->size > 0, fail_get);
  /*  XDBG_GOTO_IF_FAIL(uibuf->width == proper_width, fail_get);
    XDBG_GOTO_IF_FAIL(uibuf->height == proper_height, fail_get);*/
    
    pPixmap = pScreen->CreatePixmap (pScreen, uibuf->width, uibuf->height,
                                     32, CREATE_PIXMAP_USAGE_XV);
    XDBG_GOTO_IF_FAIL (pPixmap != NULL, fail_get);
#ifdef DEBUG_CAPTURE
    XDBG_DEBUG (MVA, "uibuf->fourcc = (unsigned int)pPort->id(%d);\n"\
                "uibuf->size = %d;\n", (unsigned int)pPort->id, uibuf->size);
#endif
    stamp = pPixmap->drawable.serialNumber; //GetTimeInMillis();

    uibuf->stamp = stamp; 
    uibuf->ref_cnt = 1;

    XDBG_RETURN_VAL_IF_FAIL (uibuf != NULL, NULL);

    uibuf->tbos[0] = tbm_bo_ref(pPort->bo[0]);
    XDBG_GOTO_IF_FAIL (uibuf->tbos[0] != NULL, fail_get);

    uibuf->names[0] = tbm_bo_export (uibuf->tbos[0]);
    XDBG_GOTO_IF_FAIL (uibuf->names[0] > 0, fail_get);

    uibuf->handles[0].u32 = bo_handle.u32;
    XDBG_GOTO_IF_FAIL (uibuf->handles[0].u32 > 0, fail_get);
    
    exynosExaPixmapSetBo (pPixmap, uibuf->tbos[0]);
    ret = tbm_bo_add_user_data(uibuf->tbos[0], TBM_BODATA_VPIXMAP, _exynosVideoBufferDestroyOutbuf);
    tbm_bo_set_user_data (uibuf->tbos[0], TBM_BODATA_VPIXMAP, (void*) uibuf);
#ifdef DEBUG_CAPTURE
    XDBG_DEBUG (MVA, "tbm_bo_add_user_data ret=%d\n",ret);
    XDBG_DEBUG (MVA, "_CaptureVideoGetUIBuffer return uibuf Ok!  \n" \
                "uibuf->tbos[0]=(%p), uibuf->names[0]=(%d),uibuf->handles[0]=(%d),uibuf->handles[1]=(%d), \n" \
                "uibuf->pitches[0]=(%d),uibuf->offsets[0]=(%d), uibuf->lengths[0]=(%d)\n", uibuf->tbos[0], uibuf->names[0], uibuf->handles[0].u32,
                uibuf->handles[1].u32, uibuf->pitches[0], uibuf->offsets[0], uibuf->lengths[0]);
#endif
    return pPixmap;

fail_get:
    if (pPort->bo[0])
        
        XDBG_DEBUG (MVA, "_CaptureVideoGetUIBuffer return uibuf Fail!  \n");
    if (uibuf)
    {
        tbm_bo_delete_user_data(uibuf->tbos[0], TBM_BODATA_VPIXMAP);
        if (uibuf->tbos[0])
                tbm_bo_unref(uibuf->tbos[0]);
        ctrl_free(uibuf);
    }
    if(pPixmap)
        pScreen->DestroyPixmap (pPixmap);
    return NULL;
}

int
_CaptureVideoPutStill (CAPTUREPortInfoPtr pPort)
{
    EXYNOSBufInfo* tempbuf = NULL;
    EXYNOSDispInfoPtr pExynosMode = (EXYNOSDispInfoPtr)EXYNOSPTR(pPort->pScrn)->pDispInfo;
    PixmapPtr  pix_buf = NULL;
    PixmapPtr  ui_buf = NULL;
   
    tbm_bo_handle bo_handle1, bo_handle2;
    
    int comp = 0;

    PropertyPtr rotate_prop;
    int rotate, hw_rotate;
    xRectangle src_rect, dst_rect;
    int vwidth, vheight;
    ScreenPtr pScreen = pPort->pScrn->pScreen;
    XDBG_GOTO_IF_FAIL (pScreen != NULL, fail_still);

    if (pPort->retire_timer)
    {
        TimerFree (pPort->retire_timer);
        pPort->retire_timer = NULL;
    }

    pix_buf = _CaptureVideoGetDrawableBuffer (pPort);
    XDBG_GOTO_IF_FAIL (pix_buf != NULL, fail_still);

    ui_buf = _CaptureVideoGetUIBuffer (pPort, DRM_MODE_CONNECTOR_LVDS);
    XDBG_GOTO_IF_FAIL (ui_buf != NULL, fail_still);
    
    /* Get ui_buf and pix_buf size*/
    src_rect.x = src_rect.y = 0;
    tempbuf=(EXYNOSBufInfo*)_CaptureBufferGetInfo (ui_buf);
    XDBG_GOTO_IF_FAIL (tempbuf != NULL, fail_still);
    src_rect.width  = tempbuf->width;
    src_rect.height = tempbuf->height;

    dst_rect.x = dst_rect.y = 0;
    tempbuf=(EXYNOSBufInfo*)_CaptureBufferGetInfo (pix_buf);
    XDBG_GOTO_IF_FAIL (tempbuf != NULL, fail_still);
    dst_rect.width  = tempbuf->width;
    dst_rect.height = tempbuf->height;
    comp = COMP_OVER; 

    vwidth = pExynosMode->main_lcd_mode.hdisplay;
    vheight =  pExynosMode->main_lcd_mode.vdisplay;

    /* Get Rotate property */
    rotate_prop = exynosUtilGetWindowProperty(pPort->pScrn->pScreen->root,
                  "_E_ILLUME_ROTATE_ROOT_ANGLE");
    if (rotate_prop)
        rotate = *(int*) rotate_prop->data;
    else
    {
        XDBG_WARNING(MVA, "failed: exynosUtilGetWindowProperty\n");
        rotate = 0;
    }
    if (rotate == 90)
        hw_rotate = 270;
    else if (rotate == 270)
        hw_rotate = 90;
    else
        hw_rotate = rotate;

    /* rotate upper_rect */
   // exynosUtilRotateRect(vwidth, vheight, 0, hw_rotate, &dst_rect);
    if (rotate % 180)
        SWAP (vwidth, vheight);

    /* scale upper_rect */
   // UtilScaleRect (vwidth, vheight, dst_rect.width, dst_rect.height, &dst_rect);
#ifdef DEBUG_CAPTURE
    XDBG_DEBUG (MVA, "%dx%d(%d,%d, %dx%d) => %dx%d(%d,%d, %dx%d) :comp(%d) r(%d)\n",
                src_rect.width, src_rect.height,
                src_rect.x, src_rect.y, src_rect.width, src_rect.height,
                dst_rect.width, dst_rect.height,
                dst_rect.x, dst_rect.y, dst_rect.width, dst_rect.height,
                comp, rotate);
#endif
    if(pPort->swcomp_on)
      {
        /*make composition with Software*/
        /*map CPU device space on destination pix.buf plane 0*/
        bo_handle1=tbm_bo_map ((_CaptureBufferGetInfo (pix_buf))->tbos[0], TBM_DEVICE_CPU, TBM_OPTION_WRITE);
        /* Take buffer object from pixmap data */
        bo_handle2=tbm_bo_map ((_CaptureBufferGetInfo (ui_buf))->tbos[0], TBM_DEVICE_CPU, TBM_OPTION_READ);
        ((EXYNOSBufInfo*)_CaptureBufferGetInfo (ui_buf))->handles[0].ptr=bo_handle2.ptr;
        ((EXYNOSBufInfo*)_CaptureBufferGetInfo (pix_buf))->handles[0].ptr=bo_handle1.ptr;
        if (!_CaptureVideoCompositeSW ((EXYNOSBufInfo*)_CaptureBufferGetInfo (ui_buf),
                                       (EXYNOSBufInfo*)_CaptureBufferGetInfo (pix_buf),
                                        src_rect.width, src_rect.height,
                                        dst_rect.width, dst_rect.height,comp, rotate))
        {
            XDBG_DEBUG (MVA, "CompositeSW FAILS  \n");

        }
    
       }
    else 
      {
        /* Take buffer object from pixmap data */

        /*map 2d device space on destination pix.buf plane 0*/
        tbm_bo_map ((_CaptureBufferGetInfo (pix_buf))->tbos[0], TBM_DEVICE_2D, TBM_OPTION_WRITE);

        /* Take buffer object from pixmap data */
        tbm_bo_map ((_CaptureBufferGetInfo (ui_buf))->tbos[0], TBM_DEVICE_2D, TBM_OPTION_READ);
        /*make composition with hw*/
        if (!_CaptureVideoCompositeHW ((EXYNOSBufInfo*)_CaptureBufferGetInfo (ui_buf),
                                       (EXYNOSBufInfo*)_CaptureBufferGetInfo (pix_buf),
                                       src_rect.x, src_rect.y,
                                       src_rect.width, src_rect.height,
                                       dst_rect.x, dst_rect.y,
                                       dst_rect.width, dst_rect.height,
                                       comp, rotate))
        {
            XDBG_DEBUG (MVA, "CompositeHW FAILS  \n");

        }
      }
    /* Take buffer object from pixmap data */
    tbm_bo_unmap ((_CaptureBufferGetInfo (pix_buf))->tbos[0]);

    /* Take buffer object from pixmap data */
    tbm_bo_unmap ((_CaptureBufferGetInfo (ui_buf))->tbos[0]);
    DamageDamageRegion (pPort->pDraw, pPort->clipBoxes);
    
fail_still: 
    if (pix_buf)
    {
        tempbuf = ((EXYNOSBufInfo*)_CaptureBufferGetInfo (pix_buf));
        if (tempbuf)
        {
            XDBG_DEBUG(MVA, "Delete pix_buf \n");
            tbm_bo_delete_user_data( tempbuf->tbos[0], TBM_BODATA_VPIXMAP);
            tbm_bo_unref(tempbuf->tbos[0]);
            ctrl_free(tempbuf);
            tempbuf = NULL;
        }
        pScreen->DestroyPixmap (pix_buf);

    }
    if (ui_buf)
    {

        tempbuf = ((EXYNOSBufInfo*)_CaptureBufferGetInfo (ui_buf));
        if (tempbuf)
        {
            XDBG_DEBUG(MVA, "Delete ui_buf \n");
            exynosMemPoolFreeHandle (pPort->pScrn, tempbuf->handles[0].u32);
            tbm_bo_delete_user_data( tempbuf->tbos[0], TBM_BODATA_VPIXMAP);
            tbm_bo_unref(tempbuf->tbos[0]);
            ctrl_free(tempbuf);
            tempbuf = NULL;
        }
         pScreen->DestroyPixmap (ui_buf);
    }

    
    pPort->need_damage = FALSE;
    XDBG_DEBUG (MVA, "SUCCESS EXIT _CaptureVideoPutStill  \n");
    return Success;
}

/*
 * # planar #
 * format: YV12    Y/V/U 420
 * format: I420    Y/U/V 420 #YU12, S420
 * format: NV12    Y/UV  420
 * format: NV12M   Y/UV  420 #SN12
 * format: NV12MT  Y/UV  420 #ST12
 * format: NV21    Y/VU  420
 * format: Y444    YUV   444
 * # packed #
 * format: YUY2  YUYV  422 #YUYV, SUYV, SUY2
 * format: YVYU  YVYU  422
 * format: UYVY  UYVY  422 #SYVY
 */
static G2dColorMode
_g2dFormat (unsigned int id)
{
    G2dColorMode g2dfmt = 0;

    switch (id)
    {
    case FOURCC_NV12:
    case FOURCC_SN12:
        g2dfmt = G2D_COLOR_FMT_YCbCr420 | G2D_YCbCr_2PLANE | G2D_YCbCr_ORDER_CrCb;
        break;
    case FOURCC_NV21:
    case FOURCC_SN21:
        g2dfmt = G2D_COLOR_FMT_YCbCr420 | G2D_YCbCr_2PLANE | G2D_YCbCr_ORDER_CbCr;
        break;
    case FOURCC_SUYV:
    case FOURCC_YUY2:
        g2dfmt = G2D_COLOR_FMT_YCbCr422 | G2D_YCbCr_ORDER_Y1CbY0Cr;
        break;
    case FOURCC_SYVY:
    case FOURCC_UYVY:
        g2dfmt = G2D_COLOR_FMT_YCbCr422 | G2D_YCbCr_ORDER_CbY1CrY0;
        break;
    case FOURCC_SR16:
    case FOURCC_RGB565:
        g2dfmt = G2D_COLOR_FMT_RGB565 | G2D_ORDER_AXRGB;
        break;
    case FOURCC_SR32:
    case FOURCC_RGB32:
        g2dfmt = G2D_COLOR_FMT_ARGB8888 | G2D_ORDER_AXRGB;
        break;
    case FOURCC_YV12:
    case FOURCC_I420:
    case FOURCC_S420:
    case FOURCC_ITLV:
    case FOURCC_ST12:
    default:
        XDBG_NEVER_GET_HERE (MVA);
        return 0;
    }

    return g2dfmt;
}

static Bool
_calculateSize (int width, int height, xRectangle *crop)
{
    if (crop->x < 0)
    {
        crop->width += (crop->x);
        crop->x = 0;
    }
    if (crop->y < 0)
    {
        crop->height += (crop->y);
        crop->y = 0;
    }

    XDBG_GOTO_IF_FAIL (width > 0 && height > 0, fail_cal);
    XDBG_GOTO_IF_FAIL (crop->width > 0 && crop->height > 0, fail_cal);
    XDBG_GOTO_IF_FAIL (crop->x >= 0 && crop->x < width, fail_cal);
    XDBG_GOTO_IF_FAIL (crop->y >= 0 && crop->y < height, fail_cal);

    if (crop->x + crop->width > width)
        crop->width = width - crop->x;

    if (crop->y + crop->height > height)
        crop->height = height - crop->y;

    return TRUE;
fail_cal:
    XDBG_ERROR (MVA, "(%dx%d : %d,%d %dx%d)\n",
                width, height, crop->x, crop->y, crop->width, crop->height);

    return FALSE;
}

#if 1

static Bool
CaptureUtilConvertImage (pixman_op_t op,
                     unsigned char      *srcbuf,
                     unsigned char      *dstbuf,
                     pixman_format_code_t src_format,
                     pixman_format_code_t dst_format,
                     int         src_width,
                     int         src_height,
                     xRectangle *src_crop,
                     int         dst_width,
                     int         dst_height,
                     xRectangle *dst_crop,
                     RegionPtr   dst_clip_region,
                     int         rotate)
{
    pixman_image_t    *src_img;
    pixman_image_t    *dst_img;

    int                src_stride, dst_stride;
    int                src_bpp;
    int                dst_bpp;
    int                ret = FALSE;

    XDBG_RETURN_VAL_IF_FAIL (srcbuf != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (dstbuf != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (src_width > 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (src_height > 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (src_crop != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (dst_width > 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (dst_height > 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (dst_crop != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (rotate < 360 && rotate > -360, FALSE);

#if 0
    ErrorF ("(%dx%d | %d,%d %dx%d) => (%d,%d | %d,%d %dx%d) r(%d)\n",
            src_width, src_height,
            src_crop->x, src_crop->y, src_crop->width, src_crop->height,
            dst_width, dst_height,
            dst_crop->x, dst_crop->y, dst_crop->width, dst_crop->height,
            rotate);
#endif

    src_bpp = PIXMAN_FORMAT_BPP (src_format) / 8;
    XDBG_RETURN_VAL_IF_FAIL (src_bpp > 0, FALSE);

    dst_bpp = PIXMAN_FORMAT_BPP (dst_format) / 8;
    XDBG_RETURN_VAL_IF_FAIL (dst_bpp > 0, FALSE);

    src_stride = src_width * src_bpp;
    dst_stride = dst_width * dst_bpp;

    src_img = pixman_image_create_bits (src_format, src_width, src_height,
                                        (uint32_t*)srcbuf, src_stride);
    dst_img = pixman_image_create_bits (dst_format, dst_width, dst_height,
                                        (uint32_t*)dstbuf, dst_stride);

    XDBG_GOTO_IF_FAIL (src_img != NULL, CANT_CONVERT);
    XDBG_GOTO_IF_FAIL (dst_img != NULL, CANT_CONVERT);

    pixman_image_composite (op, src_img, NULL, dst_img,
                            0, 0, 0, 0,
                            dst_crop->x,
                            dst_crop->y,
                            dst_crop->width, dst_crop->height);
    
    /*  sw rotation  */
#if 0    
    if (rotate)
      {
        if (rotate == 270)
            src_image->rotate_90 = 1;
        else if (rotate == 180)
        {
           src_image->xDir = 1;
            src_image->yDir = 1;
        }
        else if (rotate == 90)
        {
            src_image->rotate_90 = 1;
            src_image->xDir = 1;
            src_image->yDir = 1;
        }
          pixman_transform_t trans = 
              {{ { pixman_double_to_fixed (1.3), pixman_double_to_fixed (0), pixman_double_to_fixed (-0.5), },
              { pixman_double_to_fixed (0), pixman_double_to_fixed (1), pixman_double_to_fixed (-0.5), },
              { pixman_double_to_fixed (0), pixman_double_to_fixed (0), pixman_double_to_fixed (1.0) } 
              }};
         pixman_image_set_transform (dst_img, &trans);
        
      }
#endif    
        
    ret = TRUE;

CANT_CONVERT:
    if (src_img)
        pixman_image_unref (src_img);
    if (dst_img)
        pixman_image_unref (dst_img);

    return ret;
}

static Bool
_CaptureVideoCompositeSW (EXYNOSBufInfo *src, EXYNOSBufInfo *dst, int src_w, int src_h,
                          int dst_w, int dst_h, int comp,int rotate)
{
    pixman_op_t op;

    XDBG_DEBUG (MVA, "_CaptureVideoCompositeSW \n");
   
    if (comp == COMP_SRC)
        op = PIXMAN_OP_SRC;
    else
        op = PIXMAN_OP_OVER;
    
 
    CaptureUtilConvertImage(op, src->handles[0].ptr,
                         dst->handles[0].ptr,
                         PIXMAN_a8r8g8b8, PIXMAN_a8r8g8b8,
                         src_w, src_h,
                         &src->crop,
                         dst_w, dst_h,
                         &dst->crop,
                         NULL, rotate);
    
   
    return TRUE;
        
}
#endif
static Bool
_CaptureVideoCompositeHW ( EXYNOSBufInfo *src,  EXYNOSBufInfo *dst,
                           int src_x, int src_y, int src_w, int src_h,
                           int dst_x, int dst_y, int dst_w, int dst_h,
                           int comp, int rotate)
{
    G2dImage *src_image = NULL, *dst_image = NULL;
    G2dColorMode src_g2dfmt, dst_g2dfmt;
    G2dOp op;
    xRectangle src_rect = {0,}, dst_rect = {0,};

    XDBG_DEBUG (MVA, "_CaptureVideoCompositeHW \n");
    XDBG_RETURN_VAL_IF_FAIL (src != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (dst != NULL, FALSE);
#ifdef DEBUG_CAPTURE
    XDBG_DEBUG (MVA, "comp(%d) src : %ld %c%c%c%c  %dx%d (%d,%d %dx%d) => dst : %ld %c%c%c%c  %dx%d (%d,%d %dx%d)\n",
                comp, src->stamp, FOURCC_STR (src->fourcc), src->width, src->height,
                src_x, src_y, src_w, src_h,
                dst->stamp, FOURCC_STR (dst->fourcc), dst->width, dst->height,
                dst_x, dst_y, dst_w, dst_h);
#endif
       
    src_rect.x = src_x;
    src_rect.y = src_y;
    src_rect.width = src_w;
    src_rect.height = src_h;

    dst_rect.x = dst_x;
    dst_rect.y = dst_y;
    dst_rect.width = dst_w;
    dst_rect.height = dst_h;

    if (!_calculateSize (src->width, src->height, &src_rect))
        return TRUE;

    if (!_calculateSize (dst->width, dst->height, &dst_rect))
        return TRUE;


    src_g2dfmt = _g2dFormat ( FOURCC_RGB32);
    XDBG_RETURN_VAL_IF_FAIL (src_g2dfmt > 0, FALSE);
    dst_g2dfmt = _g2dFormat ( FOURCC_RGB32);
    XDBG_RETURN_VAL_IF_FAIL (dst_g2dfmt > 0, FALSE);

    XDBG_RETURN_VAL_IF_FAIL (src->handles[0].u32 > 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (dst->handles[0].u32 > 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (src->pitches[0] > 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (dst->pitches[0] > 0, FALSE);
#ifdef DEBUG_CAPTURE
    XDBG_DEBUG (MVA, "dst : g2dfmt(%x) size(%dx%d) h(%d %d) p(%d) \n",
                dst_g2dfmt, dst->width, dst->height,
                dst->handles[0].u32, dst->handles[1].u32, dst->pitches[0]);
#endif
    dst_image = g2d_image_create_bo2(dst_g2dfmt,
                                     (unsigned int)dst->width,
                                     (unsigned int)dst->height,
                                     (unsigned int)dst->handles[0].u32,
                                     0,
                                     (unsigned int)dst->pitches[0]);


    XDBG_GOTO_IF_FAIL (dst_image != NULL, convert_g2d_fail);

#ifdef DEBUG_CAPTURE
    XDBG_DEBUG (MVA, "src : g2dfmt(%x) src->width(%d), src->height(%d)  src->handles[0]=(%d), src->handles[1]=(%d), src->pitches[0]=(%d) \n",
                src_g2dfmt, src->width, src->height, src->handles[0].u32, src->handles[1].u32, src->pitches[0]);
#endif 
    src_image = g2d_image_create_bo2(src_g2dfmt,
                                     (unsigned int)src->width,
                                     (unsigned int)src->height,
                                     (unsigned int)src->handles[0].u32,
                                     0,
                                     (unsigned int)src->pitches[0]);

    XDBG_GOTO_IF_FAIL (src_image != NULL, convert_g2d_fail);

    if (comp == COMP_SRC)
        op = G2D_OP_SRC;
    else
        op = G2D_OP_OVER;

    if (rotate == 270)
        src_image->rotate_90 = 1;
    else if (rotate == 180)
    {
        src_image->xDir = 1;
        src_image->yDir = 1;
    }
    else if (rotate == 90)
    {
        src_image->rotate_90 = 1;
        src_image->xDir = 1;
        src_image->yDir = 1;
    }
#ifdef DEBUG_CAPTURE
    XDBG_DEBUG (MVA, "op(%d) (%d,%d %dx%d) => (%d,%d %dx%d)\n", op,
                src_rect.x, src_rect.y, src_rect.width, src_rect.height,
                dst_rect.x, dst_rect.y, dst_rect.width, dst_rect.height);
#endif 
    util_g2d_blend_with_scale (op, src_image, dst_image,
                               (int)src_rect.x, (int)src_rect.y,
                               (unsigned int)src_rect.width, (unsigned int)src_rect.height,
                               (int)dst_rect.x, (int)dst_rect.y,
                               (unsigned int)dst_rect.width, (unsigned int)dst_rect.height,
                               FALSE);
    XDBG_DEBUG (MVA, "util_g2d_blend_with_scale executed \n");
    g2d_exec();

    g2d_image_free (src_image);
    g2d_image_free (dst_image);
    XDBG_DEBUG (MVA, "SUCCESS EXIT VideoCompositeHW \n");
    return TRUE;
convert_g2d_fail:
    if (dst_image)
        g2d_image_free (dst_image);
    if (src_image)
        g2d_image_free (src_image);
    XDBG_DEBUG (MVA, "ERROR EXIT VideoCompositeHW \n");
    return FALSE;
}

static void
_CaptureVideoCloseOutBuffer (CAPTUREPortInfoPtr pPort)
{
    int i;

    if (pPort->outbuf)
    {
        for (i = 0; i < pPort->outbuf_num; i++)
        {
            if (pPort->outbuf[i])
            {
                /* make unref pPort->outbuf[i] and free video buffer */
                pPort->outbuf[i]->ref_cnt--;
                if (pPort->outbuf[i]->ref_cnt == 0)
                     ctrl_free (pPort->outbuf[i]);

            }
            pPort->outbuf[i] = NULL;
        }

        ctrl_free (pPort->outbuf);
        pPort->outbuf = NULL;
    }
    
     
    pPort->outbuf_index = -1;
}

static Bool
_CaptureCheckOutBuffers (ScrnInfoPtr pScrn, CAPTUREPortInfoPtr pPort, int id, int width, int height)
{
    EXYNOSBufInfo *bufinfo = NULL;

    tbm_bo bo[PLANE_CNT] = {0,};
    tbm_bo_handle bo_handle;

    int i;

    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);

    XDBG_RETURN_VAL_IF_FAIL (pPort->capture != CAPTURE_MODE_STILL, FALSE);

    if (pPort->capture == CAPTURE_MODE_NONE)
        pPort->outbuf_num = BUF_NUM_VIRTUAL;
    else
        pPort->outbuf_num = BUF_NUM_STREAM;

    if (!pPort->outbuf)
    {
        pPort->outbuf = (EXYNOSBufInfo**)calloc(pPort->outbuf_num, sizeof (EXYNOSBufInfo*));
        XDBG_RETURN_VAL_IF_FAIL (pPort->outbuf != NULL, FALSE);
    }

    for (i = 0; i < pPort->outbuf_num; i++)
    {
        if (pPort->outbuf[i])
            continue;

        XDBG_RETURN_VAL_IF_FAIL (width > 0, FALSE);
        XDBG_RETURN_VAL_IF_FAIL (height > 0, FALSE);

        /* pPort->pScrn can be NULL if XvPutStill isn't called. */
        bufinfo = calloc(1, sizeof(EXYNOSBufInfo));

        XDBG_GOTO_IF_FAIL(bufinfo != NULL, ensure_buffer_fail); //?
        bufinfo->pScrn = pScrn;
        bufinfo->fourcc = id;//FOURCC_RGB32;
        bufinfo->width = width;
        bufinfo->height = height;
        bufinfo->crop.x = 0;
        bufinfo->crop.y = 0;
        bufinfo->crop.width = bufinfo->width;
        bufinfo->crop.height = bufinfo->height;
        bufinfo->size = exynosVideoImageAttrs(bufinfo->fourcc, &bufinfo->width,
                                              &bufinfo->height, bufinfo->pitches, bufinfo->offsets,
                                              bufinfo->lengths);

        bo[i] = tbm_bo_alloc (pExynos->bufmgr, bufinfo->size, TBM_BO_DEFAULT);
        bo_handle = tbm_bo_get_handle(bo[i], TBM_DEVICE_DEFAULT);
        bufinfo->tbos[0] = tbm_bo_ref(bo[0]);
        XDBG_GOTO_IF_FAIL (bufinfo->tbos[0] != NULL, ensure_buffer_fail);

        bufinfo->names[0] = tbm_bo_export(bufinfo->tbos[0]);
        XDBG_GOTO_IF_FAIL (bufinfo->names[0] > 0, ensure_buffer_fail);

        bufinfo->handles[0].u32 = bo_handle.u32;
        XDBG_GOTO_IF_FAIL (bufinfo->handles[0].u32 > 0, ensure_buffer_fail); //?
        pPort->outbuf[i] = bufinfo;
        XDBG_GOTO_IF_FAIL (pPort->outbuf[i] != NULL, ensure_buffer_fail);
    }

    return TRUE;

ensure_buffer_fail:
    _CaptureVideoCloseOutBuffer (pPort);

    return FALSE;
}



int
_CaptureVideoPutWB (CAPTUREPortInfoPtr pPort)
{

    XDBG_INFO (MVA, "_CaptureVideoPutWB");

    XDBG_RETURN_VAL_IF_FAIL (pPort->pScrn != NULL, BadImplementation);
    XDBG_RETURN_VAL_IF_FAIL (pPort->pDraw != NULL, BadImplementation);

   /* Current realization  use buffers in clone object, so code below must be commented:
    *  if (!_CaptureCheckOutBuffers (pPort->pScrn, pPort, pPort->id, pPort->pDraw->width, pPort->pDraw->height))
        return BadAlloc;*/
    
    /* At first time we need open Clone and set up the buffers*/
    if (!pPort->clone)
    {
        if (exynosCloneIsOpened ())
        {
            XDBG_ERROR (MVA, "Fail : clone open. \n");
            return BadRequest;
        }

        /* For capture mode, we don't need to create contiguous buffer.
         * Rotation should be considered when wb begins.
         */
        pPort->clone = exynosCloneInit  (pPort->pScrn, pPort->id,
                                      pPort->pDraw->width, pPort->pDraw->height,
                                      (pPort->capture) ? TRUE : FALSE, pPort->wb_fps,
                                      (pPort->rotate_off) ? FALSE : TRUE);
        XDBG_RETURN_VAL_IF_FAIL (pPort->clone != NULL, BadAlloc);
         /* Current realization  use buffers in clone object, so code below must be commented:
        exynosCloneSetBuffer (pPort->wb, pPort->outbuf, pPort->outbuf_num);
          */

        XDBG_TRACE (MVA, "Clone(%p) start. \n", pPort->clone);

        if (!exynosCloneStart (pPort->clone))
        {
            XDBG_INFO (MVA, "Clone Start Error!");
            exynosCloneClose (pPort->clone);
            pPort->clone = NULL;
            return BadAlloc;
        }
        exynosCloneAddNotifyFunc (pPort->clone, CLONE_NOTI_STOP,
                                  _CaptureVideoWbStopFunc, pPort);
        exynosCloneAddNotifyFunc (pPort->clone, CLONE_NOTI_IPP_EVENT,
                                  _CaptureVideoWbDumpFunc, pPort);
        if (pPort->capture == CAPTURE_MODE_STILL)
            exynosCloneAddNotifyFunc (pPort->clone, CLONE_NOTI_IPP_EVENT_DONE,
                                      _CaptureVideoWbDumpDoneFunc, pPort);
        exynosCloneAddNotifyFunc (pPort->clone, CLONE_NOTI_CLOSED,
                                  _CaptureVideoWbCloseFunc, pPort);
    }

    /* no available buffer, need to return buffer by client. */
    if (!exynosCloneIsRunning ())
    {
        XDBG_WARNING (MVA, "clone is stopped.\n");
        return BadRequest;
    }

    /* no available buffer, need to return buffer by client. */
    if (!exynosCloneCanDequeueBuffer (pPort->clone))
    {
        XDBG_TRACE (MVA, "no available buffer\n");
        return BadRequest;
    }

    XDBG_TRACE (MVA, "clone(%p), running(%d). \n", pPort->clone, exynosCloneIsRunning());

    return Success;
}



static EXYNOSBufInfo*
_CaptureVideoGetBlackBuffer (CAPTUREPortInfoPtr pPort)
{
    int i;

    if (!pPort->outbuf[0])
    {
        XDBG_RETURN_VAL_IF_FAIL (pPort->pDraw != NULL, NULL);
        _CaptureCheckOutBuffers (pPort->pScrn, pPort, pPort->id,
                                 pPort->pDraw->width, pPort->pDraw->height);
    }

    for (i = 0; i < pPort->outbuf_num; i++)
    {
        if (pPort->outbuf[i] && !pPort->outbuf[i]->showing)
        {
            if (pPort->outbuf[i]->dirty)
                exynosUtilClearNormalVideoBuffer(pPort->outbuf[i]);
            return pPort->outbuf[i];
        }
    }

    XDBG_ERROR (MVA, "now all buffers are in showing\n");

    return NULL;
}

void
UtilScaleRect (int src_w, int src_h, int dst_w, int dst_h, xRectangle *scale)
{
    float ratio;
    xRectangle fit;

    XDBG_RETURN_IF_FAIL (scale != NULL);
    XDBG_RETURN_IF_FAIL (src_w > 0 && src_h > 0);
    XDBG_RETURN_IF_FAIL (dst_w > 0 && dst_h > 0);

    if ((src_w == dst_w) && (src_h == dst_h))
        return;

    UtilAlignRect (src_w, src_h, dst_w, dst_h, &fit, FALSE);

    ratio = (float)fit.width / src_w;

    scale->x = scale->x * ratio + fit.x;
    scale->y = scale->y * ratio + fit.y;
    scale->width = scale->width * ratio;
    scale->height = scale->height * ratio;
}
void
UtilAlignRect (int src_w, int src_h, int dst_w, int dst_h, xRectangle *fit, Bool hw)
{
    int fit_width;
    int fit_height;
    float rw, rh, max;

    if (!fit)
        return;

    XDBG_RETURN_IF_FAIL (src_w > 0 && src_h > 0);
    XDBG_RETURN_IF_FAIL (dst_w > 0 && dst_h > 0);

    rw = (float)src_w / dst_w;
    rh = (float)src_h / dst_h;
    max = MAX (rw, rh);

    fit_width = src_w / max;
    fit_height = src_h / max;

    if (hw)
        fit_width &= (~0x3);

    fit->x = (dst_w - fit_width) / 2;
    fit->y = (dst_h - fit_height) / 2;
    fit->width = fit_width;
    fit->height = fit_height;
}


static void
_CaptureRemoveNotifyFunc (EXYNOSBufInfo* vbuf, NotifyFunc func)
{
    NotifyFuncData *data = NULL, *data_next = NULL;

    XDBG_RETURN_IF_FAIL (vbuf != NULL);
    XDBG_RETURN_IF_FAIL (func != NULL);

    xorg_list_for_each_entry_safe (data, data_next, &noti_data, link)
    {
        if (data->func == func)
        {
            xorg_list_del (&data->link);
            ctrl_free (data);
        }
    }
}
static
void
_CaptureAddNotifyFunc (EXYNOSBufInfo* vbuf, NotifyFunc func, void *user_data)
{
    NotifyFuncData *data = NULL, *data_next = NULL;

    XDBG_RETURN_IF_FAIL (vbuf != NULL);
    XDBG_RETURN_IF_FAIL (func != NULL);

    xorg_list_for_each_entry_safe (data, data_next, &noti_data, link)
    {
        if (data->func == func && data->user_data == user_data)
            return;
    }

    data = calloc (sizeof (NotifyFuncData), 1);
    XDBG_RETURN_IF_FAIL (data != NULL);

    data->func      = func;
    data->user_data = user_data;

    xorg_list_add (&data->link, &noti_data);
}

static void
_CaptureVideoNotifyFunc (EXYNOSBufInfo* vbuf, int type, void *type_data, void *data)
{
    CAPTUREPortInfoPtr pPort = (CAPTUREPortInfoPtr)data;
    EXYNOSBufInfo *dbuf = (EXYNOSBufInfo*)type_data;
    EXYNOSBufInfo *black;

    XDBG_INFO (MVA, " Notify _CaptureVideoNotifyFunc \n");

    _CaptureRemoveNotifyFunc (vbuf, _CaptureVideoNotifyFunc);

    if (!vbuf)
        goto fail_video_noti;

    XDBG_GOTO_IF_FAIL (dbuf->showing == FALSE, fail_video_noti);

    XDBG_DEBUG (MVA, "------------------------------\n");

    // _VideoCompositeSubtitle (pPort, vbuf);
    _CaptureCloneVideoDraw  (pPort, dbuf);
    XDBG_DEBUG (MVA, "------------------------------...\n");

    return;

fail_video_noti:
    black = _CaptureVideoGetBlackBuffer (pPort);
    XDBG_TRACE (MVA, "black buffer(%d) return:  noti. type(%d), vbuf(%p)\n",
                (black) ? black->names[0] : 0, type, vbuf);
    _CaptureCloneVideoDraw (pPort, black);

}

int
_CapturePutVideoOnly (CAPTUREPortInfoPtr pPort)
{
   
    PixmapPtr uibuf = NULL;
    int i;

    XDBG_INFO (MVA, "!!! _CapturePutVideoOnly");
    XDBG_RETURN_VAL_IF_FAIL (pPort->capture == CAPTURE_MODE_NONE, BadRequest);

    for (i = 0; i < pPort->outbuf_num; i++)
    {
        if (!pPort->outbuf[i]->showing)
            break;
    }

    if (i == pPort->outbuf_num)
    {
        XDBG_ERROR (MVA, "now all buffers are in showing\n");
        return BadRequest;
    }

    uibuf = _CaptureVideoGetUIBuffer (pPort, DRM_MODE_CONNECTOR_LVDS);
    /* if video buffer is just created, uibuf can be null. */
    if (!uibuf)
    {
        EXYNOSBufInfo* black = _CaptureVideoGetBlackBuffer (pPort);
        XDBG_RETURN_VAL_IF_FAIL (black != NULL, BadRequest);

        XDBG_TRACE (MVA, "black buffer(%d) return: vbuf invalid\n", black->names[0]);
        _CaptureCloneVideoDraw (pPort, black);
        return Success;
    }

    /* Wait the next frame if it's same as previous one */
    if (_CaptureVideoFindReturnBuf (pPort, ((EXYNOSBufInfo*)_CaptureBufferGetInfo (uibuf))->names[0]))
    {
        _CaptureAddNotifyFunc ((EXYNOSBufInfo*)_CaptureBufferGetInfo (uibuf), _CaptureVideoNotifyFunc, pPort);
        XDBG_DEBUG (MVA, "wait notify.\n");
        return Success;
    }
    /* 1. Get subtitle buffer from layer
     * 2. Clear src_rect, dst_rect
     * 3. CompositeHW subtitle buffer with vbuf
     * CaptureVideoCompositeSubtitle (pPort, vbuf);
     * */
    /* We haven't subtitle yet
     *
     xRectangle   src_rect;
     xRectangle   dst_rect;
     CLEAR (src_rect);
     CLEAR (dst_rect);
     _CaptureVideoCompositeHW (subtitle_vbuf, vbuf,
             src_rect.x, src_rect.y, src_rect.width, src_rect.height,
             dst_rect.x, dst_rect.y, dst_rect.width, dst_rect.height,
             COMP_OVER, 0);
     */

    _CaptureCloneVideoDraw (pPort, (EXYNOSBufInfo*)_CaptureBufferGetInfo (uibuf));

    return Success;
}

