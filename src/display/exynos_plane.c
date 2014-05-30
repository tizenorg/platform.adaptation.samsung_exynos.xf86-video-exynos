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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "exynos_driver.h"
#include "exynos_display.h"
#include "exynos_display_int.h"
#include "exynos_util.h"
#include "exynos_mem_pool.h"
#include "_trace.h"
typedef struct _EXYNOSPlane
{
    unsigned int plane_id;
    unsigned int crtc_id;
    unsigned int zpos;
} EXYNOSPlane, *EXYNOSPlanePtr;

static Bool
_ComparePlaneId (pointer structure, pointer element)
{

    EXYNOSPlanePtr using_plane = (EXYNOSPlanePtr) structure;
    unsigned int plane_id = *((unsigned int *) element);
    return (using_plane->plane_id == plane_id);
}

static Bool
_ComparePlaneZpos (pointer structure, pointer element)
{

    EXYNOSPlanePtr using_plane = (EXYNOSPlanePtr) structure;
    unsigned int zpos = *((unsigned int *) element);
    return (using_plane->zpos == zpos);
}

void
exynosPlaneInit (ScrnInfoPtr pScrn, EXYNOSDispInfoPtr pDispInfo, int num)
{

    EXYNOSPlanePrivPtr pPlanePriv;

    pPlanePriv = ctrl_calloc (1, sizeof (EXYNOSPlanePrivRec));
    XDBG_RETURN_IF_FAIL (pPlanePriv != NULL);

    pPlanePriv->mode_plane = drmModeGetPlane (pDispInfo->drm_fd, pDispInfo->plane_res->planes[num]);
    if (!pPlanePriv->mode_plane)
    {
        XDBG_ERRNO (MPLN, "drmModeGetPlane failed. plane(%d)\n",
                    pDispInfo->plane_res->planes[num]);
        ctrl_free (pPlanePriv);
        return;
    }

    pPlanePriv->pDispInfo = pDispInfo;
    pPlanePriv->plane_id = pDispInfo->plane_res->planes[num];
    xf86DrvMsg (pScrn->scrnIndex, X_INFO,
                            "plane #(%i) - plane_id (%i)\n",num,pPlanePriv->plane_id);
    xorg_list_add (&pPlanePriv->link, &pDispInfo->planes);
}

void
exynosPlaneDeinit (EXYNOSPlanePrivPtr pPlanePriv)
{
    drmModeFreePlane (pPlanePriv->mode_plane);
    xorg_list_del (&pPlanePriv->link);
    ctrl_free (pPlanePriv);
}

unsigned int
exynosPlaneGetAvailable (ScrnInfoPtr pScrn)
{
    /** @todo Maybe need store plane id*/

    XDBG_RETURN_VAL_IF_FAIL (pScrn != NULL, 0);
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    XDBG_RETURN_VAL_IF_FAIL (pExynos != NULL, 0);
    EXYNOSDispInfoPtr pDispInfo = pExynos->pDispInfo;
    XDBG_RETURN_VAL_IF_FAIL (pDispInfo != NULL, 0);
    drmModePlanePtr mode_plane = NULL;
    int i = 0, ret = 0;
    while (i < pDispInfo->plane_res->count_planes && ret == 0)

    {
        mode_plane = drmModeGetPlane (
                                      pDispInfo->drm_fd, pDispInfo->plane_res->planes[i]);
        XDBG_RETURN_VAL_IF_FAIL (mode_plane != NULL, 0);
        if (mode_plane->crtc_id == 0)
        {
            ret = mode_plane->plane_id;
        }
        drmModeFreePlane (mode_plane);
        ++i;
    }
    return ret;
}
static unsigned int
_FindAvailableZpos(EXYNOSDispInfoPtr pDispInfo)
{
    XDBG_RETURN_VAL_IF_FAIL (pDispInfo != NULL, 0);
    unsigned int zpos = LAYER_LOWER1, ret = 0;
    EXYNOSPlanePtr using_plane = NULL;
    while (zpos < LAYER_EFL)
    {
        using_plane = (EXYNOSPlanePtr)
                exynosUtilStorageFindData (pDispInfo->using_planes, 
                                           (pointer) &zpos, _ComparePlaneZpos);
        if (using_plane == NULL)
        {
            ret = zpos;
            break;
        }
        zpos++;
   }
   return ret;
}
Bool
exynosPlaneDraw (ScrnInfoPtr pScrn, xRectangle fixed_box, xRectangle crtc_box,
                 unsigned int fb_id, unsigned int plane_id, unsigned int crtc_id, unsigned int zpos)
{
    XDBG_RETURN_VAL_IF_FAIL (pScrn != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (plane_id != 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (fb_id != 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (crtc_id != 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (zpos < LAYER_MAX, FALSE);
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    XDBG_RETURN_VAL_IF_FAIL (pExynos != NULL, FALSE);
    EXYNOSDispInfoPtr pDispInfo = pExynos->pDispInfo;
    XDBG_RETURN_VAL_IF_FAIL (pDispInfo != NULL, FALSE);
    EXYNOSPlanePtr using_plane = NULL;
    using_plane = (EXYNOSPlanePtr)
            exynosUtilStorageFindData (pDispInfo->using_planes, 
                                       (pointer) &plane_id, _ComparePlaneId);
    if (!using_plane)
    {
        if (zpos != LAYER_UPPER)
        {
            zpos = _FindAvailableZpos(pDispInfo);
            if (zpos == LAYER_NONE)
                return FALSE;
        }
        XDBG_RETURN_VAL_IF_FAIL (
                exynosUtilSetDrmProperty (pExynos->drm_fd, plane_id,
                                   DRM_MODE_OBJECT_PLANE, "zpos", zpos),
                                 FALSE);
    }
    else
    {
        if (zpos != LAYER_UPPER && using_plane->zpos != LAYER_LOWER1)
        {
            zpos = _FindAvailableZpos(pDispInfo);
            if (zpos == LAYER_LOWER1)
            {
               XDBG_RETURN_VAL_IF_FAIL (
                exynosUtilSetDrmProperty (pExynos->drm_fd, plane_id,
                                   DRM_MODE_OBJECT_PLANE, "zpos", zpos),
                                 FALSE); 
               using_plane->zpos = zpos;
            }
            
        }
    }

    

    /* Source values are 16.16 fixed point */
    uint32_t fixed_x = ((unsigned int) fixed_box.x) << 16;
    uint32_t fixed_y = ((unsigned int) fixed_box.y) << 16;
    uint32_t fixed_w = ((unsigned int) fixed_box.width) << 16;
    uint32_t fixed_h = ((unsigned int) fixed_box.height) << 16;
    /* restriction for exynos hw display controller */
    uint32_t crtc_x = crtc_box.x & ~1;
    uint32_t crtc_y = crtc_box.y;
    uint32_t crtc_w = crtc_box.width;
    uint32_t crtc_h = crtc_box.height;
    TRACE;
    XDBG_TRACE (MPLN,
                "Plane(%d) show: crtc(%d) fb(%d) (%d,%d %dx%d) (%d,%d %dx%d)\n",
                plane_id, crtc_id, fb_id, crtc_x, crtc_y,
                crtc_w, crtc_h, fixed_x >> 16, fixed_y >> 16, fixed_w >> 16,
                fixed_h >> 16);
    if (drmModeSetPlane (pExynos->drm_fd, plane_id, crtc_id, fb_id, 0,
                         crtc_x, crtc_y, crtc_w, crtc_h, fixed_x, fixed_y, fixed_w,
                         fixed_h))
    {
        XDBG_ERRNO (MPLN, "drmModeSetPlane failed. plane(%d) crtc(%d)\n",
                    plane_id, crtc_id);
        return FALSE;
    }

    if (!using_plane)
    {
        using_plane = (EXYNOSPlanePtr) ctrl_calloc (1, sizeof (EXYNOSPlane));
        XDBG_RETURN_VAL_IF_FAIL (using_plane != NULL, FALSE);
        using_plane->plane_id = plane_id;
        using_plane->crtc_id = crtc_id;
        using_plane->zpos = zpos;
        XDBG_RETURN_VAL_IF_FAIL(
            exynosUtilStorageAdd (pDispInfo->using_planes, (pointer) using_plane)
                                ,FALSE);
    }

    
    TRACE;
    return TRUE;
}

unsigned int
exynosPlaneGetCrtcId (ScrnInfoPtr pScrn, unsigned int plane_id)
{
    XDBG_RETURN_VAL_IF_FAIL (pScrn != NULL, 0);
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    XDBG_RETURN_VAL_IF_FAIL (pExynos != NULL, 0);
    EXYNOSDispInfoPtr pDispInfo = pExynos->pDispInfo;
    XDBG_RETURN_VAL_IF_FAIL (pDispInfo != NULL, 0);
    drmModePlanePtr mode_plane = NULL;
    int i = 0, ret = 0;
    while (i < pDispInfo->plane_res->count_planes && ret == 0)
    {
        mode_plane = drmModeGetPlane (
                                      pDispInfo->drm_fd, pDispInfo->plane_res->planes[i]);
        XDBG_RETURN_VAL_IF_FAIL (mode_plane != NULL, 0);
        if (mode_plane->plane_id == plane_id)
        {
            ret = mode_plane->crtc_id;
        }
        drmModeFreePlane (mode_plane);
        ++i;
    }
    return ret;
}

Bool
exynosPlaneHide (ScrnInfoPtr pScrn, unsigned int plane_id)
{
    XDBG_RETURN_VAL_IF_FAIL (pScrn != NULL, FALSE);
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    XDBG_RETURN_VAL_IF_FAIL (pExynos != NULL, FALSE);
    EXYNOSDispInfoPtr pDispInfo = pExynos->pDispInfo;
    XDBG_RETURN_VAL_IF_FAIL (pDispInfo != NULL, FALSE);
    unsigned int crtc_id = 0;

    EXYNOSPlanePtr using_plane = (EXYNOSPlanePtr)
            exynosUtilStorageFindData (pDispInfo->using_planes,
                                       (pointer) & plane_id, _ComparePlaneId);
    if (using_plane)
    {
        exynosUtilStorageEraseData (pDispInfo->using_planes, (pointer) using_plane);
        ctrl_free (using_plane);
    }

    crtc_id = exynosPlaneGetCrtcId (pScrn, plane_id);

    if (crtc_id == 0)
        return TRUE;

    if (drmModeSetPlane (pExynos->drm_fd, plane_id, crtc_id, 0, 0,
                         0, 0, 0, 0, 0, 0, 0, 0))
    {
        XDBG_ERRNO (MPLN, "drmModeSetPlane failed. plane(%d) crtc(%d)\n",
                    plane_id, crtc_id);
        return FALSE;
    }
    TRACE;
    return TRUE;
}
