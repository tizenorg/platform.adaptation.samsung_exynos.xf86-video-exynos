/**************************************************************************

xserver-xorg-video-exynos

Copyright 2010 - 2013 Samsung Electronics co., Ltd. All Rights Reserved.

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

#ifndef __EXYNOS_UTIL_H__
#define __EXYNOS_UTIL_H__

#include <exynos_driver.h>
#include "exynos_xdbg.h"
#include <drm_fourcc.h>
#include "exynos_video_fourcc.h"
#include <tbm_bufmgr.h>

/* helper for video */

typedef enum
{
    TYPE_NONE,
    TYPE_RGB,
    TYPE_YUV444,
    TYPE_YUV422,
    TYPE_YUV420,
} EXYNOSFormatType;

typedef struct _EXYNOSFormatTable
{
    unsigned int     fourcc;        /* defined by X */
    unsigned int     drm_fourcc;    /* defined by DRM */
    EXYNOSFormatType type;
} EXYNOSFormatTable;


#define MTST    XDBG_M('T','S','T',0)

#define rgnSAME 3

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef SWAP
#define SWAP(a, b)  ({int t; t = a; a = b; b = t;})
#endif
#ifndef CLEAR
#define CLEAR(x) memset(&(x), 0, sizeof (x))
#endif


#define ALIGN_TO_16B(x)    ((((x) + (1 <<  4) - 1) >>  4) <<  4)
#define ALIGN_TO_32B(x)    ((((x) + (1 <<  5) - 1) >>  5) <<  5)
#define ALIGN_TO_128B(x)   ((((x) + (1 <<  7) - 1) >>  7) <<  7)
#define ALIGN_TO_2KB(x)    ((((x) + (1 << 11) - 1) >> 11) << 11)
#define ALIGN_TO_8KB(x)    ((((x) + (1 << 13) - 1) >> 13) << 13)
#define ALIGN_TO_64KB(x)   ((((x) + (1 << 16) - 1) >> 16) << 16)

typedef Bool (*cmp)(void *, void *);

/* box calculations */
int exynosUtilBoxInBox (BoxPtr base, BoxPtr box);
int exynosUtilBoxArea(BoxPtr pBox);
int exynosUtilBoxIntersect(BoxPtr pDstBox, BoxPtr pBox1, BoxPtr pBox2);
void exynosUtilBoxMove(BoxPtr pBox, int dx, int dy);

const PropertyPtr exynosUtilGetWindowProperty (WindowPtr pWin, const char *prop_name);

Bool exynosUtilSetDrmProperty (int drm_fd, unsigned int obj_id, unsigned int obj_type,
                               const char *prop_name, unsigned int value);

pointer exynosUtilStorageInit(void);
Bool exynosUtilStorageAdd(pointer container, pointer data);
pointer exynosUtilStorageFindData(pointer, pointer element, cmp);
Bool exynosUtilStorageEraseData(pointer container, pointer);
void exynosUtilStorageFinit(pointer container);
Bool exynosUtilStorageGetAll (pointer container, pointer *pData, int size);
pointer exynosUtilStorageGetFirst (pointer container);
pointer exynosUtilStorageGetLast (pointer container);
Bool exynosUtilStorageIsEmpty(pointer container); /** \deprecated */
int exynosUtilStorageGetSize (pointer container);
pointer exynosUtilStorageGetData (pointer container, int index);
void exynosUtilRotateRect (int xres, int yres, int src_rot, int dst_rot, xRectangle *rect);
void exynosUtilPlaneDump (ScrnInfoPtr pScrn);

int exynosUtilGetTboRef(tbm_bo bo);


#endif /* __EXYNOS_UTIL_H__ */

