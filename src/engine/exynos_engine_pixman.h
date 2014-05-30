#ifndef EXYNOS_ENGINE_PIXMAN_H
#define EXYNOS_ENGINE_PIXMAN_H

#include <X11/Xprotostr.h>

int exynosEnginePixmanInit ( void);
int exynosEnginePixmanPrepareConvert ( int s_width, int s_height, int s_fourcc, xRectangle *s_crop,
                                       int d_width, int d_height, int d_fourcc, xRectangle *d_crop,
                                       int rotate, int hflip, int vflip);
int exynosEnginePixmanSetBuf ( int prop_id, void *srcbuf, void *dstbuf);
int exynosEnginePixmanSyncConvert ( int prop_id);
int exynosEnginePixmanFinishedConvert ( int prop_id);
#endif // EXYNOS_ENGINE_PIXMAN_H
