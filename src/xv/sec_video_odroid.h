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
#ifndef SEC_VIDEO_ODROID_H
#define SEC_VIDEO_ODROID_H

#include <xf86xv.h>


#define IMAGE_MAX_PORT        16
#define IMAGE_OUTBUF_NUM      3
#define IMAGE_HW_LAYER        1

/* PutImage's data */
typedef struct _PutData
{
    unsigned int fourcc;
    int width;
    int height;
    xRectangle src;
    xRectangle dst;
    void *buf;
    Bool sync;
    RegionPtr clip_boxes;
    void *data;
    DrawablePtr pDraw;
} IMAGEPutData;

typedef enum
{
    CVT_TYPE_SRC, CVT_TYPE_DST, CVT_TYPE_MAX,
} EXYNOSCvtType;

typedef struct _EXYNOSCvtProp
{
    unsigned int id;

    int width;
    int height;
    xRectangle crop;

    int degree;
    Bool vflip;
    Bool hflip;

} EXYNOSIppProp;

typedef struct
{
    CARD32 stamp;

    int prop_id;

    ScrnInfoPtr pScrn;
    EXYNOSIppProp props[CVT_TYPE_MAX];

#if INCREASE_NUM
    int src_index;
    int dst_index;
#endif

    Bool started;
    Bool first_event;

    void *list_of_inbuf;
    void *list_of_outbuf;
    struct xorg_list entry;

} EXYNOSIpp, *EXYNOSIppPtr;

typedef struct
{
    ScrnInfoPtr pScrn;
    int index;
    unsigned long temp_stamp;

    IMAGEPutData d;
    IMAGEPutData old_d;

    /* image_attrs */
    int rotate;
    int hw_rotate;
    int hflip;
    int vflip;

    int old_rotate;
    int old_hflip;
    int old_vflip;

    /* drawable information */
    int drawing;

    /* input buffer information */
    int in_width;
    int in_height;
    xRectangle in_crop;
    PixmapPtr inbuf;

    /* output buffer information */
    int out_width;
    int out_height;
    xRectangle out_crop;
    PixmapPtr outbuf[IMAGE_OUTBUF_NUM];
    int outbuf_next;

    /* plane information */
    int plane_id;
    int crtc_id;
    Bool punched;

    /* tvout */
    int usr_output;
    int old_output;
    int grab_tvout;
    int disable_clone;
   // SECVideoTv *tv;
    void       *gem_list;
    Bool skip_tvout;
    Bool need_start_wb;
 //   SECVideoBuf  *wait_vbuf;
    CARD32 tv_prev_time;

    /* vblank information*/
    Bool watch_vblank;
    int watch_pipe;
    PixmapPtr showing_outbuf;
    PixmapPtr waiting_outbuf;

    pointer list_of_pixmap_out;
    pointer list_of_pixmap_in;
    pointer list_of_ipp;

    struct xorg_list watch_link;
    /* Ipp converter */
    EXYNOSIpp *last_ipp_data;
    int prop_id;

} IMAGEPortInfo, *IMAGEPortInfoPtr;

/* image adaptor */
XF86VideoAdaptorPtr exynosVideoImageSetup (ScreenPtr pScreen);

/* replace PutImage function of image adaptor to get client info */
void exynosVideoImageReplacePutImageFunc (ScreenPtr pScreen);
int  exynosVideoImageAttrs (int id, int *w, int *h,
                            int *pitches, int *offsets, int *lengths);

XF86ImagePtr exynosVideoImageGetImageInfo (int id);
void exynosVideoImageIppEventHandler (int fd, unsigned int prop_id,
                                      unsigned int *buf_idx, void *data);
ClientPtr exynosVideoImageGetClient (DrawablePtr pDraw);
Bool exynosVideoImageRawCopy (IMAGEPortInfoPtr pPort, PixmapPtr dst);

#endif // SEC_VIDEO_ODROID_H
