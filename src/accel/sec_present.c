/*
 *
 * xserver-xorg-video-sec
 *
 * Copyright © 2013 Keith Packard
 * Copyright 2010 - 2014 Samsung Electronics co., Ltd. All Rights Reserved.
 *
 * Contact: Roman Marchenko <r.marchenko@samsung.com>
 * Contact: Roman Peresipkyn<r.peresipkyn@samsung.com>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <tbm_bufmgr.h>

#include "xorg-server.h"
#include "xf86.h"
#include "xf86drm.h"

#include "windowstr.h"

#include "present.h"

#include "sec.h"
#include "sec_accel.h"
#include "sec_display.h"
#include "sec_crtc.h"

typedef enum __dri3_copy_type
{
    dri3_copy_xserver = 0,
    dri3_copy_swap_bo,
    dri3_copy_swap_pix
} e_dri3_copy_type;


static int SECPresentGetUstMsc(RRCrtcPtr pRRcrtc, CARD64 *ust, CARD64 *msc);

/*-------------------------- Private structures -----------------------------*/
typedef struct _presentVblankEvent {

	uint64_t event_id;

	RRCrtcPtr pRRcrtc;	//jast for info

} PresentVblankEventRec, *PresentVblankEventPtr;

/*-------------------------- Private functions -----------------------------*/


/*-------------------------- Public functions -----------------------------*/
/*
 * Called when the queued vblank event has occurred
 */
void
secPresentVblankHandler(unsigned int frame, unsigned int tv_sec,
        				unsigned int tv_usec, void *event_data)
{
	uint64_t usec = (uint64_t) tv_sec * 1000000 + tv_usec;

	PresentVblankEventRec *pEvent = event_data;

    XDBG_DEBUG(MDRI3, "event_id %lld ust:%lld msc:%lld\n", pEvent->event_id, usec, frame);
	present_event_notify(pEvent->event_id, usec, frame);
    free(pEvent);
}

/*
 * Called when the queued vblank is aborted
 */
void
secPresentVblankAbort(ScrnInfoPtr pScrn, xf86CrtcPtr pCrtc, void *data)
{
	PresentVblankEventRec *pEvent = data;
    free(pEvent);
}

/*
 * Once the flip has been completed on all pipes, notify the
 * extension code telling it when that happened
 */

void
secPresentFlipEventHandler(unsigned int frame, unsigned int tv_sec,
                         unsigned int tv_usec, void *event_data, Bool flip_failed)
{

	PresentVblankEventRec *pEvent = event_data;
	uint64_t ust = (uint64_t) tv_sec * 1000000 + tv_usec;
	uint64_t msc = (uint64_t)frame;
    if ( msc == 0 )
    {
        uint64_t tmp_ust;
        SECPresentGetUstMsc(pEvent->pRRcrtc, &tmp_ust, &msc);
    }
	XDBG_DEBUG(MDRI3, "event_id %lld ust:%lld msc:%lld(%d)\n", pEvent->event_id, ust, msc, frame);
	present_event_notify(pEvent->event_id, ust, msc);
	free(pEvent);
}

/*
 * The flip has been aborted, free the structure
 */
void
secPresentFlipAbort(void *pageflip_data)
{
	PresentVblankEventRec *pEvent = pageflip_data;
	free(pEvent);
}

/*-------------------------- Callback functions -----------------------------*/
static RRCrtcPtr
SECPresentGetCrtc(WindowPtr pWindow)
{
	XDBG_RETURN_VAL_IF_FAIL (pWindow != NULL, NULL);

	ScreenPtr pScreen = pWindow->drawable.pScreen;
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	BoxRec box, crtcbox;
	xf86CrtcPtr pCrtc = NULL;
	RRCrtcPtr pRandrCrtc = NULL;

	box.x1 = pWindow->drawable.x;
	box.y1 = pWindow->drawable.y;
	box.x2 = box.x1 + pWindow->drawable.width;
	box.y2 = box.y1 + pWindow->drawable.height;

	pCrtc = secModeCoveringCrtc( pScrn, &box, NULL, &crtcbox );

	/* Make sure the CRTC is valid and this is the real front buffer */
	if (pCrtc != NULL && !pCrtc->rotatedData ) //TODO what is pCrtc->rotatedData pointing on?
			pRandrCrtc = pCrtc->randr_crtc;

	XDBG_DEBUG(MDRI3, "%s\n", pRandrCrtc ?  "OK" : "ERROR");

	return pRandrCrtc;
}

/*
 * The kernel sometimes reports bogus MSC values, especially when
 * suspending and resuming the machine. Deal with this by tracking an
 * offset to ensure that the MSC seen by applications increases
 * monotonically, and at a reasonable pace.
 */
static int
SECPresentGetUstMsc(RRCrtcPtr pRRcrtc, CARD64 *ust, CARD64 *msc)
{
	XDBG_RETURN_VAL_IF_FAIL (pRRcrtc != NULL, 0);

	xf86CrtcPtr pCrtc = pRRcrtc->devPrivate;
	ScreenPtr pScreen = pRRcrtc->pScreen;
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);

	SECCrtcPrivPtr pCrtcPriv = pCrtc->driver_private;

	//Get the current msc/ust value from the kernel
	Bool ret = secDisplayGetCurMSC(pScrn, pCrtcPriv->pipe, ust, msc);

	XDBG_DEBUG(MDRI3, "%s: pipe:%d ust:%lld msc:%lld\n",
			(ret?"OK":"ERROR"), pCrtcPriv->pipe, *ust, *msc);
	return (int)ret;
}

static int
SECPresentQueueVblank(RRCrtcPtr 		pRRcrtc,
						 uint64_t 		event_id,
						 uint64_t 		msc)
{
						 xf86CrtcPtr	pCrtc 			= pRRcrtc->devPrivate;
						 ScreenPtr 		pScreen 		= pRRcrtc->pScreen;
						 ScrnInfoPtr 	pScrn 			= xf86ScreenToScrn(pScreen);
						 int 			pipe 			= secModeGetCrtcPipe(pCrtc);
						 PresentVblankEventPtr pEvent 	= NULL;

	pEvent = calloc(sizeof(PresentVblankEventRec), 1);
	if (!pEvent) {
		XDBG_ERROR(MDRI3, "fail to Vblank: event_id %lld msc %llu \n", event_id, msc);
		return BadAlloc;
	}
	pEvent->event_id = event_id;
	pEvent->pRRcrtc = pRRcrtc;


	/* flip is 1 to avoid to set DRM_VBLANK_NEXTONMISS */
	if (!secDisplayVBlank(pScrn, pipe, &msc, 1, VBLANK_INFO_PRESENT,  pEvent)) {
		XDBG_WARNING(MDRI3, "fail to Vblank: event_id %lld msc %llu \n", event_id, msc);
		secPresentVblankAbort(pScrn, pCrtc, pEvent);
		return BadAlloc;
	}

	XDBG_DEBUG(MDRI3, "OK to Vblank event_id:%lld msc:%llu \n", event_id, msc);
	return Success;
}

static void
SECPresentAbortVblank(RRCrtcPtr pRRcrtc, uint64_t event_id, uint64_t msc)
{
	XDBG_INFO(MDRI3, "isn't implamentation\n");

}

static void
SECPresentFlush(WindowPtr window)
{
	XDBG_INFO(MDRI3, "isn't implamentation\n");
}

static Bool
SECPresentCheckFlip(RRCrtcPtr 	pRRcrtc,
					WindowPtr 	pWin,
		 	 	 	PixmapPtr 	pPixmap,
		 	 	 	Bool 		sync_flip)
{

    ScrnInfoPtr pScrn = xf86ScreenToScrn(pWin->drawable.pScreen);
	SECPixmapPriv *pExaPixPriv = exaGetPixmapDriverPrivate (pPixmap);

	if (pExaPixPriv->isFrameBuffer == FALSE)
    {
    	XDBG_RETURN_VAL_IF_FAIL (secSwapToRenderBo(pScrn, pWin->drawable.width, pWin->drawable.height, pExaPixPriv->bo), FALSE);

    	pExaPixPriv->owner = pWin->drawable.id;
        pExaPixPriv->isFrameBuffer = TRUE;
        pExaPixPriv->sbc = 0;
        pExaPixPriv->size = pPixmap->drawable.height * pPixmap->devKind;
    }
	return TRUE;
}

struct pixmap_visit {
    PixmapPtr old;
    PixmapPtr new;
};

int
present_set_tree_pixmap_visit(WindowPtr window, void *data)
{
    struct pixmap_visit *visit = data;
    ScreenPtr screen = window->drawable.pScreen;

    if ((*screen->GetWindowPixmap)(window) != visit->old)
        return WT_DONTWALKCHILDREN;
    (*screen->SetWindowPixmap)(window, visit->new);
    return WT_WALKCHILDREN;
}

#ifdef PRESENT_SWAP
static void
present_set_tree_pixmap(WindowPtr window, PixmapPtr pixmap)
{
    struct pixmap_visit visit;
    ScreenPtr screen = window->drawable.pScreen;

    visit.old = (*screen->GetWindowPixmap)(window);
    visit.new = pixmap;
    if (visit.old == visit.new)
        return;
    TraverseTree(window, present_set_tree_pixmap_visit, &visit);
}
#endif

#ifdef PRESENT_SWAP
static
Bool _useSwapingDRI3(SECPtr pSec)
{
    return pSec->dri3_copy_type != dri3_copy_xserver;
}
#endif

#ifdef PRESENT_SWAP
static Bool
SECPresentCopyRegion(DrawablePtr pDraw, PixmapPtr pBackPix,
        RegionPtr pUpdate, int16_t x_off, int16_t y_off)
{
    ScreenPtr pScreen = pDraw->pScreen;
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pDraw->pScreen);
    SECPtr pSec = SECPTR(pScrn);

    PixmapPtr pFrontPix = NULL;

    if (pDraw->type == DRAWABLE_WINDOW)
        pFrontPix = pScreen->GetWindowPixmap((WindowPtr) pDraw);
    else
        pFrontPix = (PixmapPtr) pDraw;

    SECPixmapPriv *pFrontExaPixPriv = exaGetPixmapDriverPrivate(pFrontPix);
    SECPixmapPriv *pBackExaPixPriv = exaGetPixmapDriverPrivate(pBackPix);

    //DEBUG
    {
        PixmapPtr pScrPix = pScreen->GetScreenPixmap(pScreen);
        SECPixmapPriv *pScrPixPriv = exaGetPixmapDriverPrivate(pScrPix);

        PixmapPtr pRootPix = pScreen->GetWindowPixmap(pScreen->root);
        SECPixmapPriv *pRootPixPriv = exaGetPixmapDriverPrivate(pRootPix);

        XDBG_DEBUG(MDRI3,"\n"
                        "\t\tfront: pix(sn:%ld p:%p ID:0x%x fb:%d hint:%d), bo(ptr:%p name:%d)\n"
                        "\t\tback : pix(sn:%ld p:%p ID:0x%x fb:%d hint:%d), bo(ptr:%p name:%d)\n"
                        "\t\tscrn : pix(sn:%ld p:%p ID:0x%x fb:%d hint:%d), bo(ptr:%p name:%d)\n"
                        "\t\troot : pix(sn:%ld p:%p ID:0x%x fb:%d hint:%d), bo(ptr:%p name:%d)\n",
                pFrontPix->drawable.serialNumber, pFrontPix, pFrontPix->drawable.id, pFrontExaPixPriv->isFrameBuffer, pFrontPix->usage_hint,
                pFrontExaPixPriv->bo, (pFrontExaPixPriv->bo ? tbm_bo_export(pFrontExaPixPriv->bo) : -1),
                pBackPix->drawable.serialNumber, pBackPix, pBackPix->drawable.id, pBackExaPixPriv->isFrameBuffer, pBackPix->usage_hint,
                pBackExaPixPriv->bo, (pBackExaPixPriv->bo ? tbm_bo_export(pBackExaPixPriv->bo) : -1),
                pScrPix->drawable.serialNumber, pScrPix, pScrPix->drawable.id, pScrPixPriv->isFrameBuffer, pScrPix->usage_hint,
                pScrPixPriv->bo,(pScrPixPriv->bo ? tbm_bo_export(pScrPixPriv->bo) : -1),
                pRootPix->drawable.serialNumber, pRootPix, pRootPix->drawable.id, pRootPixPriv->isFrameBuffer,
                pRootPix->usage_hint, pRootPixPriv->bo, (pRootPixPriv->bo ? tbm_bo_export(pRootPixPriv->bo) : -1));
    }

    /*
     * check to swap
     */
    if (!_useSwapingDRI3(pSec))
    {
        XDBG_DEBUG(MDRI3, "xserver copy\n");
        return FALSE;
    }

    if (pFrontPix->drawable.width != pBackPix->drawable.width ||
        pFrontPix->drawable.height != pBackPix->drawable.height ||
        pFrontPix->drawable.bitsPerPixel != pBackPix->drawable.bitsPerPixel)
    {
        XDBG_DEBUG(MDRI3, "xserver copy (does not match the pixmaps)\n");
        return FALSE;
    }

    if (pFrontExaPixPriv->isFrameBuffer)
    {
        XDBG_DEBUG(MDRI3, "xserver copy (the fornt pixmap is frame buffer)\n");
        return FALSE;
    }

    if (pSec->dri3_copy_type == dri3_copy_swap_pix && pDraw->type == DRAWABLE_WINDOW)
    {
        /*
         * TO-DO: save native pixmap for window
         */
        pBackPix->usage_hint = pFrontPix->usage_hint;
        present_set_tree_pixmap(((WindowPtr) pDraw), pBackPix);
        XDBG_DEBUG(MDRI3, "swap pixmap\n");
    }
    else
    {
        //save native bo to saved_bo, later it can be returned or deleted
        if (pFrontExaPixPriv->saved_bo == NULL)
        {
            pFrontExaPixPriv->saved_bo = pFrontExaPixPriv->bo;
            pFrontExaPixPriv->bo = NULL;
        }

        //set new bo to front pixmap
        if (pFrontExaPixPriv->bo != pBackExaPixPriv->bo)
        {
            if (pFrontExaPixPriv->bo != NULL)
            {
                tbm_bo_unref(pFrontExaPixPriv->bo);
            }
            pFrontExaPixPriv->bo = tbm_bo_ref(pBackExaPixPriv->bo);
        }
        XDBG_DEBUG(MDRI3, "swap bo\n");
    }

    /*
     * Report update region as damaged
     */
    if (pDraw->type == DRAWABLE_WINDOW)
    {
        RegionPtr damage;
        if (pUpdate)
        {
            damage = pUpdate;
            RegionIntersect(damage, damage, &((WindowPtr) pDraw)->clipList);
        }
        else
        {
            damage = &((WindowPtr) pDraw)->clipList;
        }

        BoxPtr pBox = RegionRects(damage);
        int nBox = RegionNumRects(damage);

        XDBG_DEBUG(MDRI3, "DamageRegion: num %d [%d,%d->%d,%d]\n", nBox,
                pBox->x1, pBox->y1, pBox->x2, pBox->y2);
        DamageDamageRegion(pDraw, damage);
    }

    return TRUE;
}
#endif
static Bool
SECPresentFlip(RRCrtcPtr		pRRcrtc,
                   uint64_t		event_id,
                   uint64_t		target_msc,
                   PixmapPtr	pPixmap,
                   Bool			sync_flip)
{
    xf86CrtcPtr pCrtc = pRRcrtc->devPrivate;
    ScreenPtr pScreen = pRRcrtc->pScreen;
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    int pipe = secModeGetCrtcPipe(pCrtc);
    PresentVblankEventPtr pEvent = NULL;

    Bool ret;

    /* TODO - process sync_flip flag
     * if (sync_flip)
     *      -//-
     * else
     *      -//-
     * */

    pEvent = calloc(sizeof(PresentVblankEventRec), 1);
    if (!pEvent)
    {
        XDBG_ERROR(MDRI3, "fail to flip\n");
        return BadAlloc;
    }
    pEvent->event_id = event_id;
    pEvent->pRRcrtc = pRRcrtc;

    SECPixmapPriv *pExaPixPriv = exaGetPixmapDriverPrivate(pPixmap);

    /*FIXME - get client id by draw id*/
    unsigned int client_idx = 0;
    XID drawable_id = pExaPixPriv->owner;

    ret = secModePageFlip(pScrn, NULL, pEvent, pipe, pExaPixPriv->bo,
                          NULL, client_idx, drawable_id, secPresentFlipEventHandler);
    if (!ret)
    {
        secPresentFlipAbort(pEvent);
        XDBG_WARNING(MDRI3, "fail to flip\n");
    }
    else
    {
        PixmapPtr pRootPix = pScreen->GetWindowPixmap (pScreen->root);
        SECPixmapPriv *pRootPixPriv = exaGetPixmapDriverPrivate (pRootPix);
        PixmapPtr pScreenPix = pScreen->GetScreenPixmap (pScreen);
        SECPixmapPriv *pScreenPixPriv = exaGetPixmapDriverPrivate (pScreenPix);

        XDBG_DEBUG(MDRI3, "doPageFlip id:0x%x Client:%d pipe:%d\n"
                "\t\t\tPresent:pix(sn:%ld p:%p ID:0x%x), bo(ptr:%p name:%d)\n"
                "\t\t\tRoot:   pix(sn:%ld p:%p ID:0x%x), bo(ptr:%p name:%d)\n"
                "\t\t\tScreen: pix(sn:%ld p:%p ID:0x%x), bo(ptr:%p name:%d)\n",
                (unsigned int )pExaPixPriv->owner, client_idx, pipe,
                pPixmap->drawable.serialNumber, pPixmap, pPixmap->drawable.id,
                pExaPixPriv->bo, tbm_bo_export(pExaPixPriv->bo),
                pRootPix->drawable.serialNumber, pRootPix, pRootPix->drawable.id,
                pRootPixPriv->bo, (pRootPixPriv->bo ? tbm_bo_export(pRootPixPriv->bo) : -1), 
                pScreenPix->drawable.serialNumber, pScreenPix, pScreenPix->drawable.id,
                pScreenPixPriv->bo, (pScreenPixPriv->bo ? tbm_bo_export(pScreenPixPriv->bo) : -1));

    }

    return ret;
}

/*
 * Queue a flip back to the normal frame buffer
 */
static void SECPresentUnflip(ScreenPtr pScreen, uint64_t event_id)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    SECPtr pSec = SECPTR(pScrn);
    PresentVblankEventPtr pEvent = NULL;
    PixmapPtr pPixmap = pScreen->GetScreenPixmap(pScreen);
    tbm_bo *bos = NULL;
    int num_bo;
    int ret;

    if (!SECPresentCheckFlip(NULL, pScreen->root, pPixmap, TRUE))
    {
        XDBG_WARNING(MDRI3, "fail to check flip for screen pixmap\n");
        return;
    }

    ret = secFbFindBo(pSec->pFb, pPixmap->drawable.x, pPixmap->drawable.y,
            pPixmap->drawable.width, pPixmap->drawable.height, &num_bo, &bos);

    if (ret != rgnSAME)
    {
        XDBG_WARNING(MDRI3, "fail to find frame buffer bo\n");
        free(bos);
        return;
    }

    pEvent = calloc(sizeof(PresentVblankEventRec), 1);
    if (!pEvent)
    {
        free(bos);
        return;
    }

    pEvent->event_id = event_id;
    pEvent->pRRcrtc = NULL;

    SECPixmapPriv *pExaPixPriv = exaGetPixmapDriverPrivate (pPixmap);


    ret = secModePageFlip(pScrn, NULL, pEvent, -1, bos[0], NULL, 0,
                0, secPresentFlipEventHandler);

    if (!ret) {
        secPresentFlipAbort(pEvent);
        XDBG_WARNING(MDRI3, "fail to flip\n");
    }
    else {

        PixmapPtr pRootPix = pScreen->GetWindowPixmap (pScreen->root);
        SECPixmapPriv *pRootPixPriv = exaGetPixmapDriverPrivate (pRootPix);

        XDBG_DEBUG(MDRI3, "doPageFlip id:0x%x Client:%d pipe:%d\n"
                "Present: pix(sn:%ld p:%p ID:0x%x), bo(ptr:%p name:%d)\n"
                "Root:    pix(sn:%ld p:%p ID:0x%x), bo(ptr:%p name:%d)\n",
                (unsigned int )pExaPixPriv->owner, 0, -1,
                pPixmap->drawable.serialNumber, pPixmap, pPixmap->drawable.id,
                bos[0], tbm_bo_export(bos[0]),
                pRootPix->drawable.serialNumber, pRootPix, pRootPix->drawable.id,
                pRootPixPriv->bo, (pRootPixPriv->bo ? tbm_bo_export(pRootPixPriv->bo) : -1));
    }

    free(bos);
}


/* The main structure which contains callback functions */
static present_screen_info_rec secPresentScreenInfo = {

        .version        = PRESENT_SCREEN_INFO_VERSION,

        .get_crtc       = SECPresentGetCrtc,
        .get_ust_msc    = SECPresentGetUstMsc,
        .queue_vblank   = SECPresentQueueVblank,
        .abort_vblank   = SECPresentAbortVblank,
        .flush          = SECPresentFlush,
        .capabilities   = PresentCapabilityNone,
        .check_flip     = SECPresentCheckFlip,
        .flip           = SECPresentFlip,
        .unflip         = SECPresentUnflip,
#ifdef PRESENT_SWAP
        .copy_region    = SECPresentCopyRegion,
#endif
};

static Bool
_hasAsyncFlip(ScreenPtr pScreen)
{
#ifdef DRM_CAP_ASYNC_PAGE_FLIP
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    SECPtr pExynos = SECPTR(pScrn);
    int ret = 0;
    uint64_t value = 0;

    ret = drmGetCap(pExynos->drm_fd, DRM_CAP_ASYNC_PAGE_FLIP, &value);
    if (ret == 0)
        return value == 1;
#endif
    return FALSE;
}

Bool
secPresentScreenInit( ScreenPtr pScreen )
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    SECPtr pSec = SECPTR(pScrn);

    pSec->dri3_copy_type = dri3_copy_swap_bo; //Default value

    const char * env_str = getenv("DRI3_SWAP");
    if (env_str)
        if (memcmp(env_str, "0", 1) == 0)
            pSec->dri3_copy_type = dri3_copy_xserver;

    env_str = getenv("DRI3_COPY_TYPE");
    if (env_str && pSec->dri3_copy_type != dri3_copy_xserver) {
        if (memcmp(env_str, "swap_bo", strlen(env_str)) == 0)
            pSec->dri3_copy_type = dri3_copy_swap_bo;
        else if (memcmp(env_str, "swap_pix", strlen(env_str)) == 0)
            pSec->dri3_copy_type = dri3_copy_swap_pix;
    }

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "DRI3 copy type: %s\n",
            pSec->dri3_copy_type == dri3_copy_xserver  ? "xserver" :
            pSec->dri3_copy_type == dri3_copy_swap_bo  ? "swap_bo" :
            pSec->dri3_copy_type == dri3_copy_swap_pix ? "swap_pix" : "unknown");

    if (_hasAsyncFlip(pScreen)) {
        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "DRI3 support async flip\n");
        secPresentScreenInfo.capabilities |= PresentCapabilityAsync;
    } else {
        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                "DRI3 doesn't support async flip\n");
    }

    int ret = present_screen_init(pScreen, &secPresentScreenInfo);
    if (!ret)
        return FALSE;

    return TRUE;
}


