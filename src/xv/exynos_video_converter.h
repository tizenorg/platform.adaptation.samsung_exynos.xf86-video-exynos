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

#ifndef __EXYNOS_VIDEO_CONVERTER_H__
#define __EXYNOS_VIDEO_CONVERTER_H__

#include <sys/types.h>
#include <fbdevhw.h>
#include <X11/Xdefs.h>
#include <tbm_bufmgr.h>
#include "xv_types.h"
#include "exynos_video_image.h"
#include "exynos_video_buffer.h"
#include "exynos_mem_pool.h"
#include <exynos_drm.h>

/** @todo EXYNOSIppBuf Temporary here */
typedef struct
{
    EXYNOSCvtType type;
    unsigned int index;
    tbm_bo_handle handles[PLANE_CNT];
    CARD32 stamp;
    EXYNOSBufInfoPtr buffer;
    PixmapPtr pVideoPixmap;

} EXYNOSIppBuf, *EXYNOSIppBufPtr;

Bool exynosVideoConverterOpen (IMAGEPortInfoPtr pPort, PixmapPtr src, PixmapPtr dst);
void exynosVideoConverterClose(IMAGEPortInfoPtr pPort);
Bool exynosVideoConverterAdaptSize (EXYNOSIppProp *src, EXYNOSIppProp *dst);
Bool exynosVideoConverterIppDequeueBuf (IMAGEPortInfoPtr pPort, EXYNOSIppPtr pIpp_data,
                                        EXYNOSIppBuf *pIpp_buf);


#endif
