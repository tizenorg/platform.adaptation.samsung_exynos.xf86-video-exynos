/*
 * test_sec_video_virtual.h
 *
 *  Created on: 12 сент. 2014
 *      Author: Alexandr Rozov
 */

#ifndef TEST_SEC_VIDEO_VIRTUAL_H_
#define TEST_SEC_VIDEO_VIRTUAL_H_

#include "sec_converter.h"
#include "sec_video_tvout.h"

#define WB_BUF_MAX      5
#define WB_BUF_DEFAULT  3
#define WB_BUF_MIN      2


typedef struct _SECWbNotifyFuncInfo
{
    SECWbNotify   noti;
    WbNotifyFunc  func;
    void         *user_data;

    struct _SECWbNotifyFuncInfo *next;
} SECWbNotifyFuncInfo;


typedef struct _SECWb
{
    int  prop_id;

    ScrnInfoPtr pScrn;

    unsigned int id;

    int     rotate;

    int     width;
    int     height;
    xRectangle  drm_dst;
    xRectangle  tv_dst;

    SECVideoBuf *dst_buf[WB_BUF_MAX];
    Bool         queued[WB_BUF_MAX];
    int          buf_num;

    int     wait_show;
    int     now_showing;

    SECWbNotifyFuncInfo *info_list;

    /* for tvout */
    Bool        tvout;
    SECVideoTv *tv;
    SECLayerPos lpos;

    Bool       need_rotate_hook;
    OsTimerPtr rotate_timer;

    /* count */
    unsigned int put_counts;
    unsigned int last_counts;
    OsTimerPtr timer;

    OsTimerPtr event_timer;
    OsTimerPtr ipp_timer;

    Bool scanout;
    int  hz;

    int     status;
    Bool    secure;
    CARD32  prev_time;
};

#endif /* TEST_SEC_VIDEO_VIRTUAL_H_ */
