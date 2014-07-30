/*
 * Copyright © 2013 Keith Packard
 * Copyright 2010 - 2014 Samsung Electronics co., Ltd. All Rights Reserved.
 *
 * Contact: Roman Marchenko <r.marchenko@samsung.com>
 * Contact: Roman Peresipkyn<r.peresipkyn@samsung.com>
 * Contact: Sergey Sizonov<s.sizonov@samsung.com>
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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include <tbm_bufmgr.h>

#include "xorg-server.h"
#include "xf86.h"
#include "xf86_OSproc.h"

#include "xf86Pci.h"
#include "xf86drm.h"

#include "windowstr.h"
#include "shadow.h"
#include "fb.h"

#include "present.h"

#include "dri3.h"


#define DebugPresent(x) ErrorF x

#include "exynos_driver.h"
#include "exynos_accel_int.h"
#include "exynos_display.h"


/*------------------------------ For Debug ----------------------------------*/
#define debug_str(s) printf("\n\n%s\n\n", s);


/*------------------------------ For Debug ----------------------------------*/
#define SCREEN()
#define SCRN()


/*-------------------------- Private structures -----------------------------*/
typedef struct _presentVblankEvent {

	uint64_t event_id;

	RRCrtcPtr pRRcrtc;	//jast for info

} PresentVblankEventRec, *PresentVblankEventPtr;

/*-------------------------- Private functions -----------------------------*/
/*static uint32_t _pipe_select(int pipe)
{
	if (pipe > 1)
		return pipe << DRM_VBLANK_HIGH_CRTC_SHIFT;
	else if (pipe > 0)
		return DRM_VBLANK_SECONDARY;
	else
		return 0;
}*/

/*
 * Flush the DRM event queue when full; this
 * makes space for new requests
 */
/*static Bool
_flush_drm_events(screen)
{
	ScrnInfoPtr             scrn = xf86ScreenToScrn(screen);
	return FALSE;
}

static int
_crtc_pipe(RRCrtcPtr randr_crtc)
{
        xf86CrtcPtr crtc;

        if (randr_crtc == NULL)
                return 0;

        crtc = randr_crtc->devPrivate;
        return 0;//intel_crtc_to_pipe(crtc);
}
*/
/*-------------------------- Public functions -----------------------------*/

/*
 * Called when the queued vblank event has occurred
 */
void
exynosPresentVblankHandler(uint64_t msc, uint64_t usec, void *data)
{
	PresentVblankEventRec *pEvent = data;

	XDBG_DEBUG(MDRI2, "event_id:%d", pEvent->event_id);
	present_event_notify(pEvent->event_id, usec, msc);
    free(pEvent);


}

/*
 * Called when the queued vblank is aborted
 */
void
exynosPresentVblankAbort(ScrnInfoPtr scrn, xf86CrtcPtr crtc, void *data)
{
	PresentVblankEventRec *event = data;
    free(event);
}

/*
 * Once the flip has been completed on all pipes, notify the
 * extension code telling it when that happened
 */

void
exynosPresentFlipEvent(uint64_t msc, uint64_t ust, void *pageflip_data)
{
	PresentVblankEventRec *event = pageflip_data;

	present_event_notify(event->event_id, ust, msc);
	free(event);
}

/*
 * The flip has been aborted, free the structure
 */
void
exynosPresentFlipAbort(void *pageflip_data)
{
	PresentVblankEventRec *event = pageflip_data;
	free(event);
}

/*-------------------------- Callback functions -----------------------------*/
static RRCrtcPtr
EXYNOSPresentGetCrtc(WindowPtr window)
{
	XDBG_RETURN_VAL_IF_FAIL (window != NULL, NULL);

	ScreenPtr pScreen = window->drawable.pScreen;
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	BoxRec box, crtcbox;
	xf86CrtcPtr pCrtc = NULL;
	RRCrtcPtr pRandrCrtc = NULL;

	box.x1 = window->drawable.x;
	box.y1 = window->drawable.y;
	box.x2 = box.x1 + window->drawable.width;
	box.y2 = box.y1 + window->drawable.height;

	pCrtc = exynosModeCoveringCrtc( pScrn, &box, NULL, &crtcbox );

	/* Make sure the CRTC is valid and this is the real front buffer */
	if (pCrtc != NULL && !pCrtc->rotatedData ) //TODO what is pCrtc->rotatedData pointing on?
			pRandrCrtc = pCrtc->randr_crtc;

	XDBG_DEBUG(MDRI2, "%s\n", pRandrCrtc ?  "OK" : "ERROR");

	return pRandrCrtc;
}

/*
 * The kernel sometimes reports bogus MSC values, especially when
 * suspending and resuming the machine. Deal with this by tracking an
 * offset to ensure that the MSC seen by applications increases
 * monotonically, and at a reasonable pace.
 */
static int
EXYNOSPresentGetUstMsc(RRCrtcPtr pRRcrtc, CARD64 *ust, CARD64 *msc)
{
	XDBG_RETURN_VAL_IF_FAIL (pRRcrtc != NULL, NULL);

	xf86CrtcPtr pCrtc = pRRcrtc->devPrivate;
	ScreenPtr pScreen = pRRcrtc->pScreen;
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);

	EXYNOSCrtcPrivPtr pCrtcPriv = pCrtc->driver_private;

	//Get the current msc/ust value from the kernel
	Bool ret = exynosDisplayGetCurMSC(pScrn, pCrtcPriv->pipe, ust, msc);

	XDBG_DEBUG(MDRI2, "%s: pipe:%d ust:%lld msc:%lld()\n",
			(ret?"OK":"ERROR"), pCrtcPriv->pipe, *ust, *msc);
	return (int)ret;
}

static int
EXYNOSPresentQueueVblank(RRCrtcPtr 		pRRcrtc,
						 uint64_t 		event_id,
						 uint64_t 		msc)
{
						 xf86CrtcPtr	pCrtc 			= pRRcrtc->devPrivate;
						 ScreenPtr 		pScreen 		= pRRcrtc->pScreen;
						 ScrnInfoPtr 	pScrn 			= xf86ScreenToScrn(pScreen);
						 int 			pipe 			= exynosDisplayGetCrtcPipe(pCrtc);
						 PresentVblankEventPtr pEvent 	= NULL;

	pEvent = calloc(sizeof(PresentVblankEventRec), 1);
	if (!pEvent) {
		XDBG_DEBUG(MDRI2, "ERROR: event_id:%lld msc:%llu \n", event_id, msc);
		return BadAlloc;
	}
	pEvent->event_id = event_id;
	pEvent->pRRcrtc = pRRcrtc;


	/* flip is 1 to avoid to set DRM_VBLANK_NEXTONMISS */
	if (!exynosDisplayVBlank(pScrn, pipe, &msc, 1, VBLANK_INFO_PRESENT,  pEvent)) {
		XDBG_DEBUG(MDRI2, "ERROR: event_id:%lld msc:%llu \n", event_id, msc);
		exynosPresentVblankAbort(pScreen, pCrtc, pEvent);
		return BadAlloc;
	}

	XDBG_DEBUG(MDRI2, "OK event_id:%lld msc:%llu \n", event_id, msc);
	return Success;
}

static void
EXYNOSPresentAbortVblank(RRCrtcPtr pRRcrtc, uint64_t event_id, uint64_t msc)
{
	XDBG_DEBUG(MDRI2, "isn't implamentation\n");

}

static void
EXYNOSPresentFlush(WindowPtr window)
{
	XDBG_DEBUG(MDRI2, "isn't implamentation\n");
}

static Bool
EXYNOSPresentCheckFlip(RRCrtcPtr 	pRRcrtc,
		 	 	 	   WindowPtr 	pWin,
		 	 	 	   PixmapPtr 	pPixmap,
		 	 	 	   Bool 		sync_flip)
{
	XDBG_DEBUG(MDRI2, "isn't implamentation (always returns TRUE)\n");
	return TRUE;
}

static Bool
EXYNOSPresentFlip(RRCrtcPtr		pRRcrtc,
                   uint64_t		event_id,
                   uint64_t		target_msc,
                   PixmapPtr	pixmap,
                   Bool			sync_flip)
{

	xf86CrtcPtr pCrtc = pRRcrtc->devPrivate;
	ScreenPtr pScreen = pRRcrtc->pScreen;
	ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
	int pipe = exynosDisplayGetCrtcPipe(pCrtc);
	PresentVblankEventPtr pEvent = NULL;
//	EXYNOSPtr pExynosScrn = EXYNOSPTR(pScrn);

	Bool ret;

	pEvent = calloc(sizeof(PresentVblankEventRec), 1);
	if (!pEvent) {
		XDBG_DEBUG(MDRI2, "flip failed\n");
		return BadAlloc;
	}
	pEvent->event_id = event_id;

	ret = exynosDisplayPageFlip(pScrn, pipe, pixmap, pEvent, exynosPresentFlipEvent);
	if (!ret) {
		exynosPresentFlipAbort(pEvent);
		XDBG_DEBUG(MDRI2, "flip failed\n");
	}
	XDBG_DEBUG(MDRI2, "flip OK\n");
	return ret;
}

static void
EXYNOSPresentUnflip(ScreenPtr pScreen, uint64_t event_id)
{
	XDBG_DEBUG(MDRI2, "isn't implamentation\n");
}


/* The main structure which contains callback functions */
static present_screen_info_rec EXYNOSPresent_screen_info = {

		.version = PRESENT_SCREEN_INFO_VERSION,

	    .get_crtc = EXYNOSPresentGetCrtc,
	    .get_ust_msc = EXYNOSPresentGetUstMsc,
	    .queue_vblank = EXYNOSPresentQueueVblank,
	    .abort_vblank = EXYNOSPresentAbortVblank,
	    .flush = EXYNOSPresentFlush,
	    .capabilities = PresentCapabilityNone,
	    .check_flip = EXYNOSPresentCheckFlip,
	    .flip = EXYNOSPresentFlip,
	    .unflip = EXYNOSPresentUnflip,
};

static Bool
_hasAsyncFlip(ScreenPtr screen)
{
#ifdef DRM_CAP_ASYNC_PAGE_FLIP
	ScrnInfoPtr pScrn = xf86Screens[screen->myNum];
	EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
	int                     ret;
	uint64_t                value;

	ret = drmGetCap(pExynos->drm_fd, DRM_CAP_ASYNC_PAGE_FLIP, &value);
	if (ret == 0)
		return value == 1;
#endif
	return FALSE;
}

Bool
exynosPresentScreenInit( ScreenPtr screen )
{
	ScrnInfoPtr pScrn = xf86Screens[screen->myNum];

	if (_hasAsyncFlip(screen))
		EXYNOSPresent_screen_info.capabilities |= PresentCapabilityAsync;

	int ret = present_screen_init( screen, &EXYNOSPresent_screen_info );
	if(!ret)
		return 0;

	XDBG_DEBUG(MDRI2, "Hello it is present""\n");

	return 1;
}


