/*
 * test_display.c
 *
 *  Created on: Oct 9, 2013
 *      Author: s.voloshynov
 */

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )

#include "test_display.h"


#define exynosDri2VblankEventHandler 	exynosDri2VblankEventHandler_fake
#define exynosCloneHandleIppEvent    	exynosCloneHandleIppEvent_fake
#define exynosVideoImageIppEventHandler exynosVideoImageIppEventHandler_fake
#define exynosDri2FlipEventHandler		exynosDri2FlipEventHandler_fake

// this file contains functions to test
#include "./display/exynos_display.c"
#include "exynos_display_int.h"

// fake-functions
extern int          get_tbm_check_refs( void );
extern unsigned int ctrl_get_cnt_calloc( void );
extern uint32_t     _add_fb( void );
extern int          cnt_fb_check_refs( void );

extern drmModeCrtcPtr get_drm_mode_crtc( void );
extern void 		  set_crtc_cnt( int dig );
extern unsigned int   get_crtc_cnt( void );
extern void 		  DestroyPixmap_fake( PixmapPtr ptr );
extern void 		  set_hndl_tbm_ptr( void* ptr );
extern int 			  get_tbm_map_cnt( void );
extern int 			  get_hndl_get_cnt( void );
extern void* 		  get_bo_data( void );



//============================================== FIXTURES ===================================================

// --------------------------------------------- page_flip --------------------------------------------------

// MUST be not-changed, except within setup_page_flip() function call
EXYNOSPageFlipPtr pPageFlip_gl = NULL;
EXYNOSCrtcPrivPtr pCrtcPriv_gl = NULL;
xf86CrtcPtr pxf86Crtc_gl = NULL;

// definition checked fixture 'page_flip' for 'tc_page_flip_logic' test case
void setup_page_flip( void )
{
  pPageFlip_gl = ctrl_calloc( 1, sizeof( EXYNOSPageFlipRec ) );
  pPageFlip_gl->pCrtc = ctrl_calloc( 1, sizeof( xf86CrtcRec ) );
  pPageFlip_gl->pCrtc->driver_private = ctrl_calloc( 1, sizeof( EXYNOSCrtcPrivRec ) );

  pxf86Crtc_gl = pPageFlip_gl->pCrtc;
  pCrtcPriv_gl = pPageFlip_gl->pCrtc->driver_private;
}

void teardown_page_flip( void )
{
  // ctrl_free does NULL pointer check
  ctrl_free( pCrtcPriv_gl );
  ctrl_free( pxf86Crtc_gl );
  // ctrl_free( pPageFlip ) call has been done in EXYNOSDispInfoPageFlipHandler()
}

// ----------------------------------------- _displayGetFreeCrtcID ------------------------------------------

// MUST be not-changed, except within setup_get_free_crtc_id() function call

int crtc_id = 0x11;		// any digital
EXYNOSDispInfoPtr pExynosMode_gl;
ScrnInfoPtr  pScrn_gl;
EXYNOSPtr exynos_ptr_gl;

// definition checked fixture 'get_free_crtc_id' for 'tc_get_free_crtc_id_fixture' test case
void setup_get_free_crtc_id( void )
{
	pScrn_gl = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn_gl->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );

	exynos_ptr_gl = pScrn_gl->driverPrivate;

	exynos_ptr_gl->pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );
	pExynosMode_gl = exynos_ptr_gl->pDispInfo;

	pExynosMode_gl->mode_res = ctrl_calloc( 1, sizeof( drmModeRes ) );
}

void teardown_get_free_crtc_id( void )
{
	// release dynamic allocated memory is not necessary
}

// ----------------------------------------- _renderBoCreate ------------------------------------------

typedef struct tc_render_bo_create_struct
{
	int width;
	int height;
	tbm_bo res;
	ScrnInfoPtr pScrn;
	EXYNOSPtr pExynos;
	char* ptr;
	EXYNOS_FbPtr pFb;
} render_bo_create_t;

render_bo_create_t render_bo_create_fixt;

// definition checked fixture 'render_bo_create' for 'tc_render_bo_create' test case
void setup_render_bo_create( void )
{
	render_bo_create_fixt.res = NULL;
	render_bo_create_fixt.width = 1920;
	render_bo_create_fixt.height = 1080;

	render_bo_create_fixt.pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	render_bo_create_fixt.pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	render_bo_create_fixt.pExynos = render_bo_create_fixt.pScrn->driverPrivate;

	render_bo_create_fixt.ptr = ctrl_calloc( render_bo_create_fixt.width * 4 * render_bo_create_fixt.height, sizeof( char ) );
	*render_bo_create_fixt.ptr = '1';	// any symbol

	render_bo_create_fixt.pExynos->pFb = ctrl_calloc( 1, sizeof( EXYNOS_FbRec ) );
	render_bo_create_fixt.pFb = render_bo_create_fixt.pExynos->pFb;
}
void teardown_render_bo_create( void )
{
	// release dynamic allocated memory is not necessary
}

//================================ Some addition function for prepare purpose ===============================

//-------------------------------------------------------------------------------------------------------------
void exynosDri2VblankEventHandler_fake( unsigned int frame, unsigned int tv_exynos, unsigned int tv_uexynos,
										void *event_data )
{
    DRI2FrameEventPtr pEvent = (DRI2FrameEventPtr) event_data;

    pEvent->pipe = 11; // any digital
}

//-------------------------------------------------------------------------------------------------------------
void exynosCloneHandleIppEvent_fake( int fd, unsigned int *buf_idx, void *data )
{
	*buf_idx = 11; // any digital
}

//-------------------------------------------------------------------------------------------------------------
void exynosVideoImageIppEventHandler_fake( int fd, unsigned int prop_id,
        								   unsigned int * buf_idx, pointer data )
{
	*buf_idx = 11; // any digital
}

//-------------------------------------------------------------------------------------------------------------
void exynosDri2FlipEventHandler_fake( unsigned int frame, unsigned int tv_exynos, unsigned int tv_uexynos,
									  void *event_data )
{
    DRI2FrameEventPtr pEvent = (DRI2FrameEventPtr) event_data;

    pEvent->pipe = 11; // any digital
}

//============================================= Unit-tests ==================================================


//--------------------------------------- _exynosDisplayInitDrmEventCtx() -----------------------------------
START_TEST( _exynosDisplayInitDrmEventCtx_mem_test )
{
  _exynosDisplayInitDrmEventCtx();

  fail_if( ctrl_get_cnt_calloc() != 1, "errors with calloc/free operations -- %d", ctrl_get_cnt_calloc() );

  // free p_drm_event_ctx is not necessary (all union-tests are launched in spited address spaces)
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( _exynosDisplayInitDrmEventCtx_test )
{
  EXYNOSDrmEventCtxPtr p_drm_event_ctx = _exynosDisplayInitDrmEventCtx();

  fail_if( p_drm_event_ctx->vblank_handler    != EXYNOSDispInfoVblankHandler,   "vblank_handler is invalid" );
  fail_if( p_drm_event_ctx->page_flip_handler != EXYNOSDispInfoPageFlipHandler, "page_flip_handler is invalid" );
  fail_if( p_drm_event_ctx->ipp_handler       != EXYNOSDispInfoIppHandler,      "ipp_handler is invalid" );
  fail_if( p_drm_event_ctx->g2d_handler       != EXYNOSDispG2dHandler,          "g2d_handler is invalid" );
}
END_TEST;

//--------------------------------------- _exynosDisplayDeInitDrmEventCtx() -----------------------------------
START_TEST( _exynosDisplayDeInitDrmEventCtx_mem_test )
{
  _exynosDisplayDeInitDrmEventCtx( NULL ); // if segmentation fault -- fail
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( _exynosDisplayDeInitDrmEventCtx_test )
{
  // it is implied, that _exynosDisplayInitDrmEventCtx is worked
  EXYNOSDrmEventCtxPtr p_drm_event_ctx = _exynosDisplayInitDrmEventCtx();

  _exynosDisplayDeInitDrmEventCtx( p_drm_event_ctx );

  fail_if( ctrl_get_cnt_calloc() != 0, "errors with calloc/free operations -- %d", ctrl_get_cnt_calloc() );
}
END_TEST;

//-------------------------------------------- EXYNOSCrtcConfigResize() -------------------------------------
START_TEST( EXYNOSCrtcConfigResize_mem_test )
{
  EXYNOSCrtcConfigResize( NULL, 1, 1 ); // if segmentation fault -- fail
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( EXYNOSCrtcConfigResize_test_1 )
{
  ScrnInfoRec Scrn;
  int width = 1920;
  int height = 1080;
  Bool res = FALSE;

  Scrn.virtualX = width;
  Scrn.virtualY = height;
  res = EXYNOSCrtcConfigResize( &Scrn, width, height );

  fail_if( res != TRUE, "result of EXYNOSCrtcConfigResize MUST be TRUE" );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( EXYNOSCrtcConfigResize_test_2 )
{
  ScrnInfoRec Scrn;
  int width = 1920;
  int height = 1080;
  Bool res = FALSE;

  Scrn.virtualX = 1024;
  Scrn.virtualY = 768;
  res = EXYNOSCrtcConfigResize( &Scrn, width, height );

  fail_if( res != TRUE, "result of EXYNOSCrtcConfigResize MUST be TRUE" );
  fail_if( Scrn.virtualX != width, "Scrn.virtualX MUST be equal width, %d != %d", Scrn.virtualX, width );
  fail_if( Scrn.virtualY != height, "Scrn.virtualY MUST be equal height, %d != %d", Scrn.virtualY, height );

  Scrn.virtualX = 100;
  EXYNOSCrtcConfigResize( &Scrn, width, height );

  fail_if( Scrn.virtualX != width, "Scrn.virtualX MUST be equal width, %d != %d", Scrn.virtualX, width );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( EXYNOSCrtcConfigResize_test_3 )
{
  ScrnInfoRec Scrn;
  int width = -1920;
  int height = -1080;

  Scrn.virtualX = 0;
  Scrn.virtualY = 0;
  EXYNOSCrtcConfigResize( &Scrn, width, height );

  fail_if( Scrn.virtualX == width,  "negative argument : Scrn.virtualX = %d", Scrn.virtualX );
  fail_if( Scrn.virtualY == height, "negative argument : Scrn.virtualY = %d", Scrn.virtualY );
}
END_TEST;

//---------------------------------------- EXYNOSDispInfoVblankHandler() ------------------------------------
START_TEST( EXYNOSDispInfoVblankHandler_mem_test_1 )
{
	EXYNOSDispInfoVblankHandler( 1, 1, 1, 1, NULL ); //if segmentation fault -- fail
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( EXYNOSDispInfoVblankHandler_mem_test_2 )
{
  EXYNOSVBlankInfoPtr pVBlankInfo = ctrl_calloc( 1, sizeof( EXYNOSVBlankInfoRec ) );

  pVBlankInfo->type = VBLANK_INFO_NONE;
  pVBlankInfo->data = NULL;

  EXYNOSDispInfoVblankHandler( 1, 1, 1, 1, (void*) pVBlankInfo );

  fail_if( ctrl_get_cnt_calloc() != 0, "errors with calloc/free operations -- %d", ctrl_get_cnt_calloc() );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( EXYNOSDispInfoVblankHandler_mem_test_3 )
{
  EXYNOSVBlankInfoPtr pVBlankInfo = ctrl_calloc( 1, sizeof( EXYNOSVBlankInfoRec ) );

  DRI2FrameEventPtr pEvent = ctrl_calloc( 1, sizeof( DRI2FrameEventRec ) );

  pVBlankInfo->type = VBLANK_INFO_SWAP;
  pVBlankInfo->data = pEvent;

  EXYNOSDispInfoVblankHandler( 1, 1, 1, 1, (void*) pVBlankInfo ); // if segmentation fault -- fail

  fail_if( pEvent->pipe != 11, "work flow is wrong (changed) -- %d", pEvent->pipe );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( EXYNOSDispInfoVblankHandler_mem_test_3_1 )
{
  EXYNOSVBlankInfoPtr pVBlankInfo = ctrl_calloc( 1, sizeof(EXYNOSVBlankInfoRec) );

  DRI2FrameEventPtr pEvent = ctrl_calloc( 1, sizeof( DRI2FrameEventRec ) );

  pVBlankInfo->type = VBLANK_INFO_SWAP;
  pVBlankInfo->data = pEvent;

  EXYNOSDispInfoVblankHandler( 1, 1, 1, 1, (void*) pVBlankInfo ); // if segmentation fault -- fail

  fail_if( ctrl_get_cnt_calloc() != 1, "errors with calloc/free operations -- %d", ctrl_get_cnt_calloc() );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( EXYNOSDispInfoVblankHandler_mem_test_4 )
{
  EXYNOSVBlankInfoPtr pVBlankInfo = ctrl_calloc( 1, sizeof(EXYNOSVBlankInfoRec) );
  IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo ) );

  pPort->watch_pipe = 1; // != 0

  pVBlankInfo->type = VBLANK_INFO_PLANE;
  pVBlankInfo->data = (void*) pPort;

  EXYNOSDispInfoVblankHandler( 1, 1, 1, 1, (void*) pVBlankInfo ); // if segmentation fault -- fail

  fail_if( pPort->watch_pipe != 0, "work flow is wrong (changed) -- %d", pPort->watch_pipe );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( EXYNOSDispInfoVblankHandler_mem_test_4_1 )
{
  EXYNOSVBlankInfoPtr pVBlankInfo = ctrl_calloc( 1, sizeof(EXYNOSVBlankInfoRec) );
  IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo ) );

  pPort->watch_pipe = 1; // != 0

  pVBlankInfo->type = VBLANK_INFO_PLANE;
  pVBlankInfo->data = (void*) pPort;

  EXYNOSDispInfoVblankHandler( 1, 1, 1, 1, (void*) pVBlankInfo ); // if segmentation fault -- fail

  fail_if( ctrl_get_cnt_calloc() != 1, "errors with calloc/free operations -- %d", ctrl_get_cnt_calloc() );
}
END_TEST;

//------------------------------------------ EXYNOSDispInfoIppHandler() -------------------------------------
START_TEST( EXYNOSDispInfoIppHandler_mem_test_1 )
{
  // if segmentation fault --> fail
  EXYNOSDispInfoIppHandler( 1, 1, NULL, 1, 1, NULL );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( EXYNOSDispInfoIppHandler_mem_test_2 )
{
  unsigned int buf_idx[10] = { 0, };
  int prop_id = -1;
  //keep_clone == NULL, because of it is global variable

  // if segmentation fault --> fail
  EXYNOSDispInfoIppHandler( 1, prop_id, buf_idx, 1, 1, NULL );

  fail_if( *buf_idx != 11, "work flow is wrong (changed) -- %d", *buf_idx );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( EXYNOSDispInfoIppHandler_mem_test_3 )
{
  unsigned int buf_idx[10] = { 0, };
  int prop_id = 0; // !-1
  //keep_clone == NULL, because of it is global variable

  // if segmentation fault --> fail
  EXYNOSDispInfoIppHandler( 1, prop_id, buf_idx, 1, 1, NULL );

  fail_if( *buf_idx != 11, "work flow is wrong (changed) -- %d", *buf_idx );
}
END_TEST;

//--------------------------------------- EXYNOSDispInfoPageFlipHandler() -----------------------------------
START_TEST( EXYNOSDispInfoPageFlipHandler_mem_test_1 )
{
  // if segmentation fault --> fail
  EXYNOSDispInfoPageFlipHandler( 1, 1, 1, 1, NULL );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( EXYNOSDispInfoPageFlipHandler_mem_test_2 )
{
  EXYNOSPageFlipPtr pPageFlip = ctrl_calloc( 1, sizeof( EXYNOSPageFlipRec ) );

  // if segmentation fault --> fail (pPageFlip->pCrtc == NULL)
  EXYNOSDispInfoPageFlipHandler( 1, 1, 1, 1, (void*) pPageFlip );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( EXYNOSDispInfoPageFlipHandler_mem_test_3 )
{
  EXYNOSPageFlipPtr pPageFlip = ctrl_calloc( 1, sizeof(EXYNOSPageFlipRec) );

  pPageFlip->pCrtc = ctrl_calloc( 1, sizeof(xf86CrtcRec) );

  // if segmentation fault --> fail (pPageFlip->pCrtc->driver_private == NULL)
  EXYNOSDispInfoPageFlipHandler( 1, 1, 1, 1, (void*) pPageFlip );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( EXYNOSDispInfoPageFlipHandler_mem_test_4 )
{
  EXYNOSPageFlipPtr pPageFlip = ctrl_calloc( 1, sizeof(EXYNOSPageFlipRec) );

  pPageFlip->pCrtc = ctrl_calloc( 1, sizeof(xf86CrtcRec) );

  pPageFlip->pCrtc->driver_private = ctrl_calloc( 1, sizeof(EXYNOSCrtcPrivRec) );

  // if segmentation fault --> fail (pPageFlip->pCrtc->driver_private->pDispInfo == NULL)
  EXYNOSDispInfoPageFlipHandler( 1, 1, 1, 1, (void*) pPageFlip );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( EXYNOSDispInfoPageFlipHandler_mem_test_5 )
{
  EXYNOSCrtcPrivPtr pCrtcPriv = NULL;
  EXYNOSPageFlipPtr pPageFlip = ctrl_calloc( 1, sizeof(EXYNOSPageFlipRec) );

  pPageFlip->pCrtc = ctrl_calloc( 1, sizeof(xf86CrtcRec) );

  pPageFlip->pCrtc->driver_private = ctrl_calloc( 1, sizeof(EXYNOSCrtcPrivRec) );

  pCrtcPriv = pPageFlip->pCrtc->driver_private;
  pCrtcPriv->pDispInfo = ctrl_calloc( 1, sizeof(EXYNOSDispInfoRec) );

  // if segmentation fault --> fail (pCrtcPriv->old_fb_id is not correct)
  EXYNOSDispInfoPageFlipHandler( 1, 1, 1, 1, (void*) pPageFlip );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( EXYNOSDispInfoPageFlipHandler_logic_test_1 )
{
  unsigned int frame = 9;
  unsigned int tv_sec = 11;
  unsigned int tv_usec = 25;

  pCrtcPriv_gl->flip_count = 2;
  pPageFlip_gl->dispatch_me = TRUE;

  EXYNOSDispInfoPageFlipHandler( 1, frame, tv_sec, tv_usec, (void*)pPageFlip_gl );

  fail_if( pCrtcPriv_gl->fe_frame   != frame,   "frame was not set" );
  fail_if( pCrtcPriv_gl->fe_tv_sec  != tv_sec,  "tv_sec was not set" );
  fail_if( pCrtcPriv_gl->fe_tv_usec != tv_usec, "tv_usec was not set" );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( EXYNOSDispInfoPageFlipHandler_logic_test_2 )
{
  unsigned int frame = 9;
  unsigned int tv_sec = 11;
  unsigned int tv_usec = 25;

  pCrtcPriv_gl->fe_frame = 10;
  pCrtcPriv_gl->fe_tv_sec = 12;
  pCrtcPriv_gl->fe_tv_usec = 26;
  pCrtcPriv_gl->flip_count = 2;
  pPageFlip_gl->dispatch_me = FALSE;

  EXYNOSDispInfoPageFlipHandler( 1, frame, tv_sec, tv_usec, (void*)pPageFlip_gl );

  fail_if( pCrtcPriv_gl->fe_frame   == frame,   "frame was set" );
  fail_if( pCrtcPriv_gl->fe_tv_sec  == tv_sec,  "tv_sec was set" );
  fail_if( pCrtcPriv_gl->fe_tv_usec == tv_usec, "tv_usec was set" );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( EXYNOSDispInfoPageFlipHandler_logic_test_3 )
{
  pCrtcPriv_gl->pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );

  pCrtcPriv_gl->flip_count = 1;
  pPageFlip_gl->dispatch_me = FALSE;
  pCrtcPriv_gl->event_data = (DRI2FrameEventPtr) ctrl_calloc( 1, sizeof( DRI2FrameEventRec ) );

  DRI2FrameEventPtr pEvent = pCrtcPriv_gl->event_data;

  // maybe this is very strange
  pCrtcPriv_gl->old_fb_id = _add_fb();

  EXYNOSDispInfoPageFlipHandler( 1, 1, 1, 1, (void*) pPageFlip_gl );

  fail_if( cnt_fb_check_refs() != 0, "drm fb error" );

  ctrl_free( pCrtcPriv_gl->pDispInfo );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( EXYNOSDispInfoPageFlipHandler_logic_test_3_1 )
{
  pCrtcPriv_gl->pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );

  pCrtcPriv_gl->flip_count = 1;
  pPageFlip_gl->dispatch_me = FALSE;
  pCrtcPriv_gl->event_data = (DRI2FrameEventPtr) ctrl_calloc( 1, sizeof( DRI2FrameEventRec ) );

  DRI2FrameEventPtr pEvent = pCrtcPriv_gl->event_data;

  // maybe this is very strange
  pCrtcPriv_gl->old_fb_id = _add_fb();

  EXYNOSDispInfoPageFlipHandler( 1, 1, 1, 1, (void*) pPageFlip_gl );

  fail_if( pEvent->pipe != 11, "work flow is wrong (changed) -- %d", pEvent->pipe );

  ctrl_free( pCrtcPriv_gl->pDispInfo );
}
END_TEST;

//---------------------------------------  EXYNOSDispInfoWakeupHanlder() ------------------------------------
START_TEST( EXYNOSDispInfoWakeupHanlder_mem_test_1 )
{
  // if segmentation fault --> fail
  EXYNOSDispInfoWakeupHanlder( NULL, 1, NULL );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( EXYNOSDispInfoWakeupHanlder_mem_test_2 )
{
  EXYNOSDispInfoRec DispInfo;

  // if segmentation fault --> fail
  EXYNOSDispInfoWakeupHanlder( &DispInfo, 1, NULL );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( EXYNOSDispInfoWakeupHanlder_mem_test_3 )
{
  EXYNOSDispInfoRec DispInfo;
  fd_set read_mask;

  // add file descriptor 0 to read_mask
  FD_ZERO( &read_mask );
  FD_SET( 0, &read_mask );

  // FD_ISSET return 0, in this case, so _exynosDisplayHandleDrmEvent() will not be called
  // and function read (in _exynosDisplayHandleDrmEvent()) will not crash test application,
  // because of absence hardware driver
  DispInfo.drm_fd = 1;

  // if process will be killed --> fail
  EXYNOSDispInfoWakeupHanlder( &DispInfo, 1, &read_mask );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( EXYNOSDispInfoWakeupHanlder_logic_test_1 )
{
  EXYNOSDispInfoRec DispInfo;

  // if segmentation fault --> fail (third parameter is not used in this case (second parameter less then zero))
  EXYNOSDispInfoWakeupHanlder( &DispInfo, -1, NULL );
}
END_TEST;

//---------------------------------------  exynosDisplayLoadPalette() ------------------------------------
START_TEST( exynosDisplayLoadPalette_mem_test_1 )
{
  // if segmentation fault --> fail
  exynosDisplayLoadPalette( NULL, 1, NULL, NULL, NULL );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( exynosDisplayLoadPalette_mem_test_2 )
{
    ScrnInfoRec scrn;

    scrn.privates = NULL;

    // if segmentation fault --> fail (macro XF86_CRTC_CONFIG_PTR is failed, p->privates == NULL)
    exynosDisplayLoadPalette( &scrn, 1, NULL, NULL, NULL );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( exynosDisplayLoadPalette_mem_test_3 )
{
    ScrnInfoRec scrn;

    // xf86CrtcConfigPrivateIndex = 0, so first parameter of ctrl_calloc is 1
    scrn.privates = ctrl_calloc( 1, sizeof( DevUnion ) );

    scrn.privates->ptr = NULL;

    // if segmentation fault --> fail (p->privates->ptr == NULL, so pCrtcConfig == NULL)
    exynosDisplayLoadPalette( &scrn, 1, NULL, NULL, NULL );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( exynosDisplayLoadPalette_mem_test_4 )
{
    ScrnInfoRec scrn;
    xf86CrtcConfigPtr pCrtcConfig = NULL;

    // xf86CrtcConfigPrivateIndex = 0, so first parameter of ctrl_calloc is 1
    scrn.privates = ctrl_calloc( 1, sizeof( DevUnion ) );
    scrn.privates->ptr = ctrl_calloc( 1, sizeof( xf86CrtcConfigRec ) );

    pCrtcConfig = scrn.privates->ptr;

    pCrtcConfig->num_crtc = 1;
    pCrtcConfig->crtc = NULL;
    scrn.depth = 16; // case 16:

    // if segmentation fault --> fail (indices = NULL)
    exynosDisplayLoadPalette( &scrn, 1, NULL, NULL, NULL );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( exynosDisplayLoadPalette_mem_test_5 )
{
    ScrnInfoRec scrn;
    xf86CrtcConfigPtr pCrtcConfig = NULL;
	int indices[1] = {0}; // less then 32

    // xf86CrtcConfigPrivateIndex = 0, so first parameter of ctrl_calloc is 1
    scrn.privates = ctrl_calloc( 1, sizeof( DevUnion ) );
    scrn.privates->ptr = ctrl_calloc( 1, sizeof( xf86CrtcConfigRec ) );

    pCrtcConfig = scrn.privates->ptr;

    pCrtcConfig->num_crtc = 1;
    pCrtcConfig->crtc = NULL;
    scrn.depth = 16; // case 16:

    // if segmentation fault --> fail (colors = NULL)
    exynosDisplayLoadPalette( &scrn, 1, indices, NULL, NULL );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( exynosDisplayLoadPalette_mem_test_6 )
{
    ScrnInfoRec scrn;
    xf86CrtcConfigPtr pCrtcConfig = NULL;
	int indices[1] = {0}; // less then 32
	LOCO colors[1];

    // xf86CrtcConfigPrivateIndex = 0, so first parameter of ctrl_calloc is 1
    scrn.privates = ctrl_calloc( 1, sizeof( DevUnion ) );
    scrn.privates->ptr = ctrl_calloc( 1, sizeof( xf86CrtcConfigRec ) );

    pCrtcConfig = scrn.privates->ptr;

    pCrtcConfig->num_crtc = 1;
    pCrtcConfig->crtc = NULL;
    scrn.depth = 16; // case 16:

    // if segmentation fault --> fail (pCrtcConfig->crtc == NULL)
    exynosDisplayLoadPalette( &scrn, 1, indices, colors, NULL );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( exynosDisplayLoadPalette_logic_test_1 )
{
    ScrnInfoRec scrn;
    xf86CrtcConfigPtr pCrtcConfig = NULL;
	int indices[1] = {1}; // less then 32
	LOCO colors[1];

    // xf86CrtcConfigPrivateIndex = 0, so first parameter of ctrl_calloc is 1
    scrn.privates = ctrl_calloc( 1, sizeof( DevUnion ) );
    scrn.privates->ptr = ctrl_calloc( 1, sizeof( xf86CrtcConfigRec ) );

    pCrtcConfig = scrn.privates->ptr;

    pCrtcConfig->num_crtc = 1;
    pCrtcConfig->crtc = ctrl_calloc( 1, sizeof( xf86CrtcPtr ) );  // not xf86CrtcRec !!!
    pCrtcConfig->crtc[0] = ctrl_calloc( 1, sizeof( xf86CrtcRec ) );
    scrn.depth = 16; // case 16:

    // if segmentation fault --> fail
    exynosDisplayLoadPalette( &scrn, 1, indices, colors, NULL );

    //  sizeof( colors[]/sizeof(LOGO) ) == 1, but  indices[1] == 2
    fail( "Buffer overflow (acses to color[1])" );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( exynosDisplayLoadPalette_logic_test_2 )
{
    ScrnInfoRec scrn;
    xf86CrtcConfigPtr pCrtcConfig = NULL;
	int indices[1] = {256}; // sizeof( lut_r[256]/sizeof(uint16_t) ) == 256
	LOCO colors[257];  // sizeof( lut_r[256]/sizeof(uint16_t) ) == 256

    // xf86CrtcConfigPrivateIndex = 0, so first parameter of ctrl_calloc is 1
    scrn.privates = ctrl_calloc( 1, sizeof( DevUnion ) );
    scrn.privates->ptr = ctrl_calloc( 1, sizeof( xf86CrtcConfigRec ) );

    pCrtcConfig = scrn.privates->ptr;

    pCrtcConfig->num_crtc = 1;
    pCrtcConfig->crtc = ctrl_calloc( 1, sizeof( xf86CrtcPtr ) );  // not xf86CrtcRec !!!
    pCrtcConfig->crtc[0] = ctrl_calloc( 1, sizeof( xf86CrtcRec ) );
    scrn.depth = 0; // !16

    // if segmentation fault --> fail
    exynosDisplayLoadPalette( &scrn, 1, indices, colors, NULL );

    fail( "Buffer overflow (acses to lut_r[256])" );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( exynosDisplayLoadPalette_logic_test_3 )
{
    ScrnInfoRec scrn;
    xf86CrtcConfigPtr pCrtcConfig = NULL;
	int indices[1] = {200}; // 200*4 + 0 > sizeof( lut_g[256]/sizeof(uint16_t) )
	LOCO colors[201];

    // xf86CrtcConfigPrivateIndex = 0, so first parameter of ctrl_calloc is 1
    scrn.privates = ctrl_calloc( 1, sizeof( DevUnion ) );
    scrn.privates->ptr = ctrl_calloc( 1, sizeof( xf86CrtcConfigRec ) );

    pCrtcConfig = scrn.privates->ptr;

    pCrtcConfig->num_crtc = 1;
    pCrtcConfig->crtc = ctrl_calloc( 1, sizeof( xf86CrtcPtr ) );  // not xf86CrtcRec !!!
    pCrtcConfig->crtc[0] = ctrl_calloc( 1, sizeof( xf86CrtcRec ) );
    scrn.depth = 16; // case 16:

    // if segmentation fault --> fail
    exynosDisplayLoadPalette( &scrn, 1, indices, colors, NULL );

    fail( "Buffer overflow (acses to lut_g[800...])" );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( exynosDisplayLoadPalette_logic_test_4 )
{
    ScrnInfoRec scrn;
    xf86CrtcConfigPtr pCrtcConfig = NULL;
	int indices[1] = {0}; // less then 32
	LOCO colors[1];

    // xf86CrtcConfigPrivateIndex = 0, so first parameter of ctrl_calloc is 1
    scrn.privates = ctrl_calloc( 1, sizeof( DevUnion ) );
    scrn.privates->ptr = ctrl_calloc( 1, sizeof( xf86CrtcConfigRec ) );

    pCrtcConfig = scrn.privates->ptr;

    pCrtcConfig->num_crtc = 1;
    pCrtcConfig->crtc = ctrl_calloc( 1, sizeof( xf86CrtcPtr ) );  // not xf86CrtcRec !!!
    pCrtcConfig->crtc[0] = ctrl_calloc( 1, sizeof( xf86CrtcRec ) );
    scrn.depth = 16; // case 16:

    // if segmentation fault --> fail
    exynosDisplayLoadPalette( &scrn, 1, indices, colors, NULL );
}
END_TEST;

//---------------------------------------  exynosDisplayModeFromKmode() ------------------------------------
START_TEST( exynosDisplayModeFromKmode_mem_test_1 )
{
  // if segmentation fault --> fail
  exynosDisplayModeFromKmode( NULL, NULL, NULL );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( exynosDisplayModeFromKmode_mem_test_2 )
{
  ScrnInfoPtr  p_scrn_info = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );

  // if segmentation fault --> fail (pMode == NULL)
  exynosDisplayModeFromKmode( p_scrn_info, NULL, NULL );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( exynosDisplayModeFromKmode_mem_test_3 )
{
  ScrnInfoPtr  p_scrn_info = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
  DisplayModePtr     pMode = ctrl_calloc( 1, sizeof( DisplayModeRec ) );

  // if segmentation fault --> fail (kmode == NULL)
  exynosDisplayModeFromKmode( p_scrn_info, NULL, pMode );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( exynosDisplayModeFromKmode_mem_test_4 )
{
  ScrnInfoPtr  p_scrn_info = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
  drmModeModeInfoPtr kmode = ctrl_calloc( 1, sizeof( drmModeModeInfo ) );
  DisplayModePtr     pMode = ctrl_calloc( 1, sizeof( DisplayModeRec ) );

  exynosDisplayModeFromKmode(  p_scrn_info, kmode, pMode );

  ctrl_free(p_scrn_info);
  ctrl_free(kmode);
  ctrl_free(pMode);

  fail_if( ctrl_get_cnt_calloc() != 1, "errors with calloc/free operations -- %d", ctrl_get_cnt_calloc() );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( exynosDisplayModeFromKmode_logic_test_1 )
{
  ScrnInfoPtr  p_scrn_info = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
  drmModeModeInfoPtr kmode = ctrl_calloc( 1, sizeof( drmModeModeInfo ) );
  DisplayModePtr     pMode = ctrl_calloc( 1, sizeof( DisplayModeRec ) );

  exynosDisplayModeFromKmode( p_scrn_info, kmode, pMode );

  fail_if( pMode->status     != MODE_OK,            "pMode->status     != MODE_OK" );
  fail_if( pMode->Clock      != kmode->clock,       "pMode->Clock      != kmode->clock" );
  fail_if( pMode->HDisplay   != kmode->hdisplay,    "pMode->HDisplay   != kmode->hdisplay" );
  fail_if( pMode->HSyncStart != kmode->hsync_start, "pMode->HSyncStart != kmode->hsync_start" );
  fail_if( pMode->HSyncEnd   != kmode->hsync_end,   "pMode->HSyncEnd   != kmode->hsync_end" );
  fail_if( pMode->HTotal     != kmode->htotal,      "pMode->HTotal     != kmode->htotal" );
  fail_if( pMode->HSkew      != kmode->hskew,       "pMode->HSkew      != kmode->hskew" );
  fail_if( pMode->VDisplay   != kmode->vdisplay,    "pMode->VDisplay   != kmode->vdisplay" );
  fail_if( pMode->VSyncStart != kmode->vsync_start, "pMode->VSyncStart != kmode->vsync_start" );
  fail_if( pMode->VSyncEnd   != kmode->vsync_end,   "pMode->VSyncEnd   != kmode->vsync_end");
  fail_if( pMode->VTotal     != kmode->vtotal,      "pMode->VTotal     != kmode->vtotal" );
  fail_if( pMode->VScan      != kmode->vscan,       "pMode->VScan      != kmode->vscan" );
  fail_if( pMode->Flags      != kmode->flags,       "pMode->Flags      != kmode->flags" );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( exynosDisplayModeFromKmode_logic_test_2 )
{
  ScrnInfoPtr  p_scrn_info = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
  drmModeModeInfoPtr kmode = ctrl_calloc( 1, sizeof( drmModeModeInfo ) );
  DisplayModePtr     pMode = ctrl_calloc( 1, sizeof( DisplayModeRec ) );

  // DRM_MODE_TYPE_DRIVER =  1 << 6
  // DRM_MODE_TYPE_PREFERRED 1 << 3
  kmode->type = DRM_MODE_TYPE_DRIVER;

  exynosDisplayModeFromKmode( p_scrn_info, kmode, pMode );

  fail_if( pMode->type != M_T_DRIVER, "pMode->type != M_T_DRIVER" );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( exynosDisplayModeFromKmode_logic_test_3 )
{
  ScrnInfoPtr  p_scrn_info = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
  drmModeModeInfoPtr kmode = ctrl_calloc( 1, sizeof( drmModeModeInfo ) );
  DisplayModePtr     pMode = ctrl_calloc( 1, sizeof( DisplayModeRec ) );

  // DRM_MODE_TYPE_DRIVER =  1 << 6
  // DRM_MODE_TYPE_PREFERRED 1 << 3
  // M_T_PREFERRED = 0x08
  kmode->type = DRM_MODE_TYPE_PREFERRED;

  // any digital, except 0x08 (M_T_PREFERRED), to check logical OR
  pMode->type = 1;

  exynosDisplayModeFromKmode( p_scrn_info, kmode, pMode );

  fail_if( pMode->type != (M_T_PREFERRED | pMode->type), "pMode->type set error" );
}
END_TEST;

//---------------------------------------  exynosDisplayModeToKmode() ------------------------------------
START_TEST( DisplayModeToKmode_passed_null_ptr_fail_1 )
{
	// if segmentation fault --> fail (kmode == NULL)
	exynosDisplayModeToKmode( NULL, NULL, NULL );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( DisplayModeToKmode_passed_null_ptr_fail_2 )
{
	drmModeModeInfoPtr kmode = ctrl_calloc( 1, sizeof( drmModeModeInfo ) );

	// if segmentation fault --> fail (pmode == NULL)
	exynosDisplayModeToKmode( NULL, kmode, NULL );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( DisplayModeToKmode_pmode_name_null_fail )
{
	drmModeModeInfoPtr kmode = ctrl_calloc( 1, sizeof( drmModeModeInfo ) );
	DisplayModePtr     pMode = ctrl_calloc( 1, sizeof( DisplayModeRec ) );

	pMode->name = NULL;

	// if segmentation fault --> fail
	exynosDisplayModeToKmode( NULL, kmode, pMode );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( DisplayModeToKmode_work_flow_success_1 )
{
	drmModeModeInfoPtr kmode = ctrl_calloc( 1, sizeof( drmModeModeInfo ) );
	DisplayModePtr     pMode = ctrl_calloc( 1, sizeof( DisplayModeRec ) );

	pMode->name = NULL;

	// if segmentation fault --> fail
	exynosDisplayModeToKmode( NULL, kmode, pMode );

	fail_if( kmode->name[DRM_DISPLAY_MODE_LEN-1] != 0, "work flow is wrong (changed)" );
}
END_TEST;


//-------------------------------------------------------------------------------------
START_TEST( DisplayModeToKmode_work_flow_success_2 )
{
	drmModeModeInfoPtr kmode = ctrl_calloc( 1, sizeof( drmModeModeInfo ) );
	DisplayModePtr     pMode = ctrl_calloc( 1, sizeof( DisplayModeRec ) );

	// kmode->name != NULL  !!! (name is static array)
	pMode->name = ctrl_calloc( 1, DRM_DISPLAY_MODE_LEN );

	pMode->name[0] = '1';
	pMode->name[1] = '1';
	pMode->name[2] = '0';
	pMode->name[3] = '9';
	pMode->name[DRM_DISPLAY_MODE_LEN-1] = 1; // !0

	// if segmentation fault --> fail
	exynosDisplayModeToKmode( NULL, kmode, pMode );

	fail_if( pMode->name[0] != kmode->name[0], "work flow is wrong (changed)" );
	fail_if( pMode->name[1] != kmode->name[1], "work flow is wrong (changed)" );
	fail_if( pMode->name[2] != kmode->name[2], "work flow is wrong (changed)" );
	fail_if( pMode->name[3] != kmode->name[3], "work flow is wrong (changed)" );

	fail_if( kmode->name[DRM_DISPLAY_MODE_LEN-1] != 0, "work flow is wrong (changed)" );
}
END_TEST;

//---------------------------------------  exynosCrtcOn() ------------------------------------
START_TEST( exynosCrtcOn_passed_null_ptr_fail_1 )
{
	// if segmentation fault --> fail (pCrtc == NULL)
    exynosCrtcOn( NULL );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( exynosCrtcOn_work_flow_1 )
{
	ScrnInfoPtr pScrn = NULL;
	xf86CrtcPtr pCrtc = NULL;
    xf86CrtcConfigPtr pCrtcConfig = NULL;
    Bool res = TRUE;

	pCrtc = ctrl_calloc( 1, sizeof( xf86CrtcRec ) );
	pCrtc->scrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );

    pScrn = pCrtc->scrn;

    // xf86CrtcConfigPrivateIndex = 0, so first parameter of ctrl_calloc is 1
    pScrn->privates = ctrl_calloc( 1, sizeof( DevUnion ) );
    pScrn->privates->ptr = ctrl_calloc( 1, sizeof( xf86CrtcConfigRec ) );

    pCrtcConfig = pScrn->privates->ptr;

    //------------

    pCrtc->enabled = FALSE;  // to return FALSE

	// if segmentation fault --> fail
    res = exynosCrtcOn( pCrtc );

    fail_if( res != FALSE, "work flow is wrong (changed) -- %d", res );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( exynosCrtcOn_work_flow_2 )
{
	ScrnInfoPtr pScrn = NULL;
	xf86CrtcPtr pCrtc = NULL;
    xf86CrtcConfigPtr pCrtcConfig = NULL;
    Bool res = TRUE;

	pCrtc = ctrl_calloc( 1, sizeof( xf86CrtcRec ) );
	pCrtc->scrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );

    pScrn = pCrtc->scrn;

    // xf86CrtcConfigPrivateIndex = 0, so first parameter of ctrl_calloc is 1
    pScrn->privates = ctrl_calloc( 1, sizeof( DevUnion ) );
    pScrn->privates->ptr = ctrl_calloc( 1, sizeof( xf86CrtcConfigRec ) );

    pCrtcConfig = pScrn->privates->ptr;

    //------------

    pCrtc->enabled = TRUE;  // !FALSE
    pCrtcConfig->num_output = 0; // to bypass for loop

	// if segmentation fault --> fail
    res = exynosCrtcOn( pCrtc );

    fail_if( res != TRUE, "work flow is wrong (changed) -- %d", res );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( exynosCrtcOn_work_flow_3 )
{
	ScrnInfoPtr pScrn = NULL;
	xf86CrtcPtr pCrtc = NULL;
    xf86CrtcConfigPtr pCrtcConfig = NULL;
    xf86OutputPtr pOutput = NULL;
    Bool res = TRUE;

	pCrtc = ctrl_calloc( 1, sizeof( xf86CrtcRec ) );
	pCrtc->scrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );

    pScrn = pCrtc->scrn;

    // xf86CrtcConfigPrivateIndex = 0, so first parameter of ctrl_calloc is 1
    pScrn->privates = ctrl_calloc( 1, sizeof( DevUnion ) );
    pScrn->privates->ptr = ctrl_calloc( 1, sizeof( xf86CrtcConfigRec ) );

    pCrtcConfig = pScrn->privates->ptr;

    //------------

    pCrtc->enabled = TRUE;  // !FALSE
    pCrtcConfig->num_output = 1;// to enter in for loop

    pCrtcConfig->output = ctrl_calloc( pCrtcConfig->num_output, sizeof( xf86OutputPtr ) );  // not xf86OutputRec !!!
    pCrtcConfig->output[0] = ctrl_calloc( 1, sizeof( xf86OutputRec ) );

    pOutput = pCrtcConfig->output[0];

    pOutput->crtc = pCrtc;  // to pass if condition

	// if segmentation fault --> fail
    res = exynosCrtcOn( pCrtc );

    fail_if( res != TRUE, "work flow is wrong (changed) -- %d", res );
}
END_TEST;

//---------------------------------------  exynosDisplayCoveringCrtc() ------------------------------------
START_TEST( exynosDisplayCoveringCrtc_passed_null_ptr_fail_1 )
{
	/*
	ScrnInfoPtr pScrn = NULL;
	BoxPtr pBox = NULL;
	xf86CrtcPtr pDesiredCrtc = NULL;
	BoxPtr pBoxCrtc = NULL;


	pBox = ctrl_calloc( 1, sizeof( BoxRec ) );
	pDesiredCrtc = ctrl_calloc( 1, sizeof( xf86CrtcRec ) );
	pBoxCrtc = ctrl_calloc( 1, sizeof( BoxRec ) );

	ScrnInfoPtr pScrn = NULL;
	xf86CrtcPtr pCrtc = NULL;
    xf86CrtcConfigPtr pCrtcConfig = NULL;
    xf86OutputPtr pOutput = NULL;
    Bool res = TRUE;

	pCrtc = ctrl_calloc( 1, sizeof( xf86CrtcRec ) );
	pCrtc->scrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );

    pScrn = pCrtc->scrn;

    // xf86CrtcConfigPrivateIndex = 0, so first parameter of ctrl_calloc is 1
    pScrn->privates = ctrl_calloc( 1, sizeof( DevUnion ) );
    pScrn->privates->ptr = ctrl_calloc( 1, sizeof( xf86CrtcConfigRec ) );

    pCrtcConfig = pScrn->privates->ptr;

    //------------

	// if segmentation fault --> fail (pCrtcConfig == NULL)
	exynosDisplayCoveringCrtc( pScrn, pBox, pDesiredCrtc, pBoxCrtc );
	*/
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( exynosDisplayCoveringCrtc_passed_null_ptr_fail_2 )
{

}
END_TEST;

//---------------------------------------  displaySetDispConnMode() ------------------------------------
START_TEST( displaySetDispConnMode_passed_null_ptr_fail_1 )
{
	EXYNOS_DisplayConnMode conn_mode = DISPLAY_CONN_MODE_NONE;

	// if segmentation fault --> fail (pScrn == NULL)
	displaySetDispConnMode( NULL, conn_mode );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( displaySetDispConnMode_passed_null_ptr_fail_2 )
{
	EXYNOS_DisplayConnMode conn_mode = DISPLAY_CONN_MODE_NONE;
	ScrnInfoPtr  pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );

	// if segmentation fault --> fail (pScrn->driverprivate == NULL)
	displaySetDispConnMode( pScrn, conn_mode );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( displaySetDispConnMode_passed_null_ptr_fail_3 )
{
	EXYNOS_DisplayConnMode conn_mode = DISPLAY_CONN_MODE_NONE;
	ScrnInfoPtr  pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );

	// if segmentation fault --> fail (pScrn->driverprivate->pDispInfo == NULL)
	displaySetDispConnMode( pScrn, conn_mode );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( displaySetDispConnMode_work_flow_success_1 )
{
	EXYNOS_DisplayConnMode conn_mode = DISPLAY_CONN_MODE_HDMI;
	ScrnInfoPtr  pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );

	EXYNOSPtr exynos_ptr = pScrn->driverPrivate;

	exynos_ptr->pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );

	// if segmentation fault --> fail
	Bool res = displaySetDispConnMode( pScrn, conn_mode );

	fail_if( exynos_ptr->pDispInfo->conn_mode != DISPLAY_CONN_MODE_HDMI, "work flow is wrong (changed)" );
	fail_if( res != TRUE, "work flow is wrong (changed)" );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( displaySetDispConnMode_work_flow_success_2 )
{
	EXYNOS_DisplayConnMode conn_mode = DISPLAY_CONN_MODE_HDMI;
	ScrnInfoPtr  pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );

	EXYNOSPtr exynos_ptr = pScrn->driverPrivate;

	exynos_ptr->pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );
	exynos_ptr->pDispInfo->conn_mode = DISPLAY_CONN_MODE_HDMI;

	// if segmentation fault --> fail
	Bool res = displaySetDispConnMode( pScrn, conn_mode );

	fail_if( exynos_ptr->pDispInfo->conn_mode != DISPLAY_CONN_MODE_HDMI, "work flow is wrong (changed)" );
	fail_if( res != TRUE, "work flow is wrong (changed)" );
}
END_TEST;


//---------------------------------------  _displayGetFreeCrtcID() ------------------------------------
START_TEST( _displayGetFreeCrtcID_passed_null_ptr_fail_1 )
{
	// if segmentation fault --> fail (pScrn == NULL)
	_displayGetFreeCrtcID( NULL );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _displayGetFreeCrtcID_passed_null_ptr_fail_2 )
{
	EXYNOSDispInfoPtr pExynosMode = NULL;
	ScrnInfoPtr  pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );

	EXYNOSPtr exynos_ptr = pScrn->driverPrivate;

	exynos_ptr->pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );
	pExynosMode = exynos_ptr->pDispInfo;

	// if segmentation fault --> fail ( pExynosMode->mode_res == NULL)
	_displayGetFreeCrtcID( pScrn );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _displayGetFreeCrtcID_passed_null_ptr_fail_3 )
{
	pExynosMode_gl->mode_res->count_crtcs = 1; // to enter in loop

	// if segmentation fault --> fail ( pExynosMode->mode_res->crtcs == NULL)
	_displayGetFreeCrtcID( pScrn_gl );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _displayGetFreeCrtcID_buffer_overflow_fail_1 )
{

	// Attention: first parameter is set to 1
	// this mean, that we have array crtcs[] with one element
	pExynosMode_gl->mode_res->crtcs = ctrl_calloc( 1, sizeof( uint32_t ) );

	// Attention: count_crtcs is set to 2,
	// but count_crtcs storages number of elememts of crtcs[], see above
	pExynosMode_gl->mode_res->count_crtcs = 2;

	// if segmentation fault --> fail
	_displayGetFreeCrtcID( pScrn_gl );

	fail( "Buffer overflow (access to crtcs[1])" );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _displayGetFreeCrtcID_work_flow_success_1 )
{
	int res = 0;

	//pExynosMode->mode_res->count_crtcs is set to 0

	// if segmentation fault --> fail
	res = _displayGetFreeCrtcID( pScrn_gl );

	fail_if( res != 0, "work flow is wrong (changed)" );
}
END_TEST;



//-------------------------------------------------------------------------------------
START_TEST( _displayGetFreeCrtcID_work_flow_success_2 )
{
	int res = 0;

	// count_crtcs is size of crtcs[] array
	pExynosMode_gl->mode_res->count_crtcs = 1; // to enter in loop
	pExynosMode_gl->mode_res->crtcs = ctrl_calloc( 1, sizeof( uint32_t ) );

	pExynosMode_gl->drm_fd = 0; // _displayGetFreeCrtcID() must return 0

	// if segmentation fault --> fail
	res = _displayGetFreeCrtcID( pScrn_gl );

	fail_if( res != 0, "work flow is wrong (changed)" );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _displayGetFreeCrtcID_work_flow_success_3 )
{
	int res = 0;
	pExynosMode_gl->drm_fd = 1; // _displayGetFreeCrtcID() must return 0 or crtc_id

	// --- setup fake-function`s variables
	// all kcrtc->buffer_id > 0
	/*TODO insert into create_crtc*/
	//get_drm_mode_crtc()->buffer_id = 1; // 1 > 0
	create_crtc(5);

	// if segmentation fault --> fail
	res = _displayGetFreeCrtcID( pScrn_gl );

	fail_if( res != 0, "work flow is wrong (changed)" );
	fail_if( get_crtc_cnt() != 0, "work flow is wrong (changed) -- %d", get_crtc_cnt() );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _displayGetFreeCrtcID_work_flow_success_4 )
{
	int res = 0;

	pExynosMode_gl->drm_fd = 1; // _displayGetFreeCrtcID() must return 0 or crtc_id

	// --- setup fake-function`s variables
	// all kcrtc->buffer_id > 0
	/*TODO insert into create_crtc*/
	//get_drm_mode_crtc()->buffer_id = 0; // 0 !> 0

	// kcrtc->crtc_id = crtc_id
	//get_drm_mode_crtc()->crtc_id = crtc_id;

	create_crtc(1);

	// if segmentation fault --> fail
	res = _displayGetFreeCrtcID( pScrn_gl );

	fail_if( res != crtc_id, "work flow is wrong (changed) -- %d", res );
	fail_if( get_crtc_cnt() != 0, "work flow is wrong (changed) -- %d", get_crtc_cnt() );
}
END_TEST;

//---------------------------------------  _lookForOutputByConnType() ------------------------------------
START_TEST( _lookForOutputByConnType_passed_null_ptr_fail_1 )
{
	int connect_type = 0; // any digital

	// if segmentation fault --> fail (pScrn == NULL)
	_lookForOutputByConnType( NULL, connect_type );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _lookForOutputByConnType_work_flow_success_1 )
{
	int connect_type = 1; // any digital
	EXYNOSOutputPrivPtr p_output_priv = 1;  // !NULL

	// should not enter the loop
	pExynosMode_gl->mode_res->count_connectors = 0;

	// if segmentation fault --> fail
	p_output_priv = _lookForOutputByConnType( pScrn_gl, connect_type );

	fail_if( p_output_priv != NULL, "work flow is wrong (changed) -- %p", p_output_priv );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _lookForOutputByConnType_work_flow_success_2 )
{
	// any digital, except 11, see to ctrl_xorg_list_for_each_entry_safe define in exynos_mem_pool.h
	int connect_type = 2;
	EXYNOSOutputPrivPtr p_output_priv = 1;  // !NULL

	// should enter the loop
	pExynosMode_gl->mode_res->count_connectors = 1;

	// if segmentation fault --> fail ()
	p_output_priv = _lookForOutputByConnType( pScrn_gl, connect_type );

	fail_if( p_output_priv != NULL, "work flow is wrong (changed) -- %p", p_output_priv );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _lookForOutputByConnType_work_flow_success_3 )
{
	int connect_type = 11;	// see to ctrl_xorg_list_for_each_entry_safe define in exynos_mem_pool.h
	EXYNOSOutputPrivPtr p_output_priv = NULL;

	// should enter the loop
	pExynosMode_gl->mode_res->count_connectors = 1;

	// if segmentation fault --> fail ()
	p_output_priv = _lookForOutputByConnType( pScrn_gl, connect_type );

	fail_if( p_output_priv == NULL, "work flow is wrong (changed) -- %p", p_output_priv );
}
END_TEST;

//---------------------------------------  _fbUnrefBo() ------------------------------------
START_TEST( _fbUnrefBo_passed_null_ptr_fail )
{
	// if segmentation fault --> fail (pScrn == NULL)
	_fbUnrefBo( NULL );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _fbUnrefBo_work_flow_success )
{
	int res = 0;
	tbm_bo bo = tbm_bo_alloc( NULL, 0, 0 );  // 1-st parameter is NULL, so bo is not NULL too

	// if segmentation fault --> fail
	res = _fbUnrefBo( bo );

	fail_if( get_tbm_check_refs() != 0, "work flow is wrong (changed) -- %d", get_tbm_check_refs() );
	fail_if( res != TRUE, "work flow is wrong (changed) -- %d", res );
}
END_TEST;

//---------------------------------------  _fbFreeBoData() ------------------------------------
START_TEST( _fbFreeBoData_passed_null_ptr_fail_1 )
{
	// if segmentation fault --> fail (data == NULL)
	_fbFreeBoData( NULL );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _fbFreeBoData_passed_null_ptr_fail_2 )
{
	EXYNOS_FbBoDataPtr bo_data = ctrl_calloc( 1, sizeof( EXYNOS_FbBoDataRec ) );

	// if segmentation fault --> fail (bo_data->pScrn == NULL)
	_fbFreeBoData( ( void* )bo_data );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _fbFreeBoData_passed_null_ptr_fail_3 )
{
	EXYNOS_FbBoDataPtr bo_data = ctrl_calloc( 1, sizeof( EXYNOS_FbBoDataRec ) );
	bo_data->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );

	bo_data->fb_id = 1; // !0 -->  should entry the if block

	// if segmentation fault --> fail (pExynos == NULL)
	_fbFreeBoData( ( void* )bo_data );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _fbFreeBoData_passed_null_ptr_fail_4 )
{
	EXYNOS_FbBoDataPtr bo_data = ctrl_calloc( 1, sizeof( EXYNOS_FbBoDataRec ) );
	bo_data->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );

	//bo_data->fb_id = 0; 		// should not entry the if block
	bo_data->pPixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );

	// if segmentation fault --> fail (pScrn->pScreen == NULL)
	_fbFreeBoData( ( void* )bo_data );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _fbFreeBoData_passed_null_ptr_fail_5 )
{
	EXYNOS_FbBoDataPtr bo_data = ctrl_calloc( 1, sizeof( EXYNOS_FbBoDataRec ) );
	bo_data->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );

	//bo_data->fb_id = 0; 		// should not entry the if block
	bo_data->pPixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );

	bo_data->pScrn->pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );

	// if segmentation fault --> fail (pScrn->pScreen->DestroyPixmap == NULL)
	_fbFreeBoData( ( void* )bo_data );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _fbFreeBoData_work_flow_success_1 )
{
	EXYNOS_FbBoDataPtr bo_data = ctrl_calloc( 1, sizeof( EXYNOS_FbBoDataRec ) );
	bo_data->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );

	//bo_data->fb_id = 0; 		// should not entry the if block
	//bo_data->pPixmap = NULL; 	// should not entry the if block

	// if segmentation fault --> fail
	_fbFreeBoData( ( void* )bo_data );

	// _fbFreeBoData resolves bo_data
	fail_if( ctrl_get_cnt_calloc() != 1, "errors with calloc/free operations -- %d", ctrl_get_cnt_calloc() );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _fbFreeBoData_work_flow_success_2 )
{
	EXYNOS_FbBoDataPtr bo_data = ctrl_calloc( 1, sizeof( EXYNOS_FbBoDataRec ) );
	bo_data->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );

	//bo_data->fb_id = 0; 		// should not entry the if block
	bo_data->pPixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );

	bo_data->pScrn->pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
	bo_data->pScrn->pScreen->DestroyPixmap = DestroyPixmap_fake;

	// if segmentation fault --> fail
	_fbFreeBoData( ( void* )bo_data );

	fail_if( bo_data->pPixmap != NULL, "work flow is wrong (changed) -- %p", bo_data->pPixmap );
	// _fbFreeBoData resolves bo_data
	fail_if( ctrl_get_cnt_calloc() != 3, "errors with calloc/free operations -- %d", ctrl_get_cnt_calloc() );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _fbFreeBoData_work_flow_success_3 )
{
	EXYNOS_FbBoDataPtr bo_data = ctrl_calloc( 1, sizeof( EXYNOS_FbBoDataRec ) );
	bo_data->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );

	// maybe this is very strange
	bo_data->fb_id = _add_fb();
	//bo_data->pPixmap = NULL; 	// should not entry the if block

	// pExynos
	bo_data->pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );

	// if segmentation fault --> fail
	_fbFreeBoData( ( void* )bo_data );

	fail_if( bo_data->fb_id != 0, "work flow is wrong (changed) -- %p", bo_data->fb_id );
	fail_if( cnt_fb_check_refs() != 0, "drm fb error" );
	// _fbFreeBoData resolves bo_data
	fail_if( ctrl_get_cnt_calloc() != 2, "errors with calloc/free operations -- %d", ctrl_get_cnt_calloc() );
}
END_TEST;

//---------------------------------------  _renderBoCreate() ------------------------------------
START_TEST( _renderBoCreate_passed_null_ptr_fail_1 )
{
	// if segmentation fault --> fail (pScrn == NULL)
	_renderBoCreate( NULL, 1, 1, 1, 1 );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _renderBoCreate_passed_null_ptr_fail_2 )
{
	// yes, this is memory leak, but do not carry, it is FORK mode
	// thought render_bo_create_fixt.pExynos is here
	render_bo_create_fixt.pScrn->driverPrivate = NULL;

	// if segmentation fault --> fail (pExynos == NULL)
	_renderBoCreate( render_bo_create_fixt.pScrn, 1, 1, 1, 1 );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _renderBoCreate_work_flow_success_1 )
{
	// if segmentation fault --> fail
	render_bo_create_fixt.res = _renderBoCreate( render_bo_create_fixt.pScrn, 1, 1, -1, -1 );

	fail_if( render_bo_create_fixt.res != NULL, "work flow is wrong (changed) -- %p", render_bo_create_fixt.res );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _renderBoCreate_work_flow_success_2 )
{
	render_bo_create_fixt.pExynos->bufmgr = 1; // ! NULL --> for jump to fail (XDBG_GOTO_IF_FAIL (bo != NULL, fail))

	// if segmentation fault --> fail
	render_bo_create_fixt.res = _renderBoCreate( render_bo_create_fixt.pScrn, 1, 1, 1, 1 );  // jump to fail

	fail_if( render_bo_create_fixt.res != NULL, "work flow is wrong (changed) -- %p", render_bo_create_fixt.res );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _renderBoCreate_work_flow_success_2_1 )
{
	render_bo_create_fixt.pExynos->bufmgr = 1; // ! NULL --> for jump to fail (XDBG_GOTO_IF_FAIL (bo != NULL, fail))

	// if segmentation fault --> fail
	_renderBoCreate( render_bo_create_fixt.pScrn, 1, 1, 1, 1 );  // jump to fail

	fail_if( get_tbm_check_refs() != 1, "work flow is wrong q(changed) -- %d", get_tbm_check_refs() );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _renderBoCreate_work_flow_success_3 )
{
	render_bo_create_fixt.pExynos->bufmgr = NULL; // for skip jump to fail (XDBG_GOTO_IF_FAIL (bo != NULL, fail))

	// if segmentation fault --> fail
	render_bo_create_fixt.res = _renderBoCreate( render_bo_create_fixt.pScrn, 1, 1, 1, 1 ); // bo_handle1.ptr == NULL

	fail_if( render_bo_create_fixt.res != NULL, "work flow is wrong (changed) -- %p", render_bo_create_fixt.res );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _renderBoCreate_work_flow_success_3_1 )
{
	render_bo_create_fixt.pExynos->bufmgr = NULL; // for skip jump to fail (XDBG_GOTO_IF_FAIL (bo != NULL, fail))

	// if segmentation fault --> fail
	_renderBoCreate( render_bo_create_fixt.pScrn, 1, 1, 1, 1 );  // bo_handle1.ptr == NULL

	fail_if( get_tbm_map_cnt() != 1, "work flow is wrong (changed) -- %d", get_tbm_map_cnt() );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _renderBoCreate_work_flow_success_4 )
{
	render_bo_create_fixt.width = 1;  	// for jump to fail ( XDBG_GOTO_IF_ERRNO(ret == Success, fail, -ret) )

	// to pass XDBG_RETURN_VAL_IF_FAIL (bo_handle1.ptr != NULL, NULL);
	set_hndl_tbm_ptr( render_bo_create_fixt.ptr );

	// if segmentation fault --> fail
	// XDBG_GOTO_IF_ERRNO(ret == Success, fail, -ret);
	_renderBoCreate( render_bo_create_fixt.pScrn, 1, 1, render_bo_create_fixt.width, render_bo_create_fixt.height );

	fail_if( *render_bo_create_fixt.ptr != 0, "work flow is wrong (changed) -- %d", *render_bo_create_fixt.ptr );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _renderBoCreate_work_flow_success_4_1 )
{
	render_bo_create_fixt.width = 1;  	// for jump to fail ( XDBG_GOTO_IF_ERRNO(ret == Success, fail, -ret) )

	// to pass XDBG_RETURN_VAL_IF_FAIL (bo_handle1.ptr != NULL, NULL);
	set_hndl_tbm_ptr( render_bo_create_fixt.ptr );

	// if segmentation fault --> fail
	// XDBG_GOTO_IF_ERRNO(ret == Success, fail, -ret);
	_renderBoCreate( render_bo_create_fixt.pScrn, 1, 1, render_bo_create_fixt.width, render_bo_create_fixt.height );

	fail_if( get_tbm_map_cnt() != 0, "work flow is wrong (changed) -- %d", get_tbm_map_cnt() );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _renderBoCreate_work_flow_success_4_2 )
{
	render_bo_create_fixt.width = 1;  	// for jump to fail ( XDBG_GOTO_IF_ERRNO(ret == Success, fail, -ret) )

	// to pass XDBG_RETURN_VAL_IF_FAIL (bo_handle1.ptr != NULL, NULL);
	set_hndl_tbm_ptr( render_bo_create_fixt.ptr );

	// if segmentation fault --> fail
	// XDBG_GOTO_IF_ERRNO(ret == Success, fail, -ret);
	_renderBoCreate( render_bo_create_fixt.pScrn, 1, 1, render_bo_create_fixt.width, render_bo_create_fixt.height );

	//??????????????????????????
	//fail_if( get_hndl_get_cnt() != 1, "work flow is wrong (changed) -- %d", get_hndl_get_cnt() );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _renderBoCreate_work_flow_success_4_3 )
{
	render_bo_create_fixt.width = 1;  	// for jump to fail ( XDBG_GOTO_IF_ERRNO(ret == Success, fail, -ret) )

	// to pass XDBG_RETURN_VAL_IF_FAIL (bo_handle1.ptr != NULL, NULL);
	set_hndl_tbm_ptr( render_bo_create_fixt.ptr );

	// if segmentation fault --> fail
	// XDBG_GOTO_IF_ERRNO(ret == Success, fail, -ret);
	_renderBoCreate( render_bo_create_fixt.pScrn, 1, 1, render_bo_create_fixt.width, render_bo_create_fixt.height );

	fail_if( get_tbm_check_refs() != 0, "work flow is wrong (changed) -- %d", get_tbm_check_refs() );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _renderBoCreate_work_flow_success_4_4 )
{
	render_bo_create_fixt.width = 1;  	// for jump to fail ( XDBG_GOTO_IF_ERRNO(ret == Success, fail, -ret) )

	// to pass XDBG_RETURN_VAL_IF_FAIL (bo_handle1.ptr != NULL, NULL);
	set_hndl_tbm_ptr( render_bo_create_fixt.ptr );

	// if segmentation fault --> fail
	// XDBG_GOTO_IF_ERRNO(ret == Success, fail, -ret);
	_renderBoCreate( render_bo_create_fixt.pScrn, 1, 1, render_bo_create_fixt.width, render_bo_create_fixt.height );

	fail_if( cnt_fb_check_refs() != 0, "work flow is wrong (changed) -- %d", cnt_fb_check_refs() );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _renderBoCreate_work_flow_success_5 )
{
	EXYNOS_FbBoDataPtr p_fb_do_data = NULL;
	int x = -1;			// see _renderBoCreate() definition
	int y = -1;			// see _renderBoCreate() definition

	// to pass XDBG_RETURN_VAL_IF_FAIL (bo_handle1.ptr != NULL, NULL);
	set_hndl_tbm_ptr( render_bo_create_fixt.ptr );

	// if segmentation fault --> fail
	_renderBoCreate( render_bo_create_fixt.pScrn, x, y, render_bo_create_fixt.width,
			               render_bo_create_fixt.height );

	// !!! getting pointer to bo_data from _renderBoCreate() function
	p_fb_do_data = ( EXYNOS_FbBoDataPtr )get_bo_data();

	fail_if( p_fb_do_data->pos.x1 != 0, "work flow is wrong (changed) -- %d", cnt_fb_check_refs() );
	fail_if( p_fb_do_data->pos.y1 != 0, "work flow is wrong (changed) -- %d", cnt_fb_check_refs() );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _renderBoCreate_work_flow_success_5_1 )
{
	// to pass XDBG_RETURN_VAL_IF_FAIL (bo_handle1.ptr != NULL, NULL);
	set_hndl_tbm_ptr( render_bo_create_fixt.ptr );

	// if segmentation fault --> fail             x,  y - any digital
	_renderBoCreate( render_bo_create_fixt.pScrn, 11, 9, render_bo_create_fixt.width,
			               render_bo_create_fixt.height );

	// _renderBoCreate() has ctrl_calloc() call also fixture make one,
	// unnecessary for this unit-test, ctrl_calloc() call -- pExynos->pFb
	fail_if( ctrl_get_cnt_calloc() != 5, "errors with calloc/free operations -- %d", ctrl_get_cnt_calloc() );
}
END_TEST;

//-------------------------------------------------------------------------------------
START_TEST( _renderBoCreate_work_flow_success_5_2 )
{
	EXYNOS_FbBoDataPtr p_fb_do_data = NULL;
	int x = 11;		// any digital
	int y = 9;		// any digital

	// to pass XDBG_RETURN_VAL_IF_FAIL (bo_handle1.ptr != NULL, NULL);
	set_hndl_tbm_ptr( render_bo_create_fixt.ptr );

	// if segmentation fault --> fail
	_renderBoCreate( render_bo_create_fixt.pScrn, x, y, render_bo_create_fixt.width,
			               render_bo_create_fixt.height );

	// !!! getting pointer to bo_data from _renderBoCreate() function
	p_fb_do_data = ( EXYNOS_FbBoDataPtr )get_bo_data();

	fail_if( p_fb_do_data->pFb 		  != render_bo_create_fixt.pFb,        "work flow is wrong 1 (changed)" );
	fail_if( p_fb_do_data->gem_handle != 11, 		                       "work flow is wrong 2 (changed) -- %d", \
			 p_fb_do_data->gem_handle ); // see to tbm_bo_get_handle()
	fail_if( p_fb_do_data->pitch 	  != render_bo_create_fixt.width*4,    "work flow is wrong 3 (changed)" );
	fail_if( p_fb_do_data->fb_id 	  == 0, 		 					   "work flow is wrong 4 (changed)" );
	fail_if( p_fb_do_data->pos.x2 	  != x + render_bo_create_fixt.width,  "work flow is wrong 7 (changed)" );
	fail_if( p_fb_do_data->pos.y2 	  != y + render_bo_create_fixt.height, "work flow is wrong 8 (changed)" );
	fail_if( p_fb_do_data->size 	  != 11, 		 					   "work flow is wrong 9 (changed)" );  // see to tbm_bo_size()
	fail_if( p_fb_do_data->pScrn 	  != render_bo_create_fixt.pScrn, 	    "work flow is wrong 9 (changed)" );
}
END_TEST;

//===========================================================================================================
//===========================================================================================================
//===========================================================================================================

int test_display(run_stat_data*stat)
{
	Suite* s = suite_create( "exynos_display.c" );

	// create test case 'tc_clone_ss'
	TCase* tc_clone_ss = 			 tcase_create( "Clone start/stop" );
	TCase* tc_crtc_conf_resize = 	 tcase_create( "CrtcConfigResize" );
	TCase* tc_vblank_hndl = 		 tcase_create( "DispInfoVblankHandler" );
	TCase* tc_ipp_hndl = 			 tcase_create( "IppHandler" );
	TCase* tc_page_flip_mem =		 tcase_create( "PageFlipHandler_memory" );
	TCase* tc_page_flip_logic = 	 tcase_create( "PageFlipHandler_logic" );
	TCase* tc_wake_up_hndl = 		 tcase_create( "DispInfoWakeupHanlder" );
	TCase* tc_load_palette = 		 tcase_create( "DisplayLoadPalette" );
	TCase* tc_disp_mode_from_kmode = tcase_create( "DisplayModeFromKmode" );
	TCase* tc_disp_mode_to_kmode = 	 tcase_create( "DisplayModeToKmode" );
	TCase* tc_set_disp_conn_mode = 	 tcase_create( "displaySetDispConnMode" );

	TCase* tc_get_free_crtc_id = 	 tcase_create( "_displayGetFreeCrtcID" );
	TCase* tc_get_free_crtc_id_fixture = tcase_create( "_displayGetFreeCrtcID_fixture" );

	TCase* tc_look_output_conn_type = tcase_create( "_lookForOutputByConnType" );
	TCase* tc_look_output_conn_type_fixture = tcase_create( "_lookForOutputByConnType_fixture" );

	TCase* tc_fb_unref_bo     = tcase_create( "_fbUnrefBo" );
	TCase* tc_fb_free_bo_data = tcase_create( "_fbFreeBoData" );
	TCase* tc_render_bo_create_fixture = tcase_create( "_render_bo_create_fixture" );
	TCase* tc_exynosCrtcOn = tcase_create( "exynosCrtcOn" );
	TCase* tc_disp_covering_crtc = tcase_create( "exynosDisplayCoveringCrtc" );


	// add unit-test 'test_clone_start_stop' to test case 'tc_clone_ss'
	//tcase_add_test( tc_clone_ss, clone_start_stop_test );
	tcase_add_test( tc_clone_ss, _exynosDisplayInitDrmEventCtx_mem_test );
	tcase_add_test( tc_clone_ss, _exynosDisplayInitDrmEventCtx_test );
	tcase_add_test( tc_clone_ss, _exynosDisplayDeInitDrmEventCtx_mem_test );
	tcase_add_test( tc_clone_ss, _exynosDisplayDeInitDrmEventCtx_test );

	//----------  EXYNOSCrtcConfigResize()
	tcase_add_test( tc_crtc_conf_resize, EXYNOSCrtcConfigResize_mem_test );
	tcase_add_test( tc_crtc_conf_resize, EXYNOSCrtcConfigResize_test_1 );
	tcase_add_test( tc_crtc_conf_resize, EXYNOSCrtcConfigResize_test_2 );
	tcase_add_test( tc_crtc_conf_resize, EXYNOSCrtcConfigResize_test_3 );

	//----------  EXYNOSDispInfoVblankHandler()
	tcase_add_test( tc_vblank_hndl, EXYNOSDispInfoVblankHandler_mem_test_1 );
	tcase_add_test( tc_vblank_hndl, EXYNOSDispInfoVblankHandler_mem_test_2 );
	tcase_add_test( tc_vblank_hndl, EXYNOSDispInfoVblankHandler_mem_test_3 );
	tcase_add_test( tc_vblank_hndl, EXYNOSDispInfoVblankHandler_mem_test_3_1 );
	tcase_add_test( tc_vblank_hndl, EXYNOSDispInfoVblankHandler_mem_test_4 );
	tcase_add_test( tc_vblank_hndl, EXYNOSDispInfoVblankHandler_mem_test_4_1 );

	//----------  EXYNOSDispInfoIppHandler()
	tcase_add_test( tc_ipp_hndl, EXYNOSDispInfoIppHandler_mem_test_1 );
	tcase_add_test( tc_ipp_hndl, EXYNOSDispInfoIppHandler_mem_test_2 );
	tcase_add_test( tc_ipp_hndl, EXYNOSDispInfoIppHandler_mem_test_3 );

	//----------  EXYNOSDispInfoPageFlipHandler()
	tcase_add_test( tc_page_flip_mem, EXYNOSDispInfoPageFlipHandler_mem_test_1 );
	tcase_add_test( tc_page_flip_mem, EXYNOSDispInfoPageFlipHandler_mem_test_2 );
	tcase_add_test( tc_page_flip_mem, EXYNOSDispInfoPageFlipHandler_mem_test_3 );
	tcase_add_test( tc_page_flip_mem, EXYNOSDispInfoPageFlipHandler_mem_test_4 );
	tcase_add_test( tc_page_flip_mem, EXYNOSDispInfoPageFlipHandler_mem_test_5 );

	//----------

	// add fixture 'page_flip' to test case 'tc_page_flip_logic'
	tcase_add_checked_fixture( tc_page_flip_logic, setup_page_flip, teardown_page_flip );
	tcase_add_test( tc_page_flip_logic, EXYNOSDispInfoPageFlipHandler_logic_test_1 );
	tcase_add_test( tc_page_flip_logic, EXYNOSDispInfoPageFlipHandler_logic_test_2 );
	tcase_add_test( tc_page_flip_logic, EXYNOSDispInfoPageFlipHandler_logic_test_3 );
	tcase_add_test( tc_page_flip_logic, EXYNOSDispInfoPageFlipHandler_logic_test_3_1 );

	//----------  EXYNOSDispInfoWakeupHanlder()
	tcase_add_test( tc_wake_up_hndl, EXYNOSDispInfoWakeupHanlder_mem_test_1 );
	tcase_add_test( tc_wake_up_hndl, EXYNOSDispInfoWakeupHanlder_mem_test_2 );
	tcase_add_test( tc_wake_up_hndl, EXYNOSDispInfoWakeupHanlder_mem_test_3 );
	tcase_add_test( tc_wake_up_hndl, EXYNOSDispInfoWakeupHanlder_logic_test_1 );

	//---------- exynosDisplayLoadPalette()
	tcase_add_test( tc_load_palette, exynosDisplayLoadPalette_mem_test_1 );
	tcase_add_test( tc_load_palette, exynosDisplayLoadPalette_mem_test_2 );
	tcase_add_test( tc_load_palette, exynosDisplayLoadPalette_mem_test_3 );
	tcase_add_test( tc_load_palette, exynosDisplayLoadPalette_mem_test_4 );
	tcase_add_test( tc_load_palette, exynosDisplayLoadPalette_mem_test_5 );
	tcase_add_test( tc_load_palette, exynosDisplayLoadPalette_mem_test_6 );
	tcase_add_test( tc_load_palette, exynosDisplayLoadPalette_logic_test_1 );
	tcase_add_test( tc_load_palette, exynosDisplayLoadPalette_logic_test_2 );
	tcase_add_test( tc_load_palette, exynosDisplayLoadPalette_logic_test_3 );
	tcase_add_test( tc_load_palette, exynosDisplayLoadPalette_logic_test_4 );

	//---------- exynosDisplayModeFromKmode()
	tcase_add_test( tc_disp_mode_from_kmode, exynosDisplayModeFromKmode_mem_test_1 );
	tcase_add_test( tc_disp_mode_from_kmode, exynosDisplayModeFromKmode_mem_test_2 );
	tcase_add_test( tc_disp_mode_from_kmode, exynosDisplayModeFromKmode_mem_test_3 );
	tcase_add_test( tc_disp_mode_from_kmode, exynosDisplayModeFromKmode_mem_test_4 );
	tcase_add_test( tc_disp_mode_from_kmode, exynosDisplayModeFromKmode_logic_test_1 );
	tcase_add_test( tc_disp_mode_from_kmode, exynosDisplayModeFromKmode_logic_test_2 );
	tcase_add_test( tc_disp_mode_from_kmode, exynosDisplayModeFromKmode_logic_test_3 );

    //---------- exynosDisplayModeToKmode()
	tcase_add_test( tc_disp_mode_to_kmode, DisplayModeToKmode_passed_null_ptr_fail_1 );
	tcase_add_test( tc_disp_mode_to_kmode, DisplayModeToKmode_passed_null_ptr_fail_2 );
	tcase_add_test( tc_disp_mode_to_kmode, DisplayModeToKmode_pmode_name_null_fail );
	tcase_add_test( tc_disp_mode_to_kmode, DisplayModeToKmode_work_flow_success_1 );
	tcase_add_test( tc_disp_mode_to_kmode, DisplayModeToKmode_work_flow_success_2 );

    //---------- secCrtcOn()
	tcase_add_test( tc_exynosCrtcOn, exynosCrtcOn_passed_null_ptr_fail_1 );
	tcase_add_test( tc_exynosCrtcOn, exynosCrtcOn_work_flow_1 );
	tcase_add_test( tc_exynosCrtcOn, exynosCrtcOn_work_flow_2 );
	tcase_add_test( tc_exynosCrtcOn, exynosCrtcOn_work_flow_3 );

	//---------- exynosDisplayCoveringCrtc()
	tcase_add_test( tc_disp_covering_crtc, exynosDisplayCoveringCrtc_passed_null_ptr_fail_1 );
	tcase_add_test( tc_disp_covering_crtc, exynosDisplayCoveringCrtc_passed_null_ptr_fail_2 );

    //---------- displaySetDispConnMode()
	tcase_add_test( tc_set_disp_conn_mode, displaySetDispConnMode_passed_null_ptr_fail_1 );
	tcase_add_test( tc_set_disp_conn_mode, displaySetDispConnMode_passed_null_ptr_fail_2 );
	tcase_add_test( tc_set_disp_conn_mode, displaySetDispConnMode_passed_null_ptr_fail_3 );
	tcase_add_test( tc_set_disp_conn_mode, displaySetDispConnMode_work_flow_success_1 );
	tcase_add_test( tc_set_disp_conn_mode, displaySetDispConnMode_work_flow_success_2 );


    //---------- _displayGetFreeCrtcID()
	tcase_add_test( tc_get_free_crtc_id, _displayGetFreeCrtcID_passed_null_ptr_fail_1 );
	tcase_add_test( tc_get_free_crtc_id, _displayGetFreeCrtcID_passed_null_ptr_fail_2 );

	tcase_add_checked_fixture( tc_get_free_crtc_id_fixture, setup_get_free_crtc_id, teardown_get_free_crtc_id );
	tcase_add_test( tc_get_free_crtc_id_fixture, _displayGetFreeCrtcID_passed_null_ptr_fail_3 );
	tcase_add_test( tc_get_free_crtc_id_fixture, _displayGetFreeCrtcID_buffer_overflow_fail_1 );
	tcase_add_test( tc_get_free_crtc_id_fixture, _displayGetFreeCrtcID_work_flow_success_1 );
	tcase_add_test( tc_get_free_crtc_id_fixture, _displayGetFreeCrtcID_work_flow_success_2 );
	tcase_add_test( tc_get_free_crtc_id_fixture, _displayGetFreeCrtcID_work_flow_success_3 );
	tcase_add_test( tc_get_free_crtc_id_fixture, _displayGetFreeCrtcID_work_flow_success_4 );

	//---------- _lookForOutputByConnType()
	tcase_add_test( tc_look_output_conn_type, _lookForOutputByConnType_passed_null_ptr_fail_1 );
	//tcase_add_test( tc_look_output_conn_type, _lookForOutputByConnType_passed_null_ptr_fail_2 );

	tcase_add_checked_fixture( tc_look_output_conn_type_fixture, setup_get_free_crtc_id, teardown_get_free_crtc_id );
	tcase_add_test( tc_look_output_conn_type_fixture, _lookForOutputByConnType_work_flow_success_1 );
	tcase_add_test( tc_look_output_conn_type_fixture, _lookForOutputByConnType_work_flow_success_2 );
	tcase_add_test( tc_look_output_conn_type_fixture, _lookForOutputByConnType_work_flow_success_3 );

	//---------- _fbUnrefBo()
	tcase_add_test( tc_fb_unref_bo, _fbUnrefBo_passed_null_ptr_fail );
	tcase_add_test( tc_fb_unref_bo, _fbUnrefBo_work_flow_success );

	//---------- _fbFreeBoData()
	tcase_add_test( tc_fb_free_bo_data, _fbFreeBoData_passed_null_ptr_fail_1 );
	tcase_add_test( tc_fb_free_bo_data, _fbFreeBoData_passed_null_ptr_fail_2 );
	tcase_add_test( tc_fb_free_bo_data, _fbFreeBoData_passed_null_ptr_fail_3 );
	tcase_add_test( tc_fb_free_bo_data, _fbFreeBoData_passed_null_ptr_fail_4 );
	tcase_add_test( tc_fb_free_bo_data, _fbFreeBoData_passed_null_ptr_fail_5 );
	tcase_add_test( tc_fb_free_bo_data, _fbFreeBoData_work_flow_success_1 );
	tcase_add_test( tc_fb_free_bo_data, _fbFreeBoData_work_flow_success_2 );
	tcase_add_test( tc_fb_free_bo_data, _fbFreeBoData_work_flow_success_3 );

	//---------- _renderBoCreate()

	tcase_add_checked_fixture( tc_render_bo_create_fixture, setup_render_bo_create, teardown_render_bo_create );
	tcase_add_test( tc_render_bo_create_fixture, _renderBoCreate_passed_null_ptr_fail_1 );
	tcase_add_test( tc_render_bo_create_fixture, _renderBoCreate_passed_null_ptr_fail_2 );
	tcase_add_test( tc_render_bo_create_fixture, _renderBoCreate_work_flow_success_1 );
	tcase_add_test( tc_render_bo_create_fixture, _renderBoCreate_work_flow_success_2 );
	tcase_add_test( tc_render_bo_create_fixture, _renderBoCreate_work_flow_success_2_1 );
	tcase_add_test( tc_render_bo_create_fixture, _renderBoCreate_work_flow_success_3 );
	tcase_add_test( tc_render_bo_create_fixture, _renderBoCreate_work_flow_success_3_1 );
	tcase_add_test( tc_render_bo_create_fixture, _renderBoCreate_work_flow_success_4 );
	tcase_add_test( tc_render_bo_create_fixture, _renderBoCreate_work_flow_success_4_1 );
	tcase_add_test( tc_render_bo_create_fixture, _renderBoCreate_work_flow_success_4_2 );
	tcase_add_test( tc_render_bo_create_fixture, _renderBoCreate_work_flow_success_4_3 );
	tcase_add_test( tc_render_bo_create_fixture, _renderBoCreate_work_flow_success_4_4 );
	tcase_add_test( tc_render_bo_create_fixture, _renderBoCreate_work_flow_success_5 );
	tcase_add_test( tc_render_bo_create_fixture, _renderBoCreate_work_flow_success_5_1 );
	tcase_add_test( tc_render_bo_create_fixture, _renderBoCreate_work_flow_success_5_2 );

	// add test case 'tc_clone_ss' to 's' suite
	suite_add_tcase( s, tc_clone_ss );
	suite_add_tcase( s, tc_crtc_conf_resize );
	suite_add_tcase( s, tc_vblank_hndl );
	suite_add_tcase( s, tc_ipp_hndl );
	suite_add_tcase( s, tc_page_flip_mem );
	suite_add_tcase( s, tc_page_flip_logic );
	suite_add_tcase( s, tc_wake_up_hndl );
	suite_add_tcase( s, tc_load_palette );
	suite_add_tcase( s, tc_disp_mode_from_kmode );
	suite_add_tcase( s, tc_disp_mode_to_kmode );
	suite_add_tcase( s, tc_exynosCrtcOn );
	suite_add_tcase( s, tc_disp_covering_crtc );
	suite_add_tcase( s, tc_set_disp_conn_mode );
	suite_add_tcase( s, tc_get_free_crtc_id );
	suite_add_tcase( s, tc_get_free_crtc_id_fixture );
	suite_add_tcase( s, tc_look_output_conn_type );
	suite_add_tcase( s, tc_look_output_conn_type_fixture );
	suite_add_tcase( s, tc_fb_unref_bo );
	suite_add_tcase( s, tc_fb_free_bo_data );
	suite_add_tcase( s, tc_render_bo_create_fixture );

	SRunner *sr = srunner_create( s );
	//srunner_set_log (sr, "display.log");
	srunner_run_all( sr, CK_VERBOSE );

	stat->num_tests = srunner_ntests_run( sr );
	stat->num_pass = srunner_ntests_run( sr ) - srunner_ntests_failed( sr );
	stat->num_failure = srunner_ntests_failed( sr );

	stat->num_func = 39;
	stat->num_tested = 20;
	stat->num_nottested = 19;

	return 0;
}
