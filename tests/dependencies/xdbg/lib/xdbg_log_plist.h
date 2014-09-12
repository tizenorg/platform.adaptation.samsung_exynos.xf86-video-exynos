/**************************************************************************

xdbg

Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved

Contact: SooChan Lim <sc1.lim@samsung.com>
         Sangjin LEE <lsj119@samsung.com>

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

#ifndef __XDBG_LOG_PLIST_H__
#define __XDBG_LOG_PLIST_H__

#include <pixmap.h>

/* pixmap usage_hint stands for back-flipbuffer pixmap */
#ifndef CREATE_PIXMAP_USAGE_DRI2_FLIP_BACK
#define CREATE_PIXMAP_USAGE_DRI2_FLIP_BACK  0x100
#endif

/* pixmap usage_hint stands for framebuffer pixmap */
#ifndef CREATE_PIXMAP_USAGE_FB
#define CREATE_PIXMAP_USAGE_FB              0x101
#endif

/* pixmap usage_hint stands for sub-framebuffer pixmap */
#ifndef CREATE_PIXMAP_USAGE_SUB_FB
#define CREATE_PIXMAP_USAGE_SUB_FB          0x202
#endif

/* pixmap usage_hint stands for backbuffer pixmap */
#ifndef CREATE_PIXMAP_USAGE_DRI2_BACK
#define CREATE_PIXMAP_USAGE_DRI2_BACK       0x404
#endif

void xDbgLogPListInit              (ScreenPtr pScreen);
void xDbgLogPListDeInit            (ScreenPtr pScreen);

void xDbgLogPListDrawAddRefPixmap    (DrawablePtr pDraw, PixmapPtr pRefPixmap);
void xDbgLogPListDrawRemoveRefPixmap (DrawablePtr pDraw, PixmapPtr pRefPixmap);

void xDbgLogPList                    (char *reply, int *len);

#endif  /* __XDBG_LOG_PLIST_H__ */
