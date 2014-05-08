/**************************************************************************

xserver-xorg-video-exynos

Copyright 2010 - 2011 Samsung Electronics co., Ltd. All Rights Reserved.

Contact: Boram Park <boram1288.park@samsung.com>

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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <X11/XWDFile.h>
#include <drm.h>
#include <drm_fourcc.h>

#include "exynos_driver.h"
#include "exynos_util.h"
#include "exynos_video_fourcc.h"
#include "exynos_video_image.h"
#include "exynos_video_buffer.h"
#include "exynos_video_capture.h"
#include "xv_types.h"
#include "exynos_video_converter.h"
#include "exynos_mem_pool.h"
#include "_trace.h"

/* key of tbo's user_data */
#define TBM_BODATA_VPIXMAP 0x1000

/* list of vpix format */
static EXYNOSFormatTable format_table[] =
{
    { FOURCC_RGB565, DRM_FORMAT_RGB565, TYPE_RGB },
    { FOURCC_SR16, DRM_FORMAT_RGB565, TYPE_RGB },
    { FOURCC_RGB32, DRM_FORMAT_XRGB8888, TYPE_RGB },
    { FOURCC_SR32, DRM_FORMAT_XRGB8888, TYPE_RGB },
    { FOURCC_YV12, DRM_FORMAT_YVU420, TYPE_YUV420 },
    { FOURCC_I420, DRM_FORMAT_YUV420, TYPE_YUV420 },
    { FOURCC_S420, DRM_FORMAT_YUV420, TYPE_YUV420 },
    { FOURCC_ST12, DRM_FORMAT_NV12MT, TYPE_YUV420 },
    { FOURCC_SN12, DRM_FORMAT_NV12M, TYPE_YUV420 },
    { FOURCC_NV12, DRM_FORMAT_NV12, TYPE_YUV420 },
    { FOURCC_SN21, DRM_FORMAT_NV21, TYPE_YUV420 },
    { FOURCC_NV21, DRM_FORMAT_NV21, TYPE_YUV420 },
    { FOURCC_YUY2, DRM_FORMAT_YUYV, TYPE_YUV422 },
    { FOURCC_SUYV, DRM_FORMAT_YUYV, TYPE_YUV422 },
    { FOURCC_UYVY, DRM_FORMAT_UYVY, TYPE_YUV422 },
    { FOURCC_SYVY, DRM_FORMAT_UYVY, TYPE_YUV422 },
    { FOURCC_ITLV, DRM_FORMAT_UYVY, TYPE_YUV422 },
};


static Bool
_exynosVideoBufferComparePixStamp (pointer structure, pointer element)
{

    PixmapPtr pVideoPix = (PixmapPtr) structure;
    unsigned long stamp = * ((unsigned long *) element);
    return (pVideoPix->drawable.serialNumber == stamp);
}

static int
_exynosVideoBufferGetNames (pointer in_buf, unsigned int *names,
                            unsigned int *type)
{

    XV_DATA_PTR data = (XV_DATA_PTR) in_buf;
    int valid = XV_VALIDATE_DATA (data);

    if (valid == XV_HEADER_ERROR)
    {
        XDBG_ERROR(MIA, "XV_HEADER_ERROR\n");
        return valid;
    }
    else if (valid == XV_VERSION_MISMATCH)
    {
        XDBG_ERROR(MIA, "XV_VERSION_MISMATCH\n");
        return valid;
    }

    names[0] = data->YBuf;
    names[1] = data->CbBuf;
    names[2] = data->CrBuf;
    *type = data->BufType;

    XDBG_TRACE(MVBUF, "GetName: %u, %u, %u type:%u\n", names[0], names[1],
               names[2], *type);
    return XV_OK;
}

EXYNOSBufInfoPtr
_exynosVideoBufferGetInfo (PixmapPtr pPixmap)
{
    EXYNOSBufInfoPtr bufinfo = NULL;
    tbm_bo tbo = NULL;

    tbo = exynosExaPixmapGetBo (pPixmap);
    XDBG_RETURN_VAL_IF_FAIL(tbo != NULL, NULL);

    bufinfo = exynosMemPoolGetBufInfo ((pointer) tbo);

    if (bufinfo == NULL)
    {
        XDBG_DEBUG(MVBUF, "Bufinfo of Pixmap (%p)-(stamp:%lu) is NULL\n", pPixmap,
                   pPixmap->drawable.serialNumber);
        return NULL;
    }
    return bufinfo;
}

int
exynosVideoBufferCreateFBID (PixmapPtr pPixmap)
{

    XDBG_RETURN_VAL_IF_FAIL(pPixmap != NULL, 0);
    EXYNOSBufInfoPtr bufinfo = _exynosVideoBufferGetInfo (pPixmap);
    XDBG_RETURN_VAL_IF_FAIL(bufinfo != NULL, 0);

    EXYNOSPtr pExynos = EXYNOSPTR (bufinfo->pScrn);
    unsigned int drmfmt;
    unsigned int handles[4] = {0,};
    unsigned int pitches[4] = {0,};
    unsigned int offsets[4] = {0,};
    unsigned int fb_id = 0;
    int i;
    drmfmt = exynosVideoBufferGetDrmFourcc (bufinfo->fourcc);

    for (i = 0 ; i < bufinfo->num_planes; i++)
    {
        handles[i] = bufinfo->handles[i].u32;
        pitches[i] = (unsigned int)bufinfo->pitches[i];
        offsets[i] = (unsigned int)bufinfo->offsets[i];
    }
    if (drmModeAddFB2 (pExynos->drm_fd,
                       pPixmap->drawable.width, pPixmap->drawable.height, drmfmt,
                       handles, pitches, offsets, &fb_id, 0))
    {
        XDBG_ERRNO (MVBUF, "VideoPixmap (%p)-stamp(%lu) drmModeAddFB2 failed.\
                    handles(%d %d %d) pitches(%d %d %d) offsets(%d %d %d) '%c%c%c%c'\n",
                    pPixmap, STAMP(pPixmap),
                    handles[0], handles[1], handles[2],
                    pitches[0], pitches[1], pitches[2],
                    offsets[0], offsets[1], offsets[2],
                    FOURCC_STR (drmfmt));
    }
    return (int)fb_id;
}


static void
_exynosVideoBufferDestroyFBID (pointer data)
{
    EXYNOSBufInfoPtr bufinfo = (EXYNOSBufInfoPtr) data;
    XDBG_RETURN_IF_FAIL(bufinfo != NULL);
    XDBG_DEBUG(MVBUF, "fb_id(%d) removed. \n", bufinfo->fb_id);
    if (bufinfo->fb_id > 0)
        drmModeRmFB (EXYNOSPTR(bufinfo->pScrn)->drm_fd, bufinfo->fb_id);
}

static void
_exynosVideoBufferDestroyOutbuf (pointer data)
{
}

static void
_exynosVideoBufferDestroyRaw (pointer data)
{
}

static void
_exynosVideoBufferReturnToClient (EXYNOSBufInfoPtr bufinfo)
{
    XDBG_RETURN_IF_FAIL (bufinfo != NULL);
    xEvent event; /* Clear */
    CLEAR(event);
    static Atom return_atom = None;

    if (return_atom == None)
        return_atom = MakeAtom ("XV_RETURN_BUFFER", strlen ("XV_RETURN_BUFFER"), TRUE);

    if (bufinfo->pClient != NULL)
    {
        event.u.u.type = ClientMessage;
        event.u.u.detail = 32;
        event.u.clientMessage.u.l.type = return_atom;
        event.u.clientMessage.u.l.longs0 = (INT32) bufinfo->names[0];
        event.u.clientMessage.u.l.longs1 = (INT32) bufinfo->names[1];
        event.u.clientMessage.u.l.longs2 = (INT32) bufinfo->names[2];
        XDBG_TRACE (MVBUF, "%d,%d,%d out.\n",
                    bufinfo->names[0], bufinfo->names[1],
                    bufinfo->names[2]);
        WriteEventsToClient (bufinfo->pClient, 1, (xEventPtr) &event);
    }
}


EXYNOSFormatType
exynosVideoBufferGetColorType (unsigned int fourcc)
{
    int i = 0, size = 0;

    size = sizeof (format_table) / sizeof(EXYNOSFormatTable);

    for (i = 0; i < size; i++)
        if (format_table[i].fourcc == fourcc)
            return format_table[i].type;

    return TYPE_NONE;
}

unsigned int
exynosVideoBufferGetDrmFourcc (unsigned int fourcc)
{
    int i = 0, size = 0;

    size = sizeof (format_table) / sizeof(EXYNOSFormatTable);

    for (i = 0; i < size; i++)
        if (format_table[i].fourcc == fourcc)
            return format_table[i].drm_fourcc;

    return 0;
}

PixmapPtr
exynosVideoBufferRef (PixmapPtr pVideoPixmap)
{
    XDBG_RETURN_VAL_IF_FAIL (pVideoPixmap != NULL, NULL);
//    EXYNOSBufInfoPtr bufinfo = NULL;

//    bufinfo = _exynosVideoBufferGetInfo (pVideoPixmap);
//    XDBG_RETURN_VAL_IF_FAIL (bufinfo != NULL, NULL);

    ++(pVideoPixmap->refcnt);
    XDBG_DEBUG (MVBUF, "VideoBuf %p refcnt %d\n", pVideoPixmap, pVideoPixmap->refcnt);

    return pVideoPixmap;
}

void
exynosVideoBufferFree (PixmapPtr pVideoPixmap)
{
    XDBG_RETURN_IF_FAIL (pVideoPixmap != NULL);
    EXYNOSBufInfoPtr bufinfo = NULL;
    ScreenPtr pScreen = NULL;

//    XDBG_RETURN_IF_FAIL (!exynosVideoBufferIsConverting(pVideoPixmap));
//    XDBG_RETURN_IF_FAIL (!exynosVideoBufferIsShowing(pVideoPixmap));
    bufinfo = _exynosVideoBufferGetInfo (pVideoPixmap);
    XDBG_RETURN_IF_FAIL (bufinfo != NULL);
    pScreen = bufinfo->pScrn->pScreen;
    XDBG_TRACE(MVBUF, "VideoPixmap(%p) - stamp(%lu) refcnt(%d)\n", pVideoPixmap,
               STAMP(pVideoPixmap), pVideoPixmap->refcnt);
    if (bufinfo->pClient)
    {
        _exynosVideoBufferReturnToClient(bufinfo);
    }
    if (bufinfo->fb_id > 0)
    {
        _exynosVideoBufferDestroyFBID(bufinfo);
    }
    exynosMemPoolDestroyBO(bufinfo);
    pScreen->DestroyPixmap (pVideoPixmap);
}
void
exynosVideoBufferUnref (ScrnInfoPtr pScrn, PixmapPtr pVideoPixmap)
{
    XDBG_RETURN_IF_FAIL (pVideoPixmap != NULL);
    EXYNOSBufInfoPtr bufinfo = NULL;

    -- (pVideoPixmap->refcnt);
    XDBG_TRACE(MVBUF, "VideoPixmap(%p) - stamp(%lu) refcnt(%d)\n", pVideoPixmap,
               STAMP(pVideoPixmap), pVideoPixmap->refcnt);
    if (pVideoPixmap->refcnt == 1)
    {
        bufinfo = _exynosVideoBufferGetInfo (pVideoPixmap);
        XDBG_RETURN_IF_FAIL (bufinfo != NULL);

        /* DestroyPixmap call tbm_bo_unref */
        exynosMemPoolDestroyBO(bufinfo);
        pScrn->pScreen->DestroyPixmap (pVideoPixmap);
    }
}

void
exynosVideoBufferConverting (PixmapPtr pVideoPixmap, Bool on)
{
    EXYNOSBufInfoPtr bufinfo = NULL;
    XDBG_RETURN_IF_FAIL (pVideoPixmap != NULL);

    bufinfo = _exynosVideoBufferGetInfo (pVideoPixmap);
    XDBG_RETURN_IF_FAIL (bufinfo != NULL);

    bufinfo->converting = (on) ? TRUE : FALSE;
}

void
exynosVideoBufferShowing (PixmapPtr pVideoPixmap, Bool on)
{
    EXYNOSBufInfoPtr bufinfo = NULL;
    XDBG_RETURN_IF_FAIL (pVideoPixmap != NULL);

    bufinfo = _exynosVideoBufferGetInfo (pVideoPixmap);
    XDBG_RETURN_IF_FAIL (bufinfo != NULL);

    bufinfo->showing = (on) ? TRUE : FALSE;
}

Bool
exynosVideoBufferIsConverting (PixmapPtr pVideoPixmap)
{
    EXYNOSBufInfoPtr bufinfo = NULL;
    XDBG_RETURN_VAL_IF_FAIL (pVideoPixmap != NULL, FALSE);

    bufinfo = _exynosVideoBufferGetInfo (pVideoPixmap);
    XDBG_RETURN_VAL_IF_FAIL (bufinfo != NULL, FALSE);

    return bufinfo->converting;
}

Bool
exynosVideoBufferIsShowing (PixmapPtr pVideoPixmap)
{
    EXYNOSBufInfoPtr bufinfo = NULL;
    XDBG_RETURN_VAL_IF_FAIL (pVideoPixmap != NULL, FALSE);

    bufinfo = _exynosVideoBufferGetInfo (pVideoPixmap);
    XDBG_RETURN_VAL_IF_FAIL (bufinfo != NULL, FALSE);

    return bufinfo->showing;
}

int
exynosVideoBufferGetFBID (PixmapPtr pVideoPixmap)
{
    EXYNOSBufInfo *bufinfo = NULL;
    XDBG_RETURN_VAL_IF_FAIL (pVideoPixmap != NULL, 0);

    bufinfo = _exynosVideoBufferGetInfo (pVideoPixmap);
    XDBG_RETURN_VAL_IF_FAIL (bufinfo != NULL, 0);

    XDBG_DEBUG(MVBUF, "VideoPixmap(%p) - stamp(%lu) fb_id(%d)\n", pVideoPixmap,
               STAMP(pVideoPixmap), bufinfo->fb_id);
    return bufinfo->fb_id;
}

Bool
exynosVideoBufferSetCrop (PixmapPtr pPixmap, xRectangle crop)
{
    XDBG_RETURN_VAL_IF_FAIL (pPixmap != NULL, FALSE);
    EXYNOSBufInfoPtr bufinfo = NULL;
    bufinfo = _exynosVideoBufferGetInfo (pPixmap);
    XDBG_RETURN_VAL_IF_FAIL (bufinfo != NULL, FALSE);
    bufinfo->crop = crop;
    return TRUE;
}

xRectangle
exynosVideoBufferGetCrop (PixmapPtr pPixmap)
{
    xRectangle ret = {0,0,0,0};
    XDBG_RETURN_VAL_IF_FAIL (pPixmap != NULL, ret);
    EXYNOSBufInfoPtr bufinfo = NULL;
    bufinfo = _exynosVideoBufferGetInfo (pPixmap);
    XDBG_RETURN_VAL_IF_FAIL (bufinfo != NULL, ret);
    return bufinfo->crop;
}

int
exynosVideoBufferGetSize (PixmapPtr pVideoPixmap, int *pitches, int *offsets,
                          int *lengths)
{
    XDBG_RETURN_VAL_IF_FAIL (pVideoPixmap != NULL, 0);
    EXYNOSBufInfoPtr bufinfo = NULL;

    bufinfo = _exynosVideoBufferGetInfo (pVideoPixmap);
    XDBG_RETURN_VAL_IF_FAIL (bufinfo != NULL, 0);
    XDBG_RETURN_VAL_IF_FAIL (bufinfo->num_planes != 0, 0);

    if (pitches)
        memcpy (pitches, bufinfo->pitches, bufinfo->num_planes * sizeof(int));
    if (offsets)
        memcpy (offsets, bufinfo->offsets, bufinfo->num_planes * sizeof(int));
    if (lengths)
        memcpy (lengths, bufinfo->lengths, bufinfo->num_planes * sizeof(int));

    return bufinfo->size;
}

void
exynosVideoBufferGetAttr (PixmapPtr pVideoPixmap, tbm_bo *tbos, tbm_bo_handle *handles,
                          unsigned int *names)
{
    XDBG_RETURN_IF_FAIL (pVideoPixmap != NULL);
    EXYNOSBufInfoPtr bufinfo = NULL;

    bufinfo = _exynosVideoBufferGetInfo (pVideoPixmap);
    XDBG_RETURN_IF_FAIL (bufinfo != NULL);
    XDBG_RETURN_IF_FAIL (bufinfo->num_planes != 0);

    if (tbos)
        memcpy (tbos, bufinfo->tbos, bufinfo->num_planes * sizeof(tbm_bo));
    if (handles)
        memcpy (handles, bufinfo->handles, bufinfo->num_planes * sizeof(tbm_bo_handle));
    if (names)
        memcpy (names, bufinfo->names, bufinfo->num_planes * sizeof(unsigned int));
}

unsigned long
exynosVideoBufferGetStamp (PixmapPtr pVideoPixmap)
{
    XDBG_RETURN_VAL_IF_FAIL (pVideoPixmap != NULL, 0);
    return pVideoPixmap->drawable.serialNumber;
}

int
exynosVideoBufferGetFourcc (PixmapPtr pVideoPixmap)
{
    EXYNOSBufInfoPtr bufinfo = NULL;
    XDBG_RETURN_VAL_IF_FAIL (pVideoPixmap != NULL, 0);

    bufinfo = _exynosVideoBufferGetInfo (pVideoPixmap);
    XDBG_RETURN_VAL_IF_FAIL (bufinfo != NULL, 0);

    return bufinfo->fourcc;
}

PixmapPtr
exynosVideoBufferCreateInbufZeroCopy (ScrnInfoPtr pScrn, int fourcc, int in_width,
                                      int in_height, pointer in_buf, ClientPtr client)
{
    XDBG_RETURN_VAL_IF_FAIL (pScrn != NULL, NULL);
    EXYNOSBufInfoPtr bufinfo = NULL;
    PixmapPtr pPixmap = NULL;
    ScreenPtr pScreen = pScrn->pScreen;
    unsigned int names[PLANE_CNT];
    unsigned int buf_type = 0;
    pPixmap = pScreen->CreatePixmap (pScreen, in_width, in_height, 32,
                                     CREATE_PIXMAP_USAGE_XV);
    XDBG_GOTO_IF_FAIL (pPixmap != NULL, zc_alloc_fail);

    XDBG_GOTO_IF_FAIL(!_exynosVideoBufferGetNames (in_buf, names, &buf_type),
                      zc_alloc_fail);

    bufinfo = exynosMemPoolAllocateBO(pScrn, fourcc,  in_width, in_height, NULL , names, buf_type);

    XDBG_GOTO_IF_FAIL(bufinfo != NULL, zc_alloc_fail);
    bufinfo->pClient = client;
    exynosExaPixmapSetBo (pPixmap, bufinfo->tbos[0]);

    XDBG_TRACE (
        MVBUF,
        "pVideoPixmap(%p) ZeroCopy create(%c%c%c%c, %dx%d, handles[%u,%u,%u], name[%u,%u,%u])\n",
        pPixmap, FOURCC_STR(bufinfo->fourcc), in_width, in_height,
        bufinfo->handles[0].u32, bufinfo->handles[1].u32, bufinfo->handles[2].u32,
        bufinfo->names[0], bufinfo->names[1], bufinfo->names[2]);


    return pPixmap;

zc_alloc_fail:

    if (pPixmap)
        pScreen->DestroyPixmap (pPixmap);
    if (bufinfo)
        exynosMemPoolDestroyBO (bufinfo);
    pPixmap = NULL;
    bufinfo = NULL;
    XDBG_ERROR (MVBUF, "Can't create video buffer\n");
    return NULL;
}


PixmapPtr
exynosVideoBufferCreateInbufRAW (ScrnInfoPtr pScrn, int fourcc, int in_width, int in_height)
{
    EXYNOSBufInfoPtr bufinfo = NULL;
    ScreenPtr pScreen = pScrn->pScreen;
    PixmapPtr pPixmap = NULL;
    TRACE;
    bufinfo = exynosMemPoolCreateBO(pScrn, fourcc,  in_width, in_height);
    TRACE;
    XDBG_GOTO_IF_FAIL (bufinfo != NULL, raw_fail_alloc);
    TRACE;
    pPixmap = pScreen->CreatePixmap (pScreen, in_width, in_height, 32,
                                     CREATE_PIXMAP_USAGE_XV);
    TRACE;
    XDBG_GOTO_IF_FAIL (pPixmap != NULL, raw_fail_alloc);
    exynosExaPixmapSetBo (pPixmap, bufinfo->tbos[0]);
    TRACE;

    XDBG_TRACE(
        MVBUF,
        "VideoPixmap (%p)-stamp:(%lu) RAW create(%c%c%c%c, %dx%d, h[%u,%u,%u], n[%u,%u,%u])\n",
        pPixmap, STAMP(pPixmap), FOURCC_STR(bufinfo->fourcc), in_width, in_height,
        bufinfo->handles[0].u32, bufinfo->handles[1].u32, bufinfo->handles[2].u32,
        bufinfo->names[0], bufinfo->names[1], bufinfo->names[2]);

    XDBG_TRACE(
        MVBUF,
        "Fourcc(%c%c%c%c),w(%d),h(%d),p(%d,%d,%d),o(%d,%d,%d),l(%d,%d,%d)\n",
        FOURCC_STR (fourcc), in_width, in_height,
        bufinfo->pitches[0], bufinfo->pitches[1], bufinfo->pitches[2],
        bufinfo->offsets[0], bufinfo->offsets[1], bufinfo->offsets[2],
        bufinfo->lengths[0], bufinfo->lengths[1], bufinfo->lengths[2]);

    return pPixmap;

raw_fail_alloc:

    if (pPixmap)
        pScreen->DestroyPixmap (pPixmap);
    if (bufinfo)
        exynosMemPoolDestroyBO (bufinfo);
    pPixmap = NULL;
    bufinfo = NULL;
    XDBG_ERROR(MVBUF, "Can't create video buffer\n");
    return NULL;
}

PixmapPtr
exynosVideoBufferModifyOutbufPixmap(ScrnInfoPtr pScrn, int fourcc, int out_width,
                                    int out_height, PixmapPtr pPixmap)
{
    EXYNOSPtr pExynos = NULL;
    ScreenPtr pScreen = NULL;
    EXYNOSBufInfoPtr bufinfo = NULL;
    tbm_bo tbm_bos[PLANE_CNT];
    XDBG_RETURN_VAL_IF_FAIL(pScrn != NULL, NULL);
    XDBG_RETURN_VAL_IF_FAIL (pPixmap != NULL, NULL);
    pExynos = EXYNOSPTR (pScrn);
    pScreen = pPixmap->drawable.pScreen;
    XDBG_RETURN_VAL_IF_FAIL (pScreen != NULL, NULL);

    if (_exynosVideoBufferGetInfo (pPixmap))
    {
        XDBG_ERROR(MVBUF, "PrivateData of Pixmap(%p)-(stamp:%d) is already created.\n",
                   pPixmap, STAMP(pPixmap));
        return exynosVideoBufferRef (pPixmap);
    }

    tbm_bos[0] = exynosExaPixmapGetBo (pPixmap);

    bufinfo = exynosMemPoolAllocateBO (pScrn, fourcc, out_width, out_height, tbm_bos,
                                       NULL, BUF_TYPE_VPIX);
    XDBG_GOTO_IF_FAIL (bufinfo != NULL, alloc_fail);

    /** @todo Modify Pixmap Header */
#if 0
    XDBG_GOTO_IF_FAIL (!_videoBufferObjectCreate(bufinfo), alloc_fail);

    old_bo = exynosExaPixmapGetBo(pPixmap);

    if (!bufinfo->tbos[0])
    {

    }
    XDBG_GOTO_IF_FAIL(bufinfo->tbos[0] != NULL, alloc_fail);

    pPixmap->usage_hint = CREATE_PIXMAP_USAGE_XV;
    pScreen->ModifyPixmapHeader(pPixmap, bufinfo->width, bufinfo->height,
                                image_info->depth, image_info->bits_per_pixel, bufinfo->width * 4, 0);
#endif

#if 0
    EXYNOSExaPrepareAccess (pPixmap, EXA_PREPARE_DEST);
    memset (pPixmap->devPrivate.ptr, 0x0, bufinfo->size);
    EXYNOSExaFinishAccess (pPixmap, EXA_PREPARE_DEST);
    exynosUtilCacheFlush (pPort->pScrn);
#endif

    XDBG_TRACE(
        MVBUF,
        "Pixmap(%p)-stamp:(%lu) OUTDraw Create (%c%c%c%c) %dx%d, h[%u,%u,%u], n[%u,%u,%u])\n",
        pPixmap, STAMP(pPixmap), FOURCC_STR(bufinfo->fourcc), out_width, out_height,
        bufinfo->handles[0].u32, bufinfo->handles[1].u32, bufinfo->handles[2].u32,
        bufinfo->names[0], bufinfo->names[1], bufinfo->names[2]);
    return exynosVideoBufferRef (pPixmap);

alloc_fail:

    if (pPixmap)
        pScreen->DestroyPixmap (pPixmap);
    if (bufinfo)
        exynosMemPoolDestroyBO (bufinfo);
    pPixmap = NULL;
    bufinfo = NULL;
    XDBG_ERROR(MVBUF, "Can't create video buffer\n");
    return NULL;
}

PixmapPtr
exynosVideoBufferCreateOutbufFB(ScrnInfoPtr pScrn, int fourcc, int out_width, int out_height)
{
    XDBG_RETURN_VAL_IF_FAIL (pScrn != NULL, NULL);
    PixmapPtr pPixmap = NULL;
    EXYNOSBufInfoPtr bufinfo = NULL;
    ScreenPtr pScreen = pScrn->pScreen;
    bufinfo = exynosMemPoolCreateBO(pScrn, fourcc, out_width, out_height);
    XDBG_GOTO_IF_FAIL(bufinfo != NULL, alloc_fail);

    pPixmap = pScreen->CreatePixmap (pScreen, out_width, out_height,
                                     32, CREATE_PIXMAP_USAGE_XV);

    XDBG_GOTO_IF_FAIL(pPixmap != NULL, alloc_fail);
    exynosExaPixmapSetBo (pPixmap, bufinfo->tbos[0]);

    bufinfo->fb_id = exynosVideoBufferCreateFBID (pPixmap);

    XDBG_GOTO_IF_FAIL (bufinfo->fb_id != 0, alloc_fail);
    XDBG_TRACE(
        MVBUF,
        "Pixmap(%p)-stamp:(%lu) OUTFB Create (%c%c%c%c) %dx%d, h[%u,%u,%u], n[%u,%u,%u])\n",
        pPixmap, STAMP(pPixmap), FOURCC_STR(bufinfo->fourcc), out_width, out_height,
        bufinfo->handles[0].u32, bufinfo->handles[1].u32, bufinfo->handles[2].u32,
        bufinfo->names[0], bufinfo->names[1], bufinfo->names[2]);

    return pPixmap;
alloc_fail:

    if (pPixmap)
        pScreen->DestroyPixmap (pPixmap);
    if (bufinfo)
        exynosMemPoolDestroyBO (bufinfo);
    XDBG_ERROR (MIA, "Can't allocate FBOut\n");
    return NULL;
}

Bool
exynosVideoBufferCopyRawData ( PixmapPtr pPixmap, pointer * src_sample, int *src_length)
{
    EXYNOSBufInfoPtr bufinfo = _exynosVideoBufferGetInfo (pPixmap);
    XDBG_RETURN_VAL_IF_FAIL(bufinfo != NULL, FALSE);
    tbm_bo_handle handles[PLANE_CNT];
    int i = 0;
    XDBG_DEBUG(MVBUF, "bufinfo %p\n", bufinfo);
    XDBG_RETURN_VAL_IF_FAIL (
        exynosMemPoolPrepareAccess ( bufinfo, handles, OPTION_WRITE ), FALSE );
    for (i = 0; i < bufinfo->num_planes; i++)
    {
        if (src_sample[i] != NULL && src_length[i] != 0
                && bufinfo->lengths[i] == src_length[i])
        {
            memcpy (handles[i].ptr, src_sample[i], src_length[i]);
        }
        else
        {
            exynosMemPoolFinishAccess ( bufinfo );
            return FALSE;
        }
    }
    exynosMemPoolFinishAccess ( bufinfo );
    exynosMemPoolCacheFlush ( bufinfo->pScrn );
    return TRUE;

}

Bool
exynosVideoBufferCopyOneChannel (PixmapPtr pPixmap, pointer * src_sample, int *src_pitches, int *src_height)
{
    EXYNOSBufInfoPtr bufinfo = _exynosVideoBufferGetInfo (pPixmap);
    XDBG_RETURN_VAL_IF_FAIL(bufinfo != NULL, FALSE);
    tbm_bo_handle handles[PLANE_CNT];
    int i = 0, j = 0;
    XDBG_DEBUG(MVBUF, "bufinfo %p\n", bufinfo);
    XDBG_RETURN_VAL_IF_FAIL (
        exynosMemPoolPrepareAccess ( bufinfo, handles, OPTION_WRITE ), FALSE );
    for (i = 0; i < bufinfo->num_planes; i++)
    {
        if (src_sample[i] != NULL)
        {
            unsigned char* temp_dst = (unsigned char*) handles[i].ptr;
            unsigned char* temp_src = (unsigned char*) src_sample[i];
            
            for (j = 0;j < src_height[i]; j++)
            {
                memcpy (temp_dst, temp_src, src_pitches[i]);
                temp_dst += bufinfo->pitches[i];
                temp_src += src_pitches[i];
            }
        }
        else
        {
            exynosMemPoolFinishAccess ( bufinfo );
            return FALSE;
        }
    }
    exynosMemPoolFinishAccess ( bufinfo );
    exynosMemPoolCacheFlush ( bufinfo->pScrn );
    return TRUE;    
}

