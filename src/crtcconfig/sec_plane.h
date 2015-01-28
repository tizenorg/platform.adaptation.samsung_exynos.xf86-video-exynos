/**************************************************************************

xserver-xorg-video-exynos

Copyright 2011 Samsung Electronics co., Ltd. All Rights Reserved.

Contact: SooChan Lim <sc1.lim@samsung.com>

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

#ifndef __SEC_PLANE_H__
#define __SEC_PLANE_H__

#include "sec_display.h"

#define PLANE_POS_0		0
#define PLANE_POS_1		1
#define PLANE_POS_2		2
#define PLANE_POS_3		3
#define PLANE_POS_4		4

typedef struct _SECPlanePriv
{
    ScrnInfoPtr pScrn;
    SECModePtr pSecMode;
    drmModePlanePtr mode_plane;

    uint32_t plane_id;

    struct xorg_list link;
} SECPlanePrivRec, *SECPlanePrivPtr;

void   secPlaneInit (ScrnInfoPtr pScrn, SECModePtr pSecMode, int num);
void   secPlaneDeinit (ScrnInfoPtr pScrn, SECPlanePrivPtr pPlanePriv);

void   secPlaneShowAll (uint32_t crtc_id);
uint32_t    secPlaneGetID   (void);
void   secPlaneFreeId  (uint32_t plane_id);
Bool   secPlaneTrun    (uint32_t plane_id, Bool onoff, Bool user);
Bool   secPlaneTrunStatus (uint32_t plane_id);
void   secPlaneFreezeUpdate (uint32_t plane_id, Bool enable);

Bool   secPlaneRemoveBuffer (uint32_t plane_id, uint32_t fb_id);
uint32_t secPlaneAddBo        (uint32_t plane_id, tbm_bo bo);
uint32_t secPlaneAddBuffer    (uint32_t plane_id, SECVideoBuf *vbuf);

uint32_t    secPlaneGetBuffer     (uint32_t plane_id, tbm_bo bo, SECVideoBuf *vbuf);
void   secPlaneGetBufferSize (uint32_t plane_id, uint32_t fb_id, int *width, int *height);

Bool   secPlaneAttach     (uint32_t plane_id, uint32_t fb_id);

Bool   secPlaneIsVisible (uint32_t  plane_id);
Bool   secPlaneHide (uint32_t plane_id);
Bool   secPlaneShow (uint32_t plane_id, uint32_t crtc_id,
                     int src_x, int src_y, int src_w, int src_h,
                     int dst_x, int dst_y, int dst_w, int dst_h,
                     int zpos, Bool need_update);

Bool   secPlaneMove (uint32_t plane_id, int x, int y);

/* for debug */
char*  secPlaneDump (char *reply, int *len);

#endif /* __SEC_PLANE_H__ */
