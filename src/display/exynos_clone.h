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

#ifndef __EXYNOS_CLONE_H_
#define __EXYNOS_CLONE_H_

#include <xf86str.h>
#include "exynos_mem_pool.h"

#define CLONE_BUF_MIN      2
#define CLONE_BUF_DEFAULT  3
#define CLONE_BUF_MAX      4

typedef struct _EXYNOS_CloneObject EXYNOS_CloneObject;

typedef enum
{
    CLONE_NOTI_INIT,
    CLONE_NOTI_START,
    CLONE_NOTI_IPP_EVENT,
    CLONE_NOTI_IPP_EVENT_DONE,
    CLONE_NOTI_STOP,
    CLONE_NOTI_CLOSED,
} EXYNOS_CLONE_NOTIFY;

typedef void (*CloneNotifyFunc) (EXYNOS_CloneObject *wb, EXYNOS_CLONE_NOTIFY noti, void *noti_data, void *user_data);

#define exynosCloneInit(s,i,w,h,c,z,n)    _exynosCloneInit(s,i,w,h,c,z,n,__FUNCTION__)

void   exynosCloneHandleIppEvent (int fd, unsigned int *buf_idx, void *data);


EXYNOS_CloneObject*         _exynosCloneInit(ScrnInfoPtr pScrn, unsigned int id, int width, int height,
                                        Bool scanout, int hz, Bool need_rotate_hook, const char *func);
Bool                        exynosCloneStart(EXYNOS_CloneObject*);
void                        exynosCloneStop (EXYNOS_CloneObject*);
void                        exynosCloneClose(EXYNOS_CloneObject*);
unsigned int                exynosCloneGetPropID (void);
void                        exynosCloneAddNotifyFunc (EXYNOS_CloneObject* clone, EXYNOS_CLONE_NOTIFY noti, CloneNotifyFunc func, void *user_data);
void                        exynosCloneRemoveNotifyFunc(EXYNOS_CloneObject* clone,CloneNotifyFunc func);
EXYNOS_CloneObject*         exynosCloneOpen(ScrnInfoPtr pScrn, unsigned int id,int width, int height, Bool scanout, int hz, Bool need_rotate_hook);
Bool                        exynosCloneSetBuffer(EXYNOS_CloneObject *wb, EXYNOSBufInfo **vbufs,int bufnum);
Bool                        exynosCloneIsOpened(void);
Bool                        exynosCloneIsRunning(void);
Bool                        exynosCloneCanDequeueBuffer(EXYNOS_CloneObject* clone);
void                        CloneQueue (EXYNOS_CloneObject *clone,EXYNOSBufInfo  *vbuf);


#endif
