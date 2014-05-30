#ifndef EXYNOS_ENGINE_H
#define EXYNOS_ENGINE_H

#include <xf86str.h>

typedef struct _EXYNOSEngineInfo
{
    int (*PrepareConvert) ( int s_width, int s_height, int s_fourcc, xRectangle *s_crop,
                            int d_width, int d_height, int d_fourcc, xRectangle *d_crop,
                            int rotate, int hflip, int vflip);
    int (*SetBuffer) (int prop_id, void *srcbuf, void *dstbuf);
    int (*SyncConverter) (int prop_id);
    int (*FinishedConvert) (int prop_id);


} *EXYNOSEngineInfoPtr, EXYNOSEngineInfoRec;

int exynosEngineInit (ScrnInfoPtr pScrn);

#endif // EXYNOS_ENGINE_H
