/**************************************************************************

xserver-xorg-video-exynos

Copyright 2013 Samsung Electronics co., Ltd. All Rights Reserved.

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

#include "exynos_util.h"
#include "regionstr.h"
#include <exynos/exynos_drm.h>
#include <stdlib.h>
#include <list.h>
#include "_trace.h"
#include "exynos_mem_pool.h"


/*  true iff two Boxes overlap */
#define EXTENTCHECK(r1, r2)    \
    (!( ((r1)->x2 <= (r2)->x1)  || \
        ((r1)->x1 >= (r2)->x2)  || \
        ((r1)->y2 <= (r2)->y1)  || \
        ((r1)->y1 >= (r2)->y2) ) )

/* true iff (x,y) is in Box */
#define INBOX(r, x, y)  \
    ( ((r)->x2 >  x) && \
      ((r)->x1 <= x) && \
      ((r)->y2 >  y) && \
 
/* true iff Box r1 contains Box r2 */
#define SUBSUMES(r1, r2)    \
    ( ((r1)->x1 <= (r2)->x1) && \
      ((r1)->x2 >= (r2)->x2) && \
      ((r1)->y1 <= (r2)->y1) && \
      ((r1)->y2 >= (r2)->y2) )

typedef struct
{
    int index;
    struct xorg_list list_of_storage;
} EXYNOSDLinkedList;

typedef struct
{
    int index;
    void *data;
    struct xorg_list link;
} EXYNOSDLinkedListData;

int
exynosUtilBoxInBox (BoxPtr base, BoxPtr box)
{
    XDBG_RETURN_VAL_IF_FAIL (base != NULL, -1);
    XDBG_RETURN_VAL_IF_FAIL (box != NULL, -1);

    if (base->x1 == box->x1 && base->y1 == box->y1 && base->x2 == box->x2 && base->y2 == box->y2)
    {
        return rgnSAME;
    }
    else if (SUBSUMES (base, box))
    {
        return rgnIN;
    }
    else if (EXTENTCHECK (base, box))
    {
        return rgnPART;
    }
    else
        return rgnOUT;

    return -1;
}

int
exynosUtilBoxArea (BoxPtr pBox)
{
    return (int) (pBox->x2 - pBox->x1) * (int) (pBox->y2 - pBox->y1);
}

int
exynosUtilBoxIntersect (BoxPtr pDstBox, BoxPtr pBox1, BoxPtr pBox2)
{
    pDstBox->x1 = pBox1->x1 > pBox2->x1 ? pBox1->x1 : pBox2->x1;
    pDstBox->x2 = pBox1->x2 < pBox2->x2 ? pBox1->x2 : pBox2->x2;
    pDstBox->y1 = pBox1->y1 > pBox2->y1 ? pBox1->y1 : pBox2->y1;
    pDstBox->y2 = pBox1->y2 < pBox2->y2 ? pBox1->y2 : pBox2->y2;

    if (pDstBox->x1 >= pDstBox->x2 || pDstBox->y1 >= pDstBox->y2)
    {
        pDstBox->x1 = 0;
        pDstBox->x2 = 0;
        pDstBox->y1 = 0;
        pDstBox->y2 = 0;
        return rgnOUT;
    }

    if (pDstBox->x1 == pBox2->x1 &&
        pDstBox->y1 == pBox2->y1 &&
        pDstBox->x2 == pBox2->x2 &&
        pDstBox->y2 == pBox2->y2)
        return rgnIN;

    return rgnPART;
}

void
exynosUtilBoxMove (BoxPtr pBox, int dx, int dy)
{
    if (dx == 0 && dy == 0) return;

    pBox->x1 += dx;
    pBox->x2 += dx;
    pBox->y1 += dy;
    pBox->y2 += dy;
}

const PropertyPtr
exynosUtilGetWindowProperty (WindowPtr pWin, const char *prop_name)
{
    int rc;
    Mask prop_mode = DixReadAccess;
    Atom property;
    PropertyPtr pProp;

    property = MakeAtom (prop_name, strlen (prop_name), FALSE);
    if (property == None)
        return NULL;

    rc = dixLookupProperty (&pProp, pWin, property, serverClient, prop_mode);
    if (rc == Success && pProp->data)
        return pProp;

    return NULL;
}

Bool
exynosUtilSetDrmProperty (int drm_fd, unsigned int obj_id, unsigned int obj_type,
                          const char *prop_name, unsigned int value)
{
    drmModeObjectPropertiesPtr props;
    unsigned int i;

    XDBG_RETURN_VAL_IF_FAIL (obj_id > 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (obj_type > 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (prop_name != NULL, FALSE);

    props = drmModeObjectGetProperties (drm_fd, obj_id, obj_type);
    if (!props)
    {
        XDBG_ERRNO (MDRV, "fail : drmModeObjectGetProperties.\n");
        return FALSE;
    }

    for (i = 0; i < props->count_props; i++)
    {
        drmModePropertyPtr prop = drmModeGetProperty (drm_fd, props->props[i]);
        int ret;

        if (!prop)
        {
            XDBG_ERRNO (MDRV, "fail : drmModeGetProperty.\n");
            drmModeFreeObjectProperties (props);
            return FALSE;
        }

        if (!strcmp (prop->name, prop_name))
        {
            ret = drmModeObjectSetProperty (drm_fd, obj_id, obj_type,
                                            prop->prop_id, value);
            if (ret < 0)
            {
                XDBG_ERRNO (MDRV, "fail : drmModeObjectSetProperty.\n");
                drmModeFreeProperty (prop);
                drmModeFreeObjectProperties (props);
                return FALSE;
            }

            drmModeFreeProperty (prop);
            drmModeFreeObjectProperties (props);

            return TRUE;
        }

        drmModeFreeProperty (prop);
    }

    XDBG_ERROR (MDRV, "fail : drm set property.\n");

    drmModeFreeObjectProperties (props);

    return FALSE;
}

pointer
exynosUtilStorageInit (void)
{
    EXYNOSDLinkedList *container = NULL;
    container = ctrl_calloc (1, sizeof (EXYNOSDLinkedList));
    XDBG_RETURN_VAL_IF_FAIL (container != NULL, NULL);
    xorg_list_init (&(container->list_of_storage));
    container->index = 0;

    XDBG_DEBUG (MDRV, "Container %p Init. Head %p\n", container,
                &container->list_of_storage);
    return (void*) container;
}

Bool
exynosUtilStorageAdd (pointer container, pointer data)
{
    XDBG_RETURN_VAL_IF_FAIL (container != NULL, FALSE);
    EXYNOSDLinkedList *pContainer = (EXYNOSDLinkedList *) container;
    EXYNOSDLinkedListData *storage = NULL;

    storage = ctrl_calloc (1, sizeof (EXYNOSDLinkedListData));
    XDBG_RETURN_VAL_IF_FAIL (storage != NULL, FALSE);

    storage->data = data;
    xorg_list_add (&storage->link, &pContainer->list_of_storage);
    ++(pContainer->index);

    XDBG_DEBUG (MDRV, "DList %p put data %p to storage %p index %d\n", container, data,
                storage, pContainer->index);
    return TRUE;
}

pointer
exynosUtilStorageFindData (pointer container, pointer element, cmp comparedata)
{
    XDBG_RETURN_VAL_IF_FAIL (container != NULL, NULL);
    XDBG_RETURN_VAL_IF_FAIL (comparedata != NULL, NULL);
    XDBG_RETURN_VAL_IF_FAIL (element != NULL, NULL);
    EXYNOSDLinkedList *pContainer = (EXYNOSDLinkedList *) container;
    if (xorg_list_is_empty (&pContainer->list_of_storage))
    {
        return NULL;
    }
    EXYNOSDLinkedListData *storage = NULL, *next = NULL;

    xorg_list_for_each_entry_safe (storage, next, &pContainer->list_of_storage, link)
    {
        if ((*comparedata) (storage->data, element))
        {
            XDBG_DEBUG (MDRV, "DList %p find element %p in storage %p\n",
                        container, element, storage);
            return storage->data;
        }
    }
    return NULL;
}

Bool
exynosUtilStorageEraseData (pointer container, void *data)
{
    XDBG_RETURN_VAL_IF_FAIL (container != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (data != NULL, FALSE);
    EXYNOSDLinkedList *pContainer = (EXYNOSDLinkedList *) container;
    EXYNOSDLinkedListData *storage = NULL, *next = NULL;
    if (xorg_list_is_empty (&pContainer->list_of_storage))
    {
        return FALSE;
    }

    xorg_list_for_each_entry_safe (storage, next, &pContainer->list_of_storage, link)
    {
        if (storage->data == data)
        {
            --(pContainer->index);
            XDBG_DEBUG (MDRV,
                        "DList %p find and erase data %p in storage %p index %d\n",
                        container, data, storage, pContainer->index);
            xorg_list_del (&storage->link);
            ctrl_free (storage);
            return TRUE;
        }
    }
    XDBG_DEBUG (MDRV, "DList %p DON'T erase data %p in storage %p index %d\n", container,
                data, storage, pContainer->index);
    return FALSE;
}

int
exynosUtilStorageGetAll (pointer container, pointer* pData, int size)
{
    /*\deprecated */
    XDBG_RETURN_VAL_IF_FAIL (container != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (pData != NULL, FALSE);
    EXYNOSDLinkedList *pContainer = (EXYNOSDLinkedList *) container;
    int i = 0;
    if (xorg_list_is_empty (&pContainer->list_of_storage))
        return TRUE;
    EXYNOSDLinkedListData *storage = NULL, *next = NULL;

    xorg_list_for_each_entry_safe (storage, next, &pContainer->list_of_storage, link)
    {
        if (i >= size)
            XDBG_RETURN_VAL_IF_FAIL (pData != NULL, FALSE);
        pData[i] = storage->data;
        ++i;
    }
    return TRUE;
}

pointer
exynosUtilStorageGetFirst (pointer container)
{
    XDBG_RETURN_VAL_IF_FAIL (container != NULL, NULL);
    EXYNOSDLinkedList *pContainer = (EXYNOSDLinkedList *) container;
    EXYNOSDLinkedListData *storage = NULL;
    if (xorg_list_is_empty (&pContainer->list_of_storage))
        return NULL;
    storage =
            xorg_list_first_entry (&pContainer->list_of_storage, EXYNOSDLinkedListData, link);
    XDBG_DEBUG (MDRV, "DList %p Get First %p\n", container, storage->data);
    return storage->data;
}

pointer
exynosUtilStorageGetLast (pointer container)
{
    XDBG_RETURN_VAL_IF_FAIL (container != NULL, NULL);
    EXYNOSDLinkedList *pContainer = (EXYNOSDLinkedList *) container;
    EXYNOSDLinkedListData *storage = NULL;
    if (xorg_list_is_empty (&pContainer->list_of_storage))
        return NULL;
    storage =
            xorg_list_last_entry (&pContainer->list_of_storage, EXYNOSDLinkedListData, link);
    XDBG_DEBUG (MDRV, "DList %p Get Last %p\n", container, storage->data);
    return storage->data;
}

void
exynosUtilStorageFinit (pointer container)
{
    if (container == NULL)
        return;
    EXYNOSDLinkedList *pContainer = (EXYNOSDLinkedList *) container;
    EXYNOSDLinkedListData *storage = NULL, *next = NULL;

    xorg_list_for_each_entry_safe (storage, next, &pContainer->list_of_storage,
                                   link)
    {
        xorg_list_del (&storage->link);
        ctrl_free (storage);
    }
    XDBG_DEBUG (MDRV, "DList %p erase all data\n", container);
    ctrl_free (pContainer);
}


Bool
exynosUtilStorageIsEmpty (pointer container)
{
/** \deprecated */
    if (container == NULL)
        return TRUE;
    EXYNOSDLinkedList *pContainer = (EXYNOSDLinkedList *) container;
    if (xorg_list_is_empty (&pContainer->list_of_storage))
    {
        XDBG_DEBUG (MDRV, "Dlist %p is empty\n", container);
        return TRUE;
    }
    return FALSE;
}

int
exynosUtilStorageGetSize (pointer container)
{
    if (container == NULL)
        return 0;
    EXYNOSDLinkedList *pContainer = (EXYNOSDLinkedList *) container;
    if (xorg_list_is_empty (&pContainer->list_of_storage))
    {
        XDBG_DEBUG (MDRV, "Dlist %p is empty\n", container);
        return 0;
    }
    return pContainer->index;
}

pointer
exynosUtilStorageGetData (pointer container, int index)
{
    XDBG_RETURN_VAL_IF_FAIL (container != NULL, NULL);
    XDBG_RETURN_VAL_IF_FAIL (index >= 0, NULL);
    EXYNOSDLinkedList *pContainer = (EXYNOSDLinkedList *) container;
    int i = 0;
    if (xorg_list_is_empty (&pContainer->list_of_storage))
        return NULL;
    XDBG_RETURN_VAL_IF_FAIL (index < pContainer->index, NULL);
    EXYNOSDLinkedListData *storage = NULL, *next = NULL;
    if (index == 0)
    {
        storage = xorg_list_last_entry (&pContainer->list_of_storage, EXYNOSDLinkedListData, link);

    }
    else if (index == (pContainer->index - 1))
    {
        storage = xorg_list_first_entry (&pContainer->list_of_storage, EXYNOSDLinkedListData, link);
    }
    else
    {

        xorg_list_for_each_entry_safe (storage, next, &pContainer->list_of_storage, link)
        {
            if (i == index)
                return storage->data;

            ++i;
        }
        return NULL;
    }

    return storage->data;
}

void
exynosUtilRotateRect (int xres,
                      int yres,
                      int src_rot,
                      int dst_rot,
                      xRectangle *rect)
{
    int diff;
    xRectangle temp;

    XDBG_RETURN_IF_FAIL (rect != NULL);

    if (src_rot == dst_rot)
        return;

    diff = (dst_rot - src_rot);
    if (diff < 0)
        diff = 360 + diff;

    if (src_rot % 180)
        SWAP (xres, yres);

    switch (diff)
    {
    case 270:
        temp.x = yres - (rect->y + rect->height);
        temp.y = rect->x;
        temp.width = rect->height;
        temp.height = rect->width;
        break;
    case 180:
        temp.x = xres - (rect->x + rect->width);
        temp.y = yres - (rect->y + rect->height);
        temp.width = rect->width;
        temp.height = rect->height;
        break;
    case 90:
        temp.x = rect->y;
        temp.y = xres - (rect->x + rect->width);
        temp.width = rect->height;
        temp.height = rect->width;
        break;
    default:
        temp.x = rect->x;
        temp.y = rect->y;
        temp.width = rect->width;
        temp.height = rect->height;
        break;
    }

    *rect = temp;
}

void
exynosUtilPlaneDump (ScrnInfoPtr pScrn)
{
    XDBG_RETURN_IF_FAIL (pScrn != NULL);
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    XDBG_RETURN_IF_FAIL (pExynos != NULL);
    EXYNOSDispInfoPtr pDispInfo = pExynos->pDispInfo;
    XDBG_RETURN_IF_FAIL (pDispInfo != NULL);
    drmModePlanePtr mode_plane = NULL;
    int i = 0;
    while (i < pDispInfo->plane_res->count_planes)

    {
        mode_plane = drmModeGetPlane (
                                      pDispInfo->drm_fd, pDispInfo->plane_res->planes[i]);
        if (mode_plane);
        {
            XDBG_TRACE (MTRC, "plane_id (%d) , crtc_id (%d), fb_id (%d)\n", mode_plane->plane_id,
              mode_plane->crtc_id, mode_plane->fb_id);
            drmModeFreePlane (mode_plane);
        }
        ++i;
    }
}

//attenshin!!!! this is hack
//It is part of private structure from tbmlib
struct _tbm_bo
{
    tbm_bufmgr bufmgr; /**< tbm buffer manager */
    int ref_cnt;       /**< ref count of bo */
};
int exynosUtilGetTboRef(tbm_bo bo)
{
    if(bo != NULL)
        return  ((struct _tbm_bo *)bo)->ref_cnt;
    else
        return -1;
}

