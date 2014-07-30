/**************************************************************************

 xserver-xorg-video-exynos

 Copyright 2013 Samsung Electronics co., Ltd. All Rights Reserved.

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

#ifndef _EXYNOS_ACCEL_INT_H_
#define _EXYNOS_ACCEL_INT_H_ 1

#include "exynos_accel.h"
#include "dri2.h"

typedef struct _exynosExaPixPriv EXYNOSExaPixPrivRec, *EXYNOSExaPixPrivPtr;

struct _exynosExaPixPriv {
    int usage_hint; /* pixmap usage hint */

    pointer pPixData; /*text glyphs or SHM-PutImage*/

    Bool isFrameBuffer;

    void* exaOpInfo; /* for exa operation */

    tbm_bo tbo; /* memory buffer object */

    XID owner;
    CARD64 sbc;
};

/* type of the frame event */
typedef enum _dri2FrameEventType {
    DRI2_NONE, DRI2_SWAP, DRI2_FLIP, DRI2_BLIT, DRI2_FB_BLIT, DRI2_WAITMSC,
} DRI2FrameEventType;

/* dri2 frame event information */
typedef struct _dri2FrameEvent {
    DRI2FrameEventType type;

    XID drawable_id;
    unsigned int client_idx;
    ClientPtr pClient;
    int crtc_pipe;
    int frame;

    /* for swaps & flips only */
    DRI2SwapEventPtr event_complete;
    void *event_data;
    DRI2BufferPtr pFrontBuf;
    DRI2BufferPtr pBackBuf;
    tbm_bo front_bo;
    tbm_bo back_bo;

    /* pending flip event */
    int pipe;
    void *pCrtc;
    struct xorg_list crtc_pending_link;
    struct _dri2FrameEvent *pPendingEvent;

    /* for SwapRegion */
    RegionPtr pRegion;
} DRI2FrameEventRec, *DRI2FrameEventPtr;

/**************************************************************************
 * EXA
 **************************************************************************/
/* EXA */
Bool exynosExaInit(ScreenPtr pScreen);
void exynosExaDeinit(ScreenPtr pScreen);
/* EXA SW */
Bool exynosExaSwInit(ScreenPtr pScreen, ExaDriverPtr pExaDriver);
void exynosExaSwDeinit(ScreenPtr pScreen);

/**************************************************************************
 * DRI2
 **************************************************************************/
/* DRI2 */
Bool exynosDri2Init(ScreenPtr pScreen);
void exynosDri2Deinit(ScreenPtr pScreen);


/**************************************************************************
 * Present
 **************************************************************************/
/* Present */
Bool exynosPresentScreenInit( ScreenPtr screen );

/**************************************************************************
 * Pixmap
 **************************************************************************/

void exynosPixmapSetFrameCount(PixmapPtr pPix, unsigned int xid, CARD64 sbc);
void exynosPixmapGetFrameCount(PixmapPtr pPix, unsigned int *xid, CARD64 *sbc);
void exynosPixmapSwap(PixmapPtr pFrontPix, PixmapPtr pBackPix);

char * pixToStr (PixmapPtr pPix, EXYNOSExaPixPrivPtr pExaPixPriv);
char * tboToStr (tbm_bo bo);
#endif /* _EXYNOS_ACCEL_INT_H_ */
