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

#include <pixman.h>
#include "exynos_engine_pixman.h"
#include "exynos_video_fourcc.h"
#include "exynos_util.h"
#include <xf86.h>

typedef struct _EXYNOSPixmanProp
{
    int prop_id;
    int width[2];
    int height[2];
    xRectangle crop[2];
    int fourcc[2];
    int degree;
    int vflip;
    int hflip;
    pixman_image_t *img[2];
} EXYNOSPixmanPropRec, *EXYNOSPixmanPropPtr;

typedef struct _EXYNOSPixmanFormatTable
{
    unsigned int     fourcc;        /* defined by X */
    unsigned int     pixman_fourcc;    /* defined by Pixman */
} EXYNOSPixmanFormatTable;


static EXYNOSPixmanFormatTable format_table[] =
{
    { FOURCC_RGB565, PIXMAN_r5g6b5 },
    { FOURCC_RGB32, PIXMAN_a8r8g8b8 },
    { FOURCC_YV12, PIXMAN_yv12 },
//    { FOURCC_I420, PIXMAN_x8b8g8r8 },
    { FOURCC_YUY2, PIXMAN_yuy2 },
};

static pointer prop_array;

static int
_fourccXorgToPixman (int fourcc)
{
    int i = 0, size = 0;

    size = sizeof (format_table) / sizeof(EXYNOSPixmanFormatTable);

    for (i = 0; i < size; i++)
        if (format_table[i].fourcc == fourcc)
            return format_table[i].pixman_fourcc;

    return 0;
}



static Bool
_enginePixmanComparePropId (pointer structure, pointer element)
{
    int stamp = * ((int *) element);
    EXYNOSPixmanPropPtr property = (EXYNOSPixmanPropPtr) structure;
    return (property->prop_id == stamp);
}

int
exynosEnginePixmanInit (void)
{
    xf86Msg(X_INFO, "[%s] %p %p\n", __FUNCTION__, prop_array, &prop_array);
    prop_array = exynosUtilStorageInit();
    xf86Msg(X_INFO, "[%s] %p %p\n", __FUNCTION__, prop_array, &prop_array);
    if (prop_array == NULL)
    {
        xf86Msg(X_ERROR,"Can't init Pixman engine\n");
        return FALSE;
    }

    return TRUE;
}
static void
_setupProperty (int way, int fourcc,int width, int height,
                xRectangle *crop, EXYNOSPixmanPropPtr config)
{
    XDBG_RETURN_IF_FAIL ((way < 2 && way >= 0));
    config->fourcc[way] = _fourccXorgToPixman(fourcc);
    config->width[way] = width;
    config->height[way] = height;
    config->crop[way].x = crop->x;
    config->crop[way].y = crop->y;
    config->crop[way].width = crop->width;
    config->crop[way].height = crop->height;
    XDBG_TRACE(MPXM, "Property setup for %s frame with current options:\n", ((way == 0)?"SRC":"DST"));
    XDBG_TRACE(MPXM, "width: %d, height: %d, crop.x: %d, crop.y: %d, crop.width: %d, crop.height: %d\n",
               width, height, crop->x, crop->y, crop->width, crop->height);
}

int
exynosEnginePixmanPrepareConvert( int s_width, int s_height, int s_fourcc, xRectangle *s_crop,
                                  int d_width, int d_height, int d_fourcc, xRectangle *d_crop,
                                  int rotate, int hflip, int vflip)
{

    //  _pixmanCheck();
        /** @todo IPP converter copy property or not ?? */
        if (_fourccXorgToPixman (s_fourcc) == 0 ||
            _fourccXorgToPixman (d_fourcc) == 0)
        {
            XDBG_WARNING(MPXM, "Unsupported fourcc format\n");
            return 0;
        }


        EXYNOSPixmanPropPtr property = calloc(1, sizeof(EXYNOSPixmanPropRec));
        XDBG_RETURN_VAL_IF_FAIL(property != NULL, 0);

        property->hflip = hflip;
        property->vflip = vflip;
        property->degree = (rotate + 360) / 90 % 4;

        _setupProperty(0, s_fourcc,
                     s_width, s_height,
                     s_crop, property);

        _setupProperty(1, d_fourcc,
                     d_width, d_height,
                     d_crop, property);

       XDBG_TRACE(MPXM, "vflip: %d, hflip: %d, rotate: %d\n", hflip, vflip, (rotate % 360));

       int prop_id = 1;

       while (exynosUtilStorageFindData (prop_array, &prop_id,
                                          _enginePixmanComparePropId))
            prop_id++;

        property->prop_id = prop_id;

        if(!exynosUtilStorageAdd(prop_array, (void*) property))
        {
            XDBG_DEBUG(MPXM,"Can't add storage\n" );
            XDBG_DEBUG(MPXM,"%p, %p\n", prop_array, &prop_array);
            return 0;
        }
        XDBG_DEBUG(MPXM, "PrepareConvert done prop_id(%d)\n", property->prop_id);
        return property->prop_id;

}

int
exynosEnginePixmanSetBuf (int prop_id, void *srcbuf, void *dstbuf)
{
    XDBG_RETURN_VAL_IF_FAIL(srcbuf != NULL, 0);
    XDBG_RETURN_VAL_IF_FAIL(dstbuf != NULL, 0);
    int                src_stride, dst_stride;
    int                src_bpp;
    int                dst_bpp;

    EXYNOSPixmanPropPtr pProp = (EXYNOSPixmanPropPtr) exynosUtilStorageFindData (prop_array, &prop_id,
                               _enginePixmanComparePropId);
    XDBG_RETURN_VAL_IF_FAIL(pProp != NULL, 0);

    src_bpp = PIXMAN_FORMAT_BPP (pProp->fourcc[0]) / 8;
    XDBG_RETURN_VAL_IF_FAIL (src_bpp > 0, 0);

    dst_bpp = PIXMAN_FORMAT_BPP (pProp->fourcc[1]) / 8;
    XDBG_RETURN_VAL_IF_FAIL (dst_bpp > 0, 0);

    src_stride = pProp->width[0] * src_bpp;
    dst_stride = pProp->width[1] * dst_bpp;
    XDBG_DEBUG(MPXM,"Set buf\n" );
    pProp->img[0] = pixman_image_create_bits (pProp->fourcc[0], pProp->width[0], pProp->height[0], (uint32_t*)srcbuf, src_stride);
    XDBG_RETURN_VAL_IF_FAIL (pProp->img[0] != NULL, 0);
    pProp->img[1] = pixman_image_create_bits (pProp->fourcc[1], pProp->width[1], pProp->height[1], (uint32_t*)dstbuf, dst_stride);
    if (pProp->img[1] == NULL)
    {
        pixman_image_unref(pProp->img[0]);
        pProp->img[0] = NULL;
        XDBG_ERROR (MPXM, "Can't create pixman image\n");
        return 0;
    }
    return 1;
}

int
exynosEnginePixmanSyncConvert(int prop_id)
{
    struct pixman_f_transform ft;
    pixman_transform_t transform;
    double             scale_x, scale_y;
    EXYNOSPixmanPropPtr pProp = exynosUtilStorageFindData (prop_array, &prop_id,
                               _enginePixmanComparePropId);
    XDBG_RETURN_VAL_IF_FAIL (pProp->img[0] != NULL, 0);
    XDBG_RETURN_VAL_IF_FAIL (pProp->img[1] != NULL, 0);
    XDBG_DEBUG(MPXM,"Converting ...\n" );
    pixman_f_transform_init_identity (&ft);

    if (pProp->hflip)
    {
        pixman_f_transform_scale (&ft, NULL, -1, 1);
        pixman_f_transform_translate (&ft, NULL, pProp->crop[1].width, 0);
    }

    if (pProp->vflip)
    {
        pixman_f_transform_scale (&ft, NULL, 1, -1);
        pixman_f_transform_translate (&ft, NULL, 0, pProp->crop[1].height);
    }

    if (pProp->degree > 0)
    {
        int c, s, tx = 0, ty = 0;
        switch (pProp->degree)
        {
        case 1:
            /* 90 degrees */
            c = 0;
            s = -1;
            tx = -pProp->crop[1].width;
            break;
        case 2:
            /* 180 degrees */
            c = -1;
            s = 0;
            tx = -pProp->crop[1].width;
            ty = -pProp->crop[1].height;
            break;
        case 3:
            /* 270 degrees */
            c = 0;
            s = 1;
            ty = -pProp->crop[1].height;
            break;
        default:
            /* 0 degrees */
            c = 0;
            s = 0;
            break;
        }

        pixman_f_transform_translate (&ft, NULL, tx, ty);
        pixman_f_transform_rotate (&ft, NULL, c, s);
    }

    if (pProp->degree % 2 == 0)
    {
        scale_x = (double) pProp->crop[0].width / pProp->crop[1].width;
        scale_y = (double) pProp->crop[0].height / pProp->crop[1].height;
    }
    else
    {
        scale_x = (double)pProp->crop[0].width / pProp->crop[1].height;
        scale_y = (double)pProp->crop[0].height / pProp->crop[1].width;
    }

    pixman_f_transform_scale (&ft, NULL, scale_x, scale_y);
    pixman_f_transform_translate (&ft, NULL, pProp->crop[0].x, pProp->crop[0].y);

    pixman_transform_from_pixman_f_transform (&transform, &ft);
    pixman_image_set_transform (pProp->img[0], &transform);

    pixman_image_composite (PIXMAN_OP_SRC, pProp->img[0], NULL, pProp->img[1],
                            0, 0, 0, 0, pProp->crop[1].x, pProp->crop[1].y, pProp->crop[1].width, pProp->crop[1].height);

    if (pixman_image_unref (pProp->img[0]))
       pProp->img[0] = NULL;
    if (pixman_image_unref (pProp->img[1]));
       pProp->img[1] = NULL;
    return 1;
}

int
exynosEnginePixmanFinishedConvert(int prop_id)
{
    EXYNOSPixmanPropPtr pProp = exynosUtilStorageFindData (prop_array, &prop_id,
                               _enginePixmanComparePropId);
    if (pProp == NULL)
        return 1;
    XDBG_RETURN_VAL_IF_FAIL (pProp->img[0] == NULL, 0);
    XDBG_RETURN_VAL_IF_FAIL (pProp->img[1] == NULL, 0);
    exynosUtilStorageEraseData (prop_array, pProp);
    free(pProp);
    return 1;
}

int
exynosEnginePixmanCopyRawToPlane(void* raw, void* plane)
{
	return 0;
}




