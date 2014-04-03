/*
 * exynos_mem_pool.c
 *
 *  Created on: Sep 9, 2013
 *  Author: Andrey Sokolenko
 */

#include "exynos_mem_pool.h"
#include "exynos_util.h"
#include "exynos_video_image.h"
#include "exynos_video_fourcc.h"
#include "exynos_video_buffer.h"
#include "_trace.h"

#define TBM_BODATA_BUFINFO 0x1001


static EXYNOSBufInfoPtr
_exynosMemPoolCreateBufInfo (ScrnInfoPtr pScrn, int id,
                             int width, int height)
{
    EXYNOSBufInfoPtr bufinfo = NULL;
    XF86ImagePtr image_info = NULL;
    bufinfo = ctrl_calloc (1, sizeof(EXYNOSBufInfo));
    XDBG_GOTO_IF_FAIL (bufinfo != NULL, buf_create_fail);
    XDBG_DEBUG (MMPL, "bufinfo %p was created\n", bufinfo);

    bufinfo->pScrn = pScrn;
    bufinfo->width = width;
    bufinfo->height = height;
    bufinfo->fourcc = id;
    bufinfo->size = exynosVideoImageAttrs (bufinfo->fourcc, &bufinfo->width,
                                           &bufinfo->height, bufinfo->pitches,
                                           bufinfo->offsets, bufinfo->lengths);
    XDBG_GOTO_IF_FAIL(bufinfo->size != 0, buf_create_fail);

    image_info = exynosVideoImageGetImageInfo (bufinfo->fourcc);
    XDBG_GOTO_IF_FAIL(image_info->num_planes != 0, buf_create_fail);
    bufinfo->num_planes = image_info->num_planes;

    XDBG_TRACE(
        MMPL,
        "Fourcc(%c%c%c%c),w(%d),h(%d),p(%d,%d,%d),o(%d,%d,%d),l(%d,%d,%d), num_planes %d\n",
        FOURCC_STR (bufinfo->fourcc), bufinfo->width, bufinfo->height,
        bufinfo->pitches[0], bufinfo->pitches[1], bufinfo->pitches[2],
        bufinfo->offsets[0], bufinfo->offsets[1], bufinfo->offsets[2],
        bufinfo->lengths[0], bufinfo->lengths[1], bufinfo->lengths[2],
        bufinfo->num_planes);

    return bufinfo;

buf_create_fail:
    XDBG_ERROR (MMPL, "bufinfo can't created\n");
    if (bufinfo)
        ctrl_free(bufinfo);
    return NULL;
}
static void _test (pointer data)
{
    TRACE;
}

/* This function call in tbm_bo_unref */
static void
_exynosMemPoolDestroyBufInfo (pointer data)
{
    XDBG_RETURN_IF_FAIL (data != NULL);
    EXYNOSBufInfoPtr bufinfo = (EXYNOSBufInfoPtr) data;
    int i = 0;
    if (bufinfo->type == BUF_TYPE_LEGACY)
        for (i = 0; i < bufinfo->num_planes; i++)
        {
            if (bufinfo->handles[i].u32)
                exynosMemPoolFreeHandle (bufinfo->pScrn, bufinfo->handles[i].u32);
        }
}

void
exynosMemPoolDestroyBO (EXYNOSBufInfoPtr bufinfo)
{
    XDBG_RETURN_IF_FAIL (bufinfo != NULL);
    int i = 0;

    if (bufinfo->free_func)
        bufinfo->free_func((pointer) bufinfo);
    bufinfo->free_func = NULL;

    for (i = 0; i < bufinfo->num_planes; i++)
    {
        if (bufinfo->tbos[i])
        {
            tbm_bo_delete_user_data(bufinfo->tbos[i], TBM_BODATA_BUFINFO);
            tbm_bo_unref (bufinfo->tbos[i]);
        }
    }
    XDBG_DEBUG (MVBUF, "bufinfo(%p) freed\n", bufinfo);
    ctrl_free (bufinfo);
}

EXYNOSBufInfoPtr
exynosMemPoolGetBufInfo (pointer b_object)
{
    XDBG_RETURN_VAL_IF_FAIL(b_object != NULL, NULL);
    tbm_bo tbo = (tbm_bo) b_object;
    EXYNOSBufInfoPtr bufinfo = NULL;
    if (!tbm_bo_get_user_data (tbo, TBM_BODATA_BUFINFO, (pointer*) &bufinfo))
        return NULL;
    return bufinfo;
}

EXYNOSBufInfoPtr
exynosMemPoolCreateBO (ScrnInfoPtr pScrn, int id, int width, int height)
{
    XDBG_RETURN_VAL_IF_FAIL(pScrn != NULL, NULL);
    XDBG_RETURN_VAL_IF_FAIL(id != 0, NULL);
    EXYNOSPtr pExynos = EXYNOSPTR(pScrn);
    EXYNOSBufInfoPtr bufinfo = _exynosMemPoolCreateBufInfo (pScrn, id, width, height);
    XDBG_GOTO_IF_FAIL(bufinfo != NULL, bo_create_fail);
    int i = 0, flag = TBM_BO_DEFAULT;
/*    
    if (!pExynos->cachable)
    {
        flag = TBM_BO_WC;
        //XDBG_INFO(MEXA,"TBM_BO_WC\n");
    }
    else
    {
        flag = TBM_BO_DEFAULT;
        //XDBG_INFO (MEXA,"TBM_BO_DEFAULT\n");
    }
 */
    for (i = 0; i < bufinfo->num_planes; i++)
    {
        int alloc_size = 0;
        alloc_size = bufinfo->lengths[i];
        if (alloc_size <= 0)
            break;
        bufinfo->tbos[i] = tbm_bo_alloc (pExynos->bufmgr, alloc_size, flag); /** @todo Add different option of memory type  */
        XDBG_GOTO_IF_FAIL(bufinfo->tbos[i] != NULL, bo_create_fail);

        bufinfo->names[i] = tbm_bo_export (bufinfo->tbos[i]);
        XDBG_GOTO_IF_FAIL(bufinfo->names[i] != 0, bo_create_fail);

        bufinfo->handles[i] = tbm_bo_get_handle (bufinfo->tbos[i],
                              TBM_DEVICE_DEFAULT);
        XDBG_GOTO_IF_FAIL(bufinfo->handles[i].ptr != NULL, bo_create_fail);
    }

    XDBG_GOTO_IF_FAIL(
        tbm_bo_add_user_data(bufinfo->tbos[0], TBM_BODATA_BUFINFO, _exynosMemPoolDestroyBufInfo),
        bo_create_fail);
    XDBG_GOTO_IF_FAIL(
        tbm_bo_set_user_data (bufinfo->tbos[0], TBM_BODATA_BUFINFO, (pointer) bufinfo),
        bo_create_fail);
    /* test */
    for (i = 1; i < bufinfo->num_planes; i++)
    {
        XDBG_GOTO_IF_FAIL(
            tbm_bo_add_user_data(bufinfo->tbos[i], TBM_BODATA_BUFINFO, _test),
            bo_create_fail);
        XDBG_GOTO_IF_FAIL(
            tbm_bo_set_user_data (bufinfo->tbos[i], TBM_BODATA_BUFINFO, (pointer) NULL),
            bo_create_fail);

    }
    XDBG_DEBUG(MMPL, "Success create bo(%p %p %p) h(%u %u %u)\n", bufinfo->tbos[0],
               bufinfo->tbos[1], bufinfo->tbos[2], bufinfo->handles[0].u32,
               bufinfo->handles[1].u32, bufinfo->handles[2].u32);

    return bufinfo;

bo_create_fail:

    if (bufinfo)
    {
        for (i = 0; i < PLANE_CNT; i++)
        {
            if (bufinfo->tbos[i])
                tbm_bo_unref (bufinfo->tbos[i]);
            bufinfo->names[i] = 0;
            bufinfo->handles[i].ptr = NULL;
        }
        ctrl_free (bufinfo);
    }
    
    XDBG_ERROR(MMPL, "Can't create Buffer Object\n");
    return NULL;
}

EXYNOSBufInfoPtr
exynosMemPoolAllocateBO (ScrnInfoPtr pScrn, int id, int width, int height, tbm_bo *tbos,
                         unsigned int *names, unsigned int buf_type)
{
    XDBG_RETURN_VAL_IF_FAIL(pScrn != NULL, NULL);
    XDBG_RETURN_VAL_IF_FAIL(id != 0, NULL);
    EXYNOSPtr pExynos = EXYNOSPTR(pScrn);
    EXYNOSBufInfoPtr bufinfo = _exynosMemPoolCreateBufInfo (pScrn, id, width, height);
    XDBG_GOTO_IF_FAIL(bufinfo != NULL, bo_alloc_fail);
    int i = 0;
    switch (buf_type)
    {
    case BUF_TYPE_LEGACY:
        for (i = 0; i < bufinfo->num_planes; i++)
            if (names[i] > 0)
            {
                exynosMemPoolGetGemHandle (pScrn, (uint64_t) names[i],
                                           (uint64_t) bufinfo->lengths[i],
                                           &bufinfo->handles[i].u32);
                XDBG_GOTO_IF_FAIL(bufinfo->handles[i].u32 > 0, bo_alloc_fail);

                exynosMemPoolGetGemBo (pScrn, bufinfo->handles[i].u32,
                                       &bufinfo->tbos[i]);
                XDBG_GOTO_IF_FAIL(bufinfo->tbos[i] != NULL, bo_alloc_fail);

                bufinfo->names[i] = names[i];
            }
        break;
    case BUF_TYPE_DMABUF:
        for (i = 0; i < bufinfo->num_planes; i++)

            if (names[i] > 0)
            {
                bufinfo->tbos[i] = tbm_bo_import (pExynos->bufmgr, names[i]);
                XDBG_GOTO_IF_FAIL(bufinfo->tbos[i] != NULL, bo_alloc_fail);

                bufinfo->handles[i] = tbm_bo_get_handle (bufinfo->tbos[i],
                                      TBM_DEVICE_DEFAULT);
                XDBG_GOTO_IF_FAIL(bufinfo->handles[i].u32 > 0, bo_alloc_fail);

                bufinfo->names[i] = names[i];
            }
        break;
    case BUF_TYPE_VPIX:
        for (i = 0; i < bufinfo->num_planes; i++)
            if (tbos[i] != NULL) /** @todo Need check */
            {
                bufinfo->tbos[i] = tbm_bo_ref(tbos[i]);

                XDBG_GOTO_IF_FAIL(bufinfo->tbos[0] != NULL, bo_alloc_fail);

                bufinfo->names[0] = tbm_bo_export (bufinfo->tbos[i]);
                XDBG_GOTO_IF_FAIL(bufinfo->names[0] > 0, bo_alloc_fail);

                bufinfo->handles[0] = tbm_bo_get_handle (bufinfo->tbos[i],
                                      TBM_DEVICE_DEFAULT);
                XDBG_GOTO_IF_FAIL(bufinfo->handles[0].u32 > 0, bo_alloc_fail);
            }
        break;
    default:
        XDBG_NEVER_GET_HERE(MMPL);
        return NULL;
        break;

    }

    bufinfo->type = buf_type;

    XDBG_GOTO_IF_FAIL(
        tbm_bo_add_user_data(bufinfo->tbos[0], TBM_BODATA_BUFINFO, _exynosMemPoolDestroyBufInfo),
        bo_alloc_fail);
    XDBG_GOTO_IF_FAIL(
        tbm_bo_set_user_data (bufinfo->tbos[0], TBM_BODATA_BUFINFO, (pointer) bufinfo),
        bo_alloc_fail);
    XDBG_TRACE(MMPL, "Success import bo(%p %p %p) names(%u %u %u) h(%u %u %u) \n",
               bufinfo->tbos[0], bufinfo->tbos[1], bufinfo->tbos[2], bufinfo->names[0],
               bufinfo->names[1], bufinfo->names[2], bufinfo->handles[0].u32,
               bufinfo->handles[1].u32, bufinfo->handles[2].u32);

    return bufinfo;

bo_alloc_fail:

    if (bufinfo)
    {
        ctrl_free (bufinfo);
    }
    XDBG_ERROR(MVBUF, "Can't allocate Buffer Object\n");
    return NULL;
}

Bool
exynosMemPoolPrepareAccess (EXYNOSBufInfoPtr bufinfo, tbm_bo_handle *handles, int option)
{
    XDBG_RETURN_VAL_IF_FAIL(bufinfo != NULL, FALSE);
    int i;
    for (i = 0; i < bufinfo->num_planes; i++)
    {
        handles[i] = tbm_bo_map(bufinfo->tbos[i], TBM_DEVICE_CPU, option);
        XDBG_GOTO_IF_FAIL (handles[i].ptr != NULL, map_fail);
    }

    return TRUE;
map_fail:
    XDBG_ERROR(MMPL, "Can't mmap Buffer Object\n");
    for (i = 0; i < PLANE_CNT; i++)
    {
        if (handles[i].ptr != NULL)
        {
            tbm_bo_unmap(bufinfo->tbos[i]);
            handles[i].ptr = NULL;
        }
    }
    return FALSE;

}

void
exynosMemPoolFinishAccess (EXYNOSBufInfoPtr bufinfo)
{
    XDBG_RETURN_IF_FAIL(bufinfo != NULL);
    int i;
    XDBG_DEBUG (MMPL, "Unmap Buffer Object\n");
    for (i = 0; i < bufinfo->num_planes; i++)
    {
        if (bufinfo->tbos[i])
        {
            tbm_bo_unmap(bufinfo->tbos[i]);
        }
    }
}

void
exynosMemPoolFreeHandle (ScrnInfoPtr pScrn, unsigned int handle)
{
    struct drm_gem_close close;
    EXYNOSPtr pExynos;

    XDBG_RETURN_IF_FAIL(pScrn != NULL);

    pExynos = EXYNOSPTR (pScrn);

    CLEAR(close);
    close.handle = handle;
    XDBG_TRACE(MMPL, "Handle %u free\n", handle);
    if (drmIoctl (pExynos->drm_fd, DRM_IOCTL_GEM_CLOSE, &close))
    {
        XDBG_ERRNO(MMPL, "DRM_IOCTL_GEM_CLOSE failed.\n");
    }
}
Bool
exynosMemPoolGetGemBo (ScrnInfoPtr pScrn, unsigned int handle, tbm_bo *bo)
{
    XDBG_RETURN_VAL_IF_FAIL(pScrn != NULL, FALSE);
    EXYNOSPtr pExynos = EXYNOSPTR(pScrn);
    struct drm_gem_flink arg_flink = {0,};
    arg_flink.handle = handle;

    if (pExynos->drm_fd)
        if (drmIoctl (pExynos->drm_fd, DRM_IOCTL_GEM_FLINK, &arg_flink) < 0)
        {
            XDBG_ERRNO(MMPL, "DRM_IOCTL_GEM_FLINK failed. %U\n", handle);
            return FALSE;
        }
    *bo = tbm_bo_import(pExynos->bufmgr, arg_flink.name);
    XDBG_DEBUG(MMPL, "TBOS: %p\n", (void * )bo);
    return TRUE;
}

Bool
exynosMemPoolGetGemHandle (ScrnInfoPtr pScrn, uint64_t phy_addr, uint64_t size,
                           unsigned int *handle)
{
    XDBG_RETURN_VAL_IF_FAIL(pScrn != NULL, FALSE);
    EXYNOSPtr pExynos = EXYNOSPTR(pScrn);
    struct drm_exynos_gem_phy_imp phy_imp = {0, };

    phy_imp.phy_addr = (uint64_t) phy_addr;
    phy_imp.size = (uint64_t) size;
    if (!phy_addr || size <= 0 || !handle)
        return FALSE;

    if (pExynos->drm_fd)
        if (drmIoctl (pExynos->drm_fd, DRM_IOCTL_EXYNOS_GEM_PHY_IMP, &phy_imp) < 0)
        {
            XDBG_ERRNO(MMPL, "DRM_IOCTL_EXYNOS_GEM_PHY_IMP failed. %p(%d)\n",
                       (void* )phy_addr, size);
            return FALSE;
        }

    *handle = phy_imp.gem_handle;

    XDBG_DEBUG(MMPL, "Handle find h %u\n", *handle);
    return TRUE;
}

void
exynosMemPoolCacheFlush (ScrnInfoPtr scrn)
{
    struct drm_exynos_gem_cache_op cache_op;
    EXYNOSPtr pExynos;
    int ret;
    /** @todo Need correct cache flush for tizen 3.0 */
    return;
    XDBG_RETURN_IF_FAIL(scrn != NULL);

    pExynos = EXYNOSPTR (scrn);

    CLEAR(cache_op);
    cache_op.flags = EXYNOS_DRM_CACHE_FSH_ALL | EXYNOS_DRM_ALL_CACHES_CORES;
    cache_op.usr_addr = 0;
    cache_op.size = 0;

    ret = drmCommandWriteRead (pExynos->drm_fd, DRM_EXYNOS_GEM_CACHE_OP, &cache_op,
                               sizeof (cache_op));
    if (ret)
        xf86DrvMsg (scrn->scrnIndex, X_ERROR, "cache flush failed. (%s)\n",
                    strerror (errno));
}


void
exynosUtilClearNormalVideoBuffer (EXYNOSBufInfoPtr vbuf)
{
    int i;
    tbm_bo_handle bo_handle[vbuf->num_planes];

    if (!vbuf)
        return;
    exynosMemPoolPrepareAccess(vbuf, bo_handle, TBM_OPTION_WRITE);

    XDBG_ERROR (MMPL, "fourcc : '%c%c%c%c'.\n", FOURCC_STR (vbuf->fourcc ));
    for (i = 0; i < vbuf->num_planes; i++)
    {
        int size = 0;
        XDBG_RETURN_IF_FAIL (bo_handle[i].ptr != NULL);
        if (vbuf->fourcc == FOURCC_SN12
                || vbuf->fourcc == FOURCC_SN21
                || vbuf->fourcc == FOURCC_ST12)
            size = vbuf->lengths[i];
        else if (i == 0)
            size = vbuf->size;

        if (size <= 0 || !vbuf->tbos[i])
            continue;


      //  bo_handle = tbm_bo_map (vbuf->tbos[i], TBM_DEVICE_CPU, TBM_OPTION_WRITE);


        if (vbuf->fourcc == FOURCC_SN12
                || vbuf->fourcc == FOURCC_SN21
                || vbuf->fourcc == FOURCC_ST12)
        {
            if (i == 0)
                memset (bo_handle[i].ptr, 0x10, size);
            else if (i == 1)
                memset (bo_handle[i].ptr, 0x80, size);
        }
        else
        {
            int format = exynosVideoBufferGetDrmFourcc (vbuf->fourcc);
            if (format == 0)
            {
                XDBG_ERROR (MDRV, "format not support : '%c%c%c%c'.\n",
                            FOURCC_STR (vbuf->fourcc));
            }

            switch (format)
            {
            case TYPE_YUV420:
                break;
            case TYPE_YUV422:
                break;
            case TYPE_RGB:
                break;
            default:
                break;
            }
            /*
            if (format == TYPE_YUV420)
                _exynosUtilYUV420BlackFrame (bo_handle.ptr, size, vbuf->width, vbuf->height);
            else if (format == TYPE_YUV422)
                _exynosUtilYUV422BlackFrame (vbuf->fourcc, bo_handle.ptr, size, vbuf->width, vbuf->height);
            else if (format == TYPE_RGB)
                memset (bo_handle.ptr, 0, size);
            else
                XDBG_NEVER_GET_HERE (MDRV);*/
        }

      //  tbm_bo_unmap (vbuf->tbos[i]);
    }
    exynosMemPoolFinishAccess(vbuf);
    /** @todo exynosUtilCacheFlush (vbuf->pScrn);
    */
}

