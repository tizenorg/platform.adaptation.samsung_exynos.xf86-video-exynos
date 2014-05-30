#include <sys/ioctl.h>
#include <drm_fourcc.h>
#include "exynos_clone.h"
#include "exynos_driver.h"
#include "exynos_drm.h"
#include "exynos_video_buffer.h"
#include "exynos_video_fourcc.h"
#include "exynos_mem_pool.h"
#include "exynos_video_ipp.h"
#include "fimg2d.h"

typedef enum {
    STATUS_STARTED, STATUS_STOPPED,
} EXYNOS_CLONING_STATUS;

typedef struct _exynosCloneNotifyFuncInfo {
    EXYNOS_CLONE_NOTIFY noti;
    CloneNotifyFunc func;
    void *user_data;

    struct _exynosCloneNotifyFuncInfo *next;
} exynosCloneNotifyFuncInfo;

struct _EXYNOS_CloneObject {
    int prop_id;

    ScrnInfoPtr pScrn;

    unsigned int id;

    int rotate;

    int width;
    int height;
    xRectangle drm_dst, tv_dst, tv_out;
    int wait_show;
    int now_showing;
    EXYNOS_CLONING_STATUS status;
    Bool scanout;
    int hz;
    int hdmi_zpos;
    exynosCloneNotifyFuncInfo *info_list;

    EXYNOSBufInfoPtr dst_buf[CLONE_BUF_MAX];
    Bool queued[CLONE_BUF_MAX];
    int buf_num;

    /* for tvout */
    Bool tvout;
    Bool need_rotate_hook;
    OsTimerPtr rotate_timer;

    /* count */
    unsigned int put_counts;
    unsigned int last_counts;
    OsTimerPtr timer;

    OsTimerPtr event_timer;
    OsTimerPtr ipp_timer;
};

static EXYNOS_CloneObject *keep_clone;

static unsigned int formats[] = {
FOURCC_RGB32,
FOURCC_ST12,
FOURCC_SN12, };

static unsigned int drmfmt_list[] = {
/* packed */
DRM_FORMAT_RGB565,
DRM_FORMAT_XRGB8888,
DRM_FORMAT_YUYV,
DRM_FORMAT_UYVY,

/* 2 plane */
DRM_FORMAT_NV12,
DRM_FORMAT_NV21,
DRM_FORMAT_NV12M,
DRM_FORMAT_NV12MT,

/* 3 plane */
DRM_FORMAT_YVU420,
DRM_FORMAT_YUV420,
DRM_FORMAT_YUV444, };

static unsigned int _exynosCloneSupportFormat(int id)
{

    int i = 0, size = 0;

    XDBG_INFO(MCLO, "id %i : '%c%c%c%c'.\n", id, FOURCC_STR (id));

    unsigned int drmfmt = exynosVideoBufferGetDrmFourcc(id);
    XDBG_INFO(MCLO, "drmfmt %x.\n", drmfmt);
    size = sizeof(formats) / sizeof(unsigned int);

    for (i = 0; i < size; i++)
        if (formats[i] == id)
            break;

    if (i == size) {
        XDBG_ERROR(MCLO, "clone not support : '%c%c%c%c'.\n", FOURCC_STR (id));
        return 0;
    }

    for (i = 0; i < sizeof(drmfmt_list) / sizeof(unsigned int); i++)
        if (drmfmt_list[i] == drmfmt) {
            XDBG_DEBUG(MCLO, "drmfmts[%i].\n", i);
            return drmfmt;
        }

    XDBG_ERROR(MCLO, "drm ipp not support : '%c%c%c%c'.\n", FOURCC_STR (id));
    return 0;
}

EXYNOS_CloneObject* _exynosCloneInit(ScrnInfoPtr pScrn, unsigned int id,
        int width, int height, Bool scanout, int hz, Bool need_rotate_hook,
        const char *func)
{
    if (keep_clone) {
        XDBG_ERROR(MCLO, "clone already init. \n");
        return NULL;
    }

    keep_clone = (EXYNOS_CloneObject*) ctrl_calloc(1,
            sizeof(EXYNOS_CloneObject));
    keep_clone->prop_id = -1;
    keep_clone->pScrn = pScrn;
    keep_clone->id = id;

    keep_clone->wait_show = -1;
    keep_clone->now_showing = -1;

    keep_clone->width = width;
    keep_clone->height = height;
    keep_clone->status = STATUS_STOPPED;
    keep_clone->buf_num = CLONE_BUF_MAX;

    keep_clone->scanout = scanout;
    keep_clone->hz = (hz > 0) ? hz : 60;

    keep_clone->need_rotate_hook = need_rotate_hook;

    XDBG_INFO(MCLO,
            "clone(%p) id(%c%c%c%c) size(%dx%d) scanout(%d) hz(%d) rhook(%d) buf_num(%d): %s\n",
            keep_clone, FOURCC_STR(id), keep_clone->width, keep_clone->height,
            scanout, hz, need_rotate_hook, keep_clone->buf_num, func);
    return keep_clone;
}

static void _exynosCloneQueue(EXYNOS_CloneObject *clone, int index)
{
    XDBG_RETURN_IF_FAIL(clone != NULL);
    int fd = EXYNOSPTR (clone->pScrn)->drm_fd;
    struct drm_exynos_ipp_queue_buf buf;
    int j;

    if (index < 0)
        return;

    XDBG_RETURN_IF_FAIL(clone->dst_buf[index]->showing == FALSE);

    CLEAR(buf);
    buf.ops_id = EXYNOS_DRM_OPS_DST;
    buf.buf_type = IPP_BUF_ENQUEUE;
    buf.prop_id = clone->prop_id;
    buf.buf_id = index;
    buf.user_data = (__u64 ) (unsigned int) clone;

    for (j = 0; j < EXYNOS_DRM_PLANAR_MAX; j++)
        buf.handle[j] = clone->dst_buf[index]->handles[j].s32;

    if (!exynosVideoIppQueueBuf(fd, &buf))
        return;

    clone->queued[index] = TRUE;
    clone->dst_buf[index]->dirty = TRUE;

    XDBG_DEBUG(MCLO, "index(%d)\n", index);
}

static void _exynosCreateFB(EXYNOS_CloneObject *clone, EXYNOSBufInfo *vbuf)
{
    EXYNOSPtr pExynos = EXYNOSPTR (clone->pScrn);
    int fd = pExynos->drm_fd;
    unsigned int drmfmt;
    unsigned int handles[4] = { 0, };
    unsigned int pitches[4] = { 0, };
    unsigned int offsets[4] = { 0, };
    int i;

    if (vbuf->fb_id > 0)
        return;

    drmfmt = exynosVideoBufferGetDrmFourcc(vbuf->fourcc);

    for (i = 0; i < vbuf->num_planes; i++) {
        handles[i] = vbuf->handles[i].u32;
        pitches[i] = vbuf->pitches[i];
        offsets[i] = vbuf->offsets[i];
    }

    if (drmModeAddFB2(fd, vbuf->width, vbuf->height, drmfmt, handles, pitches,
            offsets, &vbuf->fb_id, 0)) {
        XDBG_ERRNO(MCLO,
                "drmModeAddFB2 failed. handles(%d %d %d) pitches(%d %d %d) offsets(%d %d %d) '%c%c%c%c'\n",
                handles[0], handles[1], handles[2], pitches[0], pitches[1],
                pitches[2], offsets[0], offsets[1], offsets[2],
                FOURCC_STR (drmfmt));
    }

}

void attachFBDrm(EXYNOS_CloneObject *clone, EXYNOSBufInfo *vbuf, int zpos)
{
    int plane_id = 10; //TODO exynosPlaneGetAvailable(clone->pScrn),

    EXYNOSPtr pExynos = EXYNOSPTR (clone->pScrn);
    EXYNOSDispInfoPtr pExynosMode = (EXYNOSDispInfoPtr) pExynos->pDispInfo;
    int fd = pExynos->drm_fd;
    _exynosCreateFB(clone, vbuf);
    if (zpos != clone->hdmi_zpos) {

        exynosUtilSetDrmProperty(fd, plane_id, DRM_MODE_OBJECT_PLANE, "zpos",
                zpos);
        XDBG_DEBUG(MCLO, "exynosUtilSetDrmProperty plane %i\n", plane_id);
        clone->hdmi_zpos = zpos;
    }
    XDBG_DEBUG(MCLO, "w(%d) h(%d) plane %i\n", clone->width, clone->height,
            plane_id);
    if (drmModeSetPlane(fd, plane_id, pExynosMode->hdmi_crtc_id, vbuf->fb_id, 0,
            clone->tv_out.x, clone->tv_out.y, clone->tv_out.width,
            clone->tv_out.height, 0, 0, clone->width << 16,
            clone->height << 16)) {
        XDBG_ERRNO(MCLO, "drmModeSetPlane failed. \n");
    }
}

static void _sendStopDrm(EXYNOS_CloneObject *clone)
{
    EXYNOSPtr pExynos = EXYNOSPTR (clone->pScrn);
    int i, fd = pExynos->drm_fd;
    struct drm_exynos_ipp_cmd_ctrl ctrl;

    for (i = 0; i < clone->buf_num; i++) {
        struct drm_exynos_ipp_queue_buf buf;
        int j;

        CLEAR(buf);
        buf.ops_id = EXYNOS_DRM_OPS_DST;
        buf.buf_type = IPP_BUF_DEQUEUE;
        buf.prop_id = clone->prop_id;
        buf.buf_id = i;

        if (clone->dst_buf[i])
            for (j = 0; j < EXYNOS_DRM_PLANAR_MAX; j++)
                buf.handle[j] = (__u32) clone->dst_buf[i]->handles[j].ptr;

        exynosVideoIppQueueBuf(fd, &buf);

        clone->queued[i] = FALSE;
    }

    CLEAR(ctrl);
    ctrl.prop_id = clone->prop_id;
    ctrl.ctrl = IPP_CTRL_STOP;
    exynosVideoIppCmdCtrl(fd, &ctrl);

    clone->prop_id = -1;
}

static void _exynosCloneCloseDrmDstBuffer(EXYNOS_CloneObject *clone)
{
    int i;

    for (i = 0; i < clone->buf_num; i++)
        exynosMemPoolDestroyBO(clone->dst_buf[i]);
    XDBG_DEBUG(MCLO, "trying freeing\n", i);
}

#define _4DEBUG

static Bool _exynosCloneEnsureDrmDstBuffer(EXYNOS_CloneObject *clone)
{
    int i;
    for (i = 0; i < clone->buf_num; i++) {
        if (clone->dst_buf[i]) {
            XDBG_DEBUG(MCLO, "dst_buf[%i] exist.\n", i);
            continue;
        } else
            XDBG_DEBUG(MCLO, "dst_buf[%i] doesnt exist. \n", i);
        clone->dst_buf[i] = exynosMemPoolCreateBO(clone->pScrn, clone->id,
                clone->width, clone->height);
        XDBG_GOTO_IF_FAIL(clone->dst_buf[i] != NULL, fail_to_ensure);
    }
    XDBG_DEBUG(MCLO, "exit \n");
    return TRUE;
    fail_to_ensure: _exynosCloneCloseDrmDstBuffer(clone);
    return FALSE;
}

static void calculateSize(int src_w, int src_h, int dst_w, int dst_h,
        xRectangle *fit, Bool hw)
{
    int fit_width;
    int fit_height;
    float rw, rh, max;

    if (!fit)
        return;

    XDBG_RETURN_IF_FAIL(src_w > 0 && src_h > 0);
    XDBG_RETURN_IF_FAIL(dst_w > 0 && dst_h > 0);

    rw = (float) src_w / dst_w;
    rh = (float) src_h / dst_h;
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

static Bool _exynosPrepareTvout(EXYNOS_CloneObject *clone)
{
    EXYNOSDispInfoPtr pDispInfo = EXYNOSDISPINFOPTR(clone->pScrn);
    clone->width = pDispInfo->main_lcd_mode.hdisplay;
    clone->height = pDispInfo->main_lcd_mode.vdisplay;
    clone->drm_dst.width = clone->width;
    clone->drm_dst.height = clone->height;

    calculateSize(clone->width, clone->height,
            pDispInfo->ext_connector_mode.hdisplay,
            pDispInfo->ext_connector_mode.vdisplay, &clone->tv_out, TRUE);
    return TRUE;
}

static Bool _sendStartDrm(EXYNOS_CloneObject *clone)
{
    EXYNOSPtr pExynos = EXYNOSPTR (clone->pScrn);
    EXYNOSDispInfoPtr pDispInfo = EXYNOSDISPINFOPTR(clone->pScrn);
    int i, fd = pExynos->drm_fd;
    unsigned int drmfmt = 0;
    struct drm_exynos_ipp_config src, dst;
    enum drm_exynos_degree degree;
    struct drm_exynos_ipp_cmd_ctrl ctrl;
    XDBG_DEBUG(MCLO, "_sendStartDrm\n");

    if (!_exynosPrepareTvout(clone))
        goto fail_to_open;

    drmfmt = _exynosCloneSupportFormat(clone->id);
    XDBG_GOTO_IF_FAIL(drmfmt > 0, fail_to_open);

    switch (clone->rotate % 360) {
    case 90:
        degree = EXYNOS_DRM_DEGREE_90;
        break;
    case 180:
        degree = EXYNOS_DRM_DEGREE_180;
        break;
    case 270:
        degree = EXYNOS_DRM_DEGREE_270;
        break;
    default:
        degree = EXYNOS_DRM_DEGREE_0;
        break;
    }

    CLEAR(src);
    src.ops_id = EXYNOS_DRM_OPS_SRC;
    src.fmt = DRM_FORMAT_YUV444;
    src.sz.hsize = (__u32 ) pDispInfo->main_lcd_mode.hdisplay;
    src.sz.vsize = (__u32 ) pDispInfo->main_lcd_mode.vdisplay;
    src.pos.x = 0;
    src.pos.y = 0;
    src.pos.w = (__u32 ) pDispInfo->main_lcd_mode.hdisplay;
    src.pos.h = (__u32 ) pDispInfo->main_lcd_mode.vdisplay;

    CLEAR(dst);
    dst.ops_id = EXYNOS_DRM_OPS_DST;
    dst.degree = degree;
    dst.fmt = drmfmt;
    dst.sz.hsize = clone->width;
    dst.sz.vsize = clone->height;
    dst.pos.x = (__u32 ) clone->drm_dst.x;
    dst.pos.y = (__u32 ) clone->drm_dst.y;
    dst.pos.w = (__u32 ) clone->drm_dst.width;
    dst.pos.h = (__u32 ) clone->drm_dst.height;

    clone->prop_id = exynosVideoIppSetProperty(fd, IPP_CMD_WB, clone->prop_id,
            &src, &dst, clone->hz, FALSE, 0);
    XDBG_GOTO_IF_FAIL(clone->prop_id >= 0, fail_to_open);

    XDBG_TRACE(MCLO,
            "prop_id(%d) drmfmt(%c%c%c%c) size(%dx%d) crop(%d,%d %dx%d) rotate(%d)\n",
            clone->prop_id, FOURCC_STR(drmfmt), clone->width, clone->height,
            clone->drm_dst.x, clone->drm_dst.y, clone->drm_dst.width,
            clone->drm_dst.height, clone->rotate);

    if (!_exynosCloneEnsureDrmDstBuffer(clone))
        goto fail_to_open;

    for (i = 0; i < clone->buf_num; i++) {
        struct drm_exynos_ipp_queue_buf buf;
        int j;

        if (clone->dst_buf[i]->showing) {
            XDBG_DEBUG(MCLO, "%d. name(%d) is showing\n", i,
                    clone->dst_buf[i]->names[0]);
            continue;
        }

        CLEAR(buf);
        buf.ops_id = EXYNOS_DRM_OPS_DST;
        buf.buf_type = IPP_BUF_ENQUEUE;
        buf.prop_id = clone->prop_id;
        buf.buf_id = i;
        buf.user_data = (__u64 ) (unsigned int) clone;

        XDBG_GOTO_IF_FAIL(clone->dst_buf[i] != NULL, fail_to_open);

        for (j = 0; j < EXYNOS_DRM_PLANAR_MAX; j++)
            buf.handle[j] = (__u32) clone->dst_buf[i]->handles[j].ptr;

        if (!exynosVideoIppQueueBuf(fd, &buf))
            goto fail_to_open;

        clone->queued[i] = TRUE;
    }
    XDBG_DEBUG(MCLO, "clone->prop_id %i\n", clone->prop_id);
    CLEAR(ctrl);
    ctrl.prop_id = clone->prop_id;
    ctrl.ctrl = IPP_CTRL_PLAY;
    if (!exynosVideoIppCmdCtrl(fd, &ctrl)) {
        XDBG_TRACE(MCLO, "exynosDrmIppCmdCtrl doesnt start\n");
        goto fail_to_open;
    } else
        XDBG_TRACE(MCLO, "exynosDrmIppCmdCtrl start\n");

    return TRUE;

    fail_to_open:

    _sendStopDrm(clone);

    _exynosCloneCloseDrmDstBuffer(clone);

    return FALSE;
}

static void _exynosCloneCallNotifyFunc(EXYNOS_CloneObject* clone,
        EXYNOS_CLONE_NOTIFY noti, void *noti_data)
{
    exynosCloneNotifyFuncInfo *info;

    nt_list_for_each_entry (info, clone->info_list, next)
    {
        if (info->noti == noti && info->func)
            info->func(clone, noti, noti_data, info->user_data);
    }
}

int _exynosCloneDequeue(EXYNOS_CloneObject *clone, Bool skip_put, int index)
{
    int zpos = 2, i = 0, remain = 0;

    if (!clone->queued[index])
        XDBG_WARNING(MCLO, "buf(%d) already dequeued.\n", index);

    clone->queued[index] = FALSE;

    for (i = 0; i < clone->buf_num; i++) {
        if (clone->queued[i])
            remain++;
    }

    /* the count of remain buffers should be more than 2. */
    if (remain >= CLONE_BUF_MIN)
        _exynosCloneCallNotifyFunc(clone, CLONE_NOTI_IPP_EVENT,
                (void*) clone->dst_buf[index]);
    else
        XDBG_DEBUG(MCLO, "remain buffer count: %d\n", remain);

    XDBG_DEBUG(MCLO, "index(%d)\n", index);
    attachFBDrm(clone, clone->dst_buf[index], zpos);
    return index;
}

void exynosCloneStop(EXYNOS_CloneObject* clone)
{
    _sendStopDrm(clone);
    _exynosCloneCloseDrmDstBuffer(clone);
    XDBG_DEBUG(MCLO, "exynosCloneStop\n");
    clone->status = STATUS_STOPPED;
}

Bool exynosCloneStart(EXYNOS_CloneObject* clone)
{
    XDBG_DEBUG(MCLO, "exynosCloneStart\n");
    if (clone->status == STATUS_STARTED)
        return TRUE;

    if (!_sendStartDrm(clone)) {
        XDBG_ERROR(MCLO, "open fail. \n");
        return FALSE;
    }

    clone->buf_num = CLONE_BUF_MAX;
    clone->status = STATUS_STARTED;
    clone->hdmi_zpos = -1;
    _exynosCloneCallNotifyFunc(clone, CLONE_NOTI_START, NULL);

    return TRUE;
}

void exynosCloneClose(EXYNOS_CloneObject*clone)
{

    exynosCloneNotifyFuncInfo *info, *tmp;
    XDBG_INFO(MCLO, "clone-%p keep_clone-%p\n", clone, keep_clone);
    XDBG_RETURN_IF_FAIL(clone != NULL);
    XDBG_RETURN_IF_FAIL(keep_clone != NULL);
    exynosCloneStop(clone);

    nt_list_for_each_entry_safe (info, tmp, clone->info_list, next)
    {
        info->func;
        nt_list_del(info, clone->info_list, exynosCloneNotifyFuncInfo, next);
        ctrl_free(info);
    }

    ctrl_free(keep_clone);
    keep_clone = NULL;
}

void exynosCloneHandleIppEvent(int fd, unsigned int *buf_idx, void *data)
{
    XDBG_RETURN_IF_FAIL(buf_idx != NULL);
    XDBG_RETURN_IF_FAIL(data != NULL);
    EXYNOS_CloneObject *clone = (EXYNOS_CloneObject*) data;
    int index = buf_idx[EXYNOS_DRM_OPS_DST];
    if (!clone) {
        XDBG_ERROR(MCLO, "data is %p \n", data);
        return;
    }

    if (keep_clone != clone) {
        XDBG_WARNING(MCLO, "Useless ipp event! (%p) \n", clone);
        return;
    }

    if (clone->status == STATUS_STOPPED) {
        XDBG_ERROR(MCLO, "stopped. ignore a event.\n", data);
        return;
    };
    _exynosCloneDequeue(clone, TRUE, index);
    _exynosCloneQueue(clone, index);
    _exynosCloneCallNotifyFunc(clone, CLONE_NOTI_IPP_EVENT_DONE, NULL);
}

unsigned int exynosCloneGetPropID(void)
{
    if (!keep_clone)
        return -1;
    return keep_clone->prop_id;
}

void exynosCloneAddNotifyFunc(EXYNOS_CloneObject* clone,
        EXYNOS_CLONE_NOTIFY noti, CloneNotifyFunc func, void *user_data)
{
    exynosCloneNotifyFuncInfo *info;

    XDBG_RETURN_IF_FAIL(clone != NULL);

    if (!func)
        return;

    nt_list_for_each_entry (info, clone->info_list, next)
    {
        if (info->func == func)
            return;
    }

    XDBG_DEBUG(MCLO, "noti(%d) func(%p) user_data(%p) \n", noti, func,
            user_data);

    info = ctrl_calloc(1, sizeof(exynosCloneNotifyFuncInfo));
    XDBG_RETURN_IF_FAIL(info != NULL);

    info->noti = noti;
    info->func = func;
    info->user_data = user_data;

    if (clone->info_list)
        nt_list_append(info, clone->info_list, exynosCloneNotifyFuncInfo, next);
    else
        clone->info_list = info;
}

void exynosCloneRemoveNotifyFunc(EXYNOS_CloneObject* clone,
        CloneNotifyFunc func)
{
    exynosCloneNotifyFuncInfo *info;

    XDBG_RETURN_IF_FAIL(clone != NULL);

    if (!func)
        return;

    nt_list_for_each_entry (info, clone->info_list, next)
    {
        if (info->func == func) {
            XDBG_DEBUG(MCLO, "func(%p) \n", func);
            nt_list_del(info, clone->info_list, exynosCloneNotifyFuncInfo,
                    next);
            free(info);
            return;
        }
    }
}

Bool exynosCloneIsRunning(void)
{
    if (!keep_clone)
        return FALSE;

    return (keep_clone->status == STATUS_STARTED) ? TRUE : FALSE;
}

Bool exynosCloneIsOpened(void)
{
    return (keep_clone) ? TRUE : FALSE;
}

Bool exynosCloneCanDequeueBuffer(EXYNOS_CloneObject* clone)
{
    int i, remain = 0;

    XDBG_RETURN_VAL_IF_FAIL(clone != NULL, FALSE);

    for (i = 0; i < clone->buf_num; i++)
        if (clone->queued[i])
            remain++;

    XDBG_DEBUG(MCLO, "buf_num(%d) remain(%d)\n", clone->buf_num, remain);

    return (remain > CLONE_BUF_MIN) ? TRUE : FALSE;
}

Bool exynosCloneSetBuffer(EXYNOS_CloneObject *clone, EXYNOSBufInfo **vbufs,
        int bufnum)
{
    int i;

    XDBG_RETURN_VAL_IF_FAIL(clone != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL(clone->status != STATUS_STARTED, FALSE);
    XDBG_RETURN_VAL_IF_FAIL(vbufs != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL(clone->buf_num <= bufnum, FALSE);
    XDBG_RETURN_VAL_IF_FAIL(bufnum <= CLONE_BUF_MAX, FALSE);

    for (i = 0; i < CLONE_BUF_MAX - 1; i++) {
        if (clone->dst_buf[i]) {
            XDBG_ERROR(MCLO, "already has %d buffers\n", clone->buf_num);
            return FALSE;
        }
    }

    for (i = 0; i < bufnum; i++) {
        XDBG_GOTO_IF_FAIL(clone->id == vbufs[i]->fourcc, fail_set_buffer);
        XDBG_GOTO_IF_FAIL(clone->width == vbufs[i]->width, fail_set_buffer);
        XDBG_GOTO_IF_FAIL(clone->height == vbufs[i]->height, fail_set_buffer);

        /*remember vbuf in clone object*/
        vbufs[i]->ref_cnt++;
        clone->dst_buf[i] = vbufs[i];
        XDBG_GOTO_IF_FAIL(clone->dst_buf[i] != NULL, fail_set_buffer);

        if (!clone->dst_buf[i]->showing && clone->dst_buf[i]->need_reset) { //;// UtilClearVideoBuffer (clone->dst_buf[i]);
            clone->dst_buf[i]->dirty = FALSE;
            clone->dst_buf[i]->need_reset = FALSE;
        } else
            clone->dst_buf[i]->need_reset = TRUE;
    }

    clone->buf_num = bufnum;

    return TRUE;

    fail_set_buffer: for (i = 0; i < CLONE_BUF_MAX; i++) {
        if (clone->dst_buf[i]) {
            /*decrease vbuf's refcounter  and clear it*/
            // secUtilVideoBufferUnref (clone->dst_buf[i]);
            clone->dst_buf[i] = NULL;
        }
    }

    return FALSE;
}
