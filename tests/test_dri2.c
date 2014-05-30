/*
 * test_dri2.c
 *
 *  Created on: Oct 23, 2013
 *      Author: sizonov
 */

#include "test_dri2.h"
#include "mem.h"

#define UNIT_TESTING
#define exynosDisplayGetPipe 		fake_exynosDisplayGetPipe
#define exynosDisplayGetCurMSC    	fake_exynosDisplayGetCurMSC

#define exynosCrtcGetFlipPixmap    	fake_exynosCrtcGetFlipPixmap
#define exynosCrtcExemptFlipPixmap	fake_exynosCrtcExemptFlipPixmap

#define exynosPixmapSwap 			fake_exynosPixmapSwap

#define exynosExaPixmapGetBo		fake_exynosExaPixmapGetBo

//================================ Fake function`s definition ===============================================
tbm_bo
fake_exynosExaPixmapGetBo(PixmapPtr pPix)
{
    XDBG_RETURN_VAL_IF_FAIL (pPix != NULL, NULL);

    return (tbm_bo)pPix->devPrivates;
}
//===========================================================================================================


// this file contains functions to test
#include "./accel/exynos_dri2.c"  //watch to Makefile in root directory (../../)

// this file contains fake functions and structs for GC
#include "./fakes/gc.c"

struct _envirenment_dri2
{
	ScreenRec screen;
	ScrnInfoRec scrnInfo;
	EXYNOSRec scrnInfoPriv;
	EXYNOSDispInfoRec dispInfo;

	ScreenPtr pScreen;
	ScrnInfoPtr pScrnInfo;
	EXYNOSPtr pScrnInfoPriv;
	EXYNOSDispInfoPtr pDispInfo;

	PixmapPtr pPix;
	DrawablePtr pDraw;
	WindowPtr pWin;
	DrawablePtr pDrawWin;

	ScrnInfoRec xf86Screens[1];

	//front
	DRI2BufferPtr pBuf;
	DRI2BufferPrivPtr pBufPriv;
	//back
	DRI2BufferPtr pBackBuf;
	DRI2BufferPrivPtr pBackBufPriv;
} env_gl;


//================================ Fake function`s definition =================================================
Bool
fake_exynosDisplayPageFlip (ScrnInfoPtr pScrn, int pipe, PixmapPtr pPix, void *data)
{
	return 1;
}

xf86CrtcPtr
fake_exynosDisplayGetCrtc(DrawablePtr pDraw)
{
	xf86CrtcRec crtc;
	return crtc;

}

void fake_exynosCrtcAddPendingFlip (xf86CrtcPtr pCrtc, DRI2FrameEventPtr pEvent)
{

}

int
fake_exynosDisplayGetPipe (ScrnInfoPtr pScrn, DrawablePtr pDraw)
{
	return 0;
}

void
fake_exynosPixmapSwap (PixmapPtr pFrontPix, PixmapPtr pBackPix)
{
	PixmapPtr tmp = pFrontPix;
	pFrontPix = pBackPix;
	pBackPix = tmp;
}


int exynosDisplayGetCurMSC_ctrl_return = 0;
Bool
fake_exynosDisplayGetCurMSC (ScrnInfoPtr pScrn, int pipe, CARD64 *ust, CARD64 *msc)
{
	if (exynosDisplayGetCurMSC_ctrl_return == TRUE) return TRUE;
	else											return FALSE;
}

int exynosCrtcGetFlipPixmap_ctrl_enable = 0;
PixmapPtr
fake_exynosCrtcGetFlipPixmap (ScrnInfoPtr pScrn, int crtc_pipe, DrawablePtr pDraw, unsigned int usage_hint)
{
	if (exynosCrtcGetFlipPixmap_ctrl_enable == 1)
	{
		static PixmapRec pix;
		memset(&pix, 0, sizeof(PixmapRec));
		memcpy(&(pix.drawable), pDraw ,sizeof(pix.drawable));
		return &pix;
	}
	else
	{
		return NULL;
	}
}

void
fake_exynosCrtcExemptFlipPixmap (ScrnInfoPtr pScrn, int crtc_pipe, PixmapPtr pPixmap)
{
}

PixmapPtr fake_create_pixmap (ScreenPtr pScreen, int width, int height, int depth, unsigned usage_hint)
{
	return ctrl_calloc(1, sizeof(PixmapPtr));
}

Bool fake_destroy_pixmap (PixmapPtr pPixmap)
{
	ctrl_free(pPixmap);
	return 1;
}

PixmapPtr fake_GetWindowPixmap(WindowPtr pWin)
{
	if(pWin == env_gl.pWin)
		return env_gl.pPix;
	else
		return pWin;
}

//============================================== FIXTURES ===================================================
void setup_envirenment()
{
	calloc_init();
	memset(&env_gl, 0, sizeof(env_gl));
	env_gl.pScreen = &(env_gl.screen);
	env_gl.pScrnInfo = &(env_gl.scrnInfo);
	env_gl.pScrnInfoPriv = &(env_gl.scrnInfoPriv);
	env_gl.pDispInfo = &(env_gl.dispInfo);

	env_gl.pScreen->CreatePixmap = fake_create_pixmap;
	env_gl.pScreen->DestroyPixmap = fake_destroy_pixmap;
	env_gl.pScreen->GetWindowPixmap = fake_GetWindowPixmap;
	env_gl.pScrnInfoPriv->pDispInfo = env_gl.pDispInfo;
	env_gl.pScrnInfo->driverPrivate = env_gl.pScrnInfoPriv;

	xf86Screens = env_gl.xf86Screens;
	xf86Screens[0] = env_gl.pScrnInfo;

	//set pixmap drow
	env_gl.pPix = ctrl_calloc(1, sizeof(PixmapRec));
	memset(env_gl.pPix, 0, sizeof(PixmapRec));
	env_gl.pDraw = env_gl.pPix;
	env_gl.pDraw->pScreen = env_gl.pScreen;
	env_gl.pDraw->type = DRAWABLE_PIXMAP;
	env_gl.pDraw->id = 8;

	//set window drow
	env_gl.pWin = ctrl_calloc(1, sizeof(WindowRec));
	memset(env_gl.pWin, 0, sizeof(WindowRec));
	env_gl.pDrawWin = env_gl.pWin;
	env_gl.pDrawWin->pScreen = env_gl.pScreen;
	env_gl.pDrawWin->type = DRAWABLE_WINDOW;
	env_gl.pDrawWin->id = 7;
}

void setup_dri2_buff()
{
	//front dri2 buff
	env_gl.pBuf = ctrl_calloc(1, sizeof(DRI2BufferRec));
	memset(env_gl.pBuf,0 , sizeof(DRI2BufferRec));
	env_gl.pBufPriv = ctrl_calloc(1, sizeof(DRI2BufferPrivRec));
	memset(env_gl.pBufPriv,0 , sizeof(DRI2BufferPrivRec));
	env_gl.pBuf->driverPrivate = env_gl.pBufPriv;
	env_gl.pBufPriv->pScreen = env_gl.pScreen;

	//back dri2 buff
	env_gl.pBackBuf = ctrl_calloc(1, sizeof(DRI2BufferRec));
	memset(env_gl.pBackBuf,0 , sizeof(DRI2BufferRec));
	env_gl.pBackBufPriv = ctrl_calloc(1, sizeof(DRI2BufferPrivRec));
	memset(env_gl.pBackBufPriv,0 , sizeof(DRI2BufferPrivRec));
	env_gl.pBackBuf->driverPrivate = env_gl.pBackBufPriv;
	env_gl.pBackBufPriv->pScreen = env_gl.pScreen;
	env_gl.pBackBufPriv->pBackPixmaps = ctrl_calloc(2, sizeof(PixmapPtr));

}

//------------------------------------ TEMPLATE() ------------------------------------
typedef struct TEMPLATE_struct
{
} TEMPLATE_t;
TEMPLATE_t TEMPLATE_gl;

void setup_TEMPLATE( void )
{
}

void teardown_TEMPLATE( void )
{

}

//============================================= Unit-tests ====================

#define SETUP_ENVIRENMENT() \
		calloc_init();\
\
		ScreenRec Screen;\
		memset(&Screen,0,sizeof(Screen));\
		Screen.CreatePixmap = fake_create_pixmap;\
		Screen.DestroyPixmap = fake_destroy_pixmap;\
\
		EXYNOSDispInfoRec DispInfo;\
		memset(&DispInfo,0,sizeof(DispInfo));\
\
		EXYNOSRec ScrnInfoPriv;\
		memset(&ScrnInfoPriv,0,sizeof(ScrnInfoPriv));\
		ScrnInfoPriv.pDispInfo = &DispInfo;\
\
		ScrnInfoRec ScrnInfo;\
		memset(&ScrnInfo,0,sizeof(ScrnInfo));\
		ScrnInfo.driverPrivate = &ScrnInfoPriv;\
\
		xf86Screens[0] = &ScrnInfo;\
\
		PixmapPtr pPix = ctrl_calloc(1, sizeof(PixmapRec));\
		memset(&pPix, 0 , sizeof(PixmapRec));\
		pPix->drawable.pScreen = &Screen;\
		pPix->drawable.type = DRAWABLE_PIXMAP;\
\
		DrawablePtr pDraw = pPix;\
\


//---------------------------------------  _getName() ------------------------------------
START_TEST( _getName_null_pointer_fail )
{

//	calloc_init();\
//
//	ScreenRec Screen;
//	memset(&Screen,0,sizeof(Screen));
//	Screen.CreatePixmap = fake_create_pixmap;
//	Screen.DestroyPixmap = fake_destroy_pixmap;
//
//	EXYNOSDispInfoRec DispInfo;
//	memset(&DispInfo,0,sizeof(DispInfo));
//
//	EXYNOSRec ScrnInfoPriv;
//	memset(&ScrnInfoPriv,0,sizeof(ScrnInfoPriv));
//	ScrnInfoPriv.pDispInfo = &DispInfo;
//
//	ScrnInfoRec ScrnInfo;
//	memset(&ScrnInfo,0,sizeof(ScrnInfo));
//	ScrnInfo.driverPrivate = &ScrnInfoPriv;
//
//	xf86Screens[0] = &ScrnInfo;
//
//	PixmapPtr pPix = ctrl_calloc(1, sizeof(PixmapRec));
//	memset(&pPix, 0, sizeof(PixmapRec));
//	pPix->drawable.pScreen = &Screen;
//	pPix->drawable.type = DRAWABLE_PIXMAP;
//
//	DrawablePtr pDraw = pPix;


	int res = _getName(NULL);
    fail_if( res != 0);
}
END_TEST;

START_TEST( _getName_work_flow_success )
{
	PixmapRec Pix;
	int res = _getName(&Pix);
    fail_if( res == 0);
}
END_TEST;


//-----------------------------------------------------------------------------
#define SETUP_EYXNOSDri2CopyRegion() \
		SETUP_ENVIRENMENT()\
		RegionRec Region;\
		DRI2BufferRec 	  DstBuf,     SrcBuf;\
	    DRI2BufferPrivRec DstBufPriv, SrcBufPriv;\
	    DstBuf.driverPrivate = &DstBufPriv;\
	    SrcBuf.driverPrivate = &SrcBufPriv;\
	    SrcBufPriv.pPixmap =  DstBufPriv.pPixmap = pPix;

START_TEST( EYXNOSDri2CopyRegion_null_pointer)
{
	SETUP_EYXNOSDri2CopyRegion();

	EYXNOSDri2CopyRegion(NULL, &Region, &DstBuf, &SrcBuf);
	EYXNOSDri2CopyRegion(pDraw, NULL, &DstBuf, &SrcBuf);
	EYXNOSDri2CopyRegion(pDraw, &Region, NULL, &SrcBuf);
	EYXNOSDri2CopyRegion(pDraw, &Region, &DstBuf, NULL);
}
END_TEST;

START_TEST( EYXNOSDri2CopyRegion_flow_success_1)
{
	SETUP_EYXNOSDri2CopyRegion();
	//check crash "GetScratchGC"
	testGC.doCrashGetScratchGC = 1;
	EYXNOSDri2CopyRegion(pDraw, &Region, &DstBuf, &SrcBuf);
	testGC.doCrashGetScratchGC = 0;
}
END_TEST;

START_TEST( EYXNOSDri2CopyRegion_flow_success_2)
{
	SETUP_EYXNOSDri2CopyRegion();
	EYXNOSDri2CopyRegion(pDraw, &Region, &DstBuf, &SrcBuf);
	fail_if( testGC.createScratchCount != 0, "wrong use GC in \"EYXNOSDri2CopyRegion\" " );
}
END_TEST;

START_TEST( EYXNOSDri2CopyRegion_flow_success_3)
{
//	SETUP_EYXNOSDri2CopyRegion();
//	SrcBufPriv.attachment =  DstBufPriv.attachment = DRI2BufferBackLeft;
//	EYXNOSDri2CopyRegion(&Draw, &Region, &DstBuf, &SrcBuf);
}
END_TEST;

//-----------------------------------------------------------------------------
START_TEST(EYXNOSDri2CreateBuffer_null_pointer)
{
	EYXNOSDri2CreateBuffer(NULL, DRI2BufferFrontLeft, 1);
}
END_TEST;

START_TEST(EYXNOSDri2CreateBuffer_mem_test)
{
	SETUP_ENVIRENMENT();

	//1)-------------------------------------------------------
	set_step_to_crash(1);
    int mem_obj_cnt = ctrl_get_cnt_mem_obj(CALLOC_OBJ);
	EYXNOSDri2CreateBuffer(pDraw, DRI2BufferFrontLeft, 1);
	mem_obj_cnt -= ctrl_get_cnt_mem_obj(CALLOC_OBJ);
	fail_if (mem_obj_cnt != 0, "");

	//2)-------------------------------------------------------
	set_step_to_crash(2);
    mem_obj_cnt = ctrl_get_cnt_mem_obj(CALLOC_OBJ);
	EYXNOSDri2CreateBuffer(pDraw, DRI2BufferFrontLeft, 1);
	mem_obj_cnt -= ctrl_get_cnt_mem_obj(CALLOC_OBJ);
	fail_if (mem_obj_cnt != 0, "");

}
END_TEST;

//check create Front buffer
START_TEST(EYXNOSDri2CreateBuffer_flow_success_1)
{
	SETUP_ENVIRENMENT();
	DRI2BufferPtr pBuff = NULL;

	pBuff = EYXNOSDri2CreateBuffer(pDraw, DRI2BufferFrontLeft, 1);
	fail_if ( (pBuff == 0 || pBuff->driverPrivate == 0) , "create DRI2BufferFrontLeft");
	fail_if ( pPix->refcnt != 1, "refcnt must be == 1 (refcnt==%d)", pPix->refcnt);

}
END_TEST;

//check create Back buffer
START_TEST(EYXNOSDri2CreateBuffer_flow_success_2)
{
	SETUP_ENVIRENMENT();
	DRI2BufferPtr pBuff = NULL;

	pBuff = EYXNOSDri2CreateBuffer(pDraw, DRI2BufferBackLeft, 1);
	fail_if ( pBuff == 0 || pBuff->driverPrivate == 0 , "crash create DRI2BufferBackLeft");
	fail_if ( pPix->refcnt != 0, "refcnt must be == 0 (refcnt==%d)", pPix->refcnt);
}
END_TEST;

//try create unknown type buffer
START_TEST(EYXNOSDri2CreateBuffer_flow_success_3)
{
	SETUP_ENVIRENMENT();
	DRI2BufferPtr pBuff = NULL;

	int mem_obj_cnt = ctrl_get_cnt_mem_obj(CALLOC_OBJ);

	pBuff = EYXNOSDri2CreateBuffer(pDraw, 50, 1);

	fail_if ( pBuff != 0, "creating a buffer of unknown type is not allowed");

	mem_obj_cnt -= ctrl_get_cnt_mem_obj(CALLOC_OBJ);
	fail_if ( mem_obj_cnt != 0, "memory leak when creating a buffer of unknown type");
}
END_TEST;

//---------------------------------------------------------------------

START_TEST(EYXNOSDri2DestroyBuffer_null_pointer)
{
	SETUP_ENVIRENMENT();
	EYXNOSDri2DestroyBuffer(NULL, NULL);
}
END_TEST;

START_TEST(EYXNOSDri2DestroyBuffer_flow_success_1)
{
	SETUP_ENVIRENMENT();

	DRI2BufferPrivPtr pBufPriv = ctrl_calloc (1, sizeof (DRI2BufferPrivRec));
    pBufPriv->pScreen = &Screen;
    pBufPriv->pPixmap = pPix;
    pBufPriv->attachment = DRI2BufferFrontLeft;
    pBufPriv->refcnt = 1;

    DRI2BufferPtr pBuf = ctrl_calloc (1, sizeof (DRI2BufferRec));
    pBuf->driverPrivate = pBufPriv;
    pBuf->attachment = DRI2BufferFrontLeft;

	int diff_obj_cnt = ctrl_get_cnt_mem_obj(CALLOC_OBJ);
    EYXNOSDri2DestroyBuffer(pDraw, pBuf);
    diff_obj_cnt -= ctrl_get_cnt_mem_obj(CALLOC_OBJ);

    fail_if(diff_obj_cnt != 2, "diff_obj_cnt=%d" , diff_obj_cnt);

}
END_TEST;

START_TEST(EYXNOSDri2DestroyBuffer_flow_success_2)
{
	SETUP_ENVIRENMENT();

	DRI2BufferPrivPtr pBufPriv = ctrl_calloc (1, sizeof (DRI2BufferPrivRec));
    pBufPriv->pScreen = &Screen;
    pBufPriv->pPixmap = pPix;
    pBufPriv->refcnt = 1;

    DRI2BufferPtr pBuf = ctrl_calloc (1, sizeof (DRI2BufferRec));
    pBuf->driverPrivate = pBufPriv;
    pBuf->attachment = DRI2BufferFrontLeft+1;

	int diff_obj_cnt = ctrl_get_cnt_mem_obj(CALLOC_OBJ);
    EYXNOSDri2DestroyBuffer(pDraw, pBuf);
    diff_obj_cnt -= ctrl_get_cnt_mem_obj(CALLOC_OBJ);

    fail_if(diff_obj_cnt != 2, "diff_obj_cnt=%d" , diff_obj_cnt);

}
END_TEST;


//---------------------------------------------------------------------
//Attention! This function is very simple that would cover its real tests.
START_TEST(EYXNOSDri2GetMSC_null_pointer)
{
	SETUP_ENVIRENMENT();
	EYXNOSDri2GetMSC (NULL, NULL, NULL);
}
END_TEST;
START_TEST(EYXNOSDri2GetMSC_flow_success_1)
{
	SETUP_ENVIRENMENT();
	int ust, msc;
	exynosDisplayGetCurMSC_ctrl_return = FALSE;
	fail_if (EYXNOSDri2GetMSC (pDraw, &ust, &msc) != FALSE, "must repeat the result of the function \"exynosDisplayGetCurMSC\"");
	exynosDisplayGetCurMSC_ctrl_return = TRUE;
	fail_if (EYXNOSDri2GetMSC (pDraw, &ust, &msc) != TRUE, "must repeat the result of the function \"exynosDisplayGetCurMSC\"");
}
END_TEST;
//---------------------------------------------------------------------

//---------------------------------------------------------------------
START_TEST(EYXNOSDri2ReuseBufferNotify_null_pointer)
{
	SETUP_ENVIRENMENT();
	EYXNOSDri2ReuseBufferNotify(NULL, NULL);
}
END_TEST;
START_TEST(EYXNOSDri2ReuseBufferNotify_flow_success_1)
{
	SETUP_ENVIRENMENT();
	DRI2BufferRec buf;
	DRI2BufferPrivRec bufPriv;
	buf.driverPrivate = &bufPriv;
	PixmapRec pix;
	bufPriv.pPixmap = &pix;
	EYXNOSDri2ReuseBufferNotify(pDraw, &buf);
}
END_TEST;
//---------------------------------------------------------------------

START_TEST(EYXNOSDri2ScheduleSwapWithRegion_null_pointer)
{
}
END_TEST;
START_TEST(EYXNOSDri2ScheduleSwapWithRegion_flow_success_1)
{
}
END_TEST;

START_TEST(EYXNOSDri2ScheduleWaitMSC_null_pointer)
{
}
END_TEST;
START_TEST(EYXNOSDri2ScheduleWaitMSC_flow_success_1)
{}END_TEST;

START_TEST(exynosDri2Deinit_null_pointer)
{}END_TEST;
START_TEST(exynosDri2Deinit_flow_success_1)
{}END_TEST;

START_TEST(exynosDri2FlipEventHandler_null_pointer)
{}END_TEST;
START_TEST(exynosDri2FlipEventHandler_flow_success_1)
{}END_TEST;

extern DRI2InfoRec dri2_info;
START_TEST(exynosDri2Init_flow_success_1)
{
	setup_envirenment();
	env_gl.scrnInfoPriv.drm_device_name = "12346";
	env_gl.scrnInfoPriv.drm_fd = 1;

	exynosDri2Init(env_gl.pScreen);

    fail_if(strcmp(dri2_info.driverName, "exynos-drm") != 0);
    fail_if(strcmp(dri2_info.deviceName, "12346") != 0);
    fail_if(dri2_info.version != 106 );
    fail_if(dri2_info.fd != env_gl.scrnInfoPriv.drm_fd);
    fail_if(dri2_info.CreateBuffer != EYXNOSDri2CreateBuffer);
    fail_if(dri2_info.DestroyBuffer != EYXNOSDri2DestroyBuffer);
    fail_if(dri2_info.CopyRegion != EYXNOSDri2CopyRegion);
    fail_if(dri2_info.ScheduleSwap != NULL);
    fail_if(dri2_info.GetMSC != EYXNOSDri2GetMSC);
    fail_if(dri2_info.ScheduleWaitMSC != EYXNOSDri2ScheduleWaitMSC);
    fail_if(dri2_info.AuthMagic != EYXNOSDri2AuthMagic);
    fail_if(dri2_info.ReuseBufferNotify != EYXNOSDri2ReuseBufferNotify);
    fail_if(dri2_info.SwapLimitValidate != NULL);
    fail_if(dri2_info.GetParam != NULL);
    fail_if(dri2_info.AuthMagic2 != NULL);
    fail_if(dri2_info.CreateBuffer2 != NULL);
    fail_if(dri2_info.DestroyBuffer2 != NULL);
    fail_if(dri2_info.CopyRegion2 != NULL);
    fail_if(dri2_info.ScheduleSwapWithRegion != EYXNOSDri2ScheduleSwapWithRegion);
    fail_if(dri2_info.Wait != NULL);
    fail_if(dri2_info.numDrivers != 1);
    fail_if(strcmp(dri2_info.driverNames[0], "exynos-drm") != 0);

//	fail_if();
}END_TEST;

START_TEST(exynosDri2VblankEventHandler_null_pointer)
{}END_TEST;
START_TEST(exynosDri2VblankEventHandler_flow_success_1)
{}END_TEST;

START_TEST(_asyncSwapBuffers_null_pointer)
{}END_TEST;
START_TEST(_asyncSwapBuffers_flow_success_1)
{}END_TEST;

START_TEST(_canFlip_null_pointer)
{
	setup_envirenment();
	_canFlip(NULL);
}END_TEST;
START_TEST(_canFlip_flow_success_1)
{
	setup_envirenment();
	PixmapRec rootPix = {.drawable.type = DRAWABLE_PIXMAP};

	int res;
	//------- success test ---------------
	//1)
	env_gl.pScreen->root = env_gl.pPix;
	//2)
	((WindowPtr)env_gl.pDrawWin)->viewable = 1;
	//3)
	env_gl.pDrawWin->x = env_gl.pDraw->x = 0;
	env_gl.pDrawWin->y = env_gl.pDraw->y = 0;
	env_gl.pDrawWin->width = env_gl.pDraw->width = 1;
	env_gl.pDrawWin->height = env_gl.pDraw->height = 1;

	res = _canFlip(env_gl.pDrawWin);
	fail_if(res != TRUE);

	//------- error test 1 ---------------
	env_gl.pDraw->type = DRAWABLE_PIXMAP;
	res = _canFlip(env_gl.pDraw);
	fail_if(res != FALSE, "if the drawable is pixmap, it can't be flip" );
	env_gl.pDraw->type = DRAWABLE_WINDOW;

	//------- error test 2 ---------------
	env_gl.pScreen->root = &rootPix;
	res = _canFlip(env_gl.pDrawWin);
	fail_if(res != FALSE, "if the drawable is not equal to root window, it can't be flip" );
	env_gl.pScreen->root = env_gl.pPix;

	//------- error test 3 ---------------
	((WindowPtr)env_gl.pDrawWin)->viewable = 0;
	res = _canFlip(env_gl.pDrawWin);
	((WindowPtr)env_gl.pDrawWin)->viewable = 1;
	fail_if(res != FALSE, "if the drawable is't viewable, it can't be flip" );

	//------- error test 4 ---------------

	env_gl.pDrawWin->x = 1;
	env_gl.pDraw->x = 2;
	res = _canFlip(env_gl.pDrawWin);
	fail_if(res != FALSE, "if the window doesn't match the pixmap exactly, it can't be flip" );


}END_TEST;

START_TEST(_createBackBufPixmap_null_pointer)
{}END_TEST;
START_TEST(_createBackBufPixmap_flow_success_1)
{}END_TEST;

START_TEST(_deinitBackBufPixmap_null_pointer)
{
	_deinitBackBufPixmap(NULL, NULL, 0);
}END_TEST;
START_TEST(_deinitBackBufPixmap_flow_success_1)
{
	setup_envirenment();
	setup_dri2_buff();

	int can_flip = 1;
	exynosCrtcGetFlipPixmap_ctrl_enable = 1;	//crtc pool unavailable

	int num_mem_ob = ctrl_get_cnt_calloc();

	_initBackBufPixmap(env_gl.pBuf, env_gl.pDraw, can_flip);


	clear_mem_ctrl();

	//---- start test------
	_deinitBackBufPixmap(env_gl.pBuf, env_gl.pDraw, can_flip);
	num_mem_ob -= ctrl_get_cnt_calloc();

	fail_if(env_gl.pBufPriv->pPixmap != NULL, "pPixmaps must be NULL");
	fail_if(env_gl.pBufPriv->pBackPixmaps != NULL, "pBackPixmaps must be NULL");
	fail_if(num_mem_ob > 0, "incorrect removal of pixmap");
	fail_if(is_free_error(), "wrong use \"free\"");


}END_TEST;

START_TEST(_deleteFrameEvent_null_pointer)
{

	setup_envirenment();
	setup_dri2_buff();
	_deleteFrameEvent(env_gl.pDraw, NULL);
}END_TEST;
START_TEST(_deleteFrameEvent_flow_success_1)
{

	setup_envirenment();
	setup_dri2_buff();

	ClientRec Client;
	RegionRec Region;

	PixmapRec FrontPix, BackPix;

	FrontPix.devPrivates = tbm_bo_alloc(0,1,0);
	BackPix.devPrivates = tbm_bo_alloc(0,2,0);
	env_gl.pBufPriv->pPixmap = &FrontPix;
	env_gl.pBackBufPriv->pPixmap = &BackPix;
	env_gl.pBackBufPriv->pBackPixmaps[0] = &BackPix;

	DRI2FrameEventPtr pFrameEvent = NULL;

	pFrameEvent = _newFrameEvent(&Client, env_gl.pDraw, env_gl.pBuf, env_gl.pBackBuf, NULL, NULL, &Region);

	pFrameEvent->type = DRI2_FLIP;
	env_gl.pBackBufPriv->free_idx = 0;
	env_gl.pBackBufPriv->num_buf = 2;
	env_gl.pBackBufPriv->refcnt = 2;
	env_gl.pBufPriv->refcnt = 2;

	// --------------- test------------------------
	DRI2FrameEventPtr eventTmp = pFrameEvent;
	RegionPtr pRegionTmp = pFrameEvent->pRegion;

	_deleteFrameEvent(env_gl.pDraw, pFrameEvent);

	fail_if(find_mem_obj(eventTmp) != NULL);
	fail_if(find_mem_obj(pRegionTmp) != NULL);
	fail_if(env_gl.pBufPriv->refcnt != 1);
	fail_if(env_gl.pBackBufPriv->refcnt != 1);
	fail_if(env_gl.pBackBufPriv->free_idx != 1);


}END_TEST;

START_TEST(_disuseBackBufPixmap_null_pointer)
{
	setup_envirenment();
	_disuseBackBufPixmap(NULL, NULL);
}END_TEST;
START_TEST(_disuseBackBufPixmap_flow_success_1)
{
	setup_envirenment();
	setup_dri2_buff();

	PixmapRec BackPix;
	env_gl.pBackBufPriv->pBackPixmaps[0] = &BackPix;
	env_gl.pBackBufPriv->pBackPixmaps[1] = &BackPix;

	DRI2FrameEventRec FrameEvent;

	env_gl.pBackBufPriv->num_buf = 2;

	// --------------- test 1 ------------------------
	FrameEvent.type = DRI2_FLIP;
	env_gl.pBackBufPriv->free_idx = 0;
	_disuseBackBufPixmap(env_gl.pBackBuf, &FrameEvent);
	fail_if(env_gl.pBackBufPriv->free_idx != 1);

	// --------------- test 2 ------------------------
	FrameEvent.type = DRI2_BLIT;
	env_gl.pBackBufPriv->free_idx = 0;
	_disuseBackBufPixmap(env_gl.pBackBuf, &FrameEvent);
	fail_if(env_gl.pBackBufPriv->free_idx != 0);

}END_TEST;

START_TEST(_blitBuffers_null_pointer)
{
	setup_envirenment();
	_blitBuffers(NULL, NULL, NULL);

}END_TEST;
START_TEST(_blitBuffers_flow_success_1)
{
	setup_envirenment();
	setup_dri2_buff();

	env_gl.pDrawWin->width = 10;
	env_gl.pDrawWin->height = 10;


	_blitBuffers(env_gl.pDrawWin, env_gl.pBuf, env_gl.pBackBuf);


}END_TEST;

START_TEST(_exchangeBuffers_null_pointer)
{
	setup_envirenment();
	_exchangeBuffers(NULL, 1, NULL, NULL);
}END_TEST;
START_TEST(_exchangeBuffers_flow_success_1)
{
	setup_envirenment();
	setup_dri2_buff();
	PixmapRec 			FrontPix, 		BackPix;
	env_gl.pBufPriv->pPixmap = &FrontPix;
	env_gl.pBackBufPriv->pPixmap = &BackPix;

	env_gl.pBackBufPriv.avail_idx = 0;
	env_gl.pBackBufPriv.num_buf = 2;
	PixmapPtr pFrontPix = &FrontPix;
	// ------------ test 1----------------------------
	env_gl.pBufPriv->can_flip = BackBufPriv.can_flip = 1;
	_exchangeBuffers(env_gl.pDraw, 1, &FrontBuf, &BackBuf);
	fail_if(pFrontPix == &FrontPix, "");
	fail_if(BackBufPriv.avail_idx  != 1, "");

	// ------------ test 2----------------------------
	BackBufPriv.avail_idx = 0;
	BackBufPriv.can_flip = 0;
	_exchangeBuffers(env_gl.pDraw, 1, &FrontBuf, &BackBuf);
	fail_if(pFrontPix != &FrontPix, "");
	fail_if(BackBufPriv.avail_idx  != 0, "");

}END_TEST;

START_TEST(_exynosDri2ProcessPending_null_pointer)
{}END_TEST;
START_TEST(_exynosDri2ProcessPending_flow_success_1)
{}END_TEST;

START_TEST(_generateDamage_null_pointer)
{
	setup_envirenment();
	_generateDamage(NULL, NULL);
}END_TEST;
START_TEST(_generateDamage_flow_success_1)
{
	setup_envirenment();
	RegionRec Reg = { .extents.x1 = 1, .extents.x2 = 1, .extents.y1 = 1, .extents.y2 = 1, .data = 0 };
	DRI2FrameEventRec FrameEvent;
	FrameEvent.pRegion = &Reg;


	env_gl.pDraw->serialNumber == 0;	//only to know what the function was called

	_generateDamage(env_gl.pDraw, &FrameEvent);

	FrameEvent.pRegion = NULL;

	_generateDamage(env_gl.pDraw, &FrameEvent);

	fail_if (env_gl.pDraw->serialNumber != 2, "generateDamage must call \"DamageDamageRegion\" at least once");

}END_TEST;

START_TEST(_getBufferFlag_null_pointer)
{
	setup_envirenment();
	_getBufferFlag(NULL, 0);

}END_TEST;
START_TEST(_getBufferFlag_flow_success_1)
{
	setup_envirenment();
	WindowRec win = {.drawable.type = DRAWABLE_WINDOW};
	DRI2BufferFlags flag;

	int can_flip;
	//----------------test 1----------------------
	can_flip = 0;
    flag.flags = _getBufferFlag(env_gl.pDraw, can_flip);
    fail_if(flag.data.type != DRI2_BUFFER_TYPE_PIXMAP, "if drawable is Pixmap type must by DRI2_BUFFER_TYPE_PIXMAP");
    fail_if(flag.data.is_viewable != 1, "if drawable is Pixmap is_viewable alleys must be TRUE");
    fail_if(flag.data.is_framebuffer != 0, "if can_flip == FALSE is_framebuffer alleys must be FALSE");

	//----------------test 2----------------------
	can_flip = 1;
	flag.flags = _getBufferFlag(&win, can_flip);
    win.viewable = 0;
    fail_if(flag.data.type != DRI2_BUFFER_TYPE_WINDOW, "if drawable is Window type must by DRI2_BUFFER_TYPE_WINDOW");
    fail_if(flag.data.is_viewable != 0, "if drawable is Window is_viewable should depend on \"win.viewable\"");
    fail_if(flag.data.is_framebuffer != 1, "if can_flip == TRUE is_framebuffer alleys must be TRUE");

}END_TEST;

START_TEST(_getSwapType_null_pointer)
{
	setup_envirenment();
	_getSwapType(NULL, NULL, NULL);
}END_TEST;
START_TEST(_getSwapType_flow_success_1)
{
	setup_envirenment();

	//----------------preparing------------------------
	WindowRec 			Draw, * pDraw;
	pDraw = &Draw;
	PixmapRec 			FrontPix, 		BackPix;
	DRI2BufferPrivRec 	FrontBufPriv, 	BackBufPriv;
	DRI2BufferRec 		FrontBuf, 		BackBuf;
	memset(&FrontBuf, 0, sizeof(FrontBuf));
	memset(&BackBuf, 0, sizeof(BackBuf));
	memset(&FrontBufPriv, 0, sizeof(FrontBufPriv));
	memset(&BackBufPriv, 0, sizeof(BackBufPriv));
	FrontBuf.driverPrivate = &FrontBufPriv;
	BackBuf.driverPrivate = &BackBufPriv;
	FrontBufPriv.pPixmap = &FrontPix;
	BackBufPriv.pPixmap = &BackPix;

	//---------------- test 1------------------------
	//Draw isn't viewable
	pDraw->drawable.type = DRAWABLE_WINDOW;
	pDraw->viewable = 0;
	FrontBufPriv.can_flip = BackBufPriv.can_flip = 1;

	DRI2FrameEventType type = _getSwapType(pDraw, &FrontBuf, &BackBuf);
	((WindowPtr)(pDraw))->viewable = 1;

	fail_if(type != DRI2_NONE, "type must be DRI2_NONE (type == %d)", type);


	//---------------- test 1------------------------
	FrontBufPriv.can_flip = BackBufPriv.can_flip = 1;

	type = _getSwapType(pDraw, &FrontBuf, &BackBuf);
	fail_if(type != DRI2_FLIP, "type must be DRI2_FLIP (type == %d)", type);

	//---------------- test 2------------------------
	FrontBufPriv.can_flip = 1; BackBufPriv.can_flip = 0;

	type = _getSwapType(pDraw, &FrontBuf, &BackBuf);
	fail_if(type != DRI2_FB_BLIT, "type must be DRI2_FB_BLIT (type == %d)", type);

	//---------------- test 3------------------------
	FrontBufPriv.can_flip = 0; BackBufPriv.can_flip = 0;
	//same pixmap size
	FrontPix.drawable.width = BackPix.drawable.width = 10;
	FrontPix.drawable.height = BackPix.drawable.height = 10;
	FrontPix.drawable.bitsPerPixel = BackPix.drawable.bitsPerPixel = 10;

	type = _getSwapType(pDraw, &FrontBuf, &BackBuf);
	fail_if(type != DRI2_SWAP, "type must be DRI2_SWAP (type == %d)", type);

	//---------------- test 4------------------------
	//different pixmap size
	FrontBufPriv.can_flip = 0; BackBufPriv.can_flip = 0;
	FrontPix.drawable.width = 9;
	BackPix.drawable.width = 10;

	type = _getSwapType(pDraw, &FrontBuf, &BackBuf);
	fail_if(type != DRI2_BLIT, "type must be DRI2_BLIT (type == %d)", type);

}END_TEST;

START_TEST(_initBackBufPixmap_null_pointer)
{
	_initBackBufPixmap(NULL, NULL, 0);
}END_TEST;
START_TEST(_initBackBufPixmap_flow_success_1)
{
	setup_envirenment();
	setup_dri2_buff();

	//put invalid values
	env_gl.pBufPriv->avail_idx = 10;
	env_gl.pBufPriv->cur_idx = 10;

	//---- start test------
	int can_flip[2] = {0, 1};
	int num_buf[2]  = {1, 2};
	exynosCrtcGetFlipPixmap_ctrl_enable = 1; //crtc pool available

	int i = 0;
	for (i = 0; i < 2; i++)
	{
		//put invalid values
		env_gl.pBufPriv->pBackPixmaps = NULL;
		env_gl.pBufPriv->avail_idx = 10;
		env_gl.pBufPriv->cur_idx = 10;

		PixmapPtr p = _initBackBufPixmap(env_gl.pBuf, env_gl.pDraw, can_flip[i]);

		fail_if(env_gl.pBufPriv->pBackPixmaps == NULL, "BackPixmaps must be create");
		fail_if(env_gl.pBufPriv->pBackPixmaps[0] == NULL, "pBackPixmaps[0] must be create ");
		fail_if(env_gl.pBufPriv->avail_idx != 0, "avail_idx must be 0 %d");
		fail_if(env_gl.pBufPriv->cur_idx != 0, "avail_idx must be 0 %d");
		fail_if(env_gl.pBufPriv->num_buf != num_buf[i],
				"num_buf must be %d (current val = %d), when can_flip == %d",
				 num_buf[i], env_gl.pBufPriv->num_buf, can_flip[i]);
	}
}END_TEST;
START_TEST(_initBackBufPixmap_flow_success_2)
{
	setup_envirenment();
	setup_dri2_buff();

	//put invalid values
	env_gl.pBufPriv->avail_idx = 10;
	env_gl.pBufPriv->cur_idx = 10;

	//---- start test------
	int can_flip = 1;
	exynosCrtcGetFlipPixmap_ctrl_enable = 0;	//crtc pool unavailable

	PixmapPtr p = _initBackBufPixmap(env_gl.pBuf, env_gl.pDraw, can_flip);

	fail_if(env_gl.pBufPriv->pBackPixmaps == NULL, "BackPixmaps must be create");
	fail_if(env_gl.pBufPriv->pBackPixmaps[0] == NULL, "pBackPixmaps[0] must be create ");
	fail_if(env_gl.pBufPriv->avail_idx != 0, "avail_idx must be 0 %d");
	fail_if(env_gl.pBufPriv->cur_idx != 0, "avail_idx must be 0 %d");
	fail_if(env_gl.pBufPriv->num_buf != 1, "num_buf must be 1 (current val = %d)", env_gl.pBufPriv->num_buf);

}END_TEST;



START_TEST(_newFrameEvent_null_pointer)
{
	setup_envirenment();
	setup_dri2_buff();

	DRI2FrameEventPtr pFrameEvent = NULL;

	pFrameEvent = _newFrameEvent(NULL, NULL, NULL, NULL, NULL, NULL, NULL);

	fail_if(pFrameEvent != NULL, "pFrameEvent must be NULL");

}END_TEST;
START_TEST(_newFrameEvent_flow_success_1)
{
	setup_envirenment();
	setup_dri2_buff();

	PixmapRec FrontPix, BackPix;
	env_gl.pBufPriv->pPixmap = &FrontPix;
	env_gl.pBackBufPriv->pPixmap = &BackPix;

	DRI2FrameEventPtr pFrameEvent = NULL;

	//!!!!!
	env_gl.pWin->viewable = 0;

	pFrameEvent = _newFrameEvent(NULL, env_gl.pDrawWin, env_gl.pBuf, env_gl.pBackBuf, NULL, NULL, NULL);
	fail_if(pFrameEvent != NULL, "pFrameEvent must be NULL");

}END_TEST;

START_TEST(_newFrameEvent_flow_success_2)
{
	setup_envirenment();
	setup_dri2_buff();

	ClientRec Client = {.index = 7};
	RegionPtr Region;
	PixmapRec FrontPix, BackPix;

	FrontPix.devPrivates = tbm_bo_alloc(0,1,0);
	BackPix.devPrivates = tbm_bo_alloc(0,2,0);
	env_gl.pBufPriv->pPixmap = &FrontPix;
	env_gl.pBackBufPriv->pPixmap = &BackPix;

	DRI2FrameEventPtr pFrameEvent = NULL;

	pFrameEvent = _newFrameEvent(&Client, env_gl.pDraw, env_gl.pBuf, env_gl.pBackBuf, NULL, NULL, &Region);

	fail_if (pFrameEvent->type == DRI2_NONE);
	fail_if (pFrameEvent->drawable_id != env_gl.pDraw->id);
	fail_if (pFrameEvent->client_idx != Client.index);
	fail_if (pFrameEvent->pClient != &Client);
	fail_if (pFrameEvent->event_complete != NULL);
	fail_if (pFrameEvent->event_data != NULL);
	fail_if (pFrameEvent->pFrontBuf != env_gl.pBuf);
	fail_if (pFrameEvent->pBackBuf != env_gl.pBackBuf);
	fail_if (pFrameEvent->front_bo != FrontPix.devPrivates);
	fail_if (pFrameEvent->back_bo != BackPix.devPrivates);
	fail_if (pFrameEvent->back_bo != BackPix.devPrivates);

	fail_if (env_gl.pBufPriv->refcnt != 1);
	fail_if (env_gl.pBackBufPriv->refcnt != 1);

}END_TEST;

START_TEST(_referenceBufferPriv_null_pointer)
{
	_referenceBufferPriv(NULL);
}END_TEST;
START_TEST(_referenceBufferPriv_flow_success_1)
{
	setup_envirenment();
	setup_dri2_buff();

	env_gl.pBufPriv->refcnt = 0;
	_referenceBufferPriv(env_gl.pBuf);
	fail_if(env_gl.pBufPriv->refcnt != 1);


}END_TEST;

START_TEST(_reuseBackBufPixmap_null_pointer)
{
	//_reuseBackBufPixmap(NULL,NULL, 1);
}END_TEST;

START_TEST(_reuseBackBufPixmap_flow_success_1)
{
	setup_envirenment();
	setup_dri2_buff();

	int can_flip = 1; //create as can_flip == 1

	exynosCrtcGetFlipPixmap_ctrl_enable = 1;	//crtc pool available
	_initBackBufPixmap(env_gl.pBuf, env_gl.pDraw, can_flip);

	env_gl.pBufPriv->cur_idx = 1;
	env_gl.pBufPriv->avail_idx = 1;
	//---- start test
	can_flip = 0; //use as can_flip == 0

	_reuseBackBufPixmap(env_gl.pBuf, env_gl.pDraw, can_flip);
	fail_if(env_gl.pBufPriv->pPixmap != NULL, "pixmap must be create");
	fail_if(env_gl.pBufPriv->cur_idx != 0, "value of \"cur_idx\" should be %d (carr val == %d)",  0, env_gl.pBufPriv->cur_idx);
	fail_if(env_gl.pBufPriv->avail_idx != 0, "value of \"avail_idx\" should be %d",  0);
}END_TEST;

START_TEST(_reuseBackBufPixmap_flow_success_2)
{
	setup_envirenment();
	setup_dri2_buff();

	int can_flip = 1; //create as can_flip == 1

	exynosCrtcGetFlipPixmap_ctrl_enable = 1;	//crtc pool unavailable
	_initBackBufPixmap(env_gl.pBuf, env_gl.pDraw, can_flip);


	can_flip = 1; //use as can_flip == 1

	//---- start test
	static int cur[4]   	= {0, 0, 1, 1};
	static int avail[4] 	= {0, 1, 0, 1};
	static int next_cur[4] 	= {0, 1, 0, 1}; //correct next index
	int i;
	for ( i = 0; i < 4; i++)
	{
		env_gl.pBufPriv->cur_idx = cur[i];
		env_gl.pBufPriv->avail_idx = avail[i];
		_reuseBackBufPixmap(env_gl.pBuf, env_gl.pDraw, can_flip);
		fail_if(env_gl.pBufPriv->pPixmap != NULL, "pixmap must be create on step %d", i);
		fail_if(env_gl.pBufPriv->cur_idx != next_cur[i], "value of \"cur_idx\" should be %d on step %d",  next_cur[i], i);
		fail_if(env_gl.pBufPriv->avail_idx != avail[i], "value of \"avail_idx\" should be %d on step %d",  avail[i], i);
	}

}END_TEST;

START_TEST(_unreferenceBufferPriv_null_pointer)
{
	setup_envirenment();
	_referenceBufferPriv(NULL);
}END_TEST;
START_TEST(_unreferenceBufferPriv_flow_success_1)
{
	setup_envirenment();
	setup_dri2_buff();

	env_gl.pBufPriv->refcnt = 1;
	_unreferenceBufferPriv(env_gl.pBuf);
	fail_if(env_gl.pBufPriv->refcnt != 0);

}END_TEST;

START_TEST(_scheduleFlip_null_pointer)
{}END_TEST;
START_TEST(_scheduleFlip_flow_success_1)
{
	setup_envirenment();
	setup_dri2_buff();

	_scheduleFlip(NULL, NULL, 0);
}END_TEST;

START_TEST(_setDri2Property_null_pointer)
{}END_TEST;
START_TEST(_setDri2Property_flow_success_1)
{}END_TEST;

START_TEST(_swapFrame_null_pointer)
{
	setup_envirenment();
	_swapFrame(NULL, NULL);

}END_TEST;
START_TEST(_swapFrame_flow_success_1)
{
	setup_envirenment();
	setup_dri2_buff();

	DRI2FrameEventPtr pFEvent= ctrl_calloc(1, sizeof(DRI2FrameEventRec));
	env_gl.pDraw->serialNumber = 0;
	pFEvent = DRI2_FLIP;
	_swapFrame(env_gl.pDraw, pFEvent);
	fail_if(env_gl.pDraw->serialNumber != 0);

	env_gl.pDraw->serialNumber = 0;
	pFEvent = DRI2_SWAP;
	_swapFrame(env_gl.pDraw, pFEvent);
	fail_if(env_gl.pDraw->serialNumber != 1);

	env_gl.pDraw->serialNumber = 0;
	pFEvent = DRI2_BLIT;
	_swapFrame(env_gl.pDraw, pFEvent);
	fail_if(env_gl.pDraw->serialNumber != 0);

	env_gl.pDraw->serialNumber = 0;
	pFEvent = DRI2_NONE;
	_swapFrame(env_gl.pDraw, pFEvent);
	fail_if(env_gl.pDraw->serialNumber != 1);
}END_TEST;




//===========================================================================================================
//===========================================================================================================
//===========================================================================================================

int test_dri2(run_stat_data* stat)
{

	Suite* s = suite_create("exinos_dri2.c");

	///////////////////////////////////////////////////////////////////
	//			1) - create test cases
	///////////////////////////////////////////////////////////////////
	TCase* tc_get_name 					= tcase_create("_getName");
	TCase* tc_EYXNOSDri2CopyRegion 		= tcase_create("EYXNOSDri2CopyRegion");
	TCase* tc_EYXNOSDri2CreateBuffer 	= tcase_create("EYXNOSDri2CreateBuffer");
	TCase* tc_EYXNOSDri2DestroyBuffer 	= tcase_create("EYXNOSDri2DestroyBuffer");
	TCase* tc_EYXNOSDri2GetMSC 			= tcase_create("EYXNOSDri2GetMSC");
	TCase* tc_EYXNOSDri2ReuseBufferNotify=  tcase_create("EYXNOSDri2ReuseBufferNotify");
	TCase* tc_EYXNOSDri2ScheduleSwapWithRegion = tcase_create("EYXNOSDri2ScheduleSwapWithRegion");
	TCase* tc_EYXNOSDri2ScheduleWaitMSC = tcase_create("EYXNOSDri2ScheduleWaitMSC");

	TCase* tc_exynosDri2Deinit 			= tcase_create("exynosDri2Deinit");
	TCase* tc_exynosDri2FlipEventHandler= tcase_create("exynosDri2FlipEventHandler");
	TCase* tc_exynosDri2Init 			= tcase_create("exynosDri2Init");
	TCase* tc_exynosDri2VblankEventHandler = tcase_create("exynosDri2VblankEventHandler");
	TCase* tc_asyncSwapBuffers 			= tcase_create("_asyncSwapBuffers");
	TCase* tc_blitBuffers 				= tcase_create("_blitBuffers");
	TCase* tc_canFlip 					= tcase_create("_canFlip");
	TCase* tc_createBackBufPixmap 		= tcase_create("_createBackBufPixmap");
	TCase* tc_deinitBackBufPixmap 		= tcase_create("_deinitBackBufPixmap");
	TCase* tc_deleteFrameEvent 			= tcase_create("_deleteFrameEvent");
	TCase* tc_destroyBackBufPixmap 		= tcase_create("_destroyBackBufPixmap");
	TCase* tc_disuseBackBufPixmap 		= tcase_create("_disuseBackBufPixmap");
	TCase* tc_exchangeBuffers 			= tcase_create("_exchangeBuffers");
	TCase* tc_exynosDri2ProcessPending 	= tcase_create("_exynosDri2ProcessPending");
	TCase* tc_generateDamage 			= tcase_create("_generateDamage");
	TCase* tc_getBufferFlag 			= tcase_create("_getBufferFlag");
	TCase* tc_getPixmapFromDrawable 	= tcase_create("_getPixmapFromDrawable");
	TCase* tc_getSwapType 				= tcase_create("_getSwapType");
	TCase* tc_initBackBufPixmap 		= tcase_create("_initBackBufPixmap");
	TCase* tc_newFrameEvent 			= tcase_create("_newFrameEvent");
	TCase* tc_referenceBufferPriv 		= tcase_create("_referenceBufferPriv");
	TCase* tc_reuseBackBufPixmap 		= tcase_create("_reuseBackBufPixmap");
	TCase* tc_scheduleFlip 				= tcase_create("_scheduleFlip");
	TCase* tc_setDri2Property 			= tcase_create("_setDri2Property");
	TCase* tc_swapFrame 				= tcase_create("_swapFrame");
	TCase* tc_unreferenceBufferPriv 	= tcase_create("_unreferenceBufferPriv");


	///////////////////////////////////////////////////////////////////
	//			2) - add unit-tests
	///////////////////////////////////////////////////////////////////
	//----------  _getName()
	tcase_add_test(tc_get_name, _getName_null_pointer_fail);
	tcase_add_test(tc_get_name, _getName_work_flow_success);
	//----------  EYXNOSDri2CopyRegion()
	tcase_add_test(tc_EYXNOSDri2CopyRegion, EYXNOSDri2CopyRegion_null_pointer);
	tcase_add_test(tc_EYXNOSDri2CopyRegion, EYXNOSDri2CopyRegion_flow_success_1);
	tcase_add_test(tc_EYXNOSDri2CopyRegion, EYXNOSDri2CopyRegion_flow_success_2);
//	tcase_add_test(tc_EYXNOSDri2CopyRegion, EYXNOSDri2CopyRegion_flow_success_3);
//	tcase_add_test(tc_EYXNOSDri2CopyRegion, EYXNOSDri2CopyRegion_flow_success_4);
	//----------  EYXNOSDri2CreateBuffer()
	tcase_add_test(tc_EYXNOSDri2CreateBuffer, 		EYXNOSDri2CreateBuffer_null_pointer);
	tcase_add_test(tc_EYXNOSDri2CreateBuffer, 		EYXNOSDri2CreateBuffer_mem_test);
	tcase_add_test(tc_EYXNOSDri2CreateBuffer, 		EYXNOSDri2CreateBuffer_flow_success_1);
	tcase_add_test(tc_EYXNOSDri2CreateBuffer, 		EYXNOSDri2CreateBuffer_flow_success_2);
	tcase_add_test(tc_EYXNOSDri2CreateBuffer, 		EYXNOSDri2CreateBuffer_flow_success_3);
	//----------  EYXNOSDri2DestroyBuffer()
	tcase_add_test(tc_EYXNOSDri2DestroyBuffer, 		EYXNOSDri2DestroyBuffer_null_pointer);
	tcase_add_test(tc_EYXNOSDri2DestroyBuffer, 		EYXNOSDri2DestroyBuffer_flow_success_1);
	tcase_add_test(tc_EYXNOSDri2DestroyBuffer, 		EYXNOSDri2DestroyBuffer_flow_success_2);
	//----------  EYXNOSDri2GetMSC()
	tcase_add_test(tc_EYXNOSDri2GetMSC, 			EYXNOSDri2GetMSC_null_pointer);
	tcase_add_test(tc_EYXNOSDri2GetMSC, 			EYXNOSDri2GetMSC_flow_success_1);
	//----------  EYXNOSDri2ReuseBufferNotify()
	tcase_add_test(tc_EYXNOSDri2ReuseBufferNotify, 	EYXNOSDri2ReuseBufferNotify_null_pointer);
	tcase_add_test(tc_EYXNOSDri2ReuseBufferNotify, 	EYXNOSDri2ReuseBufferNotify_flow_success_1);
	//----------  EYXNOSDri2ScheduleSwapWithRegion()
	tcase_add_test(tc_EYXNOSDri2ScheduleSwapWithRegion, EYXNOSDri2ScheduleSwapWithRegion_null_pointer);
	tcase_add_test(tc_EYXNOSDri2ScheduleSwapWithRegion, EYXNOSDri2ScheduleSwapWithRegion_flow_success_1);
	//----------  EYXNOSDri2ScheduleWaitMSC()
	tcase_add_test(tc_EYXNOSDri2ScheduleWaitMSC, 	EYXNOSDri2ScheduleWaitMSC_null_pointer);
	tcase_add_test(tc_EYXNOSDri2ScheduleWaitMSC, 	EYXNOSDri2ScheduleWaitMSC_flow_success_1);
	//----------  exynosDri2Deinit
	tcase_add_test(tc_exynosDri2Deinit, 			exynosDri2Deinit_null_pointer);
	tcase_add_test(tc_exynosDri2Deinit, 			exynosDri2Deinit_flow_success_1);
	//----------  exynosDri2FlipEventHandler
	tcase_add_test(tc_exynosDri2FlipEventHandler, 	exynosDri2FlipEventHandler_null_pointer);
	tcase_add_test(tc_exynosDri2FlipEventHandler, 	exynosDri2FlipEventHandler_flow_success_1);
	//----------  exynosDri2Init
	tcase_add_test(tc_exynosDri2Init, 				exynosDri2Init_flow_success_1);
	//----------  exynosDri2VblankEventHandler
	tcase_add_test(tc_exynosDri2VblankEventHandler, exynosDri2VblankEventHandler_null_pointer);
	tcase_add_test(tc_exynosDri2VblankEventHandler, exynosDri2VblankEventHandler_flow_success_1);
	//----------  _asyncSwapBuffers
	tcase_add_test(tc_asyncSwapBuffers, 			_asyncSwapBuffers_null_pointer);
	tcase_add_test(tc_asyncSwapBuffers, 			_asyncSwapBuffers_flow_success_1);
	//----------  _canFlip
	tcase_add_test(tc_canFlip, 						_canFlip_null_pointer);
	tcase_add_test(tc_canFlip, 						_canFlip_flow_success_1);
	//----------  _createBackBufPixmap
	tcase_add_test(tc_createBackBufPixmap, 	_createBackBufPixmap_null_pointer);
	tcase_add_test(tc_createBackBufPixmap, 	_createBackBufPixmap_flow_success_1);
	//----------  _deinitBackBufPixmap
	tcase_add_test(tc_deinitBackBufPixmap, 	_deinitBackBufPixmap_null_pointer);
	tcase_add_test(tc_deinitBackBufPixmap, 	_deinitBackBufPixmap_flow_success_1);
	//----------  _deleteFrameEvent
	tcase_add_test(tc_deleteFrameEvent, 	_deleteFrameEvent_null_pointer);
	tcase_add_test(tc_deleteFrameEvent, 	_deleteFrameEvent_flow_success_1);
	//----------  _disuseBackBufPixmap
	tcase_add_test(tc_disuseBackBufPixmap, 	_disuseBackBufPixmap_null_pointer);
	tcase_add_test(tc_disuseBackBufPixmap, 	_disuseBackBufPixmap_flow_success_1);
	//----------  _blitBuffers
	tcase_add_test(tc_blitBuffers, 	_blitBuffers_null_pointer);
	tcase_add_test(tc_blitBuffers, 	_blitBuffers_flow_success_1);
	//----------  _exchangeBuffers
	tcase_add_test(tc_exchangeBuffers, 	_exchangeBuffers_null_pointer);
	tcase_add_test(tc_exchangeBuffers, 	_exchangeBuffers_flow_success_1);
	//----------  _exynosDri2ProcessPending
	tcase_add_test(tc_exynosDri2ProcessPending, 	_exynosDri2ProcessPending_null_pointer);
	tcase_add_test(tc_exynosDri2ProcessPending, 	_exynosDri2ProcessPending_flow_success_1);
	//----------  _generateDamage
	tcase_add_test(tc_generateDamage, 	_generateDamage_null_pointer);
	tcase_add_test(tc_generateDamage, 	_generateDamage_flow_success_1);
	//----------  _getBufferFlag
	tcase_add_test(tc_getBufferFlag, 	_getBufferFlag_null_pointer);
	tcase_add_test(tc_getBufferFlag, 	_getBufferFlag_flow_success_1);
	//----------  _getSwapType
	tcase_add_test(tc_getSwapType, 	_getSwapType_null_pointer);
	tcase_add_test(tc_getSwapType, 	_getSwapType_flow_success_1);
	//----------  _initBackBufPixmap
	tcase_add_test(tc_initBackBufPixmap, 	_initBackBufPixmap_null_pointer);
	tcase_add_test(tc_initBackBufPixmap, 	_initBackBufPixmap_flow_success_1);
	tcase_add_test(tc_initBackBufPixmap, 	_initBackBufPixmap_flow_success_2);
	//----------  _newFrameEvent
	tcase_add_test(tc_newFrameEvent, 	_newFrameEvent_null_pointer);
	tcase_add_test(tc_newFrameEvent, 	_newFrameEvent_flow_success_1);
	tcase_add_test(tc_newFrameEvent, 	_newFrameEvent_flow_success_2);
	//----------  _referenceBufferPriv
	tcase_add_test(tc_referenceBufferPriv, 	_referenceBufferPriv_null_pointer);
	tcase_add_test(tc_referenceBufferPriv, 	_referenceBufferPriv_flow_success_1);
	//----------  _reuseBackBufPixmap
	tcase_add_test(tc_reuseBackBufPixmap, 	_reuseBackBufPixmap_null_pointer);
	tcase_add_test(tc_reuseBackBufPixmap, 	_reuseBackBufPixmap_flow_success_1);
	tcase_add_test(tc_reuseBackBufPixmap, 	_reuseBackBufPixmap_flow_success_2);
	//----------  _unreferenceBufferPriv
	tcase_add_test(tc_unreferenceBufferPriv, 	_unreferenceBufferPriv_null_pointer);
	tcase_add_test(tc_unreferenceBufferPriv, 	_unreferenceBufferPriv_flow_success_1);
	//----------  _scheduleFlip
	tcase_add_test(tc_scheduleFlip, 	_scheduleFlip_null_pointer);
	tcase_add_test(tc_scheduleFlip, 	_scheduleFlip_flow_success_1);
	//----------  _setDri2Property
	tcase_add_test(tc_setDri2Property, 	_setDri2Property_null_pointer);
	tcase_add_test(tc_setDri2Property, 	_setDri2Property_flow_success_1);
	//----------  _swapFrame
	tcase_add_test(tc_swapFrame, 	_swapFrame_null_pointer);
	tcase_add_test(tc_swapFrame, 	_swapFrame_flow_success_1);


	///////////////////////////////////////////////////////////////////
	//			3) - add test cases to 's' suite
	///////////////////////////////////////////////////////////////////
//	suite_add_tcase(s, tc_get_name);
//	suite_add_tcase(s, tc_EYXNOSDri2CopyRegion);
//	suite_add_tcase(s, tc_EYXNOSDri2CreateBuffer);
//	suite_add_tcase(s, tc_EYXNOSDri2DestroyBuffer);
//	suite_add_tcase(s, tc_EYXNOSDri2GetMSC);
//	suite_add_tcase(s, tc_EYXNOSDri2ReuseBufferNotify);
//////	suite_add_tcase(s, tc_EYXNOSDri2ScheduleSwapWithRegion);
////	suite_add_tcase(s, tc_exynosDri2Deinit);
////	suite_add_tcase(s, tc_exynosDri2FlipEventHandler);
//	suite_add_tcase(s, tc_exynosDri2Init);
////	suite_add_tcase(s, tc_exynosDri2VblankEventHandler);
////	suite_add_tcase(s, tc_asyncSwapBuffers);
//	suite_add_tcase(s, tc_canFlip);
//	suite_add_tcase(s, tc_deinitBackBufPixmap);
//	suite_add_tcase(s, tc_deleteFrameEvent);
//	suite_add_tcase(s, tc_disuseBackBufPixmap);
//	suite_add_tcase(s, tc_blitBuffers);
//	suite_add_tcase(s, tc_exchangeBuffers);
////	suite_add_tcase(s, tc_exynosDri2ProcessPending);
//	suite_add_tcase(s, tc_generateDamage);
//	suite_add_tcase(s, tc_getBufferFlag);
//	suite_add_tcase(s, tc_getSwapType);
//	suite_add_tcase(s, tc_initBackBufPixmap);
//	suite_add_tcase(s, tc_newFrameEvent);
//	suite_add_tcase(s, tc_referenceBufferPriv);
//	suite_add_tcase(s, tc_reuseBackBufPixmap);
//	suite_add_tcase(s, tc_unreferenceBufferPriv);
	suite_add_tcase(s, tc_scheduleFlip);
////	suite_add_tcase(s, tc_setDri2Property);
	suite_add_tcase(s, tc_swapFrame);


	///////////////////////////////////////////////////////////////////
	//			4) - RUN TESTS
	///////////////////////////////////////////////////////////////////
	SRunner *sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);
	stat->num_failure = srunner_ntests_failed(sr);
	stat->num_pass = srunner_ntests_run(sr);
	return 0;
}

