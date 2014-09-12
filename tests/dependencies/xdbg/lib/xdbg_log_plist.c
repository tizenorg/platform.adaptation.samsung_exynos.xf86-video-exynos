/**************************************************************************

xdbg

Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved

Contact: SooChan Lim <sc1.lim@samsung.com>
         Sangjin LEE <lsj119@samsung.com>

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

#include <stdio.h>
#include "xorg-server.h"
#include "xf86.h"
#include <scrnintstr.h>
#include <resource.h>
#include <windowstr.h>
#include <pixmap.h>
#include <list.h>
#include "xdbg.h"
#include "xdbg_log_plist.h"

/* for debug message */
#define MMEM    XDBG_M('M','E','M',0)

#ifndef API
#define API __attribute__ ((visibility("default")))
#endif

/*=========================================================================*/
/* trace the usage of the pixmaps in xserver                               */
/*=========================================================================*/

#define CLIENT_BITS(id) ((id) & RESOURCE_CLIENT_MASK)
#define CLIENT_ID(id) ((int)(CLIENT_BITS(id) >> CLIENTOFFSET))
#define MAX_HISTORY  10

typedef struct {
    PixmapPtr pPixmap;
    unsigned int size;
    unsigned int refs;
    char *hint;
    XID refHistorys[MAX_HISTORY];
    int numHistory;
    struct xorg_list link;
} XDbgPixmap;

typedef struct {
    PixmapPtr pPixmap;
    struct xorg_list link;
} XDbgRefPixmap;

typedef struct {
    DrawablePtr pDraw;
    XDbgRefPixmap *pRefPixmap;
    struct xorg_list link;
    struct xorg_list refPixmaps;
} XDbgDrawable;

static struct xorg_list xdbgPixmaps;
static struct xorg_list xdbgDrawables;
static PixmapPtr pPixRoot = NULL;
unsigned int total_size = 0;
unsigned int peek_size = 0;

const struct {
    unsigned int hint;
    char* str;
} pixmap_hint[] = {
    {CREATE_PIXMAP_USAGE_SCRATCH,        "scratch"},
    {CREATE_PIXMAP_USAGE_BACKING_PIXMAP, "backing_pixmap"},
    {CREATE_PIXMAP_USAGE_GLYPH_PICTURE,  "glyph_picture"},
    {CREATE_PIXMAP_USAGE_SHARED,         "shared"},
    {CREATE_PIXMAP_USAGE_OVERLAY,        "overlay"},
    {CREATE_PIXMAP_USAGE_DRI2_FLIP_BACK, "dri2_flip_back"},
    {CREATE_PIXMAP_USAGE_FB,             "fb"},
    {CREATE_PIXMAP_USAGE_SUB_FB,         "sub_fb"},
    {CREATE_PIXMAP_USAGE_DRI2_BACK,      "dri2_back"},
    /******END********/
    {0, "normal"}
};

static char *
_get_pixmap_hint_name (signed int usage_hint)
{
    int i = 0;

    while (pixmap_hint[i].hint)
    {
        if (pixmap_hint[i].hint == usage_hint)
            return pixmap_hint[i].str;
        i++;
    }
    return NULL;
}

static XDbgPixmap *
_findXDbgPixmap (PixmapPtr pPixmap)
{
    XDbgPixmap *cur = NULL, *tmp = NULL;

    xorg_list_for_each_entry_safe (cur, tmp, &xdbgPixmaps, link)
    {
        if (cur->pPixmap == pPixmap)
            return cur;
    }

    return NULL;
}

static XDbgDrawable *
_findXDbgDrawable (DrawablePtr pDraw)
{
    XDbgDrawable *cur = NULL, *tmp = NULL;

    xorg_list_for_each_entry_safe (cur, tmp, &xdbgDrawables, link)
    {
        if (cur->pDraw == pDraw)
            return cur;
    }

    return NULL;
}

static XDbgRefPixmap*
_findXDbgRefPixmap (XDbgDrawable* pXDbgDrawable, PixmapPtr pPixmap)
{
    XDbgRefPixmap *cur = NULL, *tmp = NULL;

    xorg_list_for_each_entry_safe (cur, tmp, &pXDbgDrawable->refPixmaps, link)
    {
        if (cur->pPixmap == pPixmap)
            return cur;
    }

    return NULL;
}

static void
_addXDbgPixmap (PixmapPtr pPixmap)
{
    XDbgPixmap *cur = NULL;
    unsigned int size;

    cur = _findXDbgPixmap (pPixmap);
    if (cur)
    {
        size = pPixmap->devKind * pPixmap->drawable.height;
        if (size == cur->size)
            return;

        XDBG_TRACE (MMEM, " Change pixmap(%p) size(%d -> %d)\n",
                    cur->pPixmap, cur->size, size);

        total_size -= cur->size;
        cur->size = size;
        cur->hint = _get_pixmap_hint_name (pPixmap->usage_hint);
    }
    else
    {
        cur = calloc (1, sizeof (XDbgPixmap));
        cur->pPixmap = pPixmap;
        cur->size = pPixmap->devKind*pPixmap->drawable.height;
        cur->hint = _get_pixmap_hint_name (pPixmap->usage_hint);
        xorg_list_add (&cur->link, &xdbgPixmaps);
    }

    /* caculate the total_size of pixmaps */
    total_size += cur->size;
    if (total_size > peek_size)
        peek_size = total_size;

    if (pPixmap->usage_hint == CREATE_PIXMAP_USAGE_FB)
        pPixRoot = pPixmap;

    XDBG_TRACE (MMEM, "Add pixmap(%p) size:%d, hint:%s\n",
                cur->pPixmap, cur->size, cur->hint);
}

static void
_removeXDbgPixmap (PixmapPtr pPixmap)
{
    XDbgPixmap *cur = NULL;

    cur = _findXDbgPixmap (pPixmap);
    if (!cur)
    {
        XDBG_WARNING (MMEM, "Unknown pixmap XID:0x%x, pPix:%p\n",
                      (unsigned int)pPixmap->drawable.id, pPixmap);
        return;
    }

    if (cur->refs > 0)
        XDBG_TRACE (MMEM,"Pixmap(%p) refs:%d\n", cur->pPixmap, cur->refs);

    /* caculate the total_size of pixmaps */
    total_size -= cur->size;

    XDBG_TRACE (MMEM, " Remove pixmap(%p) size:%d, hint:%s\n",
                cur->pPixmap, cur->size, cur->hint);

    xorg_list_del(&cur->link);
    free(cur);
}

#if 0
static void
_dumpDraws (char *reply, int *len)
{
    XDbgDrawable *cur = NULL, *tmp = NULL;
    XDbgRefPixmap *p = NULL, *ptmp = NULL;
    XDbgPixmap *dp = NULL;

    xorg_list_for_each_entry_safe (cur, tmp, &xdbgDrawables, link)
    {
        XDBG_REPLY ("[%d] XID:0x%x type:%s %dx%d+%d+%d\n",
                                CLIENT_ID(cur->pDraw->id),
                                (unsigned int)cur->pDraw->id,
                                (cur->pDraw->type == DRAWABLE_WINDOW ? "window":"pixmap"),
                                cur->pDraw->width, cur->pDraw->height, cur->pDraw->x, cur->pDraw->y);

        xorg_list_for_each_entry_safe (p, ptmp, &cur->refPixmaps, link)
        {
            dp = _findXDbgPixmap (p->pPixmap);
            if(!dp)
            {
                XDBG_REPLY ("\t***[REF_Pixmap] unknown pixmap(%p)\n", p->pPixmap);
                continue;
            }

            XDBG_REPLY ("\t[REF_Pixmap] %p, hint:%s, size:%d\n",
                                     dp->pPixmap, dp->hint, (unsigned int)dp->size/1024);
        }
    }
}

static void
_dumpPixmaps (char *reply, int *len)
{
    XDbgPixmap *cur = NULL, *tmp = NULL;
    int client_id;
    int i;

    if (pPixRoot)
    {
        cur = _findXDbgPixmap (pPixRoot);
        XDBG_RETURN_IF_FAIL (cur != NULL);
        XDBG_REPLY ("ROOT_Pixmap XID:0x%x pixmap(%p) hint:%s(0x%x) size:%d\n",
                                (unsigned int)cur->pPixmap->drawable.id, pPixRoot,
                                cur->hint, cur->pPixmap->usage_hint,
                                (unsigned int)cur->size/1024);
    }

    xorg_list_for_each_entry_safe (cur, tmp, &xdbgPixmaps, link)
    {
        if (cur->pPixmap == pPixRoot)
            continue;

        if (cur->pPixmap->drawable.id || cur->refs == 0)
        {
            client_id = CLIENT_ID(cur->pPixmap->drawable.id);
            if (cur->pPixmap->drawable.id)
            {
                XDBG_REPLY ("[%d] XID:0x%x %dx%d hint:%s(0x%x) size:%d refs:%d\n",
                                        client_id, (unsigned int)cur->pPixmap->drawable.id,
                                        cur->pPixmap->drawable.width, cur->pPixmap->drawable.height,
                                        cur->hint, cur->pPixmap->usage_hint,
                                        (unsigned int)cur->size/1024, cur->refs);
            }
            else
            {
                XDBG_REPLY ("[%d] Pixmap:%p %dx%d hint:%s(0x%x) size:%d refs:%d\n",
                                        client_id, cur->pPixmap,
                                        cur->pPixmap->drawable.width, cur->pPixmap->drawable.height,
                                        cur->hint, cur->pPixmap->usage_hint,
                                        (unsigned int)cur->size/1024, cur->refs);
            }

            if (cur->numHistory)
            {
                XDBG_REPLY ("\t[RefHistory] ");
                for (i = 0; i < cur->numHistory; i++)
                {
                    XDBG_REPLY ("0x%x ", (unsigned int)cur->refHistorys[i]);
                }
                XDBG_REPLY ("\n");
            }
        }
    }
}
#endif

CreatePixmapProcPtr fnCreatePixmap;
DestroyPixmapProcPtr fnDestroyPixmap;
ModifyPixmapHeaderProcPtr fnModifyPixmap;
SetWindowPixmapProcPtr fnSetWindowPixmap;
DestroyWindowProcPtr fnDestroyWindow;

static PixmapPtr
XDbgLogCreatePixmap (ScreenPtr pScreen, int width, int height,
                        int depth, unsigned usage_hint)
{
    PixmapPtr pPixmap = NULL;

    pScreen->CreatePixmap = fnCreatePixmap;
    pPixmap = pScreen->CreatePixmap(pScreen, width, height, depth, usage_hint);
    pScreen->CreatePixmap = XDbgLogCreatePixmap;

    if(pPixmap)
        _addXDbgPixmap (pPixmap);

    return pPixmap;
}

static Bool
XDbgLogModifyPixmapHeader (PixmapPtr pPixmap, int width, int height,
                              int depth, int bitsPerPixel, int devKind, pointer pPixData)
{
    Bool ret;
    ScreenPtr pScreen = pPixmap->drawable.pScreen;

    pScreen->ModifyPixmapHeader = fnModifyPixmap;
    ret = pScreen->ModifyPixmapHeader (pPixmap, width, height,
                                       depth, bitsPerPixel, devKind, pPixData);
    pScreen->ModifyPixmapHeader = XDbgLogModifyPixmapHeader;

    _addXDbgPixmap (pPixmap);

    return ret;
}

static Bool
XDbgLogDestroyPixmap (PixmapPtr pPixmap)
{
    Bool ret;
    ScreenPtr pScreen = pPixmap->drawable.pScreen;

    pScreen->DestroyPixmap = fnDestroyPixmap;
    ret = pScreen->DestroyPixmap(pPixmap);
    pScreen->DestroyPixmap = XDbgLogDestroyPixmap;

    if (pPixmap->refcnt == 0)
        _removeXDbgPixmap (pPixmap);

    return ret;
}

static void
XDbgLogSetWindowPixmap (WindowPtr pWin, PixmapPtr pPixmap)
{
    ScreenPtr pScreen = (ScreenPtr) pWin->drawable.pScreen;
    WindowPtr pParent = pWin->parent;
    XDbgDrawable *d = NULL;
    XDbgPixmap *p = NULL;
    XDbgRefPixmap *p_ref = NULL;

    pScreen->SetWindowPixmap = fnSetWindowPixmap;
    pScreen->SetWindowPixmap (pWin, pPixmap);
    pScreen->SetWindowPixmap = XDbgLogSetWindowPixmap;

    if (pPixmap != pScreen->GetWindowPixmap(pParent))
    {
        //Add to window list
        p = _findXDbgPixmap (pPixmap);
        if (!p)
        {
            XDBG_WARNING (MMEM, "Unknown pixmap(%p) hint:%s\n",
                          pPixmap, _get_pixmap_hint_name(pPixmap->usage_hint));
            return;
        }

        d = _findXDbgDrawable (&pWin->drawable);
        if (!d)
        {
            d = calloc (1, sizeof(XDbgDrawable));
            d->pDraw = &pWin->drawable;
            xorg_list_init (&d->refPixmaps);
            xorg_list_add (&d->link, &xdbgDrawables);
            XDBG_TRACE (MMEM, " Add window(0x%x)\n", (unsigned int)pWin->drawable.id);
        }

        if (d->pRefPixmap)
        {
            XDBG_TRACE (MMEM, " Unset WinPixmap win(0x%x), pixmap(%p) hint:%s\n",
                                   (unsigned int)pWin->drawable.id, p->pPixmap, p->hint);
            xorg_list_del (&d->pRefPixmap->link);
            free (d->pRefPixmap);
            d->pRefPixmap = NULL;
        }

        p_ref = calloc (1, sizeof(XDbgRefPixmap));
        p_ref->pPixmap = pPixmap;
        xorg_list_init (&p_ref->link);
        xorg_list_add (&p_ref->link, &d->refPixmaps);
        d->pRefPixmap = p_ref;

        p->refs++;
        p->refHistorys[p->numHistory++] = pWin->drawable.id;

        XDBG_TRACE (MMEM, " Set WinPixmap win(0x%x), pixmap(%p) hint:%s\n",
                            (unsigned int)pWin->drawable.id, p->pPixmap, p->hint);
    }
    else
    {
        //find window
        d = _findXDbgDrawable (&pWin->drawable);

        //remove window
        if (d && d->pRefPixmap)
        {
            p = _findXDbgPixmap (d->pRefPixmap->pPixmap);
            if (p)
            {
                if (p->refs > 0)
                    p->refs--;
                else
                    XDBG_WARNING (MMEM, "pixmap(%p), refs(%d)\n",
                                  __FUNCTION__, __LINE__, p->pPixmap, p->refs);
            }

            XDBG_TRACE (MMEM,"Unset WinPixmap win(0x%x): pixmap(%p) to NULL\n",
                        (unsigned int)pWin->drawable.id, d->pRefPixmap->pPixmap);

            xorg_list_del (&d->pRefPixmap->link);
            free (d->pRefPixmap);
            d->pRefPixmap = NULL;

            if (xorg_list_is_empty (&d->refPixmaps))
            {
                xorg_list_del (&d->link);
                free(d);
            }
        }
    }
}

static Bool
XDbgLogDestroyWindow (WindowPtr pWindow)
{
    Bool ret;
    ScreenPtr pScreen = pWindow->drawable.pScreen;
    XDbgDrawable *d = NULL;
    XDbgPixmap *p = NULL;
    XDbgRefPixmap *pos = NULL, *tmp = NULL;

    pScreen->DestroyWindow = fnDestroyWindow;
    ret = pScreen->DestroyWindow (pWindow);
    pScreen->DestroyWindow = XDbgLogDestroyWindow;

    d = _findXDbgDrawable (&pWindow->drawable);
    if (d)
    {
        XDBG_TRACE (MMEM, "Remove drawable(0x%x)\n",
                    (unsigned int)pWindow->drawable.id);

        xorg_list_for_each_entry_safe (pos, tmp, &d->refPixmaps, link)
        {
            p = _findXDbgPixmap (pos->pPixmap);
            if(p)
                p->refs--;

            XDBG_TRACE (MMEM, "Remove ref_pixmap(%p), dbgPixmap(%p)\n",
                        pos->pPixmap, p);

            xorg_list_del (&pos->link);
            free (pos);
        }

        xorg_list_del (&d->link);
        free (d);
    }

    return ret;
}

API void
xDbgLogPListInit (ScreenPtr pScreen)
{
    xorg_list_init (&xdbgPixmaps);
    xorg_list_init (&xdbgDrawables);

    fnSetWindowPixmap = pScreen->SetWindowPixmap;
    fnDestroyWindow = pScreen->DestroyWindow;
    fnCreatePixmap = pScreen->CreatePixmap;
    fnModifyPixmap = pScreen->ModifyPixmapHeader;
    fnDestroyPixmap = pScreen->DestroyPixmap;

    pScreen->CreatePixmap = XDbgLogCreatePixmap;
    pScreen->DestroyPixmap = XDbgLogDestroyPixmap;
    pScreen->ModifyPixmapHeader = XDbgLogModifyPixmapHeader;
    pScreen->SetWindowPixmap = XDbgLogSetWindowPixmap;
    pScreen->DestroyWindow = XDbgLogDestroyWindow;
}

API void
xDbgLogPListDeinit (ScreenPtr pScreen)
{}


API void
xDbgLogPListDrawAddRefPixmap (DrawablePtr pDraw, PixmapPtr pRefPixmap)
{
    XDbgDrawable *d = NULL;
    XDbgPixmap *p = NULL;
    XDbgRefPixmap *p_ref = NULL;

    XDBG_RETURN_IF_FAIL (pDraw != NULL);
    XDBG_RETURN_IF_FAIL (pRefPixmap != NULL);

    d = _findXDbgDrawable (pDraw);
    p = _findXDbgPixmap (pRefPixmap);
    if(!p)
    {
        XDBG_WARNING (MMEM, "%s:%d : Unknown pixmap XID:0x%x, pPix:%p\n",
                      __FUNCTION__, __LINE__,
                      (unsigned int)pRefPixmap->drawable.id, pRefPixmap);
        return;
    }

    if (!d)
    {
        d = calloc (1, sizeof(XDbgDrawable));
        d->pDraw = pDraw;
        xorg_list_init (&d->refPixmaps);
        xorg_list_add (&d->link, &xdbgDrawables);

        XDBG_TRACE (MMEM, " Add window(0x%x)\n", (unsigned int)pDraw->id);
    }

    p_ref =_findXDbgRefPixmap (d, pRefPixmap);
    if(p_ref)
        return;

    p_ref = calloc (1, sizeof(XDbgRefPixmap));
    p_ref->pPixmap = pRefPixmap;
    xorg_list_init (&p_ref->link);
    xorg_list_add (&p_ref->link, &d->refPixmaps);

    p->refs++;
    if (p->numHistory < (MAX_HISTORY-1))
        p->refHistorys[p->numHistory++] = pDraw->id;

    if (pDraw->type == DRAWABLE_WINDOW)
        XDBG_TRACE (MMEM, " Add RefPixmap win(0x%x), pixmap(%p) hint:%s\n",
                    (unsigned int)pDraw->id, p->pPixmap, p->hint);
    else
        XDBG_TRACE (MMEM, " Add RefPixmap pix(0x%x), pixmap(%p) hint:%s\n",
                    (unsigned int)pDraw->id, p->pPixmap, p->hint);
}

API void
xDbgLogPListDrawRemoveRefPixmap (DrawablePtr pDraw, PixmapPtr pRefPixmap)
{
    XDbgDrawable *d = NULL;
    XDbgRefPixmap *p_ref = NULL;
    XDbgPixmap *p = NULL;

    p = _findXDbgPixmap (pRefPixmap);
    if (pDraw == NULL)
    {
        if (p && p->refs > 0)
        {
            XDBG_ERROR (MMEM, "Error:%s:%d null draw pixmap(%p)\n",
                        __FUNCTION__, __LINE__, pRefPixmap);
        }
        return;
    }

    d = _findXDbgDrawable (pDraw);
    if (!d)
    {
        XDBG_WARNING (MMEM, "%s:%d : Unknown drawable XID:0x%x, pPix:%p\n",
                      __FUNCTION__, __LINE__, (unsigned int)pDraw->id, pRefPixmap);
        return;
    }

    p_ref = _findXDbgRefPixmap (d, pRefPixmap);
    if(!p_ref)
    {
        XDBG_WARNING (MMEM, "%s:%d : Unknown refpixmap XID:0x%x, pPix:%p\n",
                                  __FUNCTION__, __LINE__, (unsigned int)pDraw->id, pRefPixmap);
        return;
    }

    xorg_list_del (&p_ref->link);
    free (p_ref);
    if (p)
        p->refs--;

    if (xorg_list_is_empty (&d->refPixmaps))
    {
        xorg_list_del(&d->link);
        free(d);
    }

    if (pDraw->type == DRAWABLE_WINDOW)
        XDBG_TRACE (MMEM, " Remove RefPixmap win(0x%x), pixmap(%p) hint:%s\n",
                                (unsigned int)pDraw->id, pRefPixmap,
                                _get_pixmap_hint_name(pRefPixmap->usage_hint));
    else
        XDBG_TRACE (MMEM, " Remove RefPixmap pix(0x%x), pixmap(%p) hint:%s\n",
                                (unsigned int)pDraw->id, pRefPixmap,
                                _get_pixmap_hint_name(pRefPixmap->usage_hint));
}


API void
xDbgLogPList (char *reply, int *len)
{
    XDBG_REPLY ("\n\n====================================\n");
    XDBG_REPLY ("    Total:%d, Peek:%d\n", (unsigned int)total_size/1024, (unsigned int)peek_size/1024);
    XDBG_REPLY ( "====================================\n");

    XDBG_REPLY ("== WINDOWS ==\n");
    XDbgDrawable *dd = NULL, *ddtmp = NULL;
    XDbgRefPixmap *rp = NULL, *rptmp = NULL;
    XDbgPixmap *dp = NULL;

    xorg_list_for_each_entry_safe (dd, ddtmp, &xdbgDrawables, link)
    {
        XDBG_REPLY ("[%d] XID:0x%x type:%s %dx%d+%d+%d\n",
                                CLIENT_ID(dd->pDraw->id),
                                (unsigned int)dd->pDraw->id,
                                (dd->pDraw->type == DRAWABLE_WINDOW ? "window":"pixmap"),
                                dd->pDraw->width, dd->pDraw->height, dd->pDraw->x, dd->pDraw->y);

        xorg_list_for_each_entry_safe (rp, rptmp, &dd->refPixmaps, link)
        {
            dp = _findXDbgPixmap (rp->pPixmap);
            if(!dp)
            {
                XDBG_REPLY ("\t***[REF_Pixmap] unknown pixmap(%p)\n", rp->pPixmap);
                continue;
            }

            XDBG_REPLY ("\t[REF_Pixmap] %p, hint:%s, size:%d\n",
                                     dp->pPixmap, dp->hint, (unsigned int)dp->size/1024);
        }
    }
    XDBG_REPLY ("\n");


    XDBG_REPLY ( "== PIXMAPS ==\n");
    XDbgPixmap *cur = NULL, *tmp = NULL;
    int client_id;
    int i;

    if (pPixRoot)
    {
        cur = _findXDbgPixmap (pPixRoot);
        XDBG_RETURN_IF_FAIL (cur != NULL);
        XDBG_REPLY ("ROOT_Pixmap XID:0x%x pixmap(%p) hint:%s(0x%x) size:%d\n",
                                (unsigned int)cur->pPixmap->drawable.id, pPixRoot,
                                cur->hint, cur->pPixmap->usage_hint,
                                (unsigned int)cur->size/1024);
    }

    xorg_list_for_each_entry_safe (cur, tmp, &xdbgPixmaps, link)
    {
        if (cur->pPixmap == pPixRoot)
            continue;

        if (cur->pPixmap->drawable.id || cur->refs == 0)
        {
            client_id = CLIENT_ID(cur->pPixmap->drawable.id);
            if (cur->pPixmap->drawable.id)
            {
                XDBG_REPLY ("[%d] XID:0x%x %dx%d hint:%s(0x%x) size:%d refs:%d\n",
                                        client_id, (unsigned int)cur->pPixmap->drawable.id,
                                        cur->pPixmap->drawable.width, cur->pPixmap->drawable.height,
                                        cur->hint, cur->pPixmap->usage_hint,
                                        (unsigned int)cur->size/1024, cur->refs);
            }
            else
            {
                XDBG_REPLY ("[%d] Pixmap:%p %dx%d hint:%s(0x%x) size:%d refs:%d\n",
                                        client_id, cur->pPixmap,
                                        cur->pPixmap->drawable.width, cur->pPixmap->drawable.height,
                                        cur->hint, cur->pPixmap->usage_hint,
                                        (unsigned int)cur->size/1024, cur->refs);
            }

            if (cur->numHistory)
            {
                XDBG_REPLY ("\t[RefHistory] ");
                for (i = 0; i < cur->numHistory; i++)
                {
                    XDBG_REPLY ("0x%x ", (unsigned int)cur->refHistorys[i]);
                }
                XDBG_REPLY ("\n");
            }
        }
    }
    XDBG_REPLY ("\n");
}

