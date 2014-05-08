/*
 * test_clone.c
 *
 *  Created on: Oct 9, 2013
 *      Author: svoloshynov
 */

#include "test_clone.h"

#define PRINTF printf( "\n kyky \n" );
#define PRINTFVAL( val ) printf( "\n %d \n", val );

//fake functions
#define exynosVideoIppQueueBuf( fd, buf ) testExynosVideoIppQueueBuf( fd, buf )
#define exynosVideoIppCmdCtrl( fd, cln )  testExynosVideoIppCmdCtrl( fd, cln )

#include "./display/exynos_clone.c"

struct drm_exynos_ipp_queue_buf check_ExynosVideoIppQueueBuf;//Structure for check exynosVideoIppQueueBuf()
struct drm_exynos_ipp_cmd_ctrl  check_ExynosVideoIppCmdCtrl;//Structure for check exynosVideoIppCmdCtrl()

extern int cnt_drmModeSetPlane;

//Implementation fake functions
//=======================================================================================================================

int testIoctl (int __fd, unsigned long int __request, void* buf)
{
	return 0;
}

Bool testExynosVideoIppQueueBuf (int fd, struct drm_exynos_ipp_queue_buf *buf)
{

	//fill drm_exynos_ipp_queue_buf check_ExynosVideoIppQueueBuf structure for testing her
	check_ExynosVideoIppQueueBuf.ops_id    = buf->ops_id;
	check_ExynosVideoIppQueueBuf.buf_type  = buf->buf_type;
	check_ExynosVideoIppQueueBuf.prop_id   = buf->prop_id;
	check_ExynosVideoIppQueueBuf.buf_id    = buf->buf_id;
	check_ExynosVideoIppQueueBuf.user_data = buf->user_data;

	return TRUE;
}

Bool testExynosVideoIppCmdCtrl (int fd, struct drm_exynos_ipp_cmd_ctrl *ctrl)
{
	//fill drm_exynos_ipp_cmd_ctrl check_ExynosVideoIppQueueBuf structure for testing her
	check_ExynosVideoIppCmdCtrl.prop_id = ctrl->prop_id;
	check_ExynosVideoIppCmdCtrl.ctrl    = ctrl->ctrl;

	return TRUE;
}

//=======================================================================================================================

//This function using for test_clone->info_list->func = _exynosDisplayCloneCloseFunc_fake or as argument of some function which
//receive pointer of this type function
static void _exynosDisplayCloneCloseFunc_fake ( EXYNOS_CloneObject* clone, EXYNOS_CLONE_NOTIFY noti, void *noti_data, void *user_data )
{

}

//This function using for test_clone->info_list->func = _exynosDisplayCloneCloseFunc_fake or as argument of some function which
//receive pointer of this type function
int value_exynosDisplayCloneCloseFunc_fakeTwo = 0;
static int _exynosDisplayCloneCloseFunc_fakeTwo ( EXYNOS_CloneObject* clone, EXYNOS_CLONE_NOTIFY noti, void *noti_data, void *user_data )
{
	value_exynosDisplayCloneCloseFunc_fakeTwo = 1;
	return 1;
}

//Preparation
//***********************************************************************************************************************
//=======================================================================================================================
//Two next functions are needed for creating ScrnInfoPtr pScr structure for tests
//First
static EXYNOSOutputPrivPtr create_output( uint32_t type )
{
	EXYNOSOutputPrivPtr out=calloc(1,sizeof(EXYNOSOutputPrivRec));
	out->mode_output=calloc(1,sizeof(drmModeConnector));
	out->mode_output->connector_type=type;
	out->mode_encoder=calloc(1,sizeof(drmModeEncoder));
	out->mode_encoder->crtc_id=7;
	return out;
}
//Second
static void prepare( ScrnInfoPtr pScr )
{
	//EXYNOSOutputPrivPtr pOutputPriv = calloc (1, sizeof (EXYNOSOutputPrivRec));
	pScr->driverPrivate=  calloc(1,sizeof(EXYNOSRec));
	EXYNOSRec* pExynos=pScr->driverPrivate;
	pExynos->pDispInfo=calloc(1,sizeof(EXYNOSDispInfoRec));
	pExynos->pDispInfo->conn_mode = DISPLAY_CONN_MODE_HDMI;
	pExynos->pDispInfo->main_lcd_mode.hdisplay=1024;
	pExynos->pDispInfo->main_lcd_mode.vdisplay=768;
	pExynos->pDispInfo->ext_connector_mode.hdisplay=100;
	pExynos->pDispInfo->ext_connector_mode.vdisplay=100;

	pExynos->pDispInfo->mode_res=calloc(1,sizeof(drmModeRes));
	pExynos->pDispInfo->mode_res->count_connectors=1;
	pExynos->pDispInfo->mode_res->count_crtcs=1;
	pExynos->pDispInfo->mode_res->count_encoders=1;

	xorg_list_init (&pExynos->pDispInfo->crtcs);
	xorg_list_init (&pExynos->pDispInfo->outputs);
	xorg_list_init (&pExynos->pDispInfo->planes);

	EXYNOSOutputPrivPtr o1=create_output(DRM_MODE_CONNECTOR_HDMIA);
	xorg_list_add(&o1->link, &pExynos->pDispInfo->outputs);
	EXYNOSOutputPrivPtr o2=create_output(DRM_MODE_CONNECTOR_LVDS);
	xorg_list_add(&o2->link, &pExynos->pDispInfo->outputs);
}
//=======================================================================================================================

//main values which using in the exynos_clone.c
static	EXYNOS_CloneObject *test_keep_clone = NULL;
static  EXYNOS_CloneObject *test_clone = NULL;
static  EXYNOSPtr pExynos = NULL;
static	ScrnInfoRec scr;

//Fixture for scr initialization
void setupInitTestKeepClone( void )
{
	prepare( &scr );
}

void teardownInitTestKeepClone( void )
{

}

//Fixture which allocate memory for values
void setupAllocMemoryTwoParam( void )
{
	test_clone = ctrl_calloc( 1, sizeof( EXYNOS_CloneObject ) );
	test_clone->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
}

void teardownAllocMemoryTwoParam ( void )
{
	test_clone = NULL;
}

//Fixture which allocate memory for values
void setupAllocMemoryThreeParam( void )
{
	test_clone = ctrl_calloc( 1, sizeof( EXYNOS_CloneObject ) );
	test_clone->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	test_clone->pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
}

void teardownAllocMemoryThreeParam ( void )
{
	test_clone = NULL;
}

void setupAllocMemoryFourParam( void )
{
	test_clone = ctrl_calloc( 1, sizeof( EXYNOS_CloneObject ) );
	test_clone->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	test_clone->pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	pExynos = test_clone->pScrn->driverPrivate;
	pExynos->pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );
}

void teardownAllocMemoryFourParam ( void )
{
	test_clone = NULL;
	pExynos = NULL;
}

//***********************************************************************************************************************

//=======================================================================================================================
//_exynosCloneSupportFormat
START_TEST (_exynosCloneSupportFormat_test)
{
	unsigned int drmfmt, t=8;
	drmfmt=_exynosCloneSupportFormat(FOURCC_RGB32);
	fail_if (drmfmt ==0 ," FOURCC_RGB32 should support");

	drmfmt=_exynosCloneSupportFormat(FOURCC_ST12);
	fail_if (drmfmt ==0 ," FOURCC_ST12 should support");
	fail_if (drmfmt != DRM_FORMAT_NV12MT,"format id error");

	drmfmt=_exynosCloneSupportFormat(FOURCC_SN12);
	fail_if (drmfmt ==0 ," FOURCC_SN12 should support");
	fail_if (drmfmt != DRM_FORMAT_NV12M,"format id error");

	drmfmt=_exynosCloneSupportFormat(FOURCC_RGB565);
	fail_if (drmfmt !=0 ," should not support");
}
END_TEST;
//=======================================================================================================================

//=======================================================================================================================
//calculateSize
START_TEST (calculateSize_test)
{
	xRectangle rect ;
	memset(&rect, 0, sizeof(rect));
	calculateSize (0,200, 640, 480, &rect, TRUE);
	fail_if (rect.height !=0 ,"h");

	calculateSize (100,200, 0, 480, &rect, TRUE);
	fail_if (rect.height !=0 ,"h");

	calculateSize (100,200, 640, 480, &rect, TRUE);
	fail_if (rect.width ==0 ,"w");
}END_TEST;
//=======================================================================================================================

//=======================================================================================================================
/* _exynosCloneInit(ScrnInfoPtr pScrn, unsigned int id, int width, int height, Bool scanout,
				   int hz, Bool need_rotate_hook, const char *func) */
START_TEST ( _exynosCloneInit_PScrnNull_Sucsses )
{
	// 1, 2, 3, 4 random numbers
	test_keep_clone = _exynosCloneInit( NULL, 1, 2, 3, TRUE, 4, FALSE, "test" );
	fail_if( ctrl_get_cnt_calloc() == 0, "Memory for test_keep_clone should be allocated" );
}
END_TEST

START_TEST ( _exynosCloneInit_test_alloc_memory )
{
	//Check that function allocate memory
	// 1, 2, 3, 4 random numbers
	test_keep_clone = _exynosCloneInit( &scr, 1, 2, 3, TRUE, 4, FALSE, "test" );
	fail_if( ctrl_get_cnt_calloc() == 0, "Memory for test_keep_clone should be allocated" );
}
END_TEST

START_TEST ( _exynosCloneInit_test_filling_fields_one )
{
	//Check filling fields of test_keep_clone structure with transmitted function arguments
	// 1, 2, 3, 4 random numbers
	test_keep_clone = _exynosCloneInit( &scr, 1, 2, 3, TRUE, 4, FALSE, "test" );
	fail_if( test_keep_clone->id      != 1,     "id should be equivalent" );
	fail_if( test_keep_clone->width   != 2,     "width should be equivalent" );
	fail_if( test_keep_clone->height  != 3,     "height should be equivalent" );
	fail_if( test_keep_clone->scanout != TRUE,  "scanout should be equivalent" );
	fail_if( test_keep_clone->hz      != 4,     "hz should be equivalent" );
	fail_if( test_keep_clone->pScrn   != &scr,  "Adress of pScrn is not equivalent to clone->pScrn" );
}
END_TEST

START_TEST( _exynosCloneInit_test_filling_fields_two )
{
	//Check if HZ < 0
	// 1, 2, 3, -4 random numbers
	test_keep_clone = _exynosCloneInit( &scr, 1, 2, 3, FALSE, -4, TRUE,"test" );
	fail_if( test_keep_clone->hz != 60, "if hz < 0 test_keep_clone->hz should be 60" );
}
END_TEST

START_TEST ( _exynosCloneInit_test_negative_values )
{
	//Check if arguments are negative
	// 1, -2, -3, 4 random numbers
	test_keep_clone = _exynosCloneInit( &scr, 1, -2, -3, TRUE, 4, FALSE, "test" );
	fail_if( test_keep_clone->width  < 0, "width can not be < 0" );
	fail_if( test_keep_clone->height < 0, "height can not be < 0" );
}
END_TEST
//=======================================================================================================================

//=======================================================================================================================
/* exynosCloneClose(EXYNOS_CloneObject* clone) */
START_TEST( exynosCloneClose_cloneNull )
{
	//Segmentation fault must not occur
	exynosCloneClose( NULL );
}
END_TEST

START_TEST( exynosCloneClose_test_free_memory_one )
{
	prepare( &scr );
	displaySetDispSetMode( &scr, DISPLAY_SET_MODE_CLONE );
	EXYNOS_CloneObject *test_clone = ( EXYNOS_CloneObject* )( ( ( EXYNOSRec* )scr.driverPrivate )->clone );

	test_clone->prop_id = -1;

	exynosCloneClose( test_clone );

	//Check that function free memory
	fail_if( ctrl_get_cnt_calloc() != 2, "Memory should be free" );
}
END_TEST

START_TEST( exynosCloneClose_test_free_memory_two )
{
	prepare( &scr );
	displaySetDispSetMode( &scr, DISPLAY_SET_MODE_CLONE );
	EXYNOS_CloneObject *test_clone = ( EXYNOS_CloneObject* )( ( ( EXYNOSRec* )scr.driverPrivate )->clone );

	exynosCloneClose( test_clone );

	//test_keep_clone after function should be NULL
	fail_if( test_keep_clone != NULL, "Pointer should be NULL" );
}
END_TEST
//=======================================================================================================================

//=======================================================================================================================
/* exynosCloneHandleIppEvent( int fd, unsigned int *buf_idx, void *data ) */
START_TEST( exynosCloneHandleIppEvent_test_one )
{
	//ScrnInfoRec scr;
	unsigned arr_param[10]={0,};
	int fd = 0;

	//If void *data == NULL should not be Segmentation fault
	exynosCloneHandleIppEvent( fd, arr_param, NULL );
}
END_TEST

START_TEST( exynosCloneHandleIppEvent_test_two )
{
	//ScrnInfoRec scr;
    prepare( &scr );
    unsigned arr_param[10]={0,};
	int fd = 0;

	//If unsigned int *buf_idx == NULL should not be Segmentation fault
	exynosCloneHandleIppEvent( fd, NULL,( ( EXYNOSRec* )scr.driverPrivate )->clone );
}
END_TEST

START_TEST( exynosCloneHandleIppEvent_test_three )
{
	//ScrnInfoRec scr;
    prepare( &scr );
    unsigned arr_param[10]={0,};
	int fd = 0;

	//If &test_kepp_clone != &pscrn should not be Segmentation fault
	exynosCloneHandleIppEvent( fd, arr_param,( ( EXYNOSRec* )scr.driverPrivate )->clone );
}
END_TEST

START_TEST (exynosCloneHandleIppEvent_test_four )
{
	int fd = 0;
	unsigned arr_param[10]={0,};
	int index = 1;

	prepare( &scr );
	displaySetDispSetMode( &scr, DISPLAY_SET_MODE_CLONE );
	EXYNOS_CloneObject *test_clone = ( EXYNOS_CloneObject* )( ( ( EXYNOSRec* )scr.driverPrivate )->clone );

	exynosCloneHandleIppEvent( fd, arr_param, scr.driverPrivate );

	fail_if( test_clone->queued[index] != TRUE, "should be TRUE" );
}
END_TEST
//=======================================================================================================================


//=======================================================================================================================
/* _exynosCloneQueue(clone, index) */
START_TEST( _exynosCloneQueue_test_one )
{
	int index = 1;

	//Check if clone == NULL should not be Segmetation fault
	_exynosCloneQueue( NULL, index );
}
END_TEST

/* EXYNOSPTR (clone->pScrn)->drm_fd */
START_TEST( macro_EXYNOSPTR_ClonePScrnDrm_fd_IfDriverPrivateNull_Failure )
{
	int index = 1;

	test_clone->dst_buf[index] = ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );
	test_clone->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );

	//If pScrn->DriverPrivate == NULL should not be segmentation fault
	_exynosCloneQueue( test_clone, index );
}
END_TEST

START_TEST( _exynosCloneQueue_CloneDstBufNull_Success )
{
	int index = 1;

	//If clone->dst_buf == NULL should not be segmentation fault
	_exynosCloneQueue( test_clone, index );
}
END_TEST

START_TEST( _exynosCloneQueue_test_negativeValue )
{
	int index = -1;
	int i = 0;

	// number 4 equivalence to CLONE_BUF_MAX
	test_clone->dst_buf[ index ] = ctrl_calloc( 1, sizeof( EXYNOSBufInfoPtr ) );

	test_clone->dst_buf[ index ]->showing = FALSE;

	//Check if clone == NULL should not be Segmetation fault
	_exynosCloneQueue( test_clone, index );

	fail_if(test_clone->queued[index] != FALSE, "test_clone should be TRUE");
}
END_TEST


START_TEST( _exynosCloneQueue_test_initBuf )
{
	int index = 1;
	test_clone->dst_buf[index] = ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );
	EXYNOSBufInfo *dstb = test_clone->dst_buf[index];

	dstb->showing = FALSE;

	int i = 0;
	for( i = 0; i < EXYNOS_DRM_PLANAR_MAX; ++i)
		dstb->handles[i].ptr = 1;

	/*
    buf.ops_id = EXYNOS_DRM_OPS_DST;
    buf.buf_type = IPP_BUF_ENQUEUE;
    buf.prop_id = clone->prop_id;
    buf.buf_id = index;
    buf.user_data = (__u64 ) (unsigned int) clone;
	 */

	//test_clone->prop_id = 1;

	_exynosCloneQueue( test_clone, index );

	fail_if( check_ExynosVideoIppQueueBuf.ops_id   != EXYNOS_DRM_OPS_DST, " should be buf.ops_id == EXYNOS_DRM_OPS_DST ");
	fail_if( check_ExynosVideoIppQueueBuf.buf_type != IPP_BUF_ENQUEUE , " should be buf_type == IPP_BUF_ENQUEUE; ");
	fail_if( check_ExynosVideoIppQueueBuf.prop_id  != test_clone->prop_id, " should be buf.prop_id  == clone->prop_id " );
	fail_if( check_ExynosVideoIppQueueBuf.buf_id   != index, " should be buf.buf_id == index ");
}
END_TEST



START_TEST( _exynosCloneQueue_test_two )
{
	int index = 1;
	test_clone->dst_buf[index] = ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );

	EXYNOSBufInfo *dstb = test_clone->dst_buf[index];

	dstb->showing = FALSE;

	int i = 0;
	for( i = 0; i < EXYNOS_DRM_PLANAR_MAX; ++i)
		dstb->handles[i].ptr = 1;

	_exynosCloneQueue( test_clone, index );

	fail_if( test_clone->queued[index] != TRUE, "clone->queued should be TRUE" );
}
END_TEST


//=======================================================================================================================


//=======================================================================================================================
/* unsigned int exynosCloneGetPropID(void) */
START_TEST( exynosCloneGetPropID_test_one )
{
	int cmp;

	//Chek if test_keep_clone not exist
	cmp = exynosCloneGetPropID();
	fail_if(cmp != -1, "If clone not exist should be return -1");
}
END_TEST

START_TEST( exynosCloneGetPropID_test_two )
{
	prepare( &scr );
	displaySetDispSetMode( &scr, DISPLAY_SET_MODE_CLONE );
	EXYNOS_CloneObject *test_clone = ( EXYNOS_CloneObject* )( ( ( EXYNOSRec* )scr.driverPrivate )->clone );

	//Should return clone->prop_id
	test_clone->prop_id = 1;

	int cmp;
	cmp = exynosCloneGetPropID();

	fail_if(cmp != 1, "If clone is exist should be return clone->prop_id");
}
END_TEST
//=======================================================================================================================


//=======================================================================================================================
/* static Bool _exynosPrepareTvout(EXYNOS_CloneObject* clone) */
START_TEST( _exynosPrepareTvout_test_one )
{
	//Correct init should be retruned TRUE
	fail_if(_exynosPrepareTvout( test_clone ) != TRUE, "should return TRUE" );
}
END_TEST

START_TEST( _exynosPrepareTvout_test_equivalence )
{
	pExynos->pDispInfo->main_lcd_mode.hdisplay = 1;
	pExynos->pDispInfo->main_lcd_mode.vdisplay = 2;

	_exynosPrepareTvout( test_clone );
	fail_if( test_clone->width  != 1, "clone->width and pDispInfo->main_lcd_mode.hdisplay should be equivalent" );

	_exynosPrepareTvout( test_clone );
	fail_if( test_clone->height != 2, "clone->height and pDispInfo->main_lcd_mode.vdisplay should be equivalent" );

	fail_if( test_clone->drm_dst.width  != 1,  "clone->drm_dst.width and clone->width should be equivalent" );

	fail_if( test_clone->drm_dst.height != 2, "clone->drm_dst.height and clone->height should be equivalent" );
}
END_TEST

START_TEST( _exynosPrepareTvout_macroEXYNOSDISPINFOPTRpDispInfoNull_Success )
{
	//If clone->pScrn->driverPrivate->pDispInfo == NULL should not be Segmentation fault
	_exynosPrepareTvout( test_clone );
}
END_TEST
//=======================================================================================================================

//=======================================================================================================================
//static void _sendStopDrm(EXYNOS_CloneObject* clone)
START_TEST( _sendStopDrm_test_equivalence )
{
	_sendStopDrm( test_clone );

	int i = 0;
	for( i = 0; i < CLONE_BUF_MAX; ++i)
		fail_if( test_clone->queued[i] != FALSE, "clone->queued should be FALSE" );

	fail_if( test_clone->prop_id != -1, " clone->prop_id should be -1 " );
}
END_TEST


START_TEST( _sendStopDrm_initLocalBuf1_Success )
{
	/*
    buf.ops_id = EXYNOS_DRM_OPS_DST;
    buf.buf_type = IPP_BUF_DEQUEUE;
    buf.prop_id = clone->prop_id;
    buf.buf_id = i;
	 */

	test_clone->buf_num = 2;
	test_clone->prop_id = -1;

	_sendStopDrm( test_clone );

	int i = 0;
	for( i = 0; i < CLONE_BUF_MAX; ++i)
		fail_if( test_clone->queued[i] != FALSE, "clone->queued should be FALSE" );

	fail_if( check_ExynosVideoIppQueueBuf.ops_id   != EXYNOS_DRM_OPS_DST, " should be buf.ops_id == EXYNOS_DRM_OPS_DST ");

	fail_if( check_ExynosVideoIppQueueBuf.buf_type != IPP_BUF_DEQUEUE , " should be buf_type == IPP_BUF_ENQUEUE; ");

	fail_if( check_ExynosVideoIppQueueBuf.prop_id  != test_clone->prop_id, " should be buf.prop_id  == clone->prop_id " );

	fail_if( check_ExynosVideoIppQueueBuf.buf_id   != (test_clone->buf_num - 1), " should be buf.buf_id == index ");
}
END_TEST


START_TEST( _sendStopDrm_initLocalBuf2_Success )
{
	/*
    ctrl.prop_id = clone->prop_id;
    ctrl.ctrl = IPP_CTRL_STOP;
	 */

	test_clone->prop_id = -1;

	_sendStopDrm( test_clone );

	fail_if(check_ExynosVideoIppCmdCtrl.prop_id != test_clone->prop_id, " should be ctrl.prop_id != test_clone->prop_id ");
	fail_if(check_ExynosVideoIppCmdCtrl.ctrl    != IPP_CTRL_STOP, " should be ctrl.ctrl != IPP_CTRL_STOP");
}
END_TEST

START_TEST( _sendStopDrm_macroEXYNOSPTRdriverPrivateNull_Failure )
{
	//if clone->pScrn-<driverPrivate == NULL should not be Segmentation fault
	_sendStopDrm( test_clone );
}
END_TEST
//=======================================================================================================================

//=======================================================================================================================
//void exynosCloneStop(EXYNOS_CloneObject* clone)
START_TEST( exynosCloneStop_initArgumentValue_Success )
{
	int i = 0;
	ScrnInfoRec scr;

	prepare( &scr );
	displaySetDispSetMode( &scr, DISPLAY_SET_MODE_CLONE );

	EXYNOS_CloneObject *test_clone = ( EXYNOS_CloneObject* )( ( ( EXYNOSRec* )scr.driverPrivate )->clone );

	test_clone->pScrn = &scr;

	//Function should rewrite these two values to correct
	test_clone->status = STATUS_STARTED;
	test_clone->prop_id = 0;

	exynosCloneStop( test_clone );

	// number 4 equivalence to CLONE_BUF_MAX
	for( i = 0; i < 4; ++i )
		fail_if( test_clone->queued[i] != FALSE, " should be clone->queued[i] == FALSE " );

	fail_if( test_clone->prop_id != test_clone->prop_id, " should be clone->prop_id == -1 " );

	fail_if( test_clone->status  != STATUS_STOPPED, " should be STATUS_STOPPED " );
}
END_TEST
//=======================================================================================================================

//=======================================================================================================================
// static void _exynosCloneCloseDrmDstBuffer(EXYNOS_CloneObject* clone)
START_TEST( _exynosCloneCloseDrmDstBuffer_initClnStructFields_Success )
{
	EXYNOS_CloneObject *test_clone = calloc( 1, sizeof( EXYNOS_CloneObject ) );

	int i = 0;
	// number 4 equivalence to CLONE_BUF_MAX
	for( i = 0; i < 4; ++i )
		test_clone->dst_buf[i] = ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );

	test_clone->buf_num = 4;

	_exynosCloneCloseDrmDstBuffer( test_clone );

	fail_if( ctrl_get_cnt_calloc() != 0, "Memory should be free" );
}
END_TEST
//=======================================================================================================================

//=======================================================================================================================
/* static Bool _exynosCloneEnsureDrmDstBuffer(EXYNOS_CloneObject* clone) */
START_TEST( _exynosCloneEnsureDrmDstBuffer_ifPScrnNULL_Failure )
{
	EXYNOS_CloneObject *test_clone = calloc( 1, sizeof( EXYNOS_CloneObject ) );

	test_clone->pScrn = NULL;//for crash the function
	test_clone->buf_num = 4;//for entrance in the function cycle

	fail_if( _exynosCloneEnsureDrmDstBuffer( test_clone ) != FALSE, "should return FALSE, pScrn == NULL" );
}
END_TEST

START_TEST( _exynosCloneEnsureDrmDstBuffer_ifCloneIdZero_Failure )
{
	EXYNOS_CloneObject *test_clone = calloc( 1, sizeof( EXYNOS_CloneObject ) );

	test_clone->id = 0;//If clone->id = 0 function should return FALSE
	test_clone->buf_num = 4;//for entrance in the function cycle

	fail_if( _exynosCloneEnsureDrmDstBuffer( test_clone ) != FALSE, "should return FALSE, pScrn == NULL" );
}
END_TEST

START_TEST( _exynosCloneEnsureDrmDstBuffer_correctInit_Success )
{
	test_clone->buf_num = 1;
	test_clone->id = FOURCC_SR16;
	test_clone->width  = 1;
	test_clone->height = 1;


	fail_if( _exynosCloneEnsureDrmDstBuffer( test_clone ) != TRUE, "should return TRUE, function receive correct parameters" );
}
END_TEST

START_TEST( _exynosCloneEnsureDrmDstBuffer_allocMemory_Success )
{
	test_clone->buf_num = 4;//Function will allocate memory for four buffers
	test_clone->id = FOURCC_ST12;
	test_clone->width  = 1;
	test_clone->height = 1;

	_exynosCloneEnsureDrmDstBuffer( test_clone );

	fail_if( ctrl_get_cnt_calloc() != 8, " Memory not allocate" );
	//ctrl_get_cnt_calloc() != 8, 4-allocate by function and another 4-allocate by fixture
}
END_TEST
//=======================================================================================================================

//=======================================================================================================================
//static Bool _sendStartDrm(EXYNOS_CloneObject* clone)
START_TEST( _sendStartDrm_macroBothDriverPrivateNull_Failure )
{
	EXYNOS_CloneObject *test_clone = calloc( 1, sizeof( EXYNOS_CloneObject ) );
	test_clone->pScrn = calloc( 1, sizeof( ScrnInfoRec ) );

	//If test_clone->pScrn->driverPrivate == NULL should not be Segmentation fault
	_sendStartDrm( test_clone );
}
END_TEST

START_TEST( _sendStartDrm_macroEXYNOSDISPINFOPTRpDispInfoNull_Failure )
{
	EXYNOS_CloneObject *test_clone = calloc( 1, sizeof( EXYNOS_CloneObject ) );
	test_clone->pScrn = calloc( 1, sizeof( ScrnInfoRec ) );
	test_clone->pScrn->driverPrivate = calloc( 1, sizeof( EXYNOSRec ) );

	//If test_clone->pScrn->driverPrivate->pDispInfo == NULL should not be Segmentation fault
	_sendStartDrm( test_clone );
}
END_TEST

START_TEST( _sendStartDrm_idWhichNotSupport_Failure )
{
	ScrnInfoRec scr;

	prepare( &scr );
	displaySetDispSetMode( &scr, DISPLAY_SET_MODE_CLONE );

	EXYNOS_CloneObject *test_clone = ( EXYNOS_CloneObject* )( ( ( EXYNOSRec* )scr.driverPrivate )->clone );

	test_clone->id = FOURCC_RGB565;//Not supported id for function exynosVideoBufferGetDrmFourcc(id)

	fail_if( _sendStartDrm( test_clone ) != FALSE, "Receive unsupported id should be return FALSE" );
}
END_TEST

START_TEST( _sendStartDrm_propIDNegative_Failure )
{
	test_clone->prop_id = -1;//Check on negative value

	fail_if( _sendStartDrm( test_clone ) != FALSE, "receive not negative id should be return FALSE" );
}
END_TEST

START_TEST( _sendStartDrm_fillLocalBufferBuf_Success )
{
	int i = 0;
	for( ; i < 4; ++i )
	test_clone->dst_buf[i] = calloc( 1, sizeof( EXYNOSBufInfo ) );

	test_clone->prop_id = 12; //special number for fake function, after receive 12 the fake function return 0
							  //to pass negative check
	test_clone->id = FOURCC_ST12;

	test_clone->buf_num = 1;

/*
	buf.ops_id = EXYNOS_DRM_OPS_DST;
	buf.buf_type = IPP_BUF_ENQUEUE;
	buf.prop_id = clone->prop_id;
	buf.buf_id = i;
	buf.user_data = (__u64 ) (unsigned int) clone;
*/
	_sendStartDrm( test_clone );

	fail_if( check_ExynosVideoIppQueueBuf.ops_id   != EXYNOS_DRM_OPS_DST, " should be buf.ops_id == EXYNOS_DRM_OPS_DST ");

	fail_if( check_ExynosVideoIppQueueBuf.buf_type != IPP_BUF_ENQUEUE , " should be buf_type == IPP_BUF_ENQUEUE; ");

	fail_if( check_ExynosVideoIppQueueBuf.prop_id  != test_clone->prop_id, " should be buf.prop_id  == clone->prop_id " );

	fail_if( check_ExynosVideoIppQueueBuf.buf_id   != (test_clone->buf_num - 1), " should be buf.buf_id == index ");
}
END_TEST

START_TEST( _sendStartDrm_fillLocalBufferCtrl_Success )
{
	int i = 0;
	for( ; i < 4; ++i )
	test_clone->dst_buf[i] = calloc( 1, sizeof( EXYNOSBufInfo ) );

	test_clone->prop_id = 12; //special number for fake function, after receive 12 the fake function return 0
							  //to pass negative check
	test_clone->id = FOURCC_ST12;

	test_clone->buf_num = 1;

/*
	CLEAR(ctrl);
	ctrl.prop_id = clone->prop_id;
	ctrl.ctrl = IPP_CTRL_PLAY;

	check_ExynosVideoIppCmdCtrl.prop_id = ctrl->prop_id;
	check_ExynosVideoIppCmdCtrl.ctrl    = ctrl->ctrl;
*/
	_sendStartDrm( test_clone );

	fail_if(check_ExynosVideoIppCmdCtrl.prop_id != test_clone->prop_id, " should be ctrl.prop_id == test_clone->prop_id ");

	fail_if(check_ExynosVideoIppCmdCtrl.ctrl    != IPP_CTRL_PLAY, " should be ctrl.ctrl == IPP_CTRL_PLAY");

}
END_TEST


START_TEST( _sendStartDrm_correctInit_Success )
{
	int i = 0;
	for( ; i < 4; ++i )
	test_clone->dst_buf[i] = calloc( 1, sizeof( EXYNOSBufInfo ) );

	test_clone->prop_id = 12; //special number for fake function, after receive 12 the fake function return 0
							  //to pass negative check
	test_clone->id = FOURCC_ST12;

	test_clone->buf_num = 1;


	fail_if( _sendStartDrm( test_clone ) != TRUE, "correct init should be return TRUE" );
}
END_TEST
//=======================================================================================================================

//=======================================================================================================================
/* Bool exynosCloneStart(EXYNOS_CloneObject* clone) */
START_TEST( exynosCloneStart_initNull_Failure )
{
	//should not be Segmentation fault
	exynosCloneStart( NULL );
}
END_TEST

START_TEST( exynosCloneStart_cloneStatusIsStatusStarted_Success )
{
	ScrnInfoRec scr;

	prepare( &scr );
	displaySetDispSetMode( &scr, DISPLAY_SET_MODE_CLONE );

	EXYNOS_CloneObject *test_clone = ( EXYNOS_CloneObject* )( ( ( EXYNOSRec* )scr.driverPrivate )->clone );

	test_clone->status = STATUS_STARTED;

	fail_if( exynosCloneStart( test_clone ) != TRUE, "clone->status == STATUS_STARTED should be return TRUE");
}
END_TEST

START_TEST( exynosCloneStart_idWhichNotSupport_Failure )
{
	test_clone->status = STATUS_STOPPED;
	test_clone->id = FOURCC_RGB565;//Not supported id for function exynosVideoBufferGetDrmFourcc(id)

	fail_if( exynosCloneStart( test_clone ) != FALSE, "Receive unsupported id should be return FALSE" );
}
END_TEST

START_TEST( exynosCloneStart_correctInit_Success )
{
	test_clone->prop_id = 12; //special number for fake function, after receive 12 the fake function return 0
							  //to pass negative check
	test_clone->id = FOURCC_ST12;
	test_clone->status = STATUS_STOPPED;

	exynosCloneStart( test_clone );

	fail_if( test_clone->buf_num   != CLONE_BUF_MAX,  "should be clone->buf_num == CLONE_BUF_MAX( CLONE_BUF_MAX = 4 )" );
	fail_if( test_clone->status    != STATUS_STARTED, " should be clone->status  == STATUS_STARTED" );
	fail_if( test_clone->hdmi_zpos != -1, "should be clone->hdmi_zpos == -1" );
	fail_if( exynosCloneStart( test_clone ) != TRUE, "should be return TRUE, init by correct values" );
}
END_TEST
//=======================================================================================================================

//=======================================================================================================================
/* void exynosCloneAddNotifyFunc(EXYNOS_CloneObject* clone,
        	EXYNOS_CLONE_NOTIFY noti, CloneNotifyFunc func, void *user_data) */
START_TEST( exynosCloneAddNotifyFunc_initCloneNull_Failure )
{
	ScrnInfoRec scr;

	//should not be Segmentation fault
	exynosCloneAddNotifyFunc( NULL, CLONE_NOTI_CLOSED, _exynosDisplayCloneCloseFunc_fake, &scr);
}
END_TEST

START_TEST( exynosCloneAddNotifyFunc_initFuncNullPtr_Failure )
{
	ScrnInfoRec scr;

	prepare( &scr );
	displaySetDispSetMode( &scr, DISPLAY_SET_MODE_CLONE );

	EXYNOS_CloneObject *test_clone = ( EXYNOS_CloneObject* )( ( ( EXYNOSRec* )scr.driverPrivate )->clone );

	//should not be Segmentation fault
	exynosCloneAddNotifyFunc( test_clone, CLONE_NOTI_CLOSED, NULL, &scr);
}
END_TEST

START_TEST( exynosCloneAddNotifyFunc_initPScrnNull_Failure )
{
	EXYNOS_CloneObject *test_clone = calloc( 1, sizeof( EXYNOS_CloneObject ) );

	exynosCloneAddNotifyFunc( test_clone, CLONE_NOTI_CLOSED, _exynosDisplayCloneCloseFunc_fake, NULL);
}
END_TEST

START_TEST( exynosCloneAddNotifyFunc_allocMemory_Success )
{
	EXYNOS_CloneObject *test_clone = calloc( 1, sizeof( EXYNOS_CloneObject ) );

	exynosCloneAddNotifyFunc( test_clone, CLONE_NOTI_CLOSED, _exynosDisplayCloneCloseFunc_fake, &scr);

	fail_if( ctrl_get_cnt_calloc() != 1, "Memory should be allocate" );
}
END_TEST

START_TEST( exynosCloneAddNotifyFunc_initInputParam_Success )
{

	ScrnInfoRec scr;

	prepare( &scr );
	displaySetDispSetMode( &scr, DISPLAY_SET_MODE_CLONE );

	EXYNOS_CloneObject *test_clone = ( EXYNOS_CloneObject* )( ( ( EXYNOSRec* )scr.driverPrivate )->clone );

	exynosCloneNotifyFuncInfo *test_info = calloc( 1, sizeof( exynosCloneNotifyFuncInfo ) );

	exynosCloneAddNotifyFunc( test_clone, CLONE_NOTI_CLOSED, _exynosDisplayCloneCloseFunc_fake, &scr);

	fail_if( test_clone->info_list == NULL, "clone->info_list not initialize" );
}
END_TEST

START_TEST( exynosCloneAddNotifyFunc_initInputParamAddInfoToList_Success )
{
	ScrnInfoRec scr;

	prepare( &scr );
	displaySetDispSetMode( &scr, DISPLAY_SET_MODE_CLONE );

	EXYNOS_CloneObject *test_clone = ( EXYNOS_CloneObject* )( ( ( EXYNOSRec* )scr.driverPrivate )->clone );

	exynosCloneNotifyFuncInfo *test_info = calloc( 1, sizeof( exynosCloneNotifyFuncInfo ) );
	test_clone->info_list = test_info;
	test_info->next = NULL;
	test_clone->info_list = test_info;

	exynosCloneAddNotifyFunc( test_clone, CLONE_NOTI_CLOSED, _exynosDisplayCloneCloseFunc_fake, &scr);

	fail_if( test_clone->info_list->next == NULL, "New element do not added to list" );
}
END_TEST
//=======================================================================================================================

//=======================================================================================================================
/* static void _exynosCloneCallNotifyFunc(EXYNOS_CloneObject* clone, EXYNOS_CLONE_NOTIFY noti, void *noti_data) */
START_TEST( _exynosCloneCallNotifyFunc_correctInit_Success )
{
	int data = 1;

	test_clone->info_list = ctrl_calloc( 1, sizeof( exynosCloneNotifyFuncInfo ) );

	test_clone->info_list->noti = CLONE_NOTI_START;
	test_clone->info_list->func = _exynosDisplayCloneCloseFunc_fakeTwo;

	_exynosCloneCallNotifyFunc( test_clone, CLONE_NOTI_START, &data);

	fail_if( value_exynosDisplayCloneCloseFunc_fakeTwo != 1, "FAIL" );
}
END_TEST
//=======================================================================================================================

//=======================================================================================================================
/* int _exynosCloneDequeue(EXYNOS_CloneObject* clone, Bool skip_put, int index) */
START_TEST( _exynosCloneDequeue_initDriverPrivateNull_Failure )
{
	test_clone->pScrn->driverPrivate = NULL;

	int index = 1;

	//EXYNOSPtr pExynos = EXYNOSPTR (clone->pScrn); unused in function, bu used inside attachFBDrm()
	//_exynosCloneDequeue( test_clone , TRUE, index);
}
END_TEST

START_TEST( _exynosCloneDequeue_correctInit_Success )
{
	int index = 1;
	test_clone->dst_buf[index] = calloc( 1, sizeof( EXYNOSBufInfo ) );

	fail_if( _exynosCloneDequeue( test_clone , TRUE, index) != 1, " correct initialization, index should be return " );
}
END_TEST
//=======================================================================================================================

//=======================================================================================================================
/* void attachFBDrm(EXYNOS_CloneObject* clone, EXYNOSBufInfo *vbuf, int zpos) */
START_TEST( attachFBDrm_correctInitZPosEqualToCloneHdmi_Zpos_Success )
{
	EXYNOSBufInfo *test_vbuf = ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );

	int zpos = 2;
	test_clone->hdmi_zpos = 2;
	test_vbuf->fb_id = 1;

	attachFBDrm( test_clone, test_vbuf, zpos );

	fail_if( cnt_drmModeSetPlane != 1, "cnt_drmModeSetPlane should be 1( drmModeSetPlane fake function )" );
}
END_TEST

START_TEST( attachFBDrm_correctInitZPosNotEqualToCloneHdmi_Zpos_Success )
{
	EXYNOSBufInfo *test_vbuf = ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );

	int zpos = 2;
	test_clone->hdmi_zpos = 4;
	test_vbuf->fb_id = 1;

	attachFBDrm( test_clone, test_vbuf, zpos );

	fail_if( zpos != test_clone->hdmi_zpos, "zpos and clone->hdmi_zpos should be equivalent" );
	fail_if( cnt_drmModeSetPlane != 1, "cnt_drmModeSetPlane should be 1( drmModeSetPlane fake function )" );
}
END_TEST
//=======================================================================================================================

//=======================================================================================================================
//static void _exynosCreateFB(EXYNOS_CloneObject* clone, EXYNOSBufInfo *vbuf)
START_TEST( exynosCreateFB_initVBufNull_Failure )
{
	//clone == NULL should not be Segmentation fault
	EXYNOS_CloneObject *test_clone = calloc( 1, sizeof( EXYNOS_CloneObject ) );
	test_clone->pScrn = calloc( 1, sizeof( ScrnInfoRec ) );
	test_clone->pScrn->driverPrivate = calloc( 1, sizeof( EXYNOSRec ) );

	_exynosCreateFB( test_clone , NULL );
}
END_TEST

START_TEST( exynosCreateFB_initVBufFb_IdNotNegative_Success )
{
	EXYNOSBufInfo *test_vbuf = ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );
	test_vbuf->fb_id = 1;

	_exynosCreateFB( test_clone , test_vbuf );

	fail_if(cnt_fb_check_refs()  != 0, "should be 0" );

}
END_TEST

START_TEST( exynosCreateFB_correctInit_Success )
{
	EXYNOSBufInfo *test_vbuf = ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );
	test_vbuf->fb_id = 0;

	_exynosCreateFB( test_clone , test_vbuf );

	fail_if( cnt_fb_check_refs()  != 1, "should be 1" );
}
END_TEST
//=======================================================================================================================


//=======================================================================================================================
/*     EXYNOSPtr pExynos = EXYNOSPTR (clone->pScrn) */
START_TEST( macro_EXYNOSPTRpScrnNull_Success )
{
	EXYNOS_CloneObject *test_clone = calloc( 1, sizeof( EXYNOS_CloneObject ) );

	//If pScrn == NULL should not be segmentation fault
	EXYNOSPtr pExynos = EXYNOSPTR ( test_clone->pScrn );
}
END_TEST

START_TEST( macro_EXYNOSPTRcorrectInit_Success )
{
	EXYNOS_CloneObject *test_clone = calloc( 1, sizeof( EXYNOS_CloneObject ) );
	test_clone->pScrn = calloc( 1, sizeof( ScrnInfoRec ) );

	//If pScrn == NULL should not be segmentation fault
	EXYNOSPtr pExynos = EXYNOSPTR ( test_clone->pScrn );
}
END_TEST

START_TEST( macro_EXYNOSDISPINFOPTRpScrnNull_Success )
{
	EXYNOS_CloneObject *test_clone = calloc( 1, sizeof( EXYNOS_CloneObject ) );

	//If clone->pScrn == NULL should not be Segmentation fault
	EXYNOSDispInfoPtr pDispInfo = EXYNOSDISPINFOPTR(test_clone->pScrn);
}
END_TEST

START_TEST( macro_EXYNOSDISPINFOPTRcorrectInit_Success )
{
	EXYNOS_CloneObject *test_clone = calloc( 1, sizeof( EXYNOS_CloneObject ) );
	test_clone->pScrn = calloc( 1, sizeof( ScrnInfoRec ) );
	test_clone->pScrn->driverPrivate = calloc( 1, sizeof( EXYNOSRec ) );

	//If clone->pScrn->driverPrivate == NULL should not be Segmentation fault
	EXYNOSDispInfoPtr pDispInfo = EXYNOSDISPINFOPTR(test_clone->pScrn);
}
END_TEST
//=======================================================================================================================

//=======================================================================================================================
/* void exynosCloneRemoveNotifyFunc (EXYNOS_CloneObject* clone, CloneNotifyFunc func) */
START_TEST( exynosCloneRemoveNotifyFunc_initByNull )
{
	//Segmentation fault must not occur
	exynosCloneRemoveNotifyFunc( NULL , _exynosDisplayCloneCloseFunc_fakeTwo );
}
END_TEST

START_TEST( exynosCloneRemoveNotifyFunc_ptrOnFuncNull )
{
	EXYNOS_CloneObject *test_clone = ctrl_calloc( 1, sizeof( EXYNOS_CloneObject ) );
	exynosCloneRemoveNotifyFunc( test_clone , NULL );
}
END_TEST
//=======================================================================================================================

//=======================================================================================================================
/* Bool exynosCloneIsRunning (void) */
START_TEST( exynosCloneIsRunning_ifCloneNotRun )
{
	fail_if( exynosCloneIsRunning() != FALSE, "False must returned" );
}
END_TEST

START_TEST( exynosCloneIsRunning_statusSTATUS_STOPPED )
{
	static	ScrnInfoRec scr;
	prepare( &scr );

	test_keep_clone = _exynosCloneInit( NULL, 1, 2, 3, TRUE, 4, FALSE, "test" );

	test_keep_clone->status = STATUS_STOPPED;

	fail_if( exynosCloneIsRunning() != FALSE, "False mast returned" );
}
END_TEST

START_TEST( exynosCloneIsRunning_statusSTATUS_STARTED )
{
	ScrnInfoRec scr;
	prepare( &scr );

	test_keep_clone = _exynosCloneInit( NULL, 1, 2, 3, TRUE, 4, FALSE, "test" );
	test_keep_clone->status = STATUS_STARTED;

	fail_if( exynosCloneIsRunning() != TRUE, "True mast returned" );
}
END_TEST
//=======================================================================================================================

//=======================================================================================================================
/* Bool exynosCloneIsOpened (void) */
START_TEST( exynosCloneIsOpened_No )
{
	fail_if( exynosCloneIsOpened() != FALSE, "False must returned" );
}
END_TEST

START_TEST( exynosCloneIsOpened_Yes )
{
	ScrnInfoRec scr;
	prepare( &scr );

	test_keep_clone = _exynosCloneInit( NULL, 1, 2, 3, TRUE, 4, FALSE, "test" );

	fail_if( exynosCloneIsOpened() != TRUE, "True must returned" );
}
END_TEST
//=======================================================================================================================

//=======================================================================================================================
/* Bool exynosCloneCanDequeueBuffer (EXYNOS_CloneObject* clone) */
START_TEST( exynosCloneCanDequeueBuffer_initByNull )
{
	//Segmentation fault must not occur
	exynosCloneCanDequeueBuffer( NULL );
}
END_TEST

START_TEST( exynosCloneCanDequeueBuffer_remainLessThenCLONE_BUF_MIN )
{
	EXYNOS_CloneObject *test_clone = ctrl_calloc( 1, sizeof( EXYNOS_CloneObject ) );
	test_clone->buf_num = 4;
	int i = 0;
	for( ; i < CLONE_BUF_MAX; ++i )
		test_clone->queued[i] = FALSE;

	fail_if( exynosCloneCanDequeueBuffer( test_clone ) != FALSE, "remain < CLONE_BUF_MIN");
}
END_TEST

START_TEST( exynosCloneCanDequeueBuffer_remainBiggerThenCLONE_BUF_MIN )
{
	EXYNOS_CloneObject *test_clone = ctrl_calloc( 1, sizeof( EXYNOS_CloneObject ) );
	test_clone->buf_num = 4;
	int i = 0;
	for( ; i < CLONE_BUF_MAX; ++i )
		test_clone->queued[i] = TRUE;

	fail_if( exynosCloneCanDequeueBuffer( test_clone ) != TRUE, "remain > CLONE_BUF_MIN");
}
END_TEST
//=======================================================================================================================

//=======================================================================================================================
/* Bool exynosCloneSetBuffer(EXYNOS_CloneObject *clone, EXYNOSBufInfo **vbufs, int bufnum) */
START_TEST( exynosCloneSetBuffer_cloneNull )
{
	int bufnum = CLONE_BUF_MAX;

	EXYNOSBufInfo **vbufs;

	int i = 0, j = 0;
	for( i = 0 ; i < 4; i++ )
		vbufs = ( EXYNOSBufInfo** )ctrl_calloc( 1, sizeof( EXYNOSBufInfo* ) );
	for( i = 0 ; i < 4; i++ )
		vbufs[i] = ( EXYNOSBufInfo* )ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );

	fail_if( exynosCloneSetBuffer( NULL, vbufs, bufnum ) != FALSE, "clone == NULL, False must returned" );
}
END_TEST

START_TEST( exynosCloneSetBuffer_statusNotSTATUS_STOPPED )
{
	int bufnum = CLONE_BUF_MAX;

	EXYNOS_CloneObject *test_clone = ctrl_calloc( 1, sizeof( EXYNOS_CloneObject ) );

	EXYNOSBufInfo **vbufs;
	int i = 0, j = 0;
	for( i = 0 ; i < 4; i++ )
		vbufs = ( EXYNOSBufInfo** )ctrl_calloc( 1, sizeof( EXYNOSBufInfo* ) );
	for( i = 0 ; i < 4; i++ )
		vbufs[i] = ( EXYNOSBufInfo* )ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );

	test_clone->status = STATUS_STARTED;

	fail_if( exynosCloneSetBuffer( test_clone, vbufs, bufnum ) != FALSE, "status != STATUS_STOPPED, False must returned" );
}
END_TEST

START_TEST( exynosCloneSetBuffer_vbufNull )
{
	int bufnum = CLONE_BUF_MAX;

	EXYNOS_CloneObject *test_clone = ctrl_calloc( 1, sizeof( EXYNOS_CloneObject ) );
	test_clone->status = STATUS_STOPPED;

	fail_if( exynosCloneSetBuffer( test_clone, NULL, bufnum ) != FALSE, "vbuf == NULL, False must returned" );
}
END_TEST

START_TEST( exynosCloneSetBuffer_cloneBufNumNotEqualToCLONE_BUF_MAX )
{
	int bufnum = CLONE_BUF_MAX;

	EXYNOS_CloneObject *test_clone = ctrl_calloc( 1, sizeof( EXYNOS_CloneObject ) );
	test_clone->status = STATUS_STOPPED;
	test_clone->buf_num = 5;//value not equal to CLONE_BUF_MAX

	int i = 0;
	EXYNOSBufInfo **vbufs = ( EXYNOSBufInfo** )ctrl_calloc( CLONE_BUF_MAX, sizeof( EXYNOSBufInfo* ) );
	for( i = 0 ; i < 4; i++ )
		vbufs[i] = ( EXYNOSBufInfo* )ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );

	fail_if( exynosCloneSetBuffer( test_clone, vbufs, bufnum ) != FALSE, "clone->buf_num != CLONE_BUF_MAX, False must returned" );
}
END_TEST

START_TEST( exynosCloneSetBuffer_bufnumNotEqualToCLONE_BUF_MAX )
{
	int bufnum = 5;//value not equal to CLONE_BUF_MAX

	EXYNOS_CloneObject *test_clone = ctrl_calloc( 1, sizeof( EXYNOS_CloneObject ) );
	test_clone->status = STATUS_STOPPED;
	test_clone->buf_num = CLONE_BUF_MAX;

	int i = 0;
	EXYNOSBufInfo **vbufs = ( EXYNOSBufInfo** )ctrl_calloc( CLONE_BUF_MAX, sizeof( EXYNOSBufInfo* ) );
	for( i = 0 ; i < 4; i++ )
		vbufs[i] = ( EXYNOSBufInfo* )ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );

	fail_if( exynosCloneSetBuffer( test_clone, vbufs, bufnum ) != FALSE, "bufnum != CLONE_BUF_MAX, False must returned" );
}
END_TEST

START_TEST( exynosCloneSetBuffer_ifCloneDst_BufAlreadyAllocate )
{
	int bufnum = CLONE_BUF_MAX;

	EXYNOS_CloneObject *test_clone = ctrl_calloc( 1, sizeof( EXYNOS_CloneObject ) );
	test_clone->status = STATUS_STOPPED;
	test_clone->buf_num = CLONE_BUF_MAX;

	int i = 0;
	EXYNOSBufInfo **vbufs = ( EXYNOSBufInfo** )ctrl_calloc( CLONE_BUF_MAX, sizeof( EXYNOSBufInfo* ) );
	for( i = 0 ; i < 4; i++ )
		vbufs[i] = ( EXYNOSBufInfo* )ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );

	for( i = 0 ; i < 4; i++ )
		test_clone->dst_buf[i] = ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );//It used to get inside of check on "if(clone->dst_buf[i])"

	fail_if( exynosCloneSetBuffer( test_clone, vbufs, bufnum ) != FALSE, "clone->dst_buf[i] already are allocated, False must returned" );
}
END_TEST

START_TEST( exynosCloneSetBuffer_cloneIdNotEqualToVbufsFourcc )
{
	int bufnum = CLONE_BUF_MAX;

	EXYNOS_CloneObject *test_clone = ctrl_calloc( 1, sizeof( EXYNOS_CloneObject ) );
	test_clone->status = STATUS_STOPPED;
	test_clone->buf_num = CLONE_BUF_MAX;
	test_clone->id = 1;

	int i = 0;
	EXYNOSBufInfo **vbufs = ( EXYNOSBufInfo** )ctrl_calloc( CLONE_BUF_MAX, sizeof( EXYNOSBufInfo* ) );
	for( i = 0 ; i < CLONE_BUF_MAX; i++ )
		vbufs[i] = ( EXYNOSBufInfo* )ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );


	vbufs[1]->fourcc = 1;

	fail_if( exynosCloneSetBuffer( test_clone, vbufs, bufnum ) != FALSE, "clone->id == vbufs[i]->fourcc, False must returned" );

	for( i = 0; i < CLONE_BUF_MAX ; i++ )
		fail_if( test_clone->dst_buf[i], "all clone->dst_buf[i] must be NULL" );
}
END_TEST

START_TEST( exynosCloneSetBuffer_cloneWidthNotEqualToVbufsWidth )
{
	int bufnum = CLONE_BUF_MAX;

	EXYNOS_CloneObject *test_clone = ctrl_calloc( 1, sizeof( EXYNOS_CloneObject ) );
	test_clone->status = STATUS_STOPPED;
	test_clone->buf_num = CLONE_BUF_MAX;
	test_clone->id = 1;
	test_clone->width = 1;

	int i = 0;
	EXYNOSBufInfo **vbufs = ( EXYNOSBufInfo** )ctrl_calloc( CLONE_BUF_MAX, sizeof( EXYNOSBufInfo* ) );
	for( i = 0 ; i < CLONE_BUF_MAX; i++ )
		vbufs[i] = ( EXYNOSBufInfo* )ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );

	for( i = 0 ; i < CLONE_BUF_MAX; i++ )
	{
		vbufs[i]->fourcc = 1;
		vbufs[i]->width = 2;
	}

	fail_if( exynosCloneSetBuffer( test_clone, vbufs, bufnum ) != FALSE, "clone->width == vbufs[i]->width, False must returned" );

	for( i = 0; i < CLONE_BUF_MAX ; i++ )
		fail_if( test_clone->dst_buf[i], "all clone->dst_buf[i] must be NULL" );
}
END_TEST

START_TEST( exynosCloneSetBuffer_cloneHeightNotEqualToVbufsHeight )
{
	int bufnum = CLONE_BUF_MAX;

	EXYNOS_CloneObject *test_clone = ctrl_calloc( 1, sizeof( EXYNOS_CloneObject ) );
	test_clone->status = STATUS_STOPPED;
	test_clone->buf_num = CLONE_BUF_MAX;
	test_clone->id = 1;
	test_clone->width = 1;
	test_clone->height = 1;

	int i = 0;
	EXYNOSBufInfo **vbufs = ( EXYNOSBufInfo** )ctrl_calloc( CLONE_BUF_MAX, sizeof( EXYNOSBufInfo* ) );
	for( i = 0 ; i < CLONE_BUF_MAX; i++ )
	{
		vbufs[i] = ( EXYNOSBufInfo* )ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );
		vbufs[i]->fourcc = 1;
		vbufs[i]->width = 1;
		vbufs[i]->height = 2;
	}

	fail_if( exynosCloneSetBuffer( test_clone, vbufs, bufnum ) != FALSE, "clone->height == vbufs[i]->height, False must returned" );

	for( i = 0; i < CLONE_BUF_MAX ; i++ )
		fail_if( test_clone->dst_buf[i], "all clone->dst_buf[i] must be NULL" );
}
END_TEST

START_TEST( exynosCloneSetBuffer_correctInit )
{
	int bufnum = CLONE_BUF_MAX;

	EXYNOS_CloneObject *test_clone = ctrl_calloc( 1, sizeof( EXYNOS_CloneObject ) );
	test_clone->status = STATUS_STOPPED;
	test_clone->buf_num = CLONE_BUF_MAX;
	test_clone->id = 1;
	test_clone->width = 1;
	test_clone->height = 1;

	int i = 0;
	EXYNOSBufInfo **vbufs = ( EXYNOSBufInfo** )ctrl_calloc( CLONE_BUF_MAX, sizeof( EXYNOSBufInfo* ) );
	for( i = 0 ; i < CLONE_BUF_MAX; i++ )
	{
		vbufs[i] = ( EXYNOSBufInfo* )ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );
		vbufs[i]->fourcc = 1;
		vbufs[i]->width = 1;
		vbufs[i]->height = 1;
	}

	fail_if( exynosCloneSetBuffer( test_clone, vbufs, bufnum ) != TRUE, "Correct initialization, TRUE must returned" );

	for( i = 0; i < CLONE_BUF_MAX ; i++ )
			fail_if( !test_clone->dst_buf[i], "all clone->dst_buf[i] must not be NULL" );

	fail_if( test_clone->buf_num!= bufnum, "must be clone->buf_num == bufnum" );
}
END_TEST
//=======================================================================================================================

//***********************************************************************************************************************
int clone_suite ( run_stat_data* stat )
{
	Suite *s = suite_create ( "CLONE.C" );

	TCase *tc_exynosCloneInit                   = tcase_create( "_exynosCloneInit" );
	TCase *tc_exynosCloneSupportFormat          = tcase_create( "_exynosCloneSupportFormat" );
	TCase *tc_calculateSize                     = tcase_create( "calculateSize" );
	TCase *tc_exynosCloneClose                  = tcase_create( "exynosCloneClose" );
	TCase *tc_exynosCloneHandleIppEvent         = tcase_create( "exynosCloneHandleIppEvent" );
	TCase *tc_exynosCloneHandleIppEvent_fixture = tcase_create( "exynosCloneHandleIppEvent" );

	TCase *tc_exynosCloneQueue                  = tcase_create( "_exynosCloneQueue" );
	TCase *tc_exynosCloneQueue_fixtureTwo       = tcase_create( "_exynosCloneQueue" );
	TCase *tc_exynosCloneQueue_fixtureThree     = tcase_create( "_exynosCloneQueue" );
	TCase *tc_exynosCloneQueue_fixtureFour      = tcase_create( "_exynosCloneQueue" );

	TCase *tc_exynosCloneGetPropID              = tcase_create( "exynosCloneGetPropID" );
	TCase *tc_exynosCloneGetPropID_fixture      = tcase_create( "exynosCloneGetPropID" );

	TCase *tc_exynosPrepareTvout                = tcase_create( "_exynosPrepareTvout" );
	TCase *tc_exynosPrepareTvout_fixtureFour    = tcase_create( "_exynosPrepareTvout" );
	TCase *tc_exynosPrepareTvout_macro          = tcase_create( "_exynosPrepareTvout" );

	TCase *tc_sendStopDrm                       = tcase_create( "_sendStopDrm" );
	TCase *tc_sendStopDrm_fixtureThree          = tcase_create( "_sendStopDrm" );
	TCase *tc_sendStopDrm_macro                 = tcase_create( "_sendStopDrm" );

	TCase *tc_exynosCloneStop	                = tcase_create( "exynosCloneStop" );
	TCase *tc_exynosCloneCloseDrmDstBuffer      = tcase_create( "_exynosCloneCloseDrmDstBuffer" );

	TCase *tc_exynosCloneEnsureDrmDstBuffer             = tcase_create( "_exynosCloneEnsureDrmDstBuffer" );
	TCase *tc_exynosCloneEnsureDrmDstBuffer_fixtureFour = tcase_create( "_exynosCloneCloseDrmDstBuffer" );

	TCase *tc_sendStartDrm                      = tcase_create( "_sendStartDrm" );
	TCase *tc_sendStartDrm_macroTwo             = tcase_create( "_sendStartDrm" );
	TCase *tc_sendStartDrm_macroThree           = tcase_create( "_sendStartDrm" );
	TCase *tc_sendStartDrm_fixtureFour          = tcase_create( "_sendStartDrm" );

	TCase *tc_exynosCloneStart                  = tcase_create( "exynosCloneStart" );
	TCase *tc_exynosCloneStart_fixtureFour      = tcase_create( "exynosCloneStart" );


	TCase *tc_exynosCloneAddNotifyFunc          = tcase_create( "exynosCloneAddNotifyFunc" );

	TCase *tc_exynosCloneCallNotifyFunc     	   = tcase_create( "_exynosCloneCallNotifyFunc" );
	TCase *tc_exynosCloneCallNotifyFunc_fixtureTwo = tcase_create( "_exynosCloneCallNotifyFunc" );

	TCase *tc_exynosCloneDequeue                = tcase_create( "_exynosCloneDequeue" );
	TCase *tc_exynosCloneDequeue_fixtureTwo     = tcase_create( "_exynosCloneDequeue" );
	TCase *tc_exynosCloneDequeue_fixtureFour    = tcase_create( "_exynosCloneDequeue" );

	TCase *tc_attachFBDrm                       = tcase_create( "attachFBDrm" );
	TCase *tc_attachFBDrm_fixtureFour           = tcase_create( "attachFBDrm" );

	TCase *tc_exynosCreateFB                    = tcase_create( "_exynosCreateFB" );
	TCase *tc_exynosCreateFB_fixtureThree       = tcase_create( "_exynosCreateFB" );

	TCase *tc_macro              				= tcase_create( "Macro" );
	TCase *tc_exynosCloneRemoveNotifyFunc       = tcase_create( "exynosCloneRemoveNotifyFunc" );
	TCase *tc_exynosCloneIsRunning              = tcase_create( "exynosCloneIsRunning" );
	TCase *tc_exynosCloneIsOpened				= tcase_create( "exynosCloneIsOpened" );
	TCase *tc_exynosCloneCanDequeueBuffer		= tcase_create( "exynosCloneCanDequeueBuffer" );
	TCase *tc_exynosCloneSetBuffer              = tcase_create( "exynosCloneCanDequeueBuffer" );


	//test for _exynosCloneInit
	tcase_add_checked_fixture( tc_exynosCloneInit, setupInitTestKeepClone, teardownInitTestKeepClone);
	tcase_add_test( tc_exynosCloneInit, _exynosCloneInit_PScrnNull_Sucsses );
	tcase_add_test( tc_exynosCloneInit, _exynosCloneInit_test_alloc_memory );
	tcase_add_test( tc_exynosCloneInit, _exynosCloneInit_test_filling_fields_one );
	tcase_add_test( tc_exynosCloneInit, _exynosCloneInit_test_filling_fields_two );
	tcase_add_test( tc_exynosCloneInit, _exynosCloneInit_test_negative_values );

	//exynosCloneSupportFormat
	tcase_add_test( tc_exynosCloneSupportFormat, _exynosCloneSupportFormat_test );

	//calculateSize
	tcase_add_test( tc_calculateSize, calculateSize_test );

	//exynosCloneClose
	//tcase_add_checked_fixture( tc_exynosCloneClose, startInitTestClone, teardownInitTestClone );
	tcase_add_test( tc_exynosCloneClose, exynosCloneClose_cloneNull );
	tcase_add_test( tc_exynosCloneClose, exynosCloneClose_test_free_memory_one );
	tcase_add_test( tc_exynosCloneClose, exynosCloneClose_test_free_memory_two );

	//exynosCloneHandleIppEvent
	tcase_add_test( tc_exynosCloneHandleIppEvent, exynosCloneHandleIppEvent_test_one );
	tcase_add_test( tc_exynosCloneHandleIppEvent, exynosCloneHandleIppEvent_test_two );
	tcase_add_test( tc_exynosCloneHandleIppEvent, exynosCloneHandleIppEvent_test_three );
	tcase_add_test( tc_exynosCloneHandleIppEvent, exynosCloneHandleIppEvent_test_four );

	//_exynosCloneQueue
	tcase_add_test( tc_exynosCloneQueue, _exynosCloneQueue_test_one );
	tcase_add_checked_fixture( tc_exynosCloneQueue_fixtureTwo, setupAllocMemoryTwoParam, teardownAllocMemoryTwoParam );
	tcase_add_test( tc_exynosCloneQueue_fixtureTwo , macro_EXYNOSPTR_ClonePScrnDrm_fd_IfDriverPrivateNull_Failure );

	tcase_add_checked_fixture( tc_exynosCloneQueue_fixtureThree, setupAllocMemoryThreeParam, teardownAllocMemoryThreeParam );
	tcase_add_test( tc_exynosCloneQueue_fixtureThree, _exynosCloneQueue_CloneDstBufNull_Success );
	tcase_add_test( tc_exynosCloneQueue_fixtureThree, _exynosCloneQueue_test_negativeValue );

	tcase_add_checked_fixture( tc_exynosCloneQueue_fixtureFour, setupAllocMemoryFourParam, teardownAllocMemoryFourParam );
	tcase_add_test( tc_exynosCloneQueue_fixtureFour, _exynosCloneQueue_test_two );
	tcase_add_test( tc_exynosCloneQueue_fixtureFour, _exynosCloneQueue_test_initBuf );

	//exynosCloneGetPropID
	tcase_add_test( tc_exynosCloneGetPropID, exynosCloneGetPropID_test_one );
	tcase_add_test( tc_exynosCloneGetPropID, exynosCloneGetPropID_test_two );

	//_exynosPrepareTvout
	tcase_add_checked_fixture( tc_exynosPrepareTvout_fixtureFour, setupAllocMemoryFourParam, teardownAllocMemoryFourParam );
	tcase_add_test( tc_exynosPrepareTvout_fixtureFour, _exynosPrepareTvout_test_one );
	tcase_add_test( tc_exynosPrepareTvout_fixtureFour, _exynosPrepareTvout_test_equivalence );
	tcase_add_checked_fixture( tc_exynosPrepareTvout_macro, setupAllocMemoryThreeParam, teardownAllocMemoryThreeParam );
	tcase_add_test( tc_exynosPrepareTvout_macro, _exynosPrepareTvout_macroEXYNOSDISPINFOPTRpDispInfoNull_Success );

	//_sendStopDrm_test
	tcase_add_checked_fixture( tc_sendStopDrm_fixtureThree, setupAllocMemoryThreeParam, teardownAllocMemoryThreeParam );
	tcase_add_test( tc_sendStopDrm_fixtureThree, _sendStopDrm_test_equivalence );
	tcase_add_test( tc_sendStopDrm_fixtureThree, _sendStopDrm_initLocalBuf1_Success );
	tcase_add_test( tc_sendStopDrm_fixtureThree, _sendStopDrm_initLocalBuf2_Success );
	tcase_add_checked_fixture( tc_sendStopDrm_macro, setupAllocMemoryTwoParam, teardownAllocMemoryTwoParam );
	tcase_add_test( tc_sendStopDrm_macro, _sendStopDrm_macroEXYNOSPTRdriverPrivateNull_Failure );

	//exynosCloneStop
	tcase_add_test( tc_exynosCloneStop, exynosCloneStop_initArgumentValue_Success );

	//_exynosCloneCloseDrmDstBuffer
	tcase_add_test( tc_exynosCloneCloseDrmDstBuffer, _exynosCloneCloseDrmDstBuffer_initClnStructFields_Success );

	//_exynosCloneEnsureDrmDstBuffer
	tcase_add_test( tc_exynosCloneEnsureDrmDstBuffer, _exynosCloneEnsureDrmDstBuffer_ifPScrnNULL_Failure );
	tcase_add_test( tc_exynosCloneEnsureDrmDstBuffer, _exynosCloneEnsureDrmDstBuffer_ifCloneIdZero_Failure );
	tcase_add_checked_fixture( tc_exynosCloneEnsureDrmDstBuffer_fixtureFour, setupAllocMemoryFourParam, teardownAllocMemoryFourParam );
	tcase_add_test( tc_exynosCloneEnsureDrmDstBuffer_fixtureFour, _exynosCloneEnsureDrmDstBuffer_correctInit_Success );
	tcase_add_test( tc_exynosCloneEnsureDrmDstBuffer_fixtureFour, _exynosCloneEnsureDrmDstBuffer_allocMemory_Success );

	//_sendStartDrm
	tcase_add_checked_fixture( tc_sendStartDrm_macroTwo, setupAllocMemoryTwoParam, teardownAllocMemoryTwoParam );
	tcase_add_test( tc_sendStartDrm_macroTwo , _sendStartDrm_macroBothDriverPrivateNull_Failure );
	tcase_add_checked_fixture( tc_sendStartDrm_macroThree, setupAllocMemoryThreeParam, teardownAllocMemoryThreeParam );
	tcase_add_test( tc_sendStartDrm_macroThree , _sendStartDrm_macroEXYNOSDISPINFOPTRpDispInfoNull_Failure );

	tcase_add_test( tc_sendStartDrm , _sendStartDrm_idWhichNotSupport_Failure );

	tcase_add_checked_fixture( tc_sendStartDrm_fixtureFour, setupAllocMemoryFourParam, teardownAllocMemoryFourParam );
	tcase_add_test( tc_sendStartDrm_fixtureFour , _sendStartDrm_propIDNegative_Failure );
	tcase_add_test( tc_sendStartDrm_fixtureFour , _sendStartDrm_fillLocalBufferBuf_Success );
	tcase_add_test( tc_sendStartDrm_fixtureFour , _sendStartDrm_fillLocalBufferCtrl_Success );
	tcase_add_test( tc_sendStartDrm_fixtureFour , _sendStartDrm_correctInit_Success );

	//exynosCloneStart
	tcase_add_test( tc_exynosCloneStart , exynosCloneStart_initNull_Failure );
	tcase_add_checked_fixture( tc_exynosCloneStart_fixtureFour, setupAllocMemoryFourParam, teardownAllocMemoryFourParam );
	tcase_add_test( tc_exynosCloneStart_fixtureFour , exynosCloneStart_cloneStatusIsStatusStarted_Success );
	tcase_add_test( tc_exynosCloneStart_fixtureFour , exynosCloneStart_idWhichNotSupport_Failure );
	tcase_add_test( tc_exynosCloneStart_fixtureFour , exynosCloneStart_correctInit_Success );

	//exynosCloneAddNotifyFunc
	tcase_add_test( tc_exynosCloneAddNotifyFunc , exynosCloneAddNotifyFunc_initCloneNull_Failure );
	tcase_add_test( tc_exynosCloneAddNotifyFunc , exynosCloneAddNotifyFunc_initFuncNullPtr_Failure );
	tcase_add_test( tc_exynosCloneAddNotifyFunc , exynosCloneAddNotifyFunc_initPScrnNull_Failure );
	tcase_add_test( tc_exynosCloneAddNotifyFunc , exynosCloneAddNotifyFunc_allocMemory_Success );
	tcase_add_test( tc_exynosCloneAddNotifyFunc , exynosCloneAddNotifyFunc_initInputParam_Success );
	tcase_add_test( tc_exynosCloneAddNotifyFunc , exynosCloneAddNotifyFunc_initInputParamAddInfoToList_Success );

	//_exynosCloneCallNotifyFunc
	tcase_add_checked_fixture( tc_exynosCloneCallNotifyFunc_fixtureTwo, setupAllocMemoryTwoParam, teardownAllocMemoryTwoParam );
	tcase_add_test( tc_exynosCloneCallNotifyFunc_fixtureTwo , _exynosCloneCallNotifyFunc_correctInit_Success );

	//_exynosCloneDequeue
	tcase_add_checked_fixture( tc_exynosCloneDequeue_fixtureTwo, setupAllocMemoryTwoParam, teardownAllocMemoryTwoParam );
	tcase_add_test( tc_exynosCloneDequeue_fixtureTwo , _exynosCloneDequeue_initDriverPrivateNull_Failure );
	tcase_add_checked_fixture( tc_exynosCloneDequeue_fixtureFour, setupAllocMemoryFourParam, teardownAllocMemoryFourParam );
	tcase_add_test( tc_exynosCloneDequeue_fixtureFour , _exynosCloneDequeue_correctInit_Success );

	//attachFBDrm
	tcase_add_checked_fixture( tc_attachFBDrm_fixtureFour, setupAllocMemoryFourParam, teardownAllocMemoryFourParam );
	tcase_add_test( tc_attachFBDrm_fixtureFour , attachFBDrm_correctInitZPosEqualToCloneHdmi_Zpos_Success );
	tcase_add_test( tc_attachFBDrm_fixtureFour , attachFBDrm_correctInitZPosNotEqualToCloneHdmi_Zpos_Success );

	//exynosCreateFB
	tcase_add_test( tc_exynosCreateFB , exynosCreateFB_initVBufNull_Failure );
	tcase_add_checked_fixture( tc_exynosCreateFB_fixtureThree, setupAllocMemoryThreeParam, setupAllocMemoryThreeParam );
	tcase_add_test( tc_exynosCreateFB_fixtureThree , exynosCreateFB_initVBufFb_IdNotNegative_Success );
	tcase_add_test( tc_exynosCreateFB_fixtureThree , exynosCreateFB_correctInit_Success );

	//macro checks
	tcase_add_test( tc_macro , macro_EXYNOSPTRpScrnNull_Success );
	tcase_add_test( tc_macro , macro_EXYNOSPTRcorrectInit_Success );
	tcase_add_test( tc_macro , macro_EXYNOSDISPINFOPTRpScrnNull_Success );
	tcase_add_test( tc_macro , macro_EXYNOSDISPINFOPTRcorrectInit_Success );

	//exynosCloneRemoveNotifyFunc
	tcase_add_test( tc_exynosCloneRemoveNotifyFunc , exynosCloneRemoveNotifyFunc_initByNull );
	tcase_add_test( tc_exynosCloneRemoveNotifyFunc , exynosCloneRemoveNotifyFunc_ptrOnFuncNull );

	//exynosCloneIsRunning
	tcase_add_test( tc_exynosCloneIsRunning , exynosCloneIsRunning_ifCloneNotRun );
	tcase_add_test( tc_exynosCloneIsRunning , exynosCloneIsRunning_statusSTATUS_STOPPED );
	tcase_add_test( tc_exynosCloneIsRunning , exynosCloneIsRunning_statusSTATUS_STARTED );

	//exynosCloneIsOpened
	tcase_add_test( tc_exynosCloneIsOpened , exynosCloneIsOpened_No );
	tcase_add_test( tc_exynosCloneIsOpened , exynosCloneIsOpened_Yes );

	//exynosCloneCanDequeueBuffer
	tcase_add_test( tc_exynosCloneCanDequeueBuffer , exynosCloneCanDequeueBuffer_initByNull );
	tcase_add_test( tc_exynosCloneCanDequeueBuffer , exynosCloneCanDequeueBuffer_remainLessThenCLONE_BUF_MIN );
	tcase_add_test( tc_exynosCloneCanDequeueBuffer , exynosCloneCanDequeueBuffer_remainBiggerThenCLONE_BUF_MIN );

	//exynosCloneSetBuffer

	tcase_add_test( tc_exynosCloneSetBuffer , exynosCloneSetBuffer_cloneNull );
	tcase_add_test( tc_exynosCloneSetBuffer , exynosCloneSetBuffer_statusNotSTATUS_STOPPED );
	tcase_add_test( tc_exynosCloneSetBuffer , exynosCloneSetBuffer_vbufNull );
	tcase_add_test( tc_exynosCloneSetBuffer , exynosCloneSetBuffer_cloneBufNumNotEqualToCLONE_BUF_MAX );
	tcase_add_test( tc_exynosCloneSetBuffer , exynosCloneSetBuffer_bufnumNotEqualToCLONE_BUF_MAX );
	tcase_add_test( tc_exynosCloneSetBuffer , exynosCloneSetBuffer_ifCloneDst_BufAlreadyAllocate );
	tcase_add_test( tc_exynosCloneSetBuffer , exynosCloneSetBuffer_cloneIdNotEqualToVbufsFourcc );
	tcase_add_test( tc_exynosCloneSetBuffer , exynosCloneSetBuffer_cloneWidthNotEqualToVbufsWidth );
	tcase_add_test( tc_exynosCloneSetBuffer , exynosCloneSetBuffer_cloneHeightNotEqualToVbufsHeight );
	tcase_add_test( tc_exynosCloneSetBuffer , exynosCloneSetBuffer_correctInit );


	//Adding tcases to Suit
	suite_add_tcase( s, tc_exynosCloneInit );
	suite_add_tcase( s, tc_exynosCloneSupportFormat );
	suite_add_tcase( s, tc_calculateSize );
	suite_add_tcase( s, tc_exynosCloneClose );
	suite_add_tcase( s, tc_exynosCloneHandleIppEvent );
	suite_add_tcase( s, tc_exynosCloneHandleIppEvent_fixture );

	suite_add_tcase( s, tc_exynosCloneQueue );
	suite_add_tcase( s, tc_exynosCloneQueue_fixtureTwo );
	suite_add_tcase( s, tc_exynosCloneQueue_fixtureThree );
	suite_add_tcase( s, tc_exynosCloneQueue_fixtureFour );

	suite_add_tcase( s, tc_exynosCloneGetPropID );
	suite_add_tcase( s, tc_exynosCloneGetPropID_fixture );

	suite_add_tcase( s, tc_exynosPrepareTvout );
	suite_add_tcase( s, tc_exynosPrepareTvout_fixtureFour );
	suite_add_tcase( s, tc_exynosPrepareTvout_macro );

	suite_add_tcase( s, tc_sendStopDrm );
	suite_add_tcase( s, tc_sendStopDrm_fixtureThree );

	suite_add_tcase( s, tc_sendStopDrm_macro );
	suite_add_tcase( s, tc_exynosCloneStop );
	suite_add_tcase( s, tc_exynosCloneCloseDrmDstBuffer );

	suite_add_tcase( s, tc_exynosCloneEnsureDrmDstBuffer );
	suite_add_tcase( s, tc_exynosCloneEnsureDrmDstBuffer_fixtureFour );

	suite_add_tcase( s, tc_sendStartDrm );
	suite_add_tcase( s, tc_sendStartDrm_macroTwo );
	suite_add_tcase( s, tc_sendStartDrm_macroThree );
	suite_add_tcase( s, tc_sendStartDrm_fixtureFour );

	suite_add_tcase( s, tc_exynosCloneStart );
	suite_add_tcase( s, tc_exynosCloneStart_fixtureFour );

	suite_add_tcase( s, tc_exynosCloneAddNotifyFunc );

	suite_add_tcase( s, tc_exynosCloneCallNotifyFunc );
	suite_add_tcase( s, tc_exynosCloneCallNotifyFunc_fixtureTwo );

	suite_add_tcase( s, tc_exynosCloneDequeue );
	suite_add_tcase( s, tc_exynosCloneDequeue_fixtureTwo );
	suite_add_tcase( s, tc_exynosCloneDequeue_fixtureFour );

	suite_add_tcase( s, tc_attachFBDrm );
	suite_add_tcase( s, tc_attachFBDrm_fixtureFour );

	suite_add_tcase( s, tc_exynosCreateFB );
	suite_add_tcase( s, tc_exynosCreateFB_fixtureThree );

	suite_add_tcase( s, tc_macro );
	suite_add_tcase( s, tc_exynosCloneRemoveNotifyFunc );
	suite_add_tcase( s, tc_exynosCloneIsRunning );
	suite_add_tcase( s, tc_exynosCloneIsOpened );
	suite_add_tcase( s, tc_exynosCloneCanDequeueBuffer );
	suite_add_tcase( s, tc_exynosCloneSetBuffer );


	SRunner *sr = srunner_create( s );
	srunner_run_all( sr, CK_VERBOSE );


	stat->num_tests = srunner_ntests_run( sr );
	stat->num_pass = srunner_ntests_run( sr ) - srunner_ntests_failed( sr );
	stat->num_failure = srunner_ntests_failed( sr );

	stat->num_tested = 19;
	stat->num_nottested = 0;

	//STATISTIC of exynos_clone.c
	return 0;
}

