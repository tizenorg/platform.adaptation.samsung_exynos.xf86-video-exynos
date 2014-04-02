/*
 * test_video_capture.c
 *
 *  Created on: Oct 21, 2013
 *      Author: Alexandr Rozov
 */

#define PRINTF ( printf( "\n\nDEBUG\n\n" ) )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )
#define PRINTVAL( v ) printf( "\n %d \n", v )

#include <pixmap.h>
#include "mem.h"
#include "test_video_capture.h"

// fake-macros
#define f_xorg_list_for_each_entry_safe( pos, tmp, head, member ) \
		pos = ctrl_calloc( 1, sizeof( RetBufInfo ) );             \
		pos->vbuf = ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );    \
		pos->vbuf->names[0] = 11;	/*any digital*/

#define f_xorg_list_add( param_1, param_2 )

#define f_xorg_list_del( param_1 )

// fake-functions
#define TimerFree(v) ctrl_free(v)


// this file contains functions to test
#include "./xv/exynos_video_capture.c"

#define _CaptureVideoGetFrontBo2  (*fake_CaptureVideoGetFrontBo2)
tbm_bo  (*fake_CaptureVideoGetFrontBo2)( void* pPort, int connector_type, tbm_bo_handle* outbo_handle );

tbm_bo _CaptureVideoGetFrontBo2_test1 ( void* pPort, int connector_type, tbm_bo_handle* outbo_handle )
{
  tbm_bo bo1 = tbm_bo_alloc(0,(1280*720)*4,0);
  //outbo_handle = tbm_bo_get_handle (bo1, 0);
  *outbo_handle = (tbm_bo_handle) 0x0A;
  return  tbm_bo_ref(bo1);
}                                       


extern int ctrl_get_cnt_calloc( void );
extern EXYNOSBufInfo* get_vbuf_gl( void );

/*-------------------------------- Fake functions   -----------------------------------------*/

//============================================== FIXTURES ===================================================
static EXYNOSOutputPrivPtr create_output( uint32_t type )
{
	EXYNOSOutputPrivPtr out=calloc(1,sizeof(EXYNOSOutputPrivRec));
	out->mode_output=calloc(1,sizeof(drmModeConnector));
	out->mode_output->connector_type=type;
	out->mode_encoder=calloc(1,sizeof(drmModeEncoder));
	out->mode_encoder->crtc_id=7;
	return out;
}
//------------------------------------ _CaptureVideoFindReturnBuf() ------------------------------------
typedef struct _CaptureVideoFindReturnBuf_struct
{
  CAPTUREPortInfoPtr pPort;
} cap_vid_find_ret_buf_t;

cap_vid_find_ret_buf_t cap_vid_find_ret_buf_gl;
// this function prepare CAPTUREPortInfoPtr struct for further using
// till only for 'test_clone_start_stop' unit-test
//-----------------------------------------------------------------------------------------------------------
static void prepare_pPort( CAPTUREPortInfoPtr pPort )
{

  pPort->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
  XDBG_GOTO_IF_FAIL (pPort->pScrn != NULL , fail_prepare);
  pPort->pScrn->driverPrivate = ctrl_calloc( 1, sizeof(EXYNOSRec) );
  
  pPort->pScrn->pScreen = ctrl_calloc( 1, sizeof(ScreenRec) );
  ScrnInfoPtr pScr = pPort->pScrn;
  
  EXYNOSRec* pExynos = EXYNOSPTR(pPort->pScrn);

  pExynos->pDispInfo = ctrl_calloc( 1, sizeof(EXYNOSDispInfoRec) );
  pExynos->pDispInfo->conn_mode = DISPLAY_CONN_MODE_HDMI;
  pExynos->pDispInfo->main_lcd_mode.hdisplay = 720;
  pExynos->pDispInfo->main_lcd_mode.vdisplay = 1280;
  pExynos->pDispInfo->ext_connector_mode.hdisplay = 100;
  pExynos->pDispInfo->ext_connector_mode.vdisplay = 100;

  pExynos->pDispInfo->mode_res = ctrl_calloc( 1, sizeof(drmModeRes) );
  pExynos->pDispInfo->mode_res->count_connectors = 1;
  pExynos->pDispInfo->mode_res->count_crtcs = 1;
  pExynos->pDispInfo->mode_res->count_encoders = 1;
  
  /* pScr->privates[0].ptr = (xf86CrtcConfigRec*) ctrl_calloc( 1, sizeof(xf86CrtcConfigRec) );
  ((xf86CrtcConfigRec*)pScr->privates[0].ptr)->crtc =  ctrl_calloc( 1, sizeof(xf86CrtcRec) );
  ((xf86CrtcRec*)((xf86CrtcConfigRec*)pScr->privates[0].ptr)->crtc)->driver_private = ctrl_calloc( 1, sizeof(EXYNOSCrtcPrivRec) );
  */

  xorg_list_init( &pExynos->pDispInfo->crtcs );
  xorg_list_init( &pExynos->pDispInfo->outputs );
  xorg_list_init( &pExynos->pDispInfo->planes );

  EXYNOSOutputPrivPtr o1 = create_output( DRM_MODE_CONNECTOR_HDMIA );
  xorg_list_add( &o1->link, &pExynos->pDispInfo->outputs );
  EXYNOSOutputPrivPtr o2 = create_output( DRM_MODE_CONNECTOR_LVDS );
  xorg_list_add( &o2->link, &pExynos->pDispInfo->outputs );
fail_prepare:
  return;  
}

static void flush_pPort( CAPTUREPortInfoPtr pPort )
{
  ScrnInfoPtr pScr = pPort->pScrn;
  EXYNOSRec* pExynos = pScr->driverPrivate;
  
  ctrl_free(pExynos->pDispInfo->mode_res);
  ctrl_free(pExynos->pDispInfo);
  ctrl_free(pPort->pScrn->pScreen);
  ctrl_free(pPort->pScrn);
  
  
}
// definition checked fixture
void setup_CaptureVideoFindReturnBuf( void )
{
  cap_vid_find_ret_buf_gl.pPort = ctrl_calloc( 1, sizeof( CAPTUREPortInfo ) );
  cap_vid_find_ret_buf_gl.pPort->capture = CAPTURE_MODE_STILL;  
  xorg_list_init (&cap_vid_find_ret_buf_gl.pPort->retbuf_info);
  
}

void teardown_CaptureVideoFindReturnBuf( void )
{
  ctrl_free(cap_vid_find_ret_buf_gl.pPort);
}

/*------------------------------------ _CaptureVideoAddReturnBuf() ------------------------------------*/
typedef struct _CaptureVideoAddReturnBuf_struct
{
  CAPTUREPortInfoPtr pPort;
  EXYNOSBufInfo *vbuf;
} cap_vid_add_ret_buf_t;

cap_vid_add_ret_buf_t cap_vid_add_ret_buf_gl;

// definition checked fixture
void setup_CaptureVideoAddReturnBuf( void )
{
  cap_vid_add_ret_buf_gl.pPort = ctrl_calloc( 1, sizeof( CAPTUREPortInfo ) );
  cap_vid_add_ret_buf_gl.pPort->capture = CAPTURE_MODE_STILL; 
  cap_vid_add_ret_buf_gl.vbuf = ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );
  xorg_list_init (&cap_vid_add_ret_buf_gl.pPort->retbuf_info);
  
}

void teardown_CaptureVideoAddReturnBuf( void )
{
  ctrl_free( cap_vid_add_ret_buf_gl.vbuf );
  ctrl_free( cap_vid_add_ret_buf_gl.pPort );
}


/* ------------------------------------ _exynosCaptureVideoRemoveReturnBuf() ------------------------------------ */
typedef struct _CaptureVideoRemoveReturnBuf_struct
{
  CAPTUREPortInfoPtr pPort;
  RetBufInfo *info;
} cap_vid_rmv_ret_buf_t;

cap_vid_rmv_ret_buf_t cap_vid_rmv_ret_buf_gl;

/* definition checked fixture */
void setup_CaptureVideoRemoveReturnBuf( void )
{
  cap_vid_rmv_ret_buf_gl.pPort = ctrl_calloc( 1, sizeof( CAPTUREPortInfo ) );
  cap_vid_rmv_ret_buf_gl.pPort->capture = CAPTURE_MODE_STREAM;  // !CAPTURE_MODE_STILL
  cap_vid_rmv_ret_buf_gl.info = ctrl_calloc( 1, sizeof( RetBufInfo ) );
  cap_vid_rmv_ret_buf_gl.info->vbuf = ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );
}

void teardown_CaptureVideoRemoveReturnBuf( void )
{
 
}

/* ------------------------------------ EXYNOSCaptureGetPortAttribute() ------------------------------------ */
typedef struct CaptureGetPortAttribute_struct
{
  Atom   attribute;
  INT32 value;
  CAPTUREPortInfoPtr pPort;
} cap_get_port_atrb_t;

cap_get_port_atrb_t cap_get_port_atrb_gl;

// definition checked fixture
void setup_CaptureGetPortAttribute( void )
{
  cap_get_port_atrb_gl.pPort = ctrl_calloc( 1, sizeof( CAPTUREPortInfo ) );
  cap_get_port_atrb_gl.value = -1;
}

void teardown_CaptureGetPortAttribute( void )
{
  ctrl_free(cap_get_port_atrb_gl.pPort);
}


/*------------------------------------ _CaptureVideoGetDrawableBuffer -------------------------*/

/*------------------------------------ _CaptureVideoGetUIBuffer -------------------------------*/
typedef struct CaptureGetUIBuffer_struct
{
  Atom   attribute;
  INT32 value;
  CAPTUREPortInfoPtr pPort;
} cap_get_ui_buffer_t;

cap_get_ui_buffer_t cap_get_ui_buffer_gl;
setup_CaptureVideoGetUIBuffer(void)
{
  cap_get_ui_buffer_gl.pPort = ctrl_calloc( 1, sizeof( CAPTUREPortInfo ) );
  
  cap_get_ui_buffer_gl.pPort->capture = CAPTURE_MODE_STILL;
}

void teardown_CaptureVideoGetUIBuffer( void )
{
  ctrl_free(cap_get_ui_buffer_gl.pPort);
}
/**********************************************************************************************/

/*------------------------------------ _CaptureVideoPutStill() -------------------------------*/
void setup_CaptureVideoPutStill()
{
  cap_vid_add_ret_buf_gl.pPort = ctrl_calloc( 1, sizeof( CAPTUREPortInfo ) );
  
  cap_vid_add_ret_buf_gl.pPort->capture = CAPTURE_MODE_STILL; 
  prepare(cap_vid_add_ret_buf_gl.pPort->pScrn); 
  cap_vid_add_ret_buf_gl.pPort->pDraw = ctrl_calloc( 1, sizeof( DrawableRec ) );
  cap_vid_add_ret_buf_gl.pPort->pDraw->pScreen = ctrl_calloc( 1, sizeof(ScreenRec) );
  
}

void teardown_CaptureVideoPutStill( void )
{
    ctrl_free(cap_get_port_atrb_gl.pPort);
}

//================================ Fake function`s definition =================================================

//============================================= Unit-tests ====================================================

/*-------------------------------------- _exynosVideoCaptureGetPortAtom() ------------------------------------*/
/*------------------------------------------------------------------------------------------------------------*/
START_TEST( _exynosVideoCaptureGetPortAtom_work_flow_success_PAA_MIN )
{
	Atom res = 1;  // !None

	res = _exynosVideoCaptureGetPortAtom( PAA_MIN );

	fail_if( res != None, "work flow is changed -- %d", res );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( _exynosVideoCaptureGetPortAtom_work_flow_success_ABOVEPAA_MAX )
{
	Atom res = 1;  // !None

	res = _exynosVideoCaptureGetPortAtom( PAA_MAX+2 );

	fail_if( res != None, "work flow is changed -- %d", res );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( _exynosVideoCaptureGetPortAtom_work_flow_success_PAA_FORMAT )
{
	Atom res = None;

	//capture_atoms[all].atom == None

	res = _exynosVideoCaptureGetPortAtom( PAA_FORMAT );

	fail_if( res != 11, "work flow is changed -- %d", res );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( _exynosVideoCaptureGetPortAtom_work_flow_success_PAA_CAPTURE )
{
	Atom res = None;

	// 1 because of PAA_CAPTURE(2)
	capture_atoms[1].atom = 9; // !None and !11

	res = _exynosVideoCaptureGetPortAtom( PAA_CAPTURE );

	fail_if( res != 9, "work flow is changed -- %d", res );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( _exynosVideoCaptureGetPortAtom_work_flow_success_PAA_DISPLAY )
{
	Atom res = None;

	// 1 because of PAA_CAPTURE(2)
	capture_atoms[2].atom = 9; // !None and !11

	res = _exynosVideoCaptureGetPortAtom( PAA_DISPLAY );

	fail_if( res != 9, "work flow is changed -- %d", res );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( _exynosVideoCaptureGetPortAtom_work_flow_success_PAA_ROTATE_OFF )
{
	Atom res = None;

	// 1 because of PAA_CAPTURE(2)
	capture_atoms[3].atom = 11; // !None and !11

	res = _exynosVideoCaptureGetPortAtom( PAA_ROTATE_OFF );

	fail_if( res != 11, "work flow is changed -- %d", res );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( _exynosVideoCaptureGetPortAtom_work_flow_success_PAA_DATA_TYPE )
{
	Atom res = None;

	// 1 because of PAA_CAPTURE(2)
	capture_atoms[4].atom = 9; // !None and !11

	res = _exynosVideoCaptureGetPortAtom( PAA_DATA_TYPE );

	fail_if( res != 9, "work flow is changed -- %d", res );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( _exynosVideoCaptureGetPortAtom_work_flow_success_PAA_SOFTWARECOMPOSITE_ON )
{
	Atom res = None;

	// 1 because of PAA_CAPTURE(2)
	capture_atoms[5].atom = 11; // !None and !11

	res = _exynosVideoCaptureGetPortAtom( PAA_SOFTWARECOMPOSITE_ON );

	fail_if( res != 11, "work flow is changed -- %d", res );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( _exynosVideoCaptureGetPortAtom_work_flow_success_PAA_RETBUF )
{
	Atom res = None;

	// 1 because of PAA_CAPTURE(2)
	capture_atoms[6].atom = 9; // !None and !11

	res = _exynosVideoCaptureGetPortAtom( PAA_RETBUF );

	fail_if( res != 9, "work flow is changed -- %d", res );
}
END_TEST;

/*--------------------------------------- _CaptureVideoFindReturnBuf() ---------------------------------------*/
/*------------------------------------------------------------------------------------------------------------*/
START_TEST( _CaptureVideoFindReturnBuf_work_flow_success_1 )
{
	RetBufInfo* res = (RetBufInfo*)1; // !NULL

	res = _CaptureVideoFindReturnBuf( cap_vid_find_ret_buf_gl.pPort ,0);

	fail_if( res != NULL, "work flow is changed -- %p", res );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( _CaptureVideoFindReturnBuf_work_flow_success_2 )
{
	RetBufInfo* res = (RetBufInfo*)1; // !NULL

	cap_vid_find_ret_buf_gl.pPort->capture =  CAPTURE_MODE_STILL;

	// 0 != 11, see to f_xorg_list_for_each_entry_safe
	res = _CaptureVideoFindReturnBuf( cap_vid_find_ret_buf_gl.pPort , 0 );

	fail_if( res != NULL, "work flow is changed -- %p", res );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( _CaptureVideoFindReturnBuf_work_flow_success_3 )
{
	RetBufInfo* res = NULL;

	cap_vid_find_ret_buf_gl.pPort->capture =  CAPTURE_MODE_STREAM;  // !CAPTURE_MODE_STILL

	// 11, see to f_xorg_list_for_each_entry_safe
	res = _CaptureVideoFindReturnBuf( cap_vid_find_ret_buf_gl.pPort , 11 );

	fail_if( res == NULL, "work flow is changed -- %p", res );
}
END_TEST;

/*--------------------------------------- _CaptureVideoAddReturnBuf() ----------------------------------------*/
/*------------------------------------------------------------------------------------------------------------*/
START_TEST( _CaptureVideoAddReturnBuf_work_flow_success_1 )
{
	Bool res = TRUE;

	res = _CaptureVideoAddReturnBuf( cap_vid_add_ret_buf_gl.pPort, NULL );

	fail_if( res != FALSE, "Check Dereferencing pointer -- %d", res );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( _CaptureVideoAddReturnBuf_work_flow_success_2 )
{
	Bool res = TRUE;

	// _CaptureVideoFindReturnBuf will return !NULL
	cap_vid_add_ret_buf_gl.vbuf->names[0] = 11; // !0
	cap_vid_add_ret_buf_gl.pPort->capture = CAPTURE_MODE_STILL;//CAPTURE_MODE_STREAM;  // !CAPTURE_MODE_STILL

	res = _CaptureVideoAddReturnBuf( cap_vid_add_ret_buf_gl.pPort, cap_vid_add_ret_buf_gl.vbuf );

	fail_if( res != FALSE, "work flow is changed -- %d", res );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( _CaptureVideoAddReturnBuf_work_flow_success_3 )
{
	Bool res = TRUE;

	// _CaptureVideoFindReturnBuf will return NULL
	cap_vid_add_ret_buf_gl.pPort->capture = CAPTURE_MODE_STILL;//CAPTURE_MODE_STREAM;  // !CAPTURE_MODE_STILL

	res = _CaptureVideoAddReturnBuf( cap_vid_add_ret_buf_gl.pPort, cap_vid_add_ret_buf_gl.vbuf );

	fail_if( res != TRUE, "work flow is changed -- %d", res );

}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( _CaptureVideoAddReturnBuf_work_flow_success_4 )
{
	Bool res = TRUE;

	// _CaptureVideoFindReturnBuf will return NULL
	cap_vid_add_ret_buf_gl.pPort->capture = CAPTURE_MODE_STREAM;  // !CAPTURE_MODE_STILL

	res = _CaptureVideoAddReturnBuf( cap_vid_add_ret_buf_gl.pPort, cap_vid_add_ret_buf_gl.vbuf );

	fail_if( ctrl_get_cnt_calloc() != 5, "work flow is changed -- %d", ctrl_get_cnt_calloc() );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( _CaptureVideoAddReturnBuf_work_flow_success_5 )
{
	Bool res = TRUE;

	// _CaptureVideoFindReturnBuf will return NULL
	cap_vid_add_ret_buf_gl.pPort->capture = CAPTURE_MODE_STREAM;  // !CAPTURE_MODE_STILL

	res = _CaptureVideoAddReturnBuf( cap_vid_add_ret_buf_gl.pPort, cap_vid_add_ret_buf_gl.vbuf );

	fail_if( cap_vid_add_ret_buf_gl.vbuf->ref_cnt != 1, "work flow is changed -- %d", res );
}
END_TEST;

/*---------------------------- _exynosCaptureVideoRemoveReturnBuf() ------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------*/
START_TEST( _CaptureVideoRemoveReturnBuf_work_flow_success_1 )
{
	EXYNOSBufInfo* temp = NULL;

	cap_vid_rmv_ret_buf_gl.info->vbuf->showing = TRUE; // !FALSE
	cap_vid_rmv_ret_buf_gl.pPort->clone = (EXYNOS_CloneObject*)1; // to call CloneQueue()

	_exynosCaptureVideoRemoveReturnBuf( cap_vid_rmv_ret_buf_gl.pPort, cap_vid_rmv_ret_buf_gl.info );

	temp = get_vbuf_gl();

	fail_if( temp->showing != FALSE, "work flow is changed -- %d", temp->showing );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( _CaptureVideoRemoveReturnBuf_work_flow_success_2 )
{
	EXYNOSBufInfo* temp = NULL;

	cap_vid_rmv_ret_buf_gl.pPort->clone = (EXYNOS_CloneObject*)1; // to call CloneQueue()

	_exynosCaptureVideoRemoveReturnBuf( cap_vid_rmv_ret_buf_gl.pPort, cap_vid_rmv_ret_buf_gl.info );

	temp = get_vbuf_gl();

	fail_if( temp->dirty != TRUE, "work flow is changed -- %d",	temp->dirty );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( _CaptureVideoRemoveReturnBuf_work_flow_success_3 )
{
	EXYNOSBufInfo* temp = NULL;

	cap_vid_rmv_ret_buf_gl.pPort->clone = (EXYNOS_CloneObject*)1; // to call CloneQueue()

	_exynosCaptureVideoRemoveReturnBuf( cap_vid_rmv_ret_buf_gl.pPort, cap_vid_rmv_ret_buf_gl.info );

	temp = get_vbuf_gl();

	fail_if( temp->ref_cnt != -1, "work flow is changed -- %d",	temp->ref_cnt );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( _CaptureVideoRemoveReturnBuf_work_flow_success_4 )
{
	_exynosCaptureVideoRemoveReturnBuf( cap_vid_rmv_ret_buf_gl.pPort, cap_vid_rmv_ret_buf_gl.info );

	fail_if( ctrl_get_cnt_calloc() != 2, "work flow is changed -- %d", ctrl_get_cnt_calloc() );
}
END_TEST;

/*--------------------------------- EXYNOSCaptureGetPortAttribute() ------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------*/
START_TEST( CaptureGetPortAttribute_work_flow_success_1 )
{
	int res = Success; 

	cap_get_port_atrb_gl.attribute = 9; 

	res = EXYNOSCaptureGetPortAttribute( NULL, cap_get_port_atrb_gl.attribute, NULL, NULL );

	fail_if( res != BadMatch, "work flow is changed -- %d", res );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( CaptureGetPortAttribute_work_flow_success_2 )
{
	int res = BadMatch; // !Success
       
        prepare_pPort( cap_get_port_atrb_gl.pPort );
        ScrnInfoPtr pScrn = cap_get_port_atrb_gl.pPort->pScrn;
	/* preparing to switching */
	capture_atoms[PAA_ROTATE_OFF -1].atom = 3; // !None and !11
	cap_get_port_atrb_gl.attribute = 3;	// must be equal to capture_atoms[].atom
               
	cap_get_port_atrb_gl.pPort->rotate_off =  11;

       /* PRINTF_ADR(pScrn); 
        PRINTF_ADR(cap_get_port_atrb_gl.pPort);
        PRINTVAL(cap_get_port_atrb_gl.value);
        */
         
	res = EXYNOSCaptureGetPortAttribute( pScrn, cap_get_port_atrb_gl.attribute,
                                                 &cap_get_port_atrb_gl.value, (pointer*) cap_get_port_atrb_gl.pPort );

	fail_if( cap_get_port_atrb_gl.value != 11, "work flow is changed 1 -- %d", cap_get_port_atrb_gl.value );
	fail_if( res != Success, "work flow is changed 2 -- %d", res );
        
        flush_pPort( cap_get_port_atrb_gl.pPort );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( CaptureGetPortAttribute_work_flow_success_3 )
{
	int res = Success;

        prepare_pPort( cap_get_port_atrb_gl.pPort );
        ScrnInfoPtr pScrn = cap_get_port_atrb_gl.pPort->pScrn;
	/* preparing to switching */
	capture_atoms[PAA_FORMAT -1 ].atom = 9; // !None and !11
	cap_get_port_atrb_gl.attribute = 9;		// must be equal to capture_atoms[].atom

	cap_get_port_atrb_gl.pPort->id = 11;    // any digital

	res = EXYNOSCaptureGetPortAttribute( pScrn, cap_get_port_atrb_gl.attribute,
								         &( cap_get_port_atrb_gl.value ), NULL );
       // fail_if( cap_get_port_atrb_gl.value != 11, "work flow is changed 1 -- %d", cap_get_port_atrb_gl.value );
	fail_if( res != BadMatch, "Dereference  failed -- %d", res );
        
        flush_pPort( cap_get_port_atrb_gl.pPort );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( CaptureGetPortAttribute_work_flow_success_4 )
{
	int res = BadMatch; // !Success

        prepare_pPort( cap_get_port_atrb_gl.pPort );
        ScrnInfoPtr pScrn = cap_get_port_atrb_gl.pPort->pScrn;
	/* preparing to switching */
	capture_atoms[PAA_CAPTURE -1].atom = 9;
	cap_get_port_atrb_gl.attribute = 9;		  // must be equal to capture_atoms[].atom

	cap_get_port_atrb_gl.pPort->capture = 11; 

	res = EXYNOSCaptureGetPortAttribute( pScrn, cap_get_port_atrb_gl.attribute,
								         &( cap_get_port_atrb_gl.value ), (pointer*)( cap_get_port_atrb_gl.pPort ) );

	fail_if( cap_get_port_atrb_gl.value != 11, "work flow is changed 1 -- %d", cap_get_port_atrb_gl.value );
	fail_if( res != Success, "work flow is changed 2 -- %d", res );
        
        flush_pPort( cap_get_port_atrb_gl.pPort );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( CaptureGetPortAttribute_work_flow_success_5 )
{
	int res = BadMatch; // !Success

        prepare_pPort( cap_get_port_atrb_gl.pPort );
        ScrnInfoPtr pScrn = cap_get_port_atrb_gl.pPort->pScrn;
	/* preparing to switching */
	capture_atoms[PAA_DATA_TYPE -1 ].atom = 9;  // !None and !11
	cap_get_port_atrb_gl.attribute = 9;	 // must be equal to capture_atoms[].atom

	cap_get_port_atrb_gl.pPort->data_type = TRUE; // any value

	res = EXYNOSCaptureGetPortAttribute( pScrn, cap_get_port_atrb_gl.attribute,
								         &( cap_get_port_atrb_gl.value ), (pointer*)( cap_get_port_atrb_gl.pPort ) );

	fail_if( cap_get_port_atrb_gl.value != (INT32)TRUE, "work flow is changed 1 -- %d", cap_get_port_atrb_gl.value );
	fail_if( res != Success, "work flow is changed 2 -- %d", res );
        
        flush_pPort( cap_get_port_atrb_gl.pPort );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( CaptureGetPortAttribute_work_flow_success_6 )
{
	int res = BadMatch; // !Success

        prepare_pPort( cap_get_port_atrb_gl.pPort );
        ScrnInfoPtr pScrn = cap_get_port_atrb_gl.pPort->pScrn;
	/* preparing to switching */
	capture_atoms[PAA_SOFTWARECOMPOSITE_ON -1 ].atom = 9;  // !None and !11
	cap_get_port_atrb_gl.attribute = 9;		  // must be equal to capture_atoms[].atom

	cap_get_port_atrb_gl.pPort->swcomp_on = TRUE; // any value

	res = EXYNOSCaptureGetPortAttribute( pScrn, cap_get_port_atrb_gl.attribute,
								         &( cap_get_port_atrb_gl.value ), (pointer*)( cap_get_port_atrb_gl.pPort ) );

	fail_if( cap_get_port_atrb_gl.value != (INT32)TRUE, "work flow is changed 1 -- %d", cap_get_port_atrb_gl.value );
	fail_if( res != Success, "work flow is changed 2 -- %d", res );
        
        flush_pPort( cap_get_port_atrb_gl.pPort );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( CaptureGetPortAttribute_work_flow_success_7 )
{
	int res =  BadMatch;//Success;

        prepare_pPort( cap_get_port_atrb_gl.pPort );
        ScrnInfoPtr pScrn = cap_get_port_atrb_gl.pPort->pScrn;
	/* preparing to switching */
	capture_atoms[PAA_DISPLAY -1 ].atom = 9;  // !None and !11
	cap_get_port_atrb_gl.attribute = 9;		  // must be equal to capture_atoms[].atom

	res = EXYNOSCaptureGetPortAttribute( pScrn, cap_get_port_atrb_gl.attribute,
								         &( cap_get_port_atrb_gl.value ), (pointer*)( cap_get_port_atrb_gl.pPort ) );

	fail_if( res != Success, "work flow is changed -- %d", res );
        flush_pPort( cap_get_port_atrb_gl.pPort );
}
END_TEST;

//--------------------------------- EXYNOSCaptureSetPortAttribute() -------------------------------------------
START_TEST( CaptureSetPortAttribute_work_flow_success_1 )
{
	int res = Success; // !BadMatch

	cap_get_port_atrb_gl.attribute = 9; // !11

	res = EXYNOSCaptureSetPortAttribute( NULL, cap_get_port_atrb_gl.attribute, 0, NULL );

	fail_if( res != BadMatch, "work flow is changed -- %d", res );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( CaptureSetPortAttribute_work_flow_success_2 )
{
	int res = BadMatch; // !Success

	/* preparing to switching */
	capture_atoms[PAA_FORMAT - 1].atom = 9; // !None and !11
	cap_get_port_atrb_gl.attribute = 9;		// must be equal to capture_atoms[].atom

	res = EXYNOSCaptureSetPortAttribute( NULL, cap_get_port_atrb_gl.attribute, 0,
			                            (pointer*)( cap_get_port_atrb_gl.pPort ) );

	fail_if( cap_get_port_atrb_gl.pPort->id != FOURCC_RGB32, "work flow is changed 1 -- %d",
			 cap_get_port_atrb_gl.pPort->id );
	fail_if( res != Success, "work flow is changed 2 -- %d", res );
}
END_TEST;

//-------------------------------------------------------------------------------------------------------------
START_TEST( CaptureSetPortAttribute_work_flow_success_3 )
{
	int res = BadMatch; // !Success

	/* preparing to switching */
	capture_atoms[PAA_CAPTURE - 1].atom = 9; // !None and !11
	cap_get_port_atrb_gl.attribute = 9;		// must be equal to capture_atoms[].atom

	//cap_get_port_atrb_gl.value =
	res = EXYNOSCaptureSetPortAttribute( NULL, cap_get_port_atrb_gl.attribute,
								         cap_get_port_atrb_gl.value, (pointer*)( cap_get_port_atrb_gl.pPort ) );

	fail_if( cap_get_port_atrb_gl.pPort->id != FOURCC_RGB32, "work flow is changed 1 -- %d",
			 cap_get_port_atrb_gl.pPort->id );
	fail_if( res != Success, "work flow is changed 2 -- %d", res );
}
END_TEST;


/*--------------------------------- _CaptureVideoPutStill()  -------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------*/
START_TEST( _CaptureVideoPutStill_work_flow_success_1 )
{
	int res = Success; // !BadMatch

	res = _CaptureVideoPutStill(cap_vid_add_ret_buf_gl.pPort);

	fail_if( res != BadMatch, "work flow is changed -- %d", res );
}
END_TEST;
/*--------------------------------- _g2dFormat() -------------------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------*/
 START_TEST( _g2dFormat_work_flow_success_1 )
{
        G2dColorMode ret=0;
        ret=_g2dFormat (FOURCC_RGB32);
        fail_if( ret == 0, "work flow is changed -- %d", ret );
}
END_TEST;

 START_TEST( _g2dFormat_test_format_FOURCC_ST12 )
{
        G2dColorMode ret=0;
        ret=_g2dFormat (FOURCC_ST12);
        fail_if( ret != 0, "_g2dFormat NOTSUPPORTED FORMAT(FOURCC_ST12) IN G2D NOW -- %d", ret );
}
END_TEST;

START_TEST( _g2dFormat_test_format_FOURCC_S420 )
{
        G2dColorMode ret=0;
        ret=_g2dFormat (FOURCC_S420);
        fail_if( ret != 0, "_g2dFormat NOTSUPPORTED FORMAT(FOURCC_S420) IN G2D NOW -- %d", ret );
}
END_TEST;


/*--------------------------------- _CaptureVideoGetUIBuffer() -----------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------*/
START_TEST( _CaptureVideoGetUIBuffer_work_flow_success_1 )
{
	Bool res=1;
	//cap_vid_add_ret_buf_gl.pPort = ctrl_calloc( 1, sizeof( CAPTUREPortInfo ) );
       
        prepare_pPort(cap_get_ui_buffer_gl.pPort); 

        //PRINTVAL(666666666);
        cap_get_ui_buffer_gl.pPort->pDraw = ctrl_calloc( 1, sizeof( DrawableRec ) );
        cap_get_ui_buffer_gl.pPort->pDraw->pScreen = ctrl_calloc( 1, sizeof(ScreenRec) );
        
        fake_CaptureVideoGetFrontBo2 = _CaptureVideoGetFrontBo2_test1;
	res = _CaptureVideoGetUIBuffer(cap_get_ui_buffer_gl.pPort, DRM_MODE_CONNECTOR_LVDS);
	fail_if( res == NULL , "work flow is changed -- %p", res );
        
        ctrl_free(cap_get_ui_buffer_gl.pPort->pDraw->pScreen);  
        ctrl_free(cap_get_ui_buffer_gl.pPort->pDraw); 
        flush_pPort(cap_get_ui_buffer_gl.pPort);
        //ctrl_free(cap_vid_add_ret_buf_gl.pPort); 
        
}
END_TEST;

START_TEST( _CaptureVideoGetUIBuffer_work_flow_success_2 )
{
	Bool res;	
        cap_vid_add_ret_buf_gl.pPort = ctrl_calloc( 1, sizeof( CAPTUREPortInfo ) );
        
        cap_vid_add_ret_buf_gl.pPort->capture = CAPTURE_MODE_STILL;
        
        prepare_pPort(cap_vid_add_ret_buf_gl.pPort); 
        cap_vid_add_ret_buf_gl.pPort->pDraw = ctrl_calloc( 1, sizeof( DrawableRec ) );
        cap_vid_add_ret_buf_gl.pPort->pDraw->pScreen = 0;
	res = _CaptureVideoGetUIBuffer(cap_vid_add_ret_buf_gl.pPort, DRM_MODE_CONNECTOR_LVDS);
        fail_if( res != NULL, "Must be failed pScreen = 0 -- %d", res );
 
        ctrl_free(cap_vid_add_ret_buf_gl.pPort->pDraw); 
        //ctrl_free(cap_vid_add_ret_buf_gl.pPort); 
        
         
}
END_TEST;

START_TEST( _CaptureVideoGetUIBuffer_work_flow_success_3 )
{
	Bool res;
        cap_vid_add_ret_buf_gl.pPort = ctrl_calloc( 1, sizeof( CAPTUREPortInfo ) );
        
        cap_vid_add_ret_buf_gl.pPort->capture = CAPTURE_MODE_STILL;
        
        prepare_pPort(cap_vid_add_ret_buf_gl.pPort); 
        cap_vid_add_ret_buf_gl.pPort->pDraw = 0;
       
	res = _CaptureVideoGetUIBuffer(cap_vid_add_ret_buf_gl.pPort, DRM_MODE_CONNECTOR_LVDS);
        fail_if( res != NULL, "Must be failed pDraw = 0 -- %d", res );
 
       // ctrl_free(cap_vid_add_ret_buf_gl.pPort); 
        
}
END_TEST;

START_TEST( _CaptureVideoGetUIBuffer_work_flow_success_4 )
{
        Bool res;
	cap_vid_add_ret_buf_gl.pPort = NULL;
        
                
	res = _CaptureVideoGetUIBuffer(cap_vid_add_ret_buf_gl.pPort, DRM_MODE_CONNECTOR_LVDS);
        fail_if( res != NULL, "Return value must be NULL if pPort = NULL -- %p", res );
        
}
END_TEST;



/*--------------------------------- _CaptureVideoGetDrawableBuffer() -----------------------------------------*/
/*------------------------------------------------------------------------------------------------------------*/
START_TEST( _CaptureVideoGetDrawableBuffer_work_flow_success_1 )
{
	//PixmapPtr res = NULL;//Success; // !BadMatch
        Bool res;
        cap_vid_add_ret_buf_gl.pPort = ctrl_calloc( 1, sizeof( CAPTUREPortInfo ) );
        
        cap_vid_add_ret_buf_gl.pPort->capture = CAPTURE_MODE_STILL;
        
        prepare_pPort(cap_vid_add_ret_buf_gl.pPort); 
        cap_vid_add_ret_buf_gl.pPort->pDraw = ctrl_calloc( 1, sizeof( DrawableRec ) );
        cap_vid_add_ret_buf_gl.pPort->pDraw->pScreen = ctrl_calloc( 1, sizeof(ScreenRec) );
        cap_vid_add_ret_buf_gl.pPort->pDraw->pScreen->myNum = 0;
	res = _CaptureVideoGetDrawableBuffer(cap_vid_add_ret_buf_gl.pPort);
        fail_if( res == NULL, "work flow is changed -- %d", res );
	
 
        ctrl_free(cap_vid_add_ret_buf_gl.pPort); 
        ctrl_free(cap_vid_add_ret_buf_gl.pPort->pDraw); 
        ctrl_free(cap_vid_add_ret_buf_gl.pPort->pDraw->pScreen);  
}
END_TEST;

START_TEST( _CaptureVideoGetDrawableBuffer_work_flow_success_2 )
{
	//PixmapPtr res = NULL;//Success; // !BadMatch
        Bool res;
        cap_vid_add_ret_buf_gl.pPort = ctrl_calloc( 1, sizeof( CAPTUREPortInfo ) );
        
        cap_vid_add_ret_buf_gl.pPort->capture = CAPTURE_MODE_STILL;
        
        prepare_pPort(cap_vid_add_ret_buf_gl.pPort); 
        cap_vid_add_ret_buf_gl.pPort->pDraw = ctrl_calloc( 1, sizeof( DrawableRec ) );
        cap_vid_add_ret_buf_gl.pPort->pDraw->pScreen = 0;
	res = _CaptureVideoGetDrawableBuffer(cap_vid_add_ret_buf_gl.pPort);
        fail_if( res != NULL, "Must be failed pScreen = 0 -- %d", res );
 
        ctrl_free(cap_vid_add_ret_buf_gl.pPort); 
        ctrl_free(cap_vid_add_ret_buf_gl.pPort->pDraw); 
         
}
END_TEST;

START_TEST( _CaptureVideoGetDrawableBuffer_work_flow_success_3 )
{
	//PixmapPtr res = NULL;//Success; // !BadMatch
        Bool res;
        cap_vid_add_ret_buf_gl.pPort = ctrl_calloc( 1, sizeof( CAPTUREPortInfo ) );
        
        cap_vid_add_ret_buf_gl.pPort->capture = CAPTURE_MODE_STILL;
        
        prepare_pPort(cap_vid_add_ret_buf_gl.pPort); 
        cap_vid_add_ret_buf_gl.pPort->pDraw = 0;
       
	res = _CaptureVideoGetDrawableBuffer(cap_vid_add_ret_buf_gl.pPort);
        fail_if( res != NULL, "Must be failed pDraw = 0 -- %d", res );
 
        ctrl_free(cap_vid_add_ret_buf_gl.pPort); 
        
}
END_TEST;

START_TEST( _CaptureVideoGetDrawableBuffer_work_flow_success_4 )
{
        Bool res;
	cap_vid_add_ret_buf_gl.pPort = NULL;
        
        cap_vid_add_ret_buf_gl.pPort->capture = CAPTURE_MODE_STILL;
        
	res = _CaptureVideoGetDrawableBuffer(cap_vid_add_ret_buf_gl.pPort);
        fail_if( res != NULL, "Must be failed pPort = 0 -- %d", res );
        
}
END_TEST;



/*-------------------------------------- _calculateSize() ----------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------*/
START_TEST( _calculateSize_work_flow_success_1 )
{
        int width,  height;
        xRectangle *crop;
        Bool res;
        
        crop = ctrl_calloc( 1, sizeof( xRectangle ) );  
        width= 720; height= 1280;
        crop->width = 720;
        crop->height = 1280;
        crop->x=0; crop->y= 0;
	res = _calculateSize ( width, height, crop);
        fail_if( res != TRUE, "work flow is changed -- %d", res );
        // fail_if( res != NULL, "Must be failed pPort = 0 -- %d", res );
        ctrl_free(crop);
}
END_TEST;

START_TEST( _calculateSize_work_flow_success_2 )
{
        int width,  height;
        xRectangle *crop;
        Bool res;
    
        crop = ctrl_calloc( 1, sizeof( xRectangle ) );  
        width = 0; height = 0;
        crop->width = 0;
        crop->height = 1280;
        crop->x = crop->y = 0;
	res = _calculateSize ( width, height, crop);
        fail_if( res != FALSE, "Must be failed width = 0; height = 0; -- %d", res );
        ctrl_free(crop);
}
END_TEST;

START_TEST( _calculateSize_work_flow_success_3 )
{
        int width,  height;
        xRectangle *crop;
        Bool res;
        
        crop = ctrl_calloc( 1, sizeof( xRectangle ) );     
        width = 720; height = 1280;
        crop->width = 0;
        crop->height = 0;
        crop->x=crop->y = 1500;
	res = _calculateSize ( width, height, crop);
        fail_if( res != FALSE, "Must be failed crop > width and heigth-- %d", res );
       // fail_if( res != NULL, "Must be failed pPort = 0 -- %d", res );
        ctrl_free(crop);
}
END_TEST;


/*--------------------------------- _CaptureVideoRetireTimeout() ---------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------*/
START_TEST( _CaptureVideoRetireTimeout_work_flow_success_1 )
{
        Bool res = 1; int tmp=0;
        
        prepare_pPort(cap_get_port_atrb_gl.pPort); 
        tmp = cap_get_port_atrb_gl.pPort->retire_time = GetTimeInMillis() - 200 ;
        
        cap_get_port_atrb_gl.pPort->retire_timer = ctrl_calloc( 1, sizeof( OsTimerPtr )); 
       	res = _CaptureVideoRetireTimeout (cap_get_port_atrb_gl.pPort->retire_timer, 
                                         GetTimeInMillis(), &cap_get_port_atrb_gl.pPort);

        fail_if( cap_get_port_atrb_gl.pPort->retire_time != tmp, "work flow is changed -- %d", res );
        ctrl_free (cap_get_port_atrb_gl.pPort->retire_timer);
        flush_pPort(cap_get_port_atrb_gl.pPort); 
        
}
END_TEST;

START_TEST( _CaptureVideoRetireTimeout_work_flow_success_2 )
{
        Bool res = 1; int tmp=0;
        
        prepare_pPort(cap_get_port_atrb_gl.pPort); 
        tmp = cap_get_port_atrb_gl.pPort->retire_time = GetTimeInMillis() - 200 ;
        
        cap_get_port_atrb_gl.pPort->retire_timer = ctrl_calloc( 1, sizeof( OsTimerPtr )); 
       	res = _CaptureVideoRetireTimeout (cap_get_port_atrb_gl.pPort->retire_timer, 
                                         GetTimeInMillis(), NULL);

        fail_if( res != 0, "Must be failed pPort = NULL -- %d", res );
        
        ctrl_free (cap_get_port_atrb_gl.pPort->retire_timer);
        flush_pPort(cap_get_port_atrb_gl.pPort); 
        
}
END_TEST;

START_TEST( _CaptureVideoRetireTimeout_work_flow_success_3 )
{
        Bool res = 1; int tmp=0;
        
        prepare_pPort(cap_get_port_atrb_gl.pPort); 
        cap_get_port_atrb_gl.pPort->retire_time = GetTimeInMillis() - 200 ;
        tmp = cap_get_port_atrb_gl.pPort->data_type = TRUE;
         
        tmp = cap_get_port_atrb_gl.pPort->retire_timer = ctrl_calloc( 1, sizeof( OsTimerPtr )); 
       	res = _CaptureVideoRetireTimeout (cap_get_port_atrb_gl.pPort->retire_timer, 
                                         GetTimeInMillis(), &cap_get_port_atrb_gl.pPort);

        fail_if( cap_get_port_atrb_gl.pPort->data_type != (INT32)TRUE, "work flow is changed -- %d", cap_get_port_atrb_gl.pPort->data_type );
        fail_if( cap_get_port_atrb_gl.pPort->retire_timer != tmp, "Must be failed retire_timer = NULL -- %p", 
                            cap_get_port_atrb_gl.pPort->retire_timer );
       
        ctrl_free (cap_get_port_atrb_gl.pPort->retire_timer);
        flush_pPort(cap_get_port_atrb_gl.pPort); 
        
}
END_TEST;

/*--------------------------------- _CaptureVideoPreProcess() ------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------*/

START_TEST( _CaptureVideoPreProcess_work_flow_success_1 )
{
   Bool res = BadMatch; int tmp=0;
   RegionPtr clipBoxes = ctrl_calloc( 1, sizeof( OsTimerPtr )); 
   prepare_pPort(cap_get_port_atrb_gl.pPort); 
   cap_get_port_atrb_gl.pPort->pDraw = ctrl_calloc( 1, sizeof( DrawableRec ) );
   
   _CaptureVideoPreProcess (cap_get_port_atrb_gl.pPort->pScrn,cap_get_port_atrb_gl.pPort,
                             clipBoxes, cap_get_port_atrb_gl.pPort->pDraw);
   fail_if( cap_get_port_atrb_gl.pPort->clipBoxes == NULL, "work flow is changed -- %p", cap_get_port_atrb_gl.pPort->clipBoxes );
      
   ctrl_free (clipBoxes);
   ctrl_free (cap_get_port_atrb_gl.pPort->pDraw);
   flush_pPort(cap_get_port_atrb_gl.pPort); 
        
}
END_TEST;

START_TEST( _CaptureVideoPreProcess_work_flow_success_2 )
{
   Bool res = BadMatch; int tmp=0;
   RegionPtr clipBoxes = ctrl_calloc( 1, sizeof( OsTimerPtr ));
   prepare_pPort(cap_get_port_atrb_gl.pPort); 
   cap_get_port_atrb_gl.pPort->pDraw = ctrl_calloc( 1, sizeof( DrawableRec ) );
   cap_get_port_atrb_gl.pPort->pDraw->id = 12;
   
   res = _CaptureVideoPreProcess (cap_get_port_atrb_gl.pPort->pScrn,cap_get_port_atrb_gl.pPort,
                             clipBoxes, cap_get_port_atrb_gl.pPort->pDraw);
   fail_if( cap_get_port_atrb_gl.pPort->pDraw == NULL, "work flow is changed 1 -- %p", cap_get_port_atrb_gl.pPort->clipBoxes );
   fail_if( cap_get_port_atrb_gl.pPort->pDraw->id != 12, "work flow is changed 2 -- %d", cap_get_port_atrb_gl.pPort->pDraw->id );
   
   ctrl_free (clipBoxes);
   ctrl_free (cap_get_port_atrb_gl.pPort->pDraw);
   flush_pPort(cap_get_port_atrb_gl.pPort); 
        
}
END_TEST;

START_TEST( _CaptureVideoPreProcess_work_flow_success_3 )
{
   Bool res = BadMatch; int tmp=0;
   RegionPtr clipBoxes = NULL;
   prepare_pPort(cap_get_port_atrb_gl.pPort); 
   cap_get_port_atrb_gl.pPort->pDraw = ctrl_calloc( 1, sizeof( DrawableRec ) );
   
   _CaptureVideoPreProcess (cap_get_port_atrb_gl.pPort->pScrn,cap_get_port_atrb_gl.pPort,
                             clipBoxes, cap_get_port_atrb_gl.pPort->pDraw);
   fail_if( cap_get_port_atrb_gl.pPort->clipBoxes != NULL, "work flow is changed -- %p", cap_get_port_atrb_gl.pPort->clipBoxes );
   ctrl_free (cap_get_port_atrb_gl.pPort->pDraw);
   flush_pPort(cap_get_port_atrb_gl.pPort); 
  
}
END_TEST;


/*--------------------------------- _CaptureVideoAddDrawableEvent() ------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------*/

START_TEST(  _CaptureVideoAddDrawableEvent_work_flow_success_1 )
{
   Bool res = BadAlloc; int tmp=0;
   
   prepare_pPort(cap_get_port_atrb_gl.pPort); 
   cap_get_port_atrb_gl.pPort->pDraw = ctrl_calloc( 1, sizeof( DrawableRec ) );
   
   res = _CaptureVideoAddDrawableEvent (cap_get_port_atrb_gl.pPort);
   fail_if( res != Success, "work flow is changed -- %d", res );
   ctrl_free (cap_get_port_atrb_gl.pPort->pDraw);
   flush_pPort(cap_get_port_atrb_gl.pPort); 
          
}
END_TEST;

START_TEST( _CaptureVideoAddDrawableEvent_work_flow_success_2 )
{
   Bool res = Success; int tmp=0;
   
   prepare_pPort(cap_get_port_atrb_gl.pPort); 
   cap_get_port_atrb_gl.pPort->pDraw = NULL;
   
   res = _CaptureVideoAddDrawableEvent (cap_get_port_atrb_gl.pPort);
   fail_if( res != BadImplementation, "work flow is changed -- %d", res );
  
   flush_pPort(cap_get_port_atrb_gl.pPort); 
        
}
END_TEST;

START_TEST( _CaptureVideoAddDrawableEvent_work_flow_success_3 )
{
   Bool res = Success; 
   
   prepare_pPort(cap_get_port_atrb_gl.pPort); 
   cap_get_port_atrb_gl.pPort->capture = 11;
   cap_get_port_atrb_gl.pPort->pDraw = NULL;
   //cap_get_port_atrb_gl.pPort->pScrn = NULL;
   
   res = _CaptureVideoAddDrawableEvent (cap_get_port_atrb_gl.pPort);
   fail_if( res != BadImplementation, "work flow is changed 1-- %p", res );
   fail_if( cap_get_port_atrb_gl.pPort->capture != 11, "work flow is changed 2-- %d", cap_get_port_atrb_gl.pPort->capture );
   flush_pPort(cap_get_port_atrb_gl.pPort); 
        
}
END_TEST;

/*--------------------------------- _CaptureVideoGetPixmap() -------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------*/
START_TEST(  _CaptureVideoGetPixmap_work_flow_success_1 )
{
   PixmapPtr res = NULL; 
   
   prepare_pPort(cap_get_port_atrb_gl.pPort); 
   cap_get_port_atrb_gl.pPort->pDraw = ctrl_calloc( 1, sizeof( DrawableRec ) );
   cap_get_port_atrb_gl.pPort->pDraw->pScreen = ctrl_calloc( 1, sizeof(ScreenRec) );
   cap_get_port_atrb_gl.pPort->pDraw->type = DRAWABLE_WINDOW + 2;
   res = _CaptureVideoGetPixmap (cap_get_port_atrb_gl.pPort->pDraw);
   fail_if( res != cap_get_port_atrb_gl.pPort->pDraw, "work flow is changed 1-- %p", res );
        
}
END_TEST;

START_TEST( _CaptureVideoGetPixmap_work_flow_success_2 )
{
   PixmapPtr res = NULL; 
   
   prepare_pPort(cap_get_port_atrb_gl.pPort); 
   cap_get_port_atrb_gl.pPort->pDraw = ctrl_calloc( 1, sizeof( DrawableRec ) );
   cap_get_port_atrb_gl.pPort->pDraw->pScreen = ctrl_calloc( 1, sizeof(ScreenRec) );
   cap_get_port_atrb_gl.pPort->pDraw->type = DRAWABLE_WINDOW + 2;
   cap_get_port_atrb_gl.pPort->capture = 11;
   res = _CaptureVideoGetPixmap (cap_get_port_atrb_gl.pPort->pDraw);
   fail_if( res != cap_get_port_atrb_gl.pPort->pDraw, "work flow is changed 1-- %p", res );
   fail_if( cap_get_port_atrb_gl.pPort->capture != 11, "work flow is changed 2-- %d", cap_get_port_atrb_gl.pPort->capture );     
}
END_TEST;

START_TEST( _CaptureVideoGetPixmap_work_flow_success_3 )
{

        
}
END_TEST;

/*--------------------------------- EXYNOSCaptureQueryBestSize() -------------------------------------------------*/
/*------------------------------------------------------------------------------------------------------------*/
START_TEST(  EXYNOSCaptureQueryBestSize_work_flow_success_1 )
{
  Bool motion = TRUE;
  unsigned int p_w, p_h;
  short vid_w, vid_h, dst_w, dst_h;
  
  prepare_pPort(cap_get_port_atrb_gl.pPort); 
  cap_get_port_atrb_gl.pPort->pScrn->virtualX = 720;
  cap_get_port_atrb_gl.pPort->pScrn->virtualY = 1280;
  dst_w = vid_w = 720;
  dst_h = vid_h = 1280; 
  EXYNOSCaptureQueryBestSize ( cap_get_port_atrb_gl.pPort->pScrn, motion, vid_w, vid_h, dst_w, dst_h,
                               &p_w, &p_h, (pointer) cap_get_port_atrb_gl.pPort);
  fail_if( p_w != cap_get_port_atrb_gl.pPort->pScrn->virtualX, "work flow is changed 1-- %d", p_w );
  fail_if( p_h != cap_get_port_atrb_gl.pPort->pScrn->virtualY, "work flow is changed 2-- %d", p_h );     
}
END_TEST;

START_TEST(  EXYNOSCaptureQueryBestSize_work_flow_success_2 )
{
  Bool motion = TRUE;
  unsigned int p_w, p_h;
  short vid_w, vid_h, dst_w, dst_h;
  
  prepare_pPort(cap_get_port_atrb_gl.pPort); 
  cap_get_port_atrb_gl.pPort->capture = 11;
 
  cap_get_port_atrb_gl.pPort->pScrn->virtualX = 720;
  cap_get_port_atrb_gl.pPort->pScrn->virtualY = 1280;
  dst_w = vid_w = 720;
  dst_h = vid_h = 1280; 
  EXYNOSCaptureQueryBestSize ( cap_get_port_atrb_gl.pPort->pScrn, motion, vid_w, vid_h, dst_w, dst_h,
                               &p_w, &p_h, (pointer) cap_get_port_atrb_gl.pPort);
  fail_if( cap_get_port_atrb_gl.pPort->capture != 11, "work flow is changed 1-- %d", cap_get_port_atrb_gl.pPort->capture );
       
}
END_TEST;

        
/*************************************************************************************************************
==============================================================================================================
==============================================================================================================
==============================================================================================================*/

int test_video_capture_suite( run_stat_data* stat )
{
  Suite* s = suite_create( "VIDEO_CAPTURE.C" );

  // create test cases

  TCase* tc_get_port_atom = tcase_create( "_exynosVideoCaptureGetPortAtom" );
  TCase* tc_capture_video_find_ret_buf = tcase_create( "_CaptureVideoFindReturnBuf" );
  TCase* tc_capture_video_add_ret_buf = tcase_create( "_CaptureVideoAddReturnBuf" );
  TCase* tc_capture_video_rmv_ret_buf = tcase_create( "_exynosCaptureVideoRemoveReturnBuf" );
  TCase* tc_capture_get_port_atrb = tcase_create( "EXYNOSCaptureGetPortAttribute" );
  TCase* tc_capture_set_port_atrb = tcase_create( "EXYNOSCaptureSetPortAttribute" );
  TCase* tc_capture_put_still = tcase_create( "_CaptureVideoPutStill" );
  TCase* tc_capture_getuibuffer = tcase_create( "_CaptureVideoGetUIBuffer" );
  TCase* tc_capture_g2dformat =  tcase_create( "_g2dFormat" );
  TCase* tc_capture_getdrawablebuffer =  tcase_create( "_CaptureVideoGetDrawableBuffer" );
  TCase* tc_capture_calculatesize =  tcase_create( "_calculateSize" );
  
  TCase* tc_capture_videoretiretimeout =  tcase_create( "_CaptureVideoRetireTimeout" );
  TCase* tc_capture_videopreprocess =  tcase_create( "_CaptureVideoPreProcess" );
  TCase* tc_capture_add_drawable_event =  tcase_create( "_CaptureVideoAddDrawableEvent" ); 
  TCase* tc_capture_videogetpixmap =  tcase_create( "_CaptureVideoGetPixmap" ); 
  TCase* tc_capture_query_best_size =  tcase_create( " EXYNOSCaptureQueryBestSize " );
  // add unit-tests to test cases

  /*----------  _exynosVideoCaptureGetPortAtom() -----------------------------------------------*/
  tcase_add_test( tc_get_port_atom, _exynosVideoCaptureGetPortAtom_work_flow_success_PAA_MIN );
  tcase_add_test( tc_get_port_atom, _exynosVideoCaptureGetPortAtom_work_flow_success_ABOVEPAA_MAX );
  tcase_add_test( tc_get_port_atom, _exynosVideoCaptureGetPortAtom_work_flow_success_PAA_CAPTURE );
  tcase_add_test( tc_get_port_atom, _exynosVideoCaptureGetPortAtom_work_flow_success_PAA_FORMAT );
  tcase_add_test( tc_get_port_atom, _exynosVideoCaptureGetPortAtom_work_flow_success_PAA_DISPLAY );
  tcase_add_test( tc_get_port_atom, _exynosVideoCaptureGetPortAtom_work_flow_success_PAA_ROTATE_OFF );
  tcase_add_test( tc_get_port_atom, _exynosVideoCaptureGetPortAtom_work_flow_success_PAA_DATA_TYPE );
  tcase_add_test( tc_get_port_atom, _exynosVideoCaptureGetPortAtom_work_flow_success_PAA_SOFTWARECOMPOSITE_ON );
  tcase_add_test( tc_get_port_atom, _exynosVideoCaptureGetPortAtom_work_flow_success_PAA_RETBUF );
  
  /*----------  _CaptureVideoFindReturnBuf() --=-----------------------------------------------*/
  tcase_add_checked_fixture( tc_capture_video_find_ret_buf, setup_CaptureVideoFindReturnBuf,
		  	  	  	  	  	 teardown_CaptureVideoFindReturnBuf );
  tcase_add_test( tc_capture_video_find_ret_buf, _CaptureVideoFindReturnBuf_work_flow_success_1 );
  tcase_add_test( tc_capture_video_find_ret_buf, _CaptureVideoFindReturnBuf_work_flow_success_2 );
  tcase_add_test( tc_capture_video_find_ret_buf, _CaptureVideoFindReturnBuf_work_flow_success_3 );

  /*----------  _CaptureVideoAddReturnBuf() ---------------------------------------------------*/
  tcase_add_checked_fixture( tc_capture_video_add_ret_buf, setup_CaptureVideoAddReturnBuf,
  		  	  	  	  	  teardown_CaptureVideoAddReturnBuf );
  tcase_add_test( tc_capture_video_add_ret_buf, _CaptureVideoAddReturnBuf_work_flow_success_1 );
  tcase_add_test( tc_capture_video_add_ret_buf, _CaptureVideoAddReturnBuf_work_flow_success_2 );
  tcase_add_test( tc_capture_video_add_ret_buf, _CaptureVideoAddReturnBuf_work_flow_success_3 );
  tcase_add_test( tc_capture_video_add_ret_buf, _CaptureVideoAddReturnBuf_work_flow_success_4 );
  tcase_add_test( tc_capture_video_add_ret_buf, _CaptureVideoAddReturnBuf_work_flow_success_5 );


  /*----------  _exynosCaptureVideoRemoveReturnBuf() -------------------------------------------*/
  tcase_add_checked_fixture( tc_capture_video_rmv_ret_buf, setup_CaptureVideoRemoveReturnBuf,
  		  	  	  	  	  teardown_CaptureVideoRemoveReturnBuf );
  tcase_add_test( tc_capture_video_rmv_ret_buf, _CaptureVideoRemoveReturnBuf_work_flow_success_1 );
  tcase_add_test( tc_capture_video_rmv_ret_buf, _CaptureVideoRemoveReturnBuf_work_flow_success_2 );
  tcase_add_test( tc_capture_video_rmv_ret_buf, _CaptureVideoRemoveReturnBuf_work_flow_success_3 );
  tcase_add_test( tc_capture_video_rmv_ret_buf, _CaptureVideoRemoveReturnBuf_work_flow_success_4 );

  /*----------  EXYNOSCaptureGetPortAttribute() ------------------------------------------------*/
  tcase_add_checked_fixture( tc_capture_get_port_atrb, setup_CaptureGetPortAttribute,
  		  	  	  	  	  teardown_CaptureGetPortAttribute );
  tcase_add_test( tc_capture_get_port_atrb, CaptureGetPortAttribute_work_flow_success_1 );
  tcase_add_test( tc_capture_get_port_atrb, CaptureGetPortAttribute_work_flow_success_2 );
  tcase_add_test( tc_capture_get_port_atrb, CaptureGetPortAttribute_work_flow_success_3 );
  tcase_add_test( tc_capture_get_port_atrb, CaptureGetPortAttribute_work_flow_success_4 );
  tcase_add_test( tc_capture_get_port_atrb, CaptureGetPortAttribute_work_flow_success_5 );
  tcase_add_test( tc_capture_get_port_atrb, CaptureGetPortAttribute_work_flow_success_6 );
  tcase_add_test( tc_capture_get_port_atrb, CaptureGetPortAttribute_work_flow_success_7 );
 
  /*----------  EXYNOSCaptureSetPortAttribute() -------------------------------------------------*/
  // fixture is some as in tc_capture_get_port_atrb
  tcase_add_checked_fixture( tc_capture_set_port_atrb, setup_CaptureGetPortAttribute,
  		  	  	  	  	  teardown_CaptureGetPortAttribute );
  tcase_add_test( tc_capture_set_port_atrb, CaptureSetPortAttribute_work_flow_success_1 );
  tcase_add_test( tc_capture_set_port_atrb, CaptureSetPortAttribute_work_flow_success_2 );

  /*-------------------  _g2dFormat() ------------------------------------------------------------*/
   
  tcase_add_test( tc_capture_g2dformat, _g2dFormat_work_flow_success_1 );
  tcase_add_test( tc_capture_g2dformat, _g2dFormat_test_format_FOURCC_ST12 );
  tcase_add_test( tc_capture_g2dformat, _g2dFormat_test_format_FOURCC_S420 );
     
  /*-------------------  _CaptureVideoPutStill() -------------------------------------------------*/
  tcase_add_checked_fixture( tc_capture_put_still, setup_CaptureVideoPutStill,
  		  	  	  	  	  teardown_CaptureVideoPutStill );
  tcase_add_test( tc_capture_put_still, _CaptureVideoPutStill_work_flow_success_1 );
  
  /*-------------------  _CaptureVideoGetUIBuffer() ----------------------------------------------*/
   tcase_add_checked_fixture( tc_capture_getuibuffer, setup_CaptureVideoGetUIBuffer,
  		  	  	  	  	  teardown_CaptureVideoGetUIBuffer );
  tcase_add_test( tc_capture_getuibuffer, _CaptureVideoGetUIBuffer_work_flow_success_1 );
  tcase_add_test( tc_capture_getuibuffer, _CaptureVideoGetUIBuffer_work_flow_success_2 );
  tcase_add_test( tc_capture_getuibuffer, _CaptureVideoGetUIBuffer_work_flow_success_3 );
  tcase_add_test( tc_capture_getuibuffer, _CaptureVideoGetUIBuffer_work_flow_success_4 );
 
  /*-------------------  _CaptureVideoGetDrawableBuffer() -----------------------------------------*/
  tcase_add_checked_fixture( tc_capture_getuibuffer, setup_CaptureVideoGetUIBuffer,
  		  	  	  	  	  teardown_CaptureVideoGetUIBuffer );
  tcase_add_test( tc_capture_getdrawablebuffer, _CaptureVideoGetDrawableBuffer_work_flow_success_1 );
  tcase_add_test( tc_capture_getdrawablebuffer, _CaptureVideoGetDrawableBuffer_work_flow_success_2 );
  tcase_add_test( tc_capture_getdrawablebuffer, _CaptureVideoGetDrawableBuffer_work_flow_success_3 );
  tcase_add_test( tc_capture_getdrawablebuffer, _CaptureVideoGetDrawableBuffer_work_flow_success_4 );
  
/*-------------------  _calculateSize() ------------------------------------------------------------*/
tcase_add_test( tc_capture_calculatesize, _calculateSize_work_flow_success_1 );
tcase_add_test( tc_capture_calculatesize, _calculateSize_work_flow_success_2 ); 
tcase_add_test( tc_capture_calculatesize, _calculateSize_work_flow_success_3 );

/*-------------------  _CaptureVideoRetireTimeout() -------------------------------------------------*/
tcase_add_checked_fixture( tc_capture_videoretiretimeout, setup_CaptureGetPortAttribute,
  		  	  	  	  	  teardown_CaptureGetPortAttribute );
tcase_add_test( tc_capture_videoretiretimeout, _CaptureVideoRetireTimeout_work_flow_success_1 );
tcase_add_test( tc_capture_videoretiretimeout, _CaptureVideoRetireTimeout_work_flow_success_2 ); 
tcase_add_test( tc_capture_videoretiretimeout, _CaptureVideoRetireTimeout_work_flow_success_3 );


/*-------------------  _CaptureVideoPreProcess() ----------------------------------------------------*/
tcase_add_checked_fixture( tc_capture_videopreprocess, setup_CaptureGetPortAttribute,
  		  	  	  	  	  teardown_CaptureGetPortAttribute );
tcase_add_test( tc_capture_videopreprocess, _CaptureVideoPreProcess_work_flow_success_1 );
tcase_add_test( tc_capture_videopreprocess, _CaptureVideoPreProcess_work_flow_success_2 ); 
tcase_add_test( tc_capture_videopreprocess, _CaptureVideoPreProcess_work_flow_success_3 );

/*-------------------  _CaptureVideoAddDrawableEvent() -----------------------------------------------*/
tcase_add_checked_fixture( tc_capture_add_drawable_event, setup_CaptureGetPortAttribute,
  		  	  	  	  	  teardown_CaptureGetPortAttribute );
tcase_add_test( tc_capture_add_drawable_event, _CaptureVideoAddDrawableEvent_work_flow_success_1 );
tcase_add_test( tc_capture_add_drawable_event, _CaptureVideoAddDrawableEvent_work_flow_success_2 ); 
tcase_add_test( tc_capture_add_drawable_event, _CaptureVideoAddDrawableEvent_work_flow_success_3 );

/*-------------------  _CaptureVideoGetPixmap() ------------------------------------------------------*/
tcase_add_checked_fixture( tc_capture_videogetpixmap, setup_CaptureGetPortAttribute,
  		  	  	  	  	  teardown_CaptureGetPortAttribute );
tcase_add_test( tc_capture_videogetpixmap, _CaptureVideoGetPixmap_work_flow_success_1 );
tcase_add_test( tc_capture_videogetpixmap, _CaptureVideoGetPixmap_work_flow_success_2 ); 
tcase_add_test( tc_capture_videogetpixmap, _CaptureVideoGetPixmap_work_flow_success_3 );
  
/*-------------------  EXYNOSCaptureQueryBestSize() ------------------------------------------------------*/
tcase_add_checked_fixture( tc_capture_query_best_size, setup_CaptureGetPortAttribute,
  		  	  	  	  	  teardown_CaptureGetPortAttribute );
tcase_add_test( tc_capture_query_best_size, EXYNOSCaptureQueryBestSize_work_flow_success_1 );
tcase_add_test( tc_capture_query_best_size, EXYNOSCaptureQueryBestSize_work_flow_success_2 );
 /*--   add test cases to 's' suite    --*/

  suite_add_tcase( s, tc_get_port_atom );
  suite_add_tcase( s, tc_capture_video_find_ret_buf );
  suite_add_tcase( s, tc_capture_video_add_ret_buf );
  suite_add_tcase( s, tc_capture_video_rmv_ret_buf );
  suite_add_tcase( s, tc_capture_get_port_atrb );
  suite_add_tcase( s, tc_capture_set_port_atrb );
  suite_add_tcase( s, tc_capture_g2dformat );
 /* suite_add_tcase( s, tc_capture_put_still );
  suite_add_tcase( s, tc_capture_getuibuffer );
  suite_add_tcase( s, tc_capture_getdrawablebuffer );*/
  suite_add_tcase( s, tc_capture_calculatesize );
  
  suite_add_tcase( s, tc_capture_videoretiretimeout );
  suite_add_tcase( s, tc_capture_videopreprocess );
  suite_add_tcase( s, tc_capture_add_drawable_event );
  suite_add_tcase( s, tc_capture_videogetpixmap );
  suite_add_tcase( s, tc_capture_query_best_size );  
  
  /**tc_num += 6;
  *tested_func_num += 6;
  *not_tested_func_num += 100500;
*/

  
  SRunner *sr = srunner_create(s);
  srunner_set_log(sr, "tests_capture.log");
  srunner_run_all(sr, CK_NORMAL);
  stat->num_failure = srunner_ntests_failed(sr);
  stat->num_pass = srunner_ntests_run(sr);
 
  return 0;
}
