/**************************************************************************

xserver-xorg-video-exynos

Copyright 2010 - 2011 Samsung Electronics co., Ltd. All Rights Reserved.

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

#ifndef _EXYNOS_ACCEL_H_
#define _EXYNOS_ACCEL_H_ 1

#include <exa.h>
#include <tbm_bufmgr.h>


/* exynos acceleration information */
typedef struct _exynosAccelInfo EXYNOSAccelInfoRec, *EXYNOSAccelInfoPtr;

/* get a display info */
#define EXYNOSACCELINFOPTR(pScrn) ((EXYNOSAccelInfoPtr)(EXYNOSPTR(pScrn)->pAccelInfoPtr))

/* pixmap usage hint */
#define CREATE_PIXMAP_USAGE_NORMAL 0
#define CREATE_PIXMAP_USAGE_DRI2_BACK 101
#define CREATE_PIXMAP_USAGE_SCANOUT 102
#define CREATE_PIXMAP_USAGE_SCANOUT_DRI2_BACK 103
#define CREATE_PIXMAP_USAGE_XV 104

/* exynos accexa driver private infomation */
struct _exynosAccelInfo {
    ExaDriverPtr pExaDriver;
};

Bool exynosAccelInit(ScreenPtr pScreen);
void exynosAccelDeinit(ScreenPtr pScreen);


/**************************************************************************
 *  dri2 event hanlers (vblank, pageflip)
 **************************************************************************/
void exynosDri2VblankEventHandler(unsigned int frame, unsigned int tv_sec, unsigned int tv_usec, void *event_data);
void exynosDri2FlipEventHandler(unsigned int frame, unsigned int tv_sec, unsigned int tv_usec, void *event_data);

/**************************************************************************
 *  present event hanlers (vblank, pageflip)
 **************************************************************************/
void exynosPresentVblankHandler(uint64_t msc, uint64_t usec, void *data);
void exynospresentVblankAbort(ScrnInfoPtr scrn, xf86CrtcPtr crtc, void *data);
void exynosPresentFlipEvent(uint64_t msc, uint64_t ust, void *pageflip_data);
void exynosPresentFlipAbort(void *pageflip_data);

/**************************************************************************
 * Pixmap
 **************************************************************************/

tbm_bo exynosExaPixmapGetBo(PixmapPtr pPix);
void exynosExaPixmapSetBo(PixmapPtr pPix, tbm_bo tbo);

/* \todo PrepareAccess function */
Bool EXYNOSExaPrepareAccess(PixmapPtr pPix, int index);
void EXYNOSExaFinishAccess(PixmapPtr pPix, int index);

int exynosExaScreenSetScrnPixmap (ScreenPtr pScreen);

int exynosExaScreenSetScrnPixmap (ScreenPtr pScreen);

#endif /* _EXYNOS_ACCEL_H_ */

