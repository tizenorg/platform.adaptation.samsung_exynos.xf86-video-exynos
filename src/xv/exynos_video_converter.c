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

#include "exynos_driver.h"
#include "exynos_util.h"
#include "exynos_video_fourcc.h"
#include "exynos_video_buffer.h"
#include "exynos_video_converter.h"
#include "exynos_mem_pool.h"
#include "xv_types.h"
#include "exynos_video_ipp.h"
#include <exynos_drm.h>

static void
_exynosVideoConverterSetupConfig (EXYNOSCvtType type, EXYNOSIppProp *prop,
                                  struct drm_exynos_ipp_config *config)
{
    config->ops_id = (type == CVT_TYPE_SRC) ? EXYNOS_DRM_OPS_SRC : EXYNOS_DRM_OPS_DST;

    if (prop->hflip)
        config->flip |= EXYNOS_DRM_FLIP_HORIZONTAL;
    if (prop->vflip)
        config->flip |= EXYNOS_DRM_FLIP_VERTICAL;

    config->fmt = exynosVideoBufferGetDrmFourcc (prop->id);
    config->sz.hsize = (__u32 ) prop->width;
    config->sz.vsize = (__u32 ) prop->height;
    config->pos.x = (__u32 ) prop->crop.x;
    config->pos.y = (__u32 ) prop->crop.y;
    config->pos.w = (__u32 ) prop->crop.width;
    config->pos.h = (__u32 ) prop->crop.height;

    switch (prop->degree % 360)
    {
    case 90:
        config->degree = EXYNOS_DRM_DEGREE_90;
        break;
    case 180:
        config->degree = EXYNOS_DRM_DEGREE_180;
        break;
    case 270:
        config->degree = EXYNOS_DRM_DEGREE_270;
        break;
    default:
        config->degree = EXYNOS_DRM_DEGREE_0;
    }

}
/*
static Bool
_exynosVideoConverterCompareIppBufStamp (pointer structure, pointer element)
{
    CARD32 stamp = * ((CARD32 *) element);
    EXYNOSIppBufPtr pIpp_buf = (EXYNOSIppBufPtr) structure;
    return (pIpp_buf->stamp == stamp);
}
*/
static Bool
_exynosVideoConverterCompareIppDataStamp (pointer structure, pointer element)
{
    CARD32 stamp = * ((CARD32 *) element);
    EXYNOSIppPtr pIpp_data = (EXYNOSIppPtr) structure;
    return (pIpp_data->stamp == stamp);
}

static Bool
_exynosVideoConverterCompareIppBufIndex (pointer structure, pointer element)
{
    unsigned int index = * ((unsigned int *) element);
    EXYNOSIppBuf *buf = (EXYNOSIppBuf *) structure;
    return (buf->index == index);
}

#if 0
static Bool
_imageChanged(EXYNOSCvtProp * prop_old, EXYNOSCvtProp * prop_new)
{
    if (memcmp(prop_old, prop_new, sizeof(EXYNOSCvtProp)))
    {
        XDBG_ERROR(MIA, "Prop changed!\n");
        XDBG_TRACE(MIA,
                   "prop_old('%c%c%c%c', %dx%d, %d,%d %dx%d, %d, %d&%d)\n",
                   FOURCC_STR(prop_old->id), prop_old->width, prop_old->height,
                   prop_old->crop.x, prop_old->crop.y, prop_old->crop.width,
                   prop_old->crop.height, prop_old->degree, prop_old->hflip,
                   prop_old->vflip);
        XDBG_TRACE(MIA,
                   "prop_new('%c%c%c%c', %dx%d, %d,%d %dx%d, %d, %d&%d)\n",
                   FOURCC_STR(prop_new->id), prop_new->width, prop_new->height,
                   prop_new->crop.x, prop_new->crop.y, prop_new->crop.width,
                   prop_new->crop.height, prop_new->degree, prop_new->hflip,
                   prop_new->vflip);

        return TRUE;
    }
    return FALSE;
}

#endif


static Bool
_exynosVideoConverterIppQueueBuf (IMAGEPortInfoPtr pPort, EXYNOSIppPtr pIpp_data,
                                  EXYNOSIppBuf *pIpp_buf)
{
    XDBG_RETURN_VAL_IF_FAIL(pIpp_data != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL(pIpp_buf != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL(pIpp_data->prop_id >= 0, FALSE);
    struct drm_exynos_ipp_queue_buf buf = { 0, };
    int i = 0;
    int fd = EXYNOSPTR (pPort->pScrn)->drm_fd;

    buf.prop_id = pIpp_data->prop_id;
    buf.ops_id =
        (pIpp_buf->type == CVT_TYPE_SRC) ? EXYNOS_DRM_OPS_SRC : EXYNOS_DRM_OPS_DST;
    buf.buf_type = IPP_BUF_ENQUEUE;
    buf.buf_id = pIpp_buf->index;
    buf.user_data = (__u64) pPort; /** @todo Need correct cast */

    for (i = 0; i < PLANE_CNT; i++)
        if (pIpp_buf->handles[i].u32)
        {
            buf.handle[i] = pIpp_buf->handles[i].u32;
        }

    XDBG_RETURN_VAL_IF_FAIL(exynosVideoIppQueueBuf (fd, &buf), FALSE)
    return TRUE;
}

Bool
exynosVideoConverterIppDequeueBuf (IMAGEPortInfoPtr pPort, EXYNOSIppPtr pIpp_data, EXYNOSIppBuf *pIpp_buf)
{
    XDBG_RETURN_VAL_IF_FAIL (pIpp_data != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (pIpp_buf != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (pIpp_data->prop_id >= 0, FALSE);
    struct drm_exynos_ipp_queue_buf ipp_buf = {0,};
    int i = 0;
    int fd = EXYNOSPTR (pPort->pScrn)->drm_fd;
    if (!exynosUtilStorageFindData(
                ((pIpp_buf->type == CVT_TYPE_SRC) ?
                 pIpp_data->list_of_inbuf : pIpp_data->list_of_outbuf),
                &pIpp_buf->index, _exynosVideoConverterCompareIppBufIndex))
    {
        XDBG_TRACE(MIA, "pIpp_data(%p) type(%d), index(%d) already dequeued!\n",
                   pIpp_data, pIpp_buf->type, pIpp_buf->index);
        return TRUE;
    }
    ipp_buf.prop_id = pIpp_data->prop_id;
    ipp_buf.ops_id = (pIpp_buf->type == CVT_TYPE_SRC) ? EXYNOS_DRM_OPS_SRC : EXYNOS_DRM_OPS_DST;
    ipp_buf.buf_type = IPP_BUF_DEQUEUE;
    ipp_buf.buf_id = pIpp_buf->index;
    ipp_buf.user_data = (__u64) pPort;

    for (i = 0; i < EXYNOS_DRM_PLANAR_MAX; i++)
        if (pIpp_buf->handles[i].u32)
        {
            ipp_buf.handle[i] = pIpp_buf->handles[i].u32;
        }
    exynosVideoBufferConverting(pIpp_buf->pVideoPixmap, FALSE);
    XDBG_RETURN_VAL_IF_FAIL(exynosVideoIppQueueBuf (fd, &ipp_buf), FALSE);
    XDBG_TRACE(MIA, "pIpp_data(%p) type(%d), index(%d) dequeued!\n",
               pIpp_data, pIpp_buf->type, pIpp_buf->index);
    return TRUE;
}

static void
_exynosVideoConverterDequeueAllBuf (IMAGEPortInfoPtr pPort, EXYNOSIppPtr pIpp_data)
{
    XDBG_RETURN_IF_FAIL(pIpp_data != NULL);
    XDBG_RETURN_IF_FAIL(pPort != NULL);
    EXYNOSIppBuf *pIpp_buf = NULL;
    int i = 0, size = 0;
    size = exynosUtilStorageGetSize(pIpp_data->list_of_outbuf);
    XDBG_RETURN_IF_FAIL (size >= 0);
    for (i = 0 ; i < size; i++)
    {
        pIpp_buf = exynosUtilStorageGetFirst (pIpp_data->list_of_outbuf);
        XDBG_RETURN_IF_FAIL(pIpp_buf != NULL);
        exynosVideoConverterIppDequeueBuf (pPort, pIpp_data, pIpp_buf);
        exynosUtilStorageEraseData (pIpp_data->list_of_outbuf, pIpp_buf);
        ctrl_free (pIpp_buf);
        pIpp_buf = NULL;

    }

    size = exynosUtilStorageGetSize(pIpp_data->list_of_inbuf);
    XDBG_RETURN_IF_FAIL (size >= 0);
    for (i = 0; i < size; i++)
    {
        pIpp_buf = exynosUtilStorageGetFirst (pIpp_data->list_of_inbuf);
        XDBG_RETURN_IF_FAIL(pIpp_buf != NULL);
        exynosVideoConverterIppDequeueBuf (pPort, pIpp_data, pIpp_buf);
        exynosUtilStorageEraseData (pIpp_data->list_of_inbuf, pIpp_buf);
        ctrl_free (pIpp_buf);
        pIpp_buf = NULL;
    }
    exynosUtilStorageFinit (pPort->last_ipp_data->list_of_inbuf);
    exynosUtilStorageFinit (pPort->last_ipp_data->list_of_outbuf);
    pPort->last_ipp_data->list_of_inbuf = NULL;
    pPort->last_ipp_data->list_of_outbuf = NULL;
    XDBG_DEBUG(MIA, "pIpp_data(%p) dequeued!\n", pIpp_data);
}

static EXYNOSIpp *
_exynosVideoConverterSetupIppData (IMAGEPortInfoPtr pPort, PixmapPtr src, PixmapPtr dst)
{

    XDBG_RETURN_VAL_IF_FAIL(src != NULL, NULL);
    XDBG_RETURN_VAL_IF_FAIL(pPort != NULL, NULL);
    XDBG_RETURN_VAL_IF_FAIL(dst != NULL, NULL);
    EXYNOSIppPtr pIpp_data = NULL;
    pIpp_data = ctrl_calloc (1, sizeof(EXYNOSIpp));

    XDBG_RETURN_VAL_IF_FAIL(pIpp_data != NULL, NULL);
    XDBG_DEBUG(MIA, "Setup Ipp_data\n");
    /* SRC */
    pIpp_data->props[CVT_TYPE_SRC].id = exynosVideoBufferGetFourcc (src);
    pIpp_data->props[CVT_TYPE_SRC].width = src->drawable.width;
    pIpp_data->props[CVT_TYPE_SRC].height = src->drawable.height;
    pIpp_data->props[CVT_TYPE_SRC].crop = pPort->in_crop;

    /* DST */
    pIpp_data->props[CVT_TYPE_DST].id = exynosVideoBufferGetFourcc (dst); /* \remark FOURCC_RGB32 */
    pIpp_data->props[CVT_TYPE_DST].width = dst->drawable.width;
    pIpp_data->props[CVT_TYPE_DST].height = dst->drawable.height;
    pIpp_data->props[CVT_TYPE_DST].crop = pPort->out_crop;
    pIpp_data->props[CVT_TYPE_DST].degree = pPort->hw_rotate;
    pIpp_data->props[CVT_TYPE_DST].hflip = pPort->hflip;
    pIpp_data->props[CVT_TYPE_DST].vflip = pPort->vflip;

    pIpp_data->prop_id = -1;

    CARD32 stamp = GetTimeInMillis ();
    while (exynosUtilStorageFindData (pPort->list_of_ipp, &stamp,
                                      _exynosVideoConverterCompareIppDataStamp))
        stamp++;

    pIpp_data->stamp = stamp;
    pIpp_data->pScrn = pPort->pScrn;

    XDBG_DEBUG(MIA, "Ipp_data(%p) stamp(%ld) created\n", pIpp_data, stamp);
    return pIpp_data;
}

static Bool
_exynosVideoConverterHasKeys (int id)
{
    switch (id)
    {
    case FOURCC_SR16:
    case FOURCC_SR32:
    case FOURCC_S420:
    case FOURCC_ST12:
    case FOURCC_SUYV:
    case FOURCC_SN12:
    case FOURCC_SN21:
    case FOURCC_SYVY:
    case FOURCC_ITLV:
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
        XDBG_NEVER_GET_HERE(MCVT);
        return FALSE;
    }
}




static void
_exynosVideoConverterRotation (IMAGEPortInfoPtr pPort)
{

    /*!
     *     [Screen]            ----------
     *                         |        |
     *     Top (RR_Rotate_90)  |        |  Top (RR_Rotate_270)
     *                         |        |
     *                         ----------
     *     [Fimc]              ----------
     *                         |        |
     *              Top (270)  |        |  Top (90)
     *                         |        |
     *                         ----------
     * rotate, hw_rotate : as same as Fimc's direction.
     *
     *    - RR_Rotate_270 + rotate (90)  = hw_rotate (180)
     *    - RR_Rotate_270 + rotate (180) = hw_rotate (270)
     *    - RR_Rotate_270 + rotate (270) = hw_rotate (0)
     */

#if 0
    switch (pExynos->rotate)
    {
    case RR_Rotate_90:
        *scn_rotate = 90;
        break;
    case RR_Rotate_180:
        *scn_rotate = 180;
        break;
    case RR_Rotate_270:
        *scn_rotate = 270;
        break;
    default:
        break;
    }

    pPort->hw_rotate = (pPort->rotate + (360 - scn_rotate)) % 360;
#endif
    pPort->hw_rotate = pPort->rotate;
}

static Bool
_exynosVideoConverterSetProperty (IMAGEPortInfoPtr pPort, PixmapPtr src, PixmapPtr dst)
{
    XDBG_RETURN_VAL_IF_FAIL(pPort != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL(src != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL(dst != NULL, FALSE);

    struct drm_exynos_ipp_config src_conf = { 0, }, dst_conf = { 0, };
    int fd = EXYNOSPTR (pPort->pScrn)->drm_fd;
    EXYNOSIppPtr pIpp_data = NULL;

    if (pPort->last_ipp_data)
        return TRUE;

    _exynosVideoConverterRotation (pPort);

    pIpp_data = _exynosVideoConverterSetupIppData (pPort, src, dst);

    XDBG_RETURN_VAL_IF_FAIL(pIpp_data != NULL, FALSE);

    exynosUtilStorageAdd (pPort->list_of_ipp, pIpp_data);

    /** @todo CheckFormat */
#if 0
    if (!secCvtSupportFormat(cvt->op, src->id))
    {
        XDBG_ERROR(MCVT, "cvt(%p) src '%c%c%c%c' not supported.\n", cvt,
                   FOURCC_STR (src->id));
        return FALSE;
    }

    if (!secCvtSupportFormat(cvt->op, dst->id))
    {
        XDBG_ERROR(MCVT, "cvt(%p) dst '%c%c%c%c' not supported.\n", cvt,
                   FOURCC_STR (dst->id));
        return FALSE;
    }
#endif

    XDBG_TRACE(
        MIA,
        "Ipp_data(%p) src('%c%c%c%c', '%c%c%c%c', %dx%d, %d,%d %dx%d, %d, %d&%d)\n",
        pIpp_data, FOURCC_STR(pIpp_data->props[CVT_TYPE_SRC].id),
        FOURCC_STR(exynosVideoBufferGetDrmFourcc(pIpp_data->props[CVT_TYPE_SRC].id)),
        pIpp_data->props[CVT_TYPE_SRC].width, pIpp_data->props[CVT_TYPE_SRC].height,
        pIpp_data->props[CVT_TYPE_SRC].crop.x, pIpp_data->props[CVT_TYPE_SRC].crop.y,
        pIpp_data->props[CVT_TYPE_SRC].crop.width,
        pIpp_data->props[CVT_TYPE_SRC].crop.height,
        pIpp_data->props[CVT_TYPE_SRC].degree, pIpp_data->props[CVT_TYPE_SRC].hflip,
        pIpp_data->props[CVT_TYPE_SRC].vflip);

    XDBG_TRACE(
        MIA,
        "Ipp_data(%p) dst('%c%c%c%c', '%c%c%c%c', %dx%d, %d,%d %dx%d, %d, %d&%d)\n",
        pIpp_data, FOURCC_STR(pIpp_data->props[CVT_TYPE_DST].id),
        FOURCC_STR(exynosVideoBufferGetDrmFourcc(pIpp_data->props[CVT_TYPE_DST].id)),
        pIpp_data->props[CVT_TYPE_DST].width, pIpp_data->props[CVT_TYPE_DST].height,
        pIpp_data->props[CVT_TYPE_DST].crop.x, pIpp_data->props[CVT_TYPE_DST].crop.y,
        pIpp_data->props[CVT_TYPE_DST].crop.width,
        pIpp_data->props[CVT_TYPE_DST].crop.height,
        pIpp_data->props[CVT_TYPE_DST].degree, pIpp_data->props[CVT_TYPE_DST].hflip,
        pIpp_data->props[CVT_TYPE_DST].vflip);

    _exynosVideoConverterSetupConfig (CVT_TYPE_SRC, &pIpp_data->props[CVT_TYPE_SRC],
                                      &src_conf);
    _exynosVideoConverterSetupConfig (CVT_TYPE_DST, &pIpp_data->props[CVT_TYPE_DST],
                                      &dst_conf);

    pIpp_data->prop_id = exynosVideoIppSetProperty (fd, IPP_CMD_M2M, pIpp_data->prop_id,
                         &src_conf, &dst_conf, 0, 0, 0);

    XDBG_RETURN_VAL_IF_FAIL(pIpp_data->prop_id >= 0, FALSE);

    if (!pIpp_data->list_of_inbuf)
        pIpp_data->list_of_inbuf = exynosUtilStorageInit ();
    if (!pIpp_data->list_of_outbuf)
        pIpp_data->list_of_outbuf = exynosUtilStorageInit ();

    pPort->last_ipp_data = pIpp_data;

    return TRUE;
}

static Bool
_exynosVideoConverterSetBuf (IMAGEPortInfoPtr pPort, PixmapPtr src, PixmapPtr dst)
{
    XDBG_RETURN_VAL_IF_FAIL(src != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL(dst != NULL, FALSE);
    CARD32 stamp = 0;
    unsigned int ret = 0, size = 0;
    EXYNOSIppBuf *pIpp_buf_src = NULL, *pIpp_buf_dst = NULL;
    EXYNOSIpp *pIpp_data = pPort->last_ipp_data;
    XDBG_RETURN_VAL_IF_FAIL(pIpp_data != NULL, FALSE);

    pIpp_buf_src = ctrl_calloc (1, sizeof(EXYNOSIppBuf));
    XDBG_GOTO_IF_FAIL(pIpp_buf_src != NULL, fail_to_convert);
    pIpp_buf_dst = ctrl_calloc (1, sizeof(EXYNOSIppBuf));
    XDBG_GOTO_IF_FAIL(pIpp_buf_dst != NULL, fail_to_convert);

    XDBG_DEBUG(MIA, "Init ipp_buf_src\n");

    pIpp_buf_src->type = CVT_TYPE_SRC;
    pIpp_buf_src->pVideoPixmap = src;
    ret = 0;
    stamp = GetTimeInMillis ();
    
    pIpp_buf_src->stamp = stamp;
    size = exynosUtilStorageGetSize (pIpp_data->list_of_inbuf);
    XDBG_DEBUG(MIA, "Size of ipp_buf_src %u\n", size);
    while (exynosUtilStorageFindData (pIpp_data->list_of_inbuf, &ret,
                                      _exynosVideoConverterCompareIppBufIndex))
    {
        ++ret;
    }

    pIpp_buf_src->index = ret;
    exynosVideoBufferGetAttr (src, NULL, pIpp_buf_src->handles, NULL);
    exynosUtilStorageAdd (pIpp_data->list_of_inbuf, pIpp_buf_src);
    XDBG_DEBUG(MIA, "ipp_buf_src index %u src_handles (%u %u %u) stamp (%lu)\n",
               pIpp_buf_src->index,
               pIpp_buf_src->handles[0].u32, pIpp_buf_src->handles[1].u32,
               pIpp_buf_src->handles[2].u32, pIpp_buf_src->stamp);

    XDBG_DEBUG(MIA, "Init ipp_buf_dst\n");
    pIpp_buf_dst->type = CVT_TYPE_DST;
    pIpp_buf_dst->pVideoPixmap = dst;
    ret = 0;
    pIpp_buf_dst->stamp = stamp;
    size = exynosUtilStorageGetSize (pIpp_data->list_of_outbuf);
    XDBG_DEBUG(MIA, "Size of ipp_buf_dst %u\n", size);
    while (exynosUtilStorageFindData (pIpp_data->list_of_outbuf, &ret,
                                      _exynosVideoConverterCompareIppBufIndex))
    {
        ++ret;
    }
    pIpp_buf_dst->index = ret;
    exynosVideoBufferGetAttr (dst, NULL, pIpp_buf_dst->handles, NULL);
    exynosUtilStorageAdd (pIpp_data->list_of_outbuf, pIpp_buf_dst);
    XDBG_DEBUG(MIA, "ipp_buf_dst index %u dst_handles (%u %u %u) stamp (%lu)\n",
               pIpp_buf_dst->index,
               pIpp_buf_dst->handles[0].u32, pIpp_buf_dst->handles[1].u32,
               pIpp_buf_dst->handles[2].u32, pIpp_buf_dst->stamp);

    XDBG_DEBUG(MIA, "Setup Ipp_buf\n");

    /** @todo Add Image change check */
    //   XDBG_RETURN_VAL_IF_FAIL(!_imageChanged(&cvt->props[CVT_TYPE_SRC], &src_prop), FALSE);
    //   XDBG_RETURN_VAL_IF_FAIL(!_imageChanged(&cvt->props[CVT_TYPE_DST], &dst_prop), FALSE);
    XDBG_GOTO_IF_FAIL(_exynosVideoConverterIppQueueBuf (pPort, pIpp_data, pIpp_buf_src),
                      fail_to_convert);
    exynosVideoBufferConverting(src, TRUE);
    XDBG_GOTO_IF_FAIL(_exynosVideoConverterIppQueueBuf (pPort, pIpp_data, pIpp_buf_dst),
                      fail_to_convert);
    exynosVideoBufferConverting(dst, TRUE);

    return TRUE;

fail_to_convert:

    if (pIpp_buf_src)
        ctrl_free (pIpp_buf_src);
    exynosVideoBufferConverting(src, FALSE);
    if (pIpp_buf_dst)
        ctrl_free (pIpp_buf_dst);
    exynosVideoBufferConverting(dst, FALSE);
    XDBG_ERROR(MIA, "Can't PUT Buf Queue\n");
    return FALSE;
}

void
exynosVideoConverterClose(IMAGEPortInfoPtr pPort)
{
    XDBG_RETURN_IF_FAIL (pPort != NULL);
    XDBG_DEBUG(MIA, "Close Converter\n");
    if (!pPort->last_ipp_data)
        return;

    struct drm_exynos_ipp_cmd_ctrl ctrl = { 0, };
    ctrl.prop_id = pPort->last_ipp_data->prop_id;
    ctrl.ctrl = IPP_CTRL_STOP;
    _exynosVideoConverterDequeueAllBuf (pPort, pPort->last_ipp_data);
    exynosVideoIppCmdCtrl (EXYNOSPTR(pPort->pScrn)->drm_fd, &ctrl);
    exynosUtilStorageEraseData (pPort->list_of_ipp, pPort->last_ipp_data);
    /** @todo Clear (list_of_...) */

    pPort->last_ipp_data->prop_id = -1;
    memset (pPort->last_ipp_data->props, 0, sizeof(EXYNOSIppProp) * CVT_TYPE_MAX);
    ctrl_free (pPort->last_ipp_data);
    pPort->last_ipp_data = NULL;

    XDBG_DEBUG(MIA, "Close Converter FINISH\n");
}

Bool
exynosVideoConverterAdaptSize (EXYNOSIppProp *src, EXYNOSIppProp *dst)
{
    if (src)
    {
        int type = exynosVideoBufferGetColorType (src->id);

        XDBG_RETURN_VAL_IF_FAIL(src->width >= 16, FALSE);
        XDBG_RETURN_VAL_IF_FAIL(src->height >= 8, FALSE);
        XDBG_RETURN_VAL_IF_FAIL(src->crop.width >= 16, FALSE);
        XDBG_RETURN_VAL_IF_FAIL(src->crop.height >= 8, FALSE);
        XDBG_RETURN_VAL_IF_FAIL(src->crop.x <= src->width, FALSE);
        XDBG_RETURN_VAL_IF_FAIL(src->crop.y <= src->height, FALSE);

        if (src->width % 16)
        {
            if (!_exynosVideoConverterHasKeys (src->id))
            {
                int new_width = (src->width + 16) & (~0xF);
                XDBG_DEBUG(MIA, "src's width : %d to %d.\n", src->width, new_width);
                src->width = new_width;
            }
        }

        if (type == TYPE_YUV420 && src->height % 2)
            XDBG_WARNING(MIA, "src's height(%d) is not multiple of 2!!!\n", src->height);

        if (type == TYPE_YUV420 || type == TYPE_YUV422)
        {
            src->crop.x = src->crop.x & (~0x1);
            src->crop.width = src->crop.width & (~0x1);
        }

        if (type == TYPE_YUV420)
            src->crop.height = src->crop.height & (~0x1);

        if (src->crop.x + src->crop.width > src->width)
            src->crop.width = src->width - src->crop.x;
        if (src->crop.width > src->width)
            return FALSE;

        if (src->crop.y + src->crop.height > src->height)
            src->crop.height = src->height - src->crop.y;
        if (src->crop.height > src->height)
            return FALSE;
    }

    if (dst)
    {
        int type = exynosVideoBufferGetColorType (dst->id);

        XDBG_RETURN_VAL_IF_FAIL(dst->width >= 16, FALSE);
        XDBG_RETURN_VAL_IF_FAIL(dst->height >= 8, FALSE);
        XDBG_RETURN_VAL_IF_FAIL(dst->crop.width >= 16, FALSE);
        XDBG_RETURN_VAL_IF_FAIL(dst->crop.height >= 4, FALSE);
        XDBG_RETURN_VAL_IF_FAIL(dst->crop.x <= dst->width, FALSE);
        XDBG_RETURN_VAL_IF_FAIL(dst->crop.y <= dst->height, FALSE)

        if (dst->width % 16)
        {
            int new_width = (dst->width + 16) & (~0xF);
            XDBG_DEBUG(MIA, "dst's width : %d to %d.\n", dst->width, new_width);
            dst->width = new_width;
        }

        dst->height = dst->height & (~0x1);

        if (type == TYPE_YUV420 && dst->height % 2)
            XDBG_WARNING(MIA, "dst's height(%d) is not multiple of 2!!!\n", dst->height);

        if (type == TYPE_YUV420 || type == TYPE_YUV422)
        {
            dst->crop.x = dst->crop.x & (~0x1);
            dst->crop.width = dst->crop.width & (~0x1);
        }

        if (type == TYPE_YUV420)
            dst->crop.height = dst->crop.height & (~0x1);

        if (dst->crop.x + dst->crop.width > dst->width)
            dst->crop.width = dst->width - dst->crop.x;
        if (dst->crop.width > dst->width)
            return FALSE;

        if (dst->crop.y + dst->crop.height > dst->height)
            dst->crop.height = dst->height - dst->crop.y;
        if (dst->crop.height > dst->height)
            return FALSE;
    }

    return TRUE;
}

Bool
exynosVideoConverterOpen (IMAGEPortInfoPtr pPort, PixmapPtr src, PixmapPtr dst)
{

    XDBG_DEBUG(MIA, "Enter %p src %p dst %p\n", pPort, src, dst);
    int fd = EXYNOSPTR (pPort->pScrn)->drm_fd;

    XDBG_GOTO_IF_FAIL(_exynosVideoConverterSetProperty(pPort, src, dst), fail_convert)

    XDBG_GOTO_IF_FAIL(_exynosVideoConverterSetBuf(pPort, src, dst), fail_convert)

    if (!pPort->last_ipp_data->started)
    {
        struct drm_exynos_ipp_cmd_ctrl ctrl = {0,};

        ctrl.prop_id = pPort->last_ipp_data->prop_id;
        ctrl.ctrl = IPP_CTRL_PLAY;

        XDBG_GOTO_IF_FAIL(exynosVideoIppCmdCtrl (fd, &ctrl), fail_convert);

        XDBG_TRACE (MIA, "IPP Converter (%p) start.\n", pPort->last_ipp_data);
        pPort->last_ipp_data->started = TRUE;
    }
    return TRUE;
fail_convert:
    exynosVideoBufferConverting(src, FALSE);
    exynosVideoBufferConverting(dst, FALSE);
    XDBG_ERROR (MIA, "Can't start HW Converter.\n");
    return FALSE;
}
