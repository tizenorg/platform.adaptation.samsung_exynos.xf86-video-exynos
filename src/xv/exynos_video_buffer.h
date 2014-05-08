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

#ifndef __EXYNOS_VIDEO_BUFFER_H__
#define __EXYNOS_VIDEO_BUFFER_H__

#include <sys/types.h>
#include <xorg/fbdevhw.h>
#include <X11/Xdefs.h>
#include <tbm_bufmgr.h>

#include "exynos_video_image.h"

/** \remark Normally, pixmap represents RGB buffer.
 * But we will use pixmap as YUV buffer only inside of x video driver.
 * Also, pixmap can be used as framebuffer as well as image buffer.
 */

#define STAMP(v)    exynosVideoBufferGetStamp(v)


void exynosVideoBufferFree (PixmapPtr pVideoPixmap);
PixmapPtr exynosVideoBufferRef (PixmapPtr pVideoPixmap);
void exynosVideoBufferUnref (ScrnInfoPtr pScrn, PixmapPtr pVideoPixmap);

void exynosVideoBufferConverting (PixmapPtr pVideoPixmap, Bool on);
void exynosVideoBufferShowing (PixmapPtr pVideoPixmap, Bool on);
Bool exynosVideoBufferIsConverting (PixmapPtr pVideoPixmap);
Bool exynosVideoBufferIsShowing (PixmapPtr pVideoPixmap);

int  exynosVideoBufferGetFBID (PixmapPtr pVideoPixmap);
int  exynosVideoBufferGetSize (PixmapPtr pVideoPixmap, int *pitches, int *offsets,
                               int *lengths);
void exynosVideoBufferGetAttr (PixmapPtr pVideoPixmap, tbm_bo *tbos,
                               tbm_bo_handle *handles, unsigned int *names);
int  exynosVideoBufferGetFourcc (PixmapPtr pVideoPixmap);
/* identifier for vpix */
unsigned long exynosVideoBufferGetStamp (PixmapPtr pVideoPixmap);

EXYNOSFormatType exynosVideoBufferGetColorType (unsigned int fourcc);
unsigned int     exynosVideoBufferGetDrmFourcc (unsigned int fourcc);
PixmapPtr exynosVideoBufferCreateInbufRAW (ScrnInfoPtr pScrn, int fourcc,
                                           int in_width, int in_height);
PixmapPtr exynosVideoBufferCreateInbufZeroCopy (ScrnInfoPtr pScrn, int fourcc, 
                                                int in_width, int in_height, 
                                                pointer in_buf, ClientPtr client);
PixmapPtr exynosVideoBufferModifyOutbufPixmap (ScrnInfoPtr pScrn, int fourcc,
                                               int out_width, int out_height, 
                                               PixmapPtr pPixmap);
PixmapPtr exynosVideoBufferCreateOutbufFB(ScrnInfoPtr pScrn, int fourcc, 
                                          int out_width, int out_height);
Bool exynosVideoBufferCopyRawData (PixmapPtr dst, pointer * src_sample, 
                                   int *src_length);
int exynosVideoBufferCreateFBID (PixmapPtr pPixmap);

Bool exynosVideoBufferSetCrop (PixmapPtr pPixmap, xRectangle crop);
xRectangle exynosVideoBufferGetCrop (PixmapPtr pPixmap);
Bool exynosVideoBufferCopyOneChannel (PixmapPtr pPixmap, pointer * src_sample,
                                      int *src_pitches, int *src_height);


#endif
