
#include "exynos_util.h"


/* list of vpix format */
static EXYNOSFormatTable format_table[] =
{
        { FOURCC_RGB565, DRM_FORMAT_RGB565, TYPE_RGB },
        { FOURCC_SR16, DRM_FORMAT_RGB565, TYPE_RGB },
        { FOURCC_RGB32, DRM_FORMAT_XRGB8888, TYPE_RGB },
        { FOURCC_SR32, DRM_FORMAT_XRGB8888, TYPE_RGB },
        { FOURCC_YV12, DRM_FORMAT_YVU420, TYPE_YUV420 },
        { FOURCC_I420, DRM_FORMAT_YUV420, TYPE_YUV420 },
        { FOURCC_S420, DRM_FORMAT_YUV420, TYPE_YUV420 },
        { FOURCC_ST12, DRM_FORMAT_NV12MT, TYPE_YUV420 },
        { FOURCC_SN12, DRM_FORMAT_NV12M, TYPE_YUV420 },
        { FOURCC_NV12, DRM_FORMAT_NV12, TYPE_YUV420 },
        { FOURCC_SN21, DRM_FORMAT_NV21, TYPE_YUV420 },
        { FOURCC_NV21, DRM_FORMAT_NV21, TYPE_YUV420 },
        { FOURCC_YUY2, DRM_FORMAT_YUYV, TYPE_YUV422 },
        { FOURCC_SUYV, DRM_FORMAT_YUYV, TYPE_YUV422 },
        { FOURCC_UYVY, DRM_FORMAT_UYVY, TYPE_YUV422 },
        { FOURCC_SYVY, DRM_FORMAT_UYVY, TYPE_YUV422 },
        { FOURCC_ITLV, DRM_FORMAT_UYVY, TYPE_YUV422 },
};

/*
unsigned int
exynosVideoBufferGetDrmFourcc (unsigned int fourcc)
{
    int i = 0, size = 0;

    size = sizeof (format_table) / sizeof(EXYNOSFormatTable);

    for (i = 0; i < size; i++)
        if (format_table[i].fourcc == fourcc)
            return format_table[i].drm_fourcc;

    return 0;
}
*/
