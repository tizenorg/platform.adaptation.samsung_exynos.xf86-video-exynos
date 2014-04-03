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

#include <sys/ioctl.h>
#include "exynos_driver.h"
#include "exynos_video_fourcc.h"
#include "exynos_video_ipp.h"
#include "exynos_util.h"
#include <drm_fourcc.h>

int
exynosVideoIppSetProperty (int fd,
                      enum drm_exynos_ipp_cmd cmd,
                      int prop_id,
                      struct drm_exynos_ipp_config *src,
                      struct drm_exynos_ipp_config *dst,
                      int wb_hz,
                      int protect,
                      int csc_range)
{
    struct drm_exynos_ipp_property property;
    int ret = 0;

    XDBG_RETURN_VAL_IF_FAIL (fd >= 0, -1);
    XDBG_RETURN_VAL_IF_FAIL (src != NULL, -1);
    XDBG_RETURN_VAL_IF_FAIL (dst != NULL, -1);

    XDBG_RETURN_VAL_IF_FAIL (src->ops_id == EXYNOS_DRM_OPS_SRC, -1);
    XDBG_RETURN_VAL_IF_FAIL (dst->ops_id == EXYNOS_DRM_OPS_DST, -1);

    XDBG_RETURN_VAL_IF_FAIL (cmd > 0, -1);

    CLEAR (property);
    property.config[EXYNOS_DRM_OPS_SRC] = *src;
    property.config[EXYNOS_DRM_OPS_DST] = *dst;

    XDBG_INFO (MIA, "src : flip(%x) deg(%d) fmt(%c%c%c%c) sz(%dx%d) pos(%d,%d %dx%d)\n",
               src->flip, src->degree, FOURCC_STR (src->fmt),
               src->sz.hsize, src->sz.vsize,
               src->pos.x, src->pos.y, src->pos.w, src->pos.h);
    XDBG_INFO (MIA, "dst : flip(%x) deg(%d) fmt(%c%c%c%c) sz(%dx%d) pos(%d,%d %dx%d)\n",
               dst->flip, dst->degree, FOURCC_STR (dst->fmt),
               dst->sz.hsize, dst->sz.vsize,
               dst->pos.x, dst->pos.y, dst->pos.w, dst->pos.h);

    property.cmd = cmd;
    if (prop_id >= 0)
        property.prop_id = prop_id;

    property.refresh_rate = wb_hz;
    property.protect = protect;
    property.range = csc_range;

    XDBG_INFO (MIA, "cmd(%d) ipp_id(%d) prop_id(%d) hz(%d) protect(%d) range(%d)\n",
               cmd, property.ipp_id, prop_id, wb_hz, protect, csc_range);

    ret = drmIoctl (fd, DRM_IOCTL_EXYNOS_IPP_SET_PROPERTY, &property);
    if (ret)
    {
        XDBG_ERRNO (MIA, "failed. \n");
        return -1;
    }

//    XDBG_RETURN_VAL_IF_FAIL (property.prop_id > 0, 0);

    XDBG_TRACE (MIA, "success. prop_id(%d) \n", property.prop_id);

    return property.prop_id;
}

Bool
exynosVideoIppQueueBuf (int fd, struct drm_exynos_ipp_queue_buf *buf)
{
    int ret = 0;

    XDBG_RETURN_VAL_IF_FAIL (fd >= 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (buf != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (buf->handle[0] != 0, FALSE);
    //    XDBG_RETURN_VAL_IF_FAIL (buf->prop_id > 0, FALSE);

    XDBG_DEBUG (MIA, "prop_id(%d) ops_id(%d) ctrl(%d) id(%d) handles(%u %u %u). \n",
                buf->prop_id, buf->ops_id, buf->buf_type, buf->buf_id,
                buf->handle[0], buf->handle[1], buf->handle[2]);

    ret = drmIoctl (fd, DRM_IOCTL_EXYNOS_IPP_QUEUE_BUF, buf);
    if (ret)
    {
        XDBG_ERRNO (MIA, "failed. prop_id(%d) op(%d) buf(%d) id(%d)\n",
                    buf->prop_id, buf->ops_id, buf->buf_type, buf->buf_id);
        return FALSE;
    }

    return TRUE;
}

Bool
exynosVideoIppCmdCtrl (int fd, struct drm_exynos_ipp_cmd_ctrl *ctrl)
{
    int ret = 0;

    XDBG_RETURN_VAL_IF_FAIL (fd >= 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (ctrl != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (ctrl->prop_id > 0, FALSE);

    XDBG_TRACE (MIA, "prop_id(%d) ctrl(%d). \n", ctrl->prop_id, ctrl->ctrl);

    ret = drmIoctl (fd, DRM_IOCTL_EXYNOS_IPP_CMD_CTRL, ctrl);
    if (ret)
    {
        XDBG_ERRNO (MIA, "failed. prop_id(%d) ctrl(%d)\n",
                    ctrl->prop_id, ctrl->ctrl);
        return FALSE;
    }

    return TRUE;
}
