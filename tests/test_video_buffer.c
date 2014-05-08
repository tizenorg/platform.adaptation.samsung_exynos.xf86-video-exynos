/*
 * test_video_buffer.c
 *
 *  Created on: Oct 21, 2013
 *      Author: svoloshynov
 */
#include "test_video_buffer.h"
#include "exynos_driver.h"
//#include "exynos_mem_pool.h"
#include "mem.h"

//Useful functions for debug
#define PRINTF printf( "\n debug \n\n" );
#define PRINTFVAL( val ) printf( "\n %d \n", val);

//************************************************ Additional Functions *****************************************************

#define exynosMemPoolGetBufInfo fake_exynosMemPoolGetBufInfo
//#define _exynosVideoBufferGetInfo fake_exynosVideoBufferGetInfo
#define memcpy test_memcpy
//#define exynosMemPoolAllocateBO fake_exynosMemPoolAllocateBO

int cnt_test_memcpy = 0;
void *test_memcpy (void *__restrict __dest, const void *__restrict __src, size_t __n)
{
	cnt_test_memcpy += 1;
	memmove( __dest, __src, __n );
}

tbm_bo testExynosExaPixmapGetBo (PixmapPtr pPix)
{
	if( NULL == pPix )
		return NULL;
	else
	{
		//tbm_bo pbo;
		//return pbo;
	}
}

int cnt_testMemoryDestroyPixmap = 0;

Bool testMemoryDestroyPixmap( PixmapPtr pVideoPixmap )
{
	cnt_testMemoryDestroyPixmap = 1;
	return TRUE;
}

int imp_val = 0;

EXYNOSBufInfoPtr exynosMemPoolGetBufInfo (pointer b_object)
{
	if( NULL == b_object  )
		return;


	EXYNOSBufInfoPtr test_bufinfo = ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );

	test_bufinfo->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	test_bufinfo->pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	test_bufinfo->pScrn->pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
	ScreenPtr ppScreen = test_bufinfo->pScrn->pScreen;

	test_bufinfo->num_planes = 3;

	ppScreen->DestroyPixmap = testMemoryDestroyPixmap;

	EXYNOSPtr pExynos = NULL;

	int i = 0;

	switch ( imp_val )
	{
	case 101:
		test_bufinfo->pClient = ctrl_calloc( 1, sizeof( ClientRec ) );

		return test_bufinfo; break;

	case 102:
		test_bufinfo->pClient = ctrl_calloc( 1, sizeof( ClientRec ) );
		test_bufinfo->fb_id = 1;
		test_bufinfo->crop.x = 1;
		test_bufinfo->num_planes = 1;
		test_bufinfo->fourcc = 1;
		pExynos = test_bufinfo->pScrn->driverPrivate;
		pExynos->drm_fd = 5;

		return test_bufinfo; break;

	case 103: //For checking what will be if functions received NULL

		return NULL;

	case 104:
		test_bufinfo->converting = TRUE;
		test_bufinfo->showing = TRUE;

		return test_bufinfo; break;

	case 105:
		test_bufinfo->num_planes = 0;

		return test_bufinfo; break;

	case 106:
		test_bufinfo->pClient = ctrl_calloc( 1, sizeof( ClientRec ) );
		test_bufinfo->fb_id = 1;
		test_bufinfo->crop.x = 1;
		test_bufinfo->num_planes = PLANE_CNT;
		test_bufinfo->fourcc = 1;
		pExynos = test_bufinfo->pScrn->driverPrivate;
		pExynos->drm_fd = 5;

		tbm_bufmgr bufmgr;
		for( i = 0; i < PLANE_CNT; i++ )
			test_bufinfo->tbos[i] = tbm_bo_alloc( bufmgr, 1, 1 );

		for( i = 0; i < PLANE_CNT; i++ )
			test_bufinfo->lengths[i] = 1;

		return test_bufinfo; break;

	case 107:
		for( i = 0; i < PLANE_CNT; i++ )
		{
			test_bufinfo->handles[i].u32 = 1;
		}
		return test_bufinfo; break;

	default:
		return test_bufinfo; break;//return bufinfo without any additional settings
	}
}

//this function used for initialized pPixmap as NULL or allocate memory for it
PixmapPtr funcForPScreen_CreatePixmap ( ScreenPtr pScreen, int width, int height, int depth, unsigned usage_hint )
{
	if( 0 == pScreen->myNum )
		return NULL;
	else
	{
		PixmapPtr pPixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );
		pPixmap->refcnt = 106;
		imp_val = 106;
	}
}

//This function used as initialized variable for pScreen->DestroyPixmap
void funcForPScreen_DestroyPixmap( PixmapPtr pPixmap )
{
	ctrl_free( pPixmap );
}

//EXYNOSBufInfoPtr exynosMemPoolAllocateBO (ScrnInfoPtr pScrn, int id, int width, int height, tbm_bo *tbos,
//                         unsigned int *names, unsigned int buf_type)
//{
//
//}

#include "./xv/exynos_video_buffer.c"


//****************************************************** Fixtures ***********************************************************

//global variables which used for fixtures
PixmapPtr test_pVideoPix = NULL;
ScrnInfoPtr pScrn = NULL;

//Fixture for PixmapPtr test_pVideoPix
void setup_InitTestpVideoPix( void )
{
	test_pVideoPix = ctrl_calloc( 1, sizeof( PixmapRec ) );
}

void teardown_InitTestpVideoPix( void )
{
	ctrl_free( test_pVideoPix );
}

//Fixture for ScrnInfoPtr pScrn
void setup_InitPScrnPScreen( void )
{
	pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
}

void teardown_DestroyPScrnPScreen( void )
{
	ctrl_free( pScrn );
}

//Fixture for ScrnInfoPtr pScrn with extension
void setup_InitPScrnPScreen_2( void )
{
	pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
	pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
}

void teardown_DestroyPScrnPScreen_2( void )
{
	ctrl_free( pScrn );
}

//***************************************************** Unit Tests **********************************************************

//===========================================================================================================================
/* static Bool _exynosVideoBufferComparePixStamp(pointer structure, pointer element) */
START_TEST ( _exynosVideoBufferComparePixStamp_IfEqual_Success )
{
	test_pVideoPix->drawable.serialNumber = 1;

	unsigned long elem = 1;

	fail_if( _exynosVideoBufferComparePixStamp( test_pVideoPix, &elem ) != TRUE, "should be return TRUE"  );
}
END_TEST

START_TEST ( _exynosVideoBufferComparePixStamp_IfNotEqual_Success )
{
	test_pVideoPix->drawable.serialNumber = -1;

	unsigned long elem = 1;

	fail_if( _exynosVideoBufferComparePixStamp( test_pVideoPix, &elem ) != FALSE, "should be return FALSE"  );
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================
//static EXYNOSBufInfoPtr _exynosVideoBufferGetInfo (PixmapPtr pPixmap)
START_TEST ( _exynosVideoBufferGetInfo_bufinfoInitiByNull_Failure )
{
	EXYNOSBufInfoPtr test_bufinfo;
	test_bufinfo = _exynosVideoBufferGetInfo( NULL );

	//fail_if( test_bufinfo != NULL, "Should be returned NULL" );
}
END_TEST

START_TEST ( _exynosVideoBufferGetInfo_correctInit_Success )
{
	test_pVideoPix->refcnt = 102;
	EXYNOSBufInfoPtr test_bufinfo = _exynosVideoBufferGetInfo( test_pVideoPix );

	fail_if( test_bufinfo == NULL, "Should not be NULL" );
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================
/* static int _exynosVideoBufferGetNames(pointer in_buf, unsigned int *names, unsigned int *type) */
START_TEST ( _exynosVideoBufferGetNames_IfDataHeaderNotEqualToXV_DATA_HEADER )
{
	XV_DATA_PTR test_in_buf = ctrl_calloc( 1, sizeof( XV_DATA ) );

	test_in_buf->_header = 1;
	unsigned int tnames[3];
	unsigned int ttype;

	fail_if( _exynosVideoBufferGetNames( test_in_buf, tnames, &ttype ) != XV_HEADER_ERROR, " Header error received XV_HEADER_ERROR");
}
END_TEST

START_TEST ( _exynosVideoBufferGetNames_IfValidNotEqualToXV_VERSION_MISMATCH )
{
	XV_DATA_PTR test_in_buf = ctrl_calloc( 1, sizeof( XV_DATA ) );

	test_in_buf->_header  = XV_DATA_HEADER;
	test_in_buf->_version = 1;
	unsigned int tnames[3];
	unsigned int ttype;

	fail_if( _exynosVideoBufferGetNames( test_in_buf, tnames, &ttype ) != XV_VERSION_MISMATCH, " Version mismatch received XV_VERSION_MISMATCH");
}
END_TEST

START_TEST ( _exynosVideoBufferGetNames_correctInit )
{
	XV_DATA_PTR test_in_buf = ctrl_calloc( 1, sizeof( XV_DATA ) );

	test_in_buf->_header  = XV_DATA_HEADER;
	test_in_buf->_version = XV_DATA_VERSION;
	unsigned int tnames[3];
	unsigned int ttype;

	fail_if( _exynosVideoBufferGetNames( test_in_buf, tnames, &ttype ) != XV_OK, "correct initialization XV_OK should be return");
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================
/* int exynosVideoBufferCreateFBID (PixmapPtr pPixmap) */
START_TEST ( exynosVideoBufferCreateFBID_initPixmapNull )
{
	//Segmentation fault must not occur
	exynosVideoBufferCreateFBID ( NULL );
}
END_TEST

START_TEST ( exynosVideoBufferCreateFBID_correctInit )
{
	PixmapPtr test_pPixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );

	test_pPixmap->drawable.height = 1;
	test_pPixmap->refcnt = 106;

	fail_if( exynosVideoBufferCreateFBID ( test_pPixmap ) != 1, " correctInit 1 should be return " );

}
END_TEST

START_TEST ( exynosVideoBufferCreateFBID_correctInit_1 )
{
    PixmapPtr test_pPixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );
    test_pPixmap->drawable.height = 1;
	test_pPixmap->refcnt = 106;

    int prev_fb=cnt_fb_check_refs() ;
    exynosVideoBufferCreateFBID ( test_pPixmap );
    fail_if( cnt_fb_check_refs() != prev_fb+1, " correctInit FB should be created " );
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================
/* static void _exynosVideoBufferDestroyFBID (pointer data) */
START_TEST ( exynosVideoBufferDestroyFBID_ifDataIsNull )
{
	//Segmentation fault must not occur
	_exynosVideoBufferDestroyFBID( NULL );
}
END_TEST

START_TEST ( exynosVideoBufferDestroyFBID_ifBufinfoFb_idIsNegative )
{
	EXYNOSBufInfoPtr test_bufinfo = ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );
	test_bufinfo->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	test_bufinfo->pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );

	int fb = _add_fb(12346);

	test_bufinfo->fb_id = -1;

	_exynosVideoBufferDestroyFBID( ( pointer )test_bufinfo );
	fail_if( cnt_fb_check_refs() != 1, "value of fake variable should be 0" );
}
END_TEST

START_TEST ( exynosVideoBufferDestroyFBID_correctInit )
{
	EXYNOSBufInfoPtr test_bufinfo = ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );
	test_bufinfo->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	test_bufinfo->pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );

	EXYNOSPtr pExynos = test_bufinfo->pScrn->driverPrivate;
	pExynos->drm_fd = 5;

	test_bufinfo->fb_id = _add_fb( 12345 );

	_exynosVideoBufferDestroyFBID( ( pointer )test_bufinfo );

	fail_if( cnt_fb_check_refs() != 0, "value of fake variable should be 1" );
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================
/* static void _exynosVideoBufferReturnToClient (EXYNOSBufInfoPtr bufinfo) */
START_TEST ( _exynosVideoBufferReturnToClient_ifBufinfoNull )
{
	//Should not be Segmentation fault
	_exynosVideoBufferReturnToClient( NULL );
}
END_TEST

START_TEST ( _exynosVideoBufferReturnToClient_ifPClientNull )
{
	EXYNOSBufInfoPtr test_bufinfo = ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );

	extern int ctrl_WriteEventsToClient;

	_exynosVideoBufferReturnToClient( test_bufinfo );

	fail_if( ctrl_WriteEventsToClient != 0, "fake value should be 1" );
}
END_TEST

START_TEST ( _exynosVideoBufferReturnToClient_correctInit )
{
	EXYNOSBufInfoPtr test_bufinfo = ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );
	test_bufinfo->pClient = (ClientPtr)1;//It using for pass bufinfo->pClient != NULL only

	extern int ctrl_WriteEventsToClient;

	_exynosVideoBufferReturnToClient( test_bufinfo );

	fail_if( ctrl_WriteEventsToClient != 1, "fake value should be 1" );
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================
/* EXYNOSFormatType exynosVideoBufferGetColorType (unsigned int fourcc) */
START_TEST ( exynosVideoBufferGetColorType_initiNotSupportedType )
{
	unsigned int test_fourcc = 1;

	fail_if( exynosVideoBufferGetColorType( test_fourcc ) != TYPE_NONE, "received unsupported type, TYPE_NONE should be return" );
}
END_TEST

START_TEST ( exynosVideoBufferGetColorType_correctInit )
{
	unsigned int test_fourcc = FOURCC_RGB565;

	fail_if( exynosVideoBufferGetColorType( test_fourcc ) != TYPE_RGB, "correct initialization, returned wrong value" );
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================
/* unsigned int exynosVideoBufferGetDrmFourcc (unsigned int fourcc)*/
START_TEST ( exynosVideoBufferGetColorType_iniByUnsuppotedDrmFourcc )
{
	unsigned int test_fourcc = 1;

	fail_if( exynosVideoBufferGetDrmFourcc( test_fourcc ) != 0, "received unsupported DRM fourcc, 0 should be return" );
}
END_TEST

START_TEST ( exynosVideoBufferGetDrmFourcc_correctinit )
{
	unsigned int test_fourcc = DRM_FORMAT_NV12MT;

	fail_if( exynosVideoBufferGetDrmFourcc( test_fourcc ) == TYPE_YUV420, "received unsupported DRM fourcc, 0 should be return" );
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================
/* PixmapPtr exynosVideoBufferRef (PixmapPtr pVideoPixmap) */
START_TEST ( exynosVideoBufferRef_PVideoPixmapIsNull )
{
	//Segmentation fault must not occur
	exynosVideoBufferRef( NULL );
}
END_TEST

START_TEST ( exynosVideoBufferRef_correctInit)
{
	PixmapPtr test_pVideoPixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );
	test_pVideoPixmap->refcnt = 0;

	exynosVideoBufferRef( test_pVideoPixmap );

	fail_if( test_pVideoPixmap->refcnt != 1, "test_pVideoPixmap->refcnt should be increased" );
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================
/* void exynosVideoBufferFree (PixmapPtr pVideoPixmap) */
START_TEST ( exynosVideoBufferFree_initNull)
{
	//Segmentation fault must not occur
	exynosVideoBufferFree( NULL );
}
END_TEST

START_TEST ( exynosVideoBufferFree_ifChecksNotPass)
{
	extern cnt_testMemoryDestroyPixmap;
	extern int ctrl_WriteEventsToClient;
	extern int cnt_drmModeRmFB;

	test_pVideoPix->refcnt = 106;


	int fb = _add_fb( 54321 );

	exynosVideoBufferFree( test_pVideoPix );


	fail_if( ctrl_WriteEventsToClient    != 0, "fake ctrl_WriteEventsToClient variable should be 0" );
	fail_if( cnt_fb_check_refs()         != 1, "fb must not destroyed, this function does not call" );
	fail_if( cnt_testMemoryDestroyPixmap != 1, "fake cnt_testMemoryDestroyPixmap variable should be 1" );
}
END_TEST

START_TEST ( exynosVideoBufferFree_ifBufinfo_PClientISNotNull )
{
	extern cnt_testMemoryDestroyPixmap;
	extern int ctrl_WriteEventsToClient;
	extern int cnt_drmModeRmFB;

	int fb = _add_fb( 54321 );
	test_pVideoPix->refcnt = 106;
	imp_val = 101;

	exynosVideoBufferFree( test_pVideoPix );

	fail_if( ctrl_WriteEventsToClient    != 1, "fake ctrl_WriteEventsToClient variable should be 1" );
	fail_if( cnt_fb_check_refs()         != 1, "fb must not destroyed, this function does not call" );
	fail_if( cnt_testMemoryDestroyPixmap != 1, "fake cnt_testMemoryDestroyPixmap variable should be 1" );
}
END_TEST

START_TEST ( exynosVideoBufferFree_ifBufinfoFb_IdBiggestThenZero )
{
	extern cnt_testMemoryDestroyPixmap;
	extern int ctrl_WriteEventsToClient;
	extern int cnt_drmModeRmFB;

	test_pVideoPix->refcnt = 106;
	imp_val = 106;
	int fb = _add_fb( 54321 );

	exynosVideoBufferFree( test_pVideoPix );

	fail_if( ctrl_WriteEventsToClient    != 1, "fake ctrl_WriteEventsToClient variable should be 1" );
	fail_if( cnt_fb_check_refs()         != 0, "fb must destroyed, cnt of drm must be 0" );
	fail_if( cnt_testMemoryDestroyPixmap != 1, "fake cnt_testMemoryDestroyPixmap variable should be 1" );
}
END_TEST

//===========================================================================================================================

//===========================================================================================================================
/* void exynosVideoBufferConverting (PixmapPtr pVideoPixmap, Bool on) */
START_TEST ( exynosVideoBufferConverting_initByNull )
{
	//Should not be Segmentation fault
	exynosVideoBufferConverting( NULL, TRUE );
}
END_TEST

START_TEST ( exynosVideoBufferConverting_ifBufinfoNull )
{
	test_pVideoPix->refcnt = 103;

	//Should not be Segmentation fault
	exynosVideoBufferConverting( test_pVideoPix, TRUE );
}
END_TEST

//===========================================================================================================================
/* void exynosVideoBufferShowing (PixmapPtr pVideoPixmap, Bool on) */
START_TEST ( exynosVideoBufferShowing_initByNull )
{
	//Segmentation fault must not occur
	exynosVideoBufferShowing( NULL, TRUE );
}
END_TEST

START_TEST ( exynosVideoBufferShowing_ifBufinfoNull )
{
	test_pVideoPix->refcnt = 103;//Special initialization which initialize bufinfo pointer by NULL

	//Segmentation fault must not occur
	exynosVideoBufferShowing( test_pVideoPix, TRUE );
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================
/* Bool exynosVideoBufferIsConverting (PixmapPtr pVideoPixmap) */
START_TEST ( exynosVideoBufferIsConverting_initNull )
{
	Bool res;
	//Segmentation fault must not occur
	res = exynosVideoBufferIsConverting( NULL );
}
END_TEST

START_TEST ( exynosVideoBufferIsConverting_ifBufinfoNull )
{
	Bool res;
	test_pVideoPix->refcnt = 103;//Special initialization which initialize bufinfo pointer by NULL

	//Segmentation fault must not occur
	res = exynosVideoBufferIsConverting( test_pVideoPix );
}
END_TEST

START_TEST ( exynosVideoBufferIsConverting_returnValue_Success )
{
	Bool res;
	test_pVideoPix->refcnt = 106;
	imp_val = 104;

	res = exynosVideoBufferIsConverting( test_pVideoPix );

	fail_if( res != TRUE, "TRUE should be returned" );
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================
/* Bool exynosVideoBufferIsShowing (PixmapPtr pVideoPixmap) */
START_TEST ( exynosVideoBufferIsShowing_initNull )
{
	//Segmentation fault must not occur
	exynosVideoBufferIsShowing( NULL );
}
END_TEST

START_TEST ( exynosVideoBufferIsShowing_ifBufinfoNull )
{
	Bool res;
	test_pVideoPix->refcnt = 106;
	imp_val = 103;//Special initialization which initialize bufinfo pointer by NULL

	//Segmentation fault must not occur
	res = exynosVideoBufferIsShowing( test_pVideoPix );
}
END_TEST

START_TEST ( exynosVideoBufferIsShowing_returnValue_Success )
{
	Bool res;
	test_pVideoPix->refcnt = 106;
	imp_val = 104;

	res = exynosVideoBufferIsShowing( test_pVideoPix );

	fail_if( res != TRUE, "TRUE should be returned" );
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================
/* int exynosVideoBufferGetFBID (PixmapPtr pVideoPixmap) */
START_TEST ( exynosVideoBufferGetFBID_initByNull )
{
	//Segmentation fault must not occur
	exynosVideoBufferGetFBID( NULL );
}
END_TEST

START_TEST ( exynosVideoBufferGetFBID_ifBufinfoNull )
{
	int res;
	test_pVideoPix->refcnt = 106;
	imp_val = 103;//Special initialization which initialize bufinfo pointer by NULL
	//Segmentation fault must not occur
	res = exynosVideoBufferGetFBID( test_pVideoPix );
}
END_TEST

START_TEST ( exynosVideoBufferGetFBID_returnValue )
{
	int res;
	test_pVideoPix->refcnt = 106;
	imp_val = 102;

	//Segmentation fault must not occur
	res = exynosVideoBufferGetFBID( test_pVideoPix );

	fail_if( res != 1, "1 must returned" );
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================
/* Bool exynosVideoBufferSetCrop (PixmapPtr pPixmap, xRectangle crop) */
START_TEST ( exynosVideoBufferSetCrop_initNull )
{
	xRectangle crop;

	//Segmentation fault must not occur
	exynosVideoBufferSetCrop( NULL, crop );
}
END_TEST

START_TEST ( exynosVideoBufferSetCrop_ifBufinfoNull )
{
	Bool res;
	xRectangle crop;
	test_pVideoPix->refcnt = 106;
	imp_val = 103;//Special initialization which initialize bufinfo pointer by NULL

	//Segmentation fault must not occur
	res = exynosVideoBufferSetCrop( test_pVideoPix, crop );
}
END_TEST

START_TEST ( exynosVideoBufferSetCrop_correctInit )
{
	Bool res;
	xRectangle crop;

	test_pVideoPix->refcnt = 106;
	imp_val = 102;

	res = exynosVideoBufferSetCrop( test_pVideoPix, crop );

	fail_if( res != TRUE, "TRUE must return" );
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================
/* xRectangle exynosVideoBufferGetCrop (PixmapPtr pPixmap) */
START_TEST ( exynosVideoBufferGetCrop_initNull )
{
	//Segmentation fault must not occur
	exynosVideoBufferGetCrop( NULL );
}
END_TEST

START_TEST ( exynosVideoBufferGetCrop_ifBufinfoNull )
{
	xRectangle res_crop;
	test_pVideoPix->refcnt = 106;
	imp_val = 103;//Special initialization which initialize bufinfo pointer by NULL

	//Segmentation fault must not occur
	res_crop = exynosVideoBufferGetCrop( test_pVideoPix );
}
END_TEST

START_TEST ( exynosVideoBufferGetCrop_correctInit )
{
	test_pVideoPix->refcnt = 106;
	imp_val = 102;
	//Segmentation fault must not occur
	xRectangle res_crop = exynosVideoBufferGetCrop( test_pVideoPix );

	fail_if( res_crop.x != 1, "1 must not return" );
}
END_TEST
//===========================================================================================================================


//===========================================================================================================================
/* void exynosVideoBufferGetAttr (PixmapPtr pVideoPixmap, tbm_bo *tbos, tbm_bo_handle *handles,
                          unsigned int *names) */
START_TEST ( exynosVideoBufferGetAttr_initNull )
{
	tbm_bo tbos;
	tbm_bo_handle handles;
	unsigned int names;

	//Segmentation fault must not occur
	exynosVideoBufferGetAttr( NULL, &tbos, &handles, &names );
}
END_TEST

START_TEST ( exynosVideoBufferGetAttr_IfBufinfoIsNull )
{
	tbm_bo tbos;
	tbm_bo_handle handles;
	unsigned int names;

	test_pVideoPix->refcnt = 103;//Special initialization which initialize bufinfo pointer by NULL

	//Segmentation fault must not occur
	exynosVideoBufferGetAttr( test_pVideoPix, &tbos, &handles, &names );
}
END_TEST

START_TEST ( exynosVideoBufferGetAttr_IfNumPlanesIsZero )
{
	tbm_bo tbos;
	tbm_bo_handle handles;
	unsigned int names;

	test_pVideoPix->refcnt = 105;//Special initialization which initialize bufinfo pointer by NULL

	//Segmentation fault must not occur
	exynosVideoBufferGetAttr( test_pVideoPix, &tbos, &handles, &names );
}
END_TEST

START_TEST ( exynosVideoBufferGetAttr_ifTbosHandlesNamesAreNull )
{
	tbm_bo tbos;
	tbm_bo_handle handles;
	unsigned int names;

	test_pVideoPix->refcnt = 102;//Special initialization which initialize bufinfo pointer by NULL

	exynosVideoBufferGetAttr( test_pVideoPix, NULL, NULL, NULL);

	fail_if( cnt_test_memcpy != 0, "fake variable must initialization by 0" );
}
END_TEST

START_TEST ( exynosVideoBufferGetAttr_ifHandlesNamesAreNull )
{
	tbm_bo tbos;
	tbm_bo_handle handles;
	unsigned int names;

	test_pVideoPix->refcnt = 106;
	imp_val = 102;

	exynosVideoBufferGetAttr( test_pVideoPix, &tbos, NULL, NULL);

	fail_if( cnt_test_memcpy != 1, "fake variable must initialization by 1" );
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================
/* unsigned long exynosVideoBufferGetStamp (PixmapPtr pVideoPixmap) */
START_TEST ( exynosVideoBufferGetStamp_initNull )
{
	//Segmentation fault must not occur
	exynosVideoBufferGetStamp( NULL );
}
END_TEST

START_TEST ( exynosVideoBufferGetStamp_returnValue )
{
	test_pVideoPix->drawable.serialNumber = 1;

	int res = exynosVideoBufferGetStamp( test_pVideoPix );
	fail_if( res != 1, "1 must return" );
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================
/* int exynosVideoBufferGetFourcc (PixmapPtr pVideoPixmap) */
START_TEST ( exynosVideoBufferGetFourcc_initNull )
{
	//Segmentation fault must not occur
	exynosVideoBufferGetFourcc( NULL );
}
END_TEST

START_TEST ( exynosVideoBufferGetFourcc_ifBufinfoZero )
{
	test_pVideoPix->refcnt = 103;//Special initialization which initialize bufinfo pointer by NULL

	//Segmentation fault must not occur
	exynosVideoBufferGetFourcc( test_pVideoPix );
}
END_TEST

START_TEST ( exynosVideoBufferGetFourcc_returnValue)
{
	test_pVideoPix->refcnt = 106;
	imp_val = 102;

	//Segmentation fault must not occur
	int res = exynosVideoBufferGetFourcc( test_pVideoPix );

	fail_if( res != 1, "1 must returned" );
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================
/* PixmapPtr exynosVideoBufferCreateInbufZeroCopy (ScrnInfoPtr pScrn, int fourcc, int in_width,
		int in_height, pointer in_buf, ClientPtr client) */
START_TEST ( exynosVideoBufferCreateInbufZeroCopy_initNull )
{
	pointer in_buf;
	ClientPtr client = (ClientPtr)1;

	//Segmentation fault must not occur
	exynosVideoBufferCreateInbufZeroCopy( NULL, 1, 2, 3, &in_buf, &client );
}
END_TEST

START_TEST ( exynosVideoBufferCreateInbufZeroCopy_ifPPixmapNull )
{
	PixmapPtr res = NULL;

	pScrn->pScreen->CreatePixmap = funcForPScreen_CreatePixmap;
	pScrn->pScreen->myNum = 0;//with it initialized pPixmap initialized by NULL
	pointer in_buf;
	ClientPtr client = (ClientPtr)1;

	//Segmentation fault must not occur
	res = exynosVideoBufferCreateInbufZeroCopy( pScrn, 1, 2, 3, &in_buf, &client );

	fail_if( res != NULL, "NULL must returned" );
}
END_TEST

START_TEST ( exynosVideoBufferCreateInbufZeroCopy_GetNamesGivesUnsupportedId )
{
	PixmapPtr res = NULL;

	pScrn->pScreen->CreatePixmap = funcForPScreen_CreatePixmap;
	pScrn->pScreen->DestroyPixmap = funcForPScreen_DestroyPixmap;
	pScrn->pScreen->myNum = 1;//with it initialized pPixmap initialized correctly

	XV_DATA_PTR test_in_buf = ctrl_calloc( 1, sizeof( XV_DATA ) );
	//XV_DATA test_in_buf;
	test_in_buf->_header  = XV_DATA_HEADER;
	test_in_buf->_version = 1;

	ClientPtr client = (ClientPtr)1;

	res = exynosVideoBufferCreateInbufZeroCopy( pScrn, 1, 2, 3, test_in_buf, &client );

	fail_if( res != NULL, "NULL must returned" );
	fail_if( ctrl_get_cnt_calloc( CALLOC_OBJ ) != 3, "memory must free" );
}
END_TEST

//void func( ScrnInfoPtr S )
//{
//	S->pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
//	ctrl_free( S->pScreen );
//}
//
//START_TEST ( exynosVideoBufferCreateInbufZeroCopy_ifBufinfoNull )
//{
//	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
//
//	PRINTFVAL( ctrl_print_mem() )
//	func( pScrn );
//
//	PRINTFVAL( ctrl_print_mem() )
//}
//END_TEST

START_TEST ( exynosVideoBufferCreateInbufZeroCopy_ifBufinfoNull )
{
	PixmapPtr res = NULL;

	pScrn->pScreen->CreatePixmap = funcForPScreen_CreatePixmap;
	pScrn->pScreen->DestroyPixmap = funcForPScreen_DestroyPixmap;
	pScrn->pScreen->myNum = 1;//with it initialized pPixmap initialized correctly

	XV_DATA_PTR test_in_buf = ctrl_calloc( 1, sizeof( XV_DATA ) );
	test_in_buf->_header  = XV_DATA_HEADER;
	test_in_buf->_version = XV_DATA_VERSION;

	ClientPtr client = (ClientPtr)1;

	//fourc is 0 for initialized bufinfo by NULL
	int fourcc = 0;

	res = exynosVideoBufferCreateInbufZeroCopy( pScrn, fourcc, 2, 3, test_in_buf, &client );

	fail_if( res != NULL, "NULL must returned" );
	fail_if( ctrl_get_cnt_calloc( CALLOC_OBJ ) != 3, "memory must free" );
}
END_TEST

//START_TEST ( exynosVideoBufferCreateInbufZeroCopy_correctInit )
//{
//	PixmapPtr res = NULL;
//
//	pScrn->pScreen->CreatePixmap = funcForPScreen_CreatePixmap;
//	pScrn->pScreen->DestroyPixmap = funcForPScreen_DestroyPixmap;
//	pScrn->pScreen->myNum = 1;//with it initialized pPixmap initialized correctly
//
//	XV_DATA_PTR test_in_buf = ctrl_calloc( 1, sizeof( XV_DATA ) );
//	test_in_buf->_header  = XV_DATA_HEADER;
//	test_in_buf->_version = XV_DATA_VERSION;
//	test_in_buf->BufType = BUF_TYPE_DMABUF ;
//	int a = 1234;
//	ctrl_print_mem();
//
//	_add_calloc( &a, TBM_OBJ );
//
//	ctrl_print_mem();
//
//	ClientPtr client = (ClientPtr)1;
//
//	int fourcc = FOURCC_RGB565;
//
//	res = exynosVideoBufferCreateInbufZeroCopy( pScrn, fourcc, 2, 3, test_in_buf, &client );
//
//	fail_if( res == NULL, "NULL must returned" );
//	fail_if( ctrl_get_cnt_calloc( CALLOC_OBJ ) != 6, "memory must free" );
//}
//END_TEST
//===========================================================================================================================

//===========================================================================================================================
/* PixmapPtr exynosVideoBufferCreateInbufRAW (ScrnInfoPtr pScrn, int fourcc, int in_width, int in_height) */
START_TEST ( exynosVideoBufferCreateInbufRAW_ifBufinfoNull )
{
	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );

	PixmapPtr res = NULL;

	pScrn->pScreen->CreatePixmap = funcForPScreen_CreatePixmap;
	pScrn->pScreen->DestroyPixmap = funcForPScreen_DestroyPixmap;
	pScrn->pScreen->myNum = 1;//with it initialized pPixmap initialized correctly

	//fourc is 0 for initialized bufinfo by NULL
	int fourcc = 0;

	res = exynosVideoBufferCreateInbufRAW( pScrn, fourcc, 2, 3 );

	fail_if( res != NULL, "NULL mus returned" );
}
END_TEST

START_TEST ( exynosVideoBufferCreateInbufRAW_ifPPixmapNull )
{
	PixmapPtr res = NULL;

	pScrn->pScreen->CreatePixmap = funcForPScreen_CreatePixmap;
	pScrn->pScreen->DestroyPixmap = funcForPScreen_DestroyPixmap;
	pScrn->pScreen->myNum = 0;//with it initialized pPixmap initialized byNULL

	int fourcc = FOURCC_RGB565;

	res = exynosVideoBufferCreateInbufRAW( pScrn, fourcc, 2, 3 );

	fail_if( ctrl_get_cnt_calloc( CALLOC_OBJ ) != 3, "Memory must be free" );
	fail_if( res != NULL, "NULL must returned" );
}
END_TEST

START_TEST ( exynosVideoBufferCreateInbufRAW_correctInit )
{
	PixmapPtr res = NULL;

	pScrn->pScreen->CreatePixmap = funcForPScreen_CreatePixmap;
	pScrn->pScreen->DestroyPixmap = funcForPScreen_DestroyPixmap;
	pScrn->pScreen->myNum = 1;//with it initialized pPixmap initialized correctly

	int fourcc = FOURCC_RGB565;

	res = exynosVideoBufferCreateInbufRAW( pScrn, fourcc, 2, 3 );

	fail_if( ctrl_get_cnt_calloc( CALLOC_OBJ ) != 6, "Memory must be free" );
	fail_if( res == NULL, "NULL must returned" );
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================
/* PixmapPtr exynosVideoBufferModifyOutbufPixmap(ScrnInfoPtr pScrn, int fourcc, int out_width,
                                    int out_height, PixmapPtr pPixmap) */
START_TEST ( exynosVideoBufferModifyOutbufPixmap_pScrnNull )
{
	PixmapPtr pPixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );

	int fourcc = 1;
	int out_with = 2;
	int out_height = 3;

	//Segmentation fault must not occure
	exynosVideoBufferModifyOutbufPixmap( NULL, fourcc, out_with, out_height, pPixmap );
}
END_TEST

START_TEST ( exynosVideoBufferModifyOutbufPixmap_pPixMapNull )
{
	int fourcc = 1;
	int out_with = 2;
	int out_height = 3;

	//Segmentation fault must not occure
	exynosVideoBufferModifyOutbufPixmap( pScrn, fourcc, out_with, out_height, NULL );
}
END_TEST

START_TEST ( exynosVideoBufferModifyOutbufPixmap_PScreenNull )
{
	PixmapPtr pPixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );

	int fourcc = 1;
	int out_with = 2;
	int out_height = 3;

	//Segmentation fault must not occure
	exynosVideoBufferModifyOutbufPixmap( pScrn, fourcc, out_with, out_height, pPixmap );
}
END_TEST

START_TEST ( exynosVideoBufferModifyOutbufPixmap_ifExynosVideoBufferGetInfoReturnNotNull )
{
	PixmapPtr pPixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );
	pPixmap->drawable.pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
	pPixmap->refcnt = 102;//Special initialization which initialize bufinfo pointer by NULL

	int fourcc = 1;
	int out_with = 2;
	int out_height = 3;

	PixmapPtr res;

	res = exynosVideoBufferModifyOutbufPixmap( pScrn, fourcc, out_with, out_height, pPixmap );

	fail_if( res == NULL, "Null must returned ");
	fail_if( res->refcnt != 103, "wrong value returned" );
}
END_TEST

START_TEST ( exynosVideoBufferModifyOutbufPixmap_ )
{
	PixmapPtr pPixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );
	pPixmap->drawable.pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
	pPixmap->refcnt = 103;//Special initialization which initialize bufinfo pointer by NULL

	pPixmap->drawable.pScreen->CreatePixmap = funcForPScreen_CreatePixmap;
	pPixmap->drawable.pScreen->DestroyPixmap = funcForPScreen_DestroyPixmap;

	int fourcc = 0;
	int out_with = 2;
	int out_height = 3;

	PixmapPtr res;

	res = exynosVideoBufferModifyOutbufPixmap( pScrn, fourcc, out_with, out_height, pPixmap );

	fail_if( res != NULL, "Null must returned ");
	fail_if( ctrl_get_cnt_calloc( CALLOC_OBJ ) != 9, "Memory must free");
}
END_TEST

START_TEST ( exynosVideoBufferModifyOutbufPixmap_ifBufinfoNull )
{
	PixmapPtr pPixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );
	pPixmap->drawable.pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
	pPixmap->refcnt = 106;
	imp_val = 103;//Special initialization which initialize bufinfo pointer by NULL

	pPixmap->drawable.pScreen->CreatePixmap = funcForPScreen_CreatePixmap;
	pPixmap->drawable.pScreen->DestroyPixmap = funcForPScreen_DestroyPixmap;

	int fourcc = 0;
	int out_with = 2;
	int out_height = 3;

	PixmapPtr res;

	res = exynosVideoBufferModifyOutbufPixmap( pScrn, fourcc, out_with, out_height, pPixmap );

	fail_if( res != NULL, "Null must returned ");
	fail_if( ctrl_get_cnt_calloc( CALLOC_OBJ ) != 10, "Memory must free");
}
END_TEST

START_TEST ( exynosVideoBufferModifyOutbufPixmap_correctInit )
{
	PixmapPtr pPixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );
	pPixmap->drawable.pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
	pPixmap->refcnt = 106;
	imp_val = 103;//Special initialization which initialize bufinfo pointer by NULL

	pPixmap->drawable.pScreen->CreatePixmap = funcForPScreen_CreatePixmap;
	pPixmap->drawable.pScreen->DestroyPixmap = funcForPScreen_DestroyPixmap;

	int fourcc = FOURCC_RGB565;
	int out_with = 2;
	int out_height = 3;

	PixmapPtr res;

	res = exynosVideoBufferModifyOutbufPixmap( pScrn, fourcc, out_with, out_height, pPixmap );

	fail_if( res == NULL, "Null must not returned ");
	fail_if( ctrl_get_cnt_calloc( CALLOC_OBJ ) != 12, "Memory must allocate");
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================
/* PixmapPtr exynosVideoBufferCreateOutbufFB(ScrnInfoPtr pScrn, int fourcc, int out_width, int out_height) */
START_TEST ( exynosVideoBufferCreateOutbufFB_pScrnNull )
{
	int fourcc = 1;
	int out_with = 2;
	int out_height = 3;

	//Segmentation fault must not occur
	exynosVideoBufferCreateOutbufFB( NULL, fourcc, out_with, out_height );
}
END_TEST

START_TEST ( exynosVideoBufferCreateOutbufFB_bufinfoNull )
{
	PixmapPtr res;

	int fourcc = 0;//For make bufinfo NULL
	int out_with = 2;
	int out_height = 3;

	//Segmentation fault must not occur
	res = exynosVideoBufferCreateOutbufFB( pScrn, fourcc, out_with, out_height );

	fail_if( res != NULL, "NULL must returned" );
}
END_TEST

START_TEST ( exynosVideoBufferCreateOutbufFB_pPixmapNull )
{
	pScrn->pScreen->CreatePixmap = funcForPScreen_CreatePixmap;
	pScrn->pScreen->DestroyPixmap = funcForPScreen_DestroyPixmap;

	PixmapPtr res;

	pScrn->pScreen->myNum = 0;

	int fourcc = FOURCC_RGB565;//For make bufinfo NULL
	int out_with = 2;
	int out_height = 3;

	//Segmentation fault must not occur
	res = exynosVideoBufferCreateOutbufFB( pScrn, fourcc, out_with, out_height );

	fail_if( res != NULL, "NULL must returned" );
	fail_if( ctrl_get_cnt_calloc( CALLOC_OBJ ) != 3, "Memory must destroyed" );
}
END_TEST

START_TEST ( exynosVideoBufferCreateOutbufFB_correctInit )
{
	pScrn->pScreen->CreatePixmap = funcForPScreen_CreatePixmap;
	pScrn->pScreen->DestroyPixmap = funcForPScreen_DestroyPixmap;

	PixmapPtr res;

	pScrn->pScreen->myNum = 1;

	int fourcc = FOURCC_RGB565;//For make bufinfo NULL
	int out_with = 2;
	int out_height = 3;

	res = exynosVideoBufferCreateOutbufFB( pScrn, fourcc, out_with, out_height );
	fail_if( res == NULL, "NULL must not returned" );
	fail_if( ctrl_get_cnt_calloc( CALLOC_OBJ ) != 12, "Memory must allocated" );
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================
/* Bool exynosVideoBufferCopyRawData ( PixmapPtr dst, pointer * src_sample, int *src_length) */
START_TEST ( exynosVideoBufferCopyRawData_bufinfoNull )
{
	pointer src_sample[4];
	int *src_length[4];

	PixmapPtr dst = ctrl_calloc( 1, sizeof( PixmapRec ) );
	dst->refcnt = 106;
	imp_val = 103;

	//Segmentation fault must not occur
	exynosVideoBufferCopyRawData( dst, src_sample, src_length );
}
END_TEST

START_TEST ( exynosVideoBufferCopyRawData_srcSampleSrcLenghthAreNull )
{
	pointer src_sample[PLANE_CNT];
	int src_length[PLANE_CNT];
	Bool res;

	int i = 0;
	for( i = 0; i < PLANE_CNT; i++ )
	{
		src_sample[i] = NULL;
		src_length[i] = 0;
	}

	PixmapPtr dst = ctrl_calloc( 1, sizeof( PixmapRec ) );
	dst->refcnt = 106;
	imp_val = 106;

	//Segmentation fault must not occur
	res = exynosVideoBufferCopyRawData( dst, src_sample, src_length );

	fail_if( res != FALSE, "FALSE must returned" );
}
END_TEST

START_TEST ( exynosVideoBufferCopyRawData_correctInit )
{
	pointer src_sample[PLANE_CNT];
	int src_length[PLANE_CNT];
	Bool res;

	int i = 0;
	for( i = 0; i < PLANE_CNT; i++ )
	{
		src_sample[i] = 1;
		src_length[i] = 1;
	}

	PixmapPtr dst = ctrl_calloc( 1, sizeof( PixmapRec ) );
	dst->refcnt = 106;
	imp_val = 106;

	//Segmentation fault must not occur
	res = exynosVideoBufferCopyRawData( dst, src_sample, &src_length );

	fail_if( res != TRUE, "TRUE must returned, correct initialization" );
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================
/* Bool exynosVideoBufferCopyOneChannel (PixmapPtr pPixmap, pointer * src_sample, int *src_pitches, int *src_height) */
START_TEST ( exynosVideoBufferCopyOneChannel_bufinfoNull )
{
	pointer src_sample[PLANE_CNT];
	int src_pitches[PLANE_CNT];
	int src_height[PLANE_CNT];

	PixmapPtr pPixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );
	//pPixmap->drawable.pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
	pPixmap->refcnt = 106;
	imp_val = 103;//Special initialization which initialize bufinfo pointer by NULL

	fail_if( exynosVideoBufferCopyOneChannel( pPixmap, src_sample, src_pitches, src_height ) != FALSE, "FALSE must returned" );
}
END_TEST

START_TEST ( exynosVideoBufferCopyOneChannel_ifSrc_SampleNull )
{
	pointer src_sample[PLANE_CNT];
	int src_pitches[PLANE_CNT];
	int src_height[PLANE_CNT];

	PixmapPtr pPixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );
	pPixmap->drawable.pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
	pPixmap->refcnt = 106;
	imp_val = 106;//Special initialization which initialize bufinfo pointer by NULL

	int i = 0;
	for( i = 0; i < PLANE_CNT; i++ )
	{
		src_sample[i] = NULL;
	}

	fail_if( exynosVideoBufferCopyOneChannel( pPixmap, src_sample, src_pitches, src_height ) != FALSE, "FALSE must returned" );
	fail_if( cnt_test_memcpy != 0, "fake variable must be 1" );
}
END_TEST

START_TEST ( exynosVideoBufferCopyOneChannel_correctInit )
{
	pointer src_sample[PLANE_CNT];
	int src_pitches[PLANE_CNT];
	int src_height[PLANE_CNT] = { 1, 1, 1 ,1 };

	PixmapPtr pPixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );
	pPixmap->drawable.pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
	pPixmap->refcnt = 106;
	imp_val = 106;//Special initialization which initialize bufinfo pointer by NULL

	int i = 0;
	for( i = 0; i < PLANE_CNT; i++ )
	{
		src_sample[i] = 1;
	}

	fail_if( exynosVideoBufferCopyOneChannel( pPixmap, src_sample, src_pitches, src_height ) != TRUE, "TRUE must returned" );
	fail_if( cnt_test_memcpy != 3, "fake variable must be 1" );
}
END_TEST
//===========================================================================================================================

//===========================================================================================================================

START_TEST ( valgrind_test )
{
	PixmapPtr A = ctrl_calloc( 1, sizeof( PixmapRec ) );
	//PixmapPtr B = ctrl_calloc( 1, sizeof( PixmapRec ) );

	//PRINTFVAL( sizeof( PixmapRec ) )
	//PRINTFVAL( sizeof( calloc_obj_item ) )

	//ctrl_free( A );
	A = NULL;
	//ctrl_free( B );
}
END_TEST
//===========================================================================================================================


//***************************************************************************************************************************

/*
EXYNOSBufInfoPtr test_bufinfo = ctrl_calloc( 1, sizeof( EXYNOSBufInfo ) );
//test_bufinfo->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
//test_bufinfo->pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
pExynos = test_bufinfo->pScrn->driverPrivate;
pExynos->pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );
*/

int test_video_buffer_suite (run_stat_data* stat)
{
	Suite *s = suite_create ( "VIDEO_BUFFER.C" );
	/* Core test case */

	TCase *tc_exynosVideoBufferComparePixStamp      = tcase_create ( "_exynosVideoBufferComparePixStamp" );
	TCase *tc_exynosVideoBufferGetInfo              = tcase_create ( "_exynosVideoBufferGetInfo" );
	TCase *tc_exynosVideoBufferGetNames             = tcase_create ( "_exynosVideoBufferGetNames" );
	TCase *tc_exynosVideoBufferCreateFBID           = tcase_create ( "exynosVideoBufferCreateFBID" );
	TCase *tc_exynosVideoBufferDestroyFBID          = tcase_create ( "_exynosVideoBufferDestroyFBID" );
	TCase *tc_exynosVideoBufferReturnToClient       = tcase_create ( "_exynosVideoBufferReturnToClient" );
	TCase *tc_exynosVideoBufferGetColorType         = tcase_create ( "_exynosVideoBufferGetColorType" );
	TCase *tc_exynosVideoBufferGetDrmFourcc         = tcase_create ( "exynosVideoBufferGetDrmFourcc" );
	TCase *tc_exynosVideoBufferRef                  = tcase_create ( "exynosVideoBufferRef" );
	TCase *tc_exynosVideoBufferFree                 = tcase_create ( "exynosVideoBufferFree" );
	TCase *tc_exynosVideoBufferConverting           = tcase_create ( "exynosVideoBufferConverting" );
	TCase *tc_exynosVideoBufferShowing              = tcase_create ( "exynosVideoBufferShowing" );
	TCase *tc_exynosVideoBufferIsConverting         = tcase_create ( "exynosVideoBufferIsConverting" );
	TCase *tc_exynosVideoBufferIsShowing            = tcase_create ( "exynosVideoBufferIsShowing" );
	TCase *tc_exynosVideoBufferGetFBID              = tcase_create ( "exynosVideoBufferGetFBID" );
	TCase *tc_exynosVideoBufferSetCrop              = tcase_create ( "exynosVideoBufferSetCrop" );
	TCase *tc_exynosVideoBufferGetCrop              = tcase_create ( "exynosVideoBufferGetCrop" );
	TCase *tc_exynosVideoBufferGetAttr              = tcase_create ( "exynosVideoBufferGetAttr" );
	TCase *tc_exynosVideoBufferGetStamp             = tcase_create ( "exynosVideoBufferGetStamp" );
	TCase *tc_exynosVideoBufferGetFourcc            = tcase_create ( "exynosVideoBufferGetFourcc" );
	TCase *tc_exynosVideoBufferCreateInbufZeroCopy  = tcase_create ( "exynosVideoBufferCreateInbufZeroCopy" );
	TCase *tc_exynosVideoBufferCreateInbufRAW       = tcase_create ( "exynosVideoBufferCreateInbufRAW" );
	TCase *tc_exynosVideoBufferCreateInbufRAW_2     = tcase_create ( "exynosVideoBufferCreateInbufRAW" );
	TCase *tc_exynosVideoBufferModifyOutbufPixmap   = tcase_create ( "exynosVideoBufferModifyOutbufPixmap" );
	TCase *tc_exynosVideoBufferModifyOutbufPixmap_2 = tcase_create ( "exynosVideoBufferModifyOutbufPixmap" );
	TCase *tc_exynosVideoBufferCreateOutbufFB       = tcase_create ( "exynosVideoBufferCreateOutbufFB" );
	TCase *tc_exynosVideoBufferCreateOutbufFB_2     = tcase_create ( "exynosVideoBufferCreateOutbufFB" );
	TCase *tc_exynosVideoBufferCopyRawData          = tcase_create ( "exynosVideoBufferCopyRawData" );
	TCase *tc_exynosVideoBufferCopyOneChannel       = tcase_create ( "exynosVideoBufferCopyOneChannel" );
	TCase *tc_valgrind_test                         = tcase_create ( "valgrind" );


	//_exynosVideoBufferComparePixStamp
	tcase_add_checked_fixture( tc_exynosVideoBufferComparePixStamp, setup_InitTestpVideoPix, teardown_InitTestpVideoPix );
	tcase_add_test ( tc_exynosVideoBufferComparePixStamp, _exynosVideoBufferComparePixStamp_IfEqual_Success );
	tcase_add_test ( tc_exynosVideoBufferComparePixStamp, _exynosVideoBufferComparePixStamp_IfNotEqual_Success );

	//_exynosVideoBufferGetInfo
	tcase_add_checked_fixture( tc_exynosVideoBufferGetInfo, setup_InitTestpVideoPix, teardown_InitTestpVideoPix );
	tcase_add_test ( tc_exynosVideoBufferGetInfo, _exynosVideoBufferGetInfo_bufinfoInitiByNull_Failure);
	tcase_add_test ( tc_exynosVideoBufferGetInfo, _exynosVideoBufferGetInfo_correctInit_Success);

	//_exynosVideoBufferGetNames
	tcase_add_test ( tc_exynosVideoBufferGetNames, _exynosVideoBufferGetNames_IfDataHeaderNotEqualToXV_DATA_HEADER );
	tcase_add_test ( tc_exynosVideoBufferGetNames, _exynosVideoBufferGetNames_IfValidNotEqualToXV_VERSION_MISMATCH );
	tcase_add_test ( tc_exynosVideoBufferGetNames, _exynosVideoBufferGetNames_correctInit );

	//exynosVideoBufferCreateFBID
	tcase_add_test ( tc_exynosVideoBufferCreateFBID, exynosVideoBufferCreateFBID_initPixmapNull );
	tcase_add_test ( tc_exynosVideoBufferCreateFBID, exynosVideoBufferCreateFBID_correctInit );
	tcase_add_test ( tc_exynosVideoBufferCreateFBID, exynosVideoBufferCreateFBID_correctInit_1 );

	//_exynosVideoBufferDestroyFBID
	tcase_add_test ( tc_exynosVideoBufferDestroyFBID, exynosVideoBufferDestroyFBID_ifDataIsNull );
	tcase_add_test ( tc_exynosVideoBufferDestroyFBID, exynosVideoBufferDestroyFBID_ifBufinfoFb_idIsNegative );
	tcase_add_test ( tc_exynosVideoBufferDestroyFBID, exynosVideoBufferDestroyFBID_correctInit );

	//_exynosVideoBufferReturnToClient
	tcase_add_test ( tc_exynosVideoBufferReturnToClient, _exynosVideoBufferReturnToClient_ifBufinfoNull );
	tcase_add_test ( tc_exynosVideoBufferReturnToClient, _exynosVideoBufferReturnToClient_ifPClientNull );
	tcase_add_test ( tc_exynosVideoBufferReturnToClient, _exynosVideoBufferReturnToClient_correctInit );

	//exynosVideoBufferGetColorType
	tcase_add_test ( tc_exynosVideoBufferGetColorType, exynosVideoBufferGetColorType_initiNotSupportedType );
	tcase_add_test ( tc_exynosVideoBufferGetColorType, exynosVideoBufferGetColorType_correctInit );

	//exynosVideoBufferGetDrmFourcc
	tcase_add_test ( tc_exynosVideoBufferGetDrmFourcc, exynosVideoBufferGetColorType_iniByUnsuppotedDrmFourcc );
	tcase_add_test ( tc_exynosVideoBufferGetDrmFourcc, exynosVideoBufferGetDrmFourcc_correctinit );

	//exynosVideoBufferRef
	tcase_add_test ( tc_exynosVideoBufferRef, exynosVideoBufferRef_PVideoPixmapIsNull );
	tcase_add_test ( tc_exynosVideoBufferRef, exynosVideoBufferRef_correctInit );

	//exynosVideoBufferFree
	tcase_add_checked_fixture( tc_exynosVideoBufferFree, setup_InitTestpVideoPix, teardown_InitTestpVideoPix );
	tcase_add_test ( tc_exynosVideoBufferFree, exynosVideoBufferFree_initNull );
	tcase_add_test ( tc_exynosVideoBufferFree, exynosVideoBufferFree_ifChecksNotPass );
	tcase_add_test ( tc_exynosVideoBufferFree, exynosVideoBufferFree_ifBufinfo_PClientISNotNull );
	tcase_add_test ( tc_exynosVideoBufferFree, exynosVideoBufferFree_ifBufinfoFb_IdBiggestThenZero );

	//exynosVideoBufferConverting
	tcase_add_checked_fixture( tc_exynosVideoBufferConverting, setup_InitTestpVideoPix, teardown_InitTestpVideoPix );
	tcase_add_test ( tc_exynosVideoBufferConverting, exynosVideoBufferConverting_initByNull );
	tcase_add_test ( tc_exynosVideoBufferConverting, exynosVideoBufferConverting_ifBufinfoNull );

	//exynosVideoBufferShowing
	tcase_add_checked_fixture( tc_exynosVideoBufferShowing, setup_InitTestpVideoPix, teardown_InitTestpVideoPix );
	tcase_add_test ( tc_exynosVideoBufferShowing, exynosVideoBufferShowing_initByNull );
	tcase_add_test ( tc_exynosVideoBufferShowing, exynosVideoBufferShowing_ifBufinfoNull );

	//exynosVideoBufferIsConverting
	tcase_add_checked_fixture( tc_exynosVideoBufferIsConverting, setup_InitTestpVideoPix, teardown_InitTestpVideoPix );
	tcase_add_test ( tc_exynosVideoBufferIsConverting, exynosVideoBufferIsConverting_initNull );
	tcase_add_test ( tc_exynosVideoBufferIsConverting, exynosVideoBufferIsConverting_ifBufinfoNull );
	tcase_add_test ( tc_exynosVideoBufferIsConverting, exynosVideoBufferIsConverting_returnValue_Success );

	//exynosVideoBufferIsShowing
	tcase_add_checked_fixture( tc_exynosVideoBufferIsShowing, setup_InitTestpVideoPix, teardown_InitTestpVideoPix );
	tcase_add_test ( tc_exynosVideoBufferIsShowing, exynosVideoBufferIsShowing_initNull );
	tcase_add_test ( tc_exynosVideoBufferIsShowing, exynosVideoBufferIsShowing_ifBufinfoNull );
	tcase_add_test ( tc_exynosVideoBufferIsShowing, exynosVideoBufferIsShowing_returnValue_Success );

	//exynosVideoBufferGetFBID
	tcase_add_checked_fixture( tc_exynosVideoBufferGetFBID, setup_InitTestpVideoPix, teardown_InitTestpVideoPix );
	tcase_add_test ( tc_exynosVideoBufferGetFBID, exynosVideoBufferGetFBID_initByNull );
	tcase_add_test ( tc_exynosVideoBufferGetFBID, exynosVideoBufferGetFBID_ifBufinfoNull );
	tcase_add_test ( tc_exynosVideoBufferGetFBID, exynosVideoBufferGetFBID_returnValue );

	//exynosVideoBufferSetCrop
	tcase_add_checked_fixture( tc_exynosVideoBufferSetCrop, setup_InitTestpVideoPix, teardown_InitTestpVideoPix );
	tcase_add_test ( tc_exynosVideoBufferSetCrop, exynosVideoBufferSetCrop_initNull );
	tcase_add_test ( tc_exynosVideoBufferSetCrop, exynosVideoBufferSetCrop_ifBufinfoNull );
	tcase_add_test ( tc_exynosVideoBufferSetCrop, exynosVideoBufferSetCrop_correctInit );

	//exynosVideoBufferGetCrop
	tcase_add_checked_fixture( tc_exynosVideoBufferGetCrop, setup_InitTestpVideoPix, teardown_InitTestpVideoPix );
	tcase_add_test ( tc_exynosVideoBufferGetCrop, exynosVideoBufferGetCrop_initNull );
	tcase_add_test ( tc_exynosVideoBufferGetCrop, exynosVideoBufferGetCrop_ifBufinfoNull );
	tcase_add_test ( tc_exynosVideoBufferGetCrop, exynosVideoBufferGetCrop_correctInit );

	//exynosVideoBufferGetAttr
	tcase_add_checked_fixture( tc_exynosVideoBufferGetAttr, setup_InitTestpVideoPix, teardown_InitTestpVideoPix );
	tcase_add_test ( tc_exynosVideoBufferGetAttr, exynosVideoBufferGetAttr_initNull );
	tcase_add_test ( tc_exynosVideoBufferGetAttr, exynosVideoBufferGetAttr_IfBufinfoIsNull );
	tcase_add_test ( tc_exynosVideoBufferGetAttr, exynosVideoBufferGetAttr_IfNumPlanesIsZero );
	tcase_add_test ( tc_exynosVideoBufferGetAttr, exynosVideoBufferGetAttr_ifTbosHandlesNamesAreNull );
	tcase_add_test ( tc_exynosVideoBufferGetAttr, exynosVideoBufferGetAttr_ifHandlesNamesAreNull );

	//exynosVideoBufferGetStamp
	tcase_add_checked_fixture( tc_exynosVideoBufferGetStamp, setup_InitTestpVideoPix, teardown_InitTestpVideoPix );
	tcase_add_test ( tc_exynosVideoBufferGetStamp, exynosVideoBufferGetStamp_initNull );
	tcase_add_test ( tc_exynosVideoBufferGetStamp, exynosVideoBufferGetStamp_returnValue );

	//exynosVideoBufferGetFourcc
	tcase_add_checked_fixture( tc_exynosVideoBufferGetFourcc, setup_InitTestpVideoPix, teardown_InitTestpVideoPix );
	tcase_add_test ( tc_exynosVideoBufferGetFourcc, exynosVideoBufferGetFourcc_initNull );
	tcase_add_test ( tc_exynosVideoBufferGetFourcc, exynosVideoBufferGetFourcc_ifBufinfoZero );
	tcase_add_test ( tc_exynosVideoBufferGetFourcc, exynosVideoBufferGetFourcc_returnValue );

	//exynosVideoBufferCreateInbufZeroCopy
	tcase_add_checked_fixture( tc_exynosVideoBufferCreateInbufZeroCopy, setup_InitPScrnPScreen, teardown_DestroyPScrnPScreen );
	tcase_add_test ( tc_exynosVideoBufferCreateInbufZeroCopy, exynosVideoBufferCreateInbufZeroCopy_initNull );
	tcase_add_test ( tc_exynosVideoBufferCreateInbufZeroCopy, exynosVideoBufferCreateInbufZeroCopy_ifPPixmapNull );
	tcase_add_test ( tc_exynosVideoBufferCreateInbufZeroCopy, exynosVideoBufferCreateInbufZeroCopy_GetNamesGivesUnsupportedId );
	tcase_add_test ( tc_exynosVideoBufferCreateInbufZeroCopy, exynosVideoBufferCreateInbufZeroCopy_ifBufinfoNull );
	//tcase_add_test ( tc_exynosVideoBufferCreateInbufZeroCopy, exynosVideoBufferCreateInbufZeroCopy_correctInit );

	//exynosVideoBufferCreateInbufRAW
	tcase_add_test ( tc_exynosVideoBufferCreateInbufRAW, exynosVideoBufferCreateInbufRAW_ifBufinfoNull );
	tcase_add_checked_fixture( tc_exynosVideoBufferCreateInbufRAW_2, setup_InitPScrnPScreen_2, teardown_DestroyPScrnPScreen_2 );
	tcase_add_test ( tc_exynosVideoBufferCreateInbufRAW_2, exynosVideoBufferCreateInbufRAW_ifPPixmapNull );
	tcase_add_test ( tc_exynosVideoBufferCreateInbufRAW_2, exynosVideoBufferCreateInbufRAW_correctInit );

	//exynosVideoBufferModifyOutbufPixmap
	tcase_add_test ( tc_exynosVideoBufferModifyOutbufPixmap, exynosVideoBufferModifyOutbufPixmap_pScrnNull );
	tcase_add_checked_fixture( tc_exynosVideoBufferModifyOutbufPixmap_2, setup_InitPScrnPScreen_2, teardown_DestroyPScrnPScreen_2 );
	tcase_add_test ( tc_exynosVideoBufferModifyOutbufPixmap_2, exynosVideoBufferModifyOutbufPixmap_pPixMapNull );
	tcase_add_test ( tc_exynosVideoBufferModifyOutbufPixmap_2, exynosVideoBufferModifyOutbufPixmap_PScreenNull );
	tcase_add_test ( tc_exynosVideoBufferModifyOutbufPixmap_2, exynosVideoBufferModifyOutbufPixmap_ifExynosVideoBufferGetInfoReturnNotNull );
	tcase_add_test ( tc_exynosVideoBufferModifyOutbufPixmap_2, exynosVideoBufferModifyOutbufPixmap_ifBufinfoNull );
	tcase_add_test ( tc_exynosVideoBufferModifyOutbufPixmap_2, exynosVideoBufferModifyOutbufPixmap_correctInit );

	//exynosVideoBufferCreateOutbufFB
	tcase_add_test ( tc_exynosVideoBufferCreateOutbufFB, exynosVideoBufferCreateOutbufFB_pScrnNull );
	tcase_add_checked_fixture( tc_exynosVideoBufferCreateOutbufFB_2, setup_InitPScrnPScreen_2, teardown_DestroyPScrnPScreen_2 );
	tcase_add_test ( tc_exynosVideoBufferCreateOutbufFB_2, exynosVideoBufferCreateOutbufFB_bufinfoNull );
	tcase_add_test ( tc_exynosVideoBufferCreateOutbufFB_2, exynosVideoBufferCreateOutbufFB_pPixmapNull );
	tcase_add_test ( tc_exynosVideoBufferCreateOutbufFB_2, exynosVideoBufferCreateOutbufFB_correctInit );

	//exynosVideoBufferSaveRawData
	tcase_add_test ( tc_exynosVideoBufferCopyRawData, exynosVideoBufferCopyRawData_bufinfoNull );
	tcase_add_test ( tc_exynosVideoBufferCopyRawData, exynosVideoBufferCopyRawData_srcSampleSrcLenghthAreNull );
	tcase_add_test ( tc_exynosVideoBufferCopyRawData, exynosVideoBufferCopyRawData_correctInit );

	//tc_exynosVideoBufferCopyOneChannel
	tcase_add_test ( tc_exynosVideoBufferCopyOneChannel, exynosVideoBufferCopyOneChannel_bufinfoNull );
	tcase_add_test ( tc_exynosVideoBufferCopyOneChannel, exynosVideoBufferCopyOneChannel_ifSrc_SampleNull );
	tcase_add_test ( tc_exynosVideoBufferCopyOneChannel, exynosVideoBufferCopyOneChannel_correctInit );

	//valgrind
	tcase_add_test ( tc_valgrind_test, valgrind_test );



	suite_add_tcase ( s, tc_exynosVideoBufferComparePixStamp );
	suite_add_tcase ( s, tc_exynosVideoBufferGetInfo );
	suite_add_tcase ( s, tc_exynosVideoBufferGetNames );
	suite_add_tcase ( s, tc_exynosVideoBufferCreateFBID );
	suite_add_tcase ( s, tc_exynosVideoBufferDestroyFBID );
	suite_add_tcase ( s, tc_exynosVideoBufferReturnToClient );
	suite_add_tcase ( s, tc_exynosVideoBufferGetColorType );
	suite_add_tcase ( s, tc_exynosVideoBufferGetDrmFourcc );
	suite_add_tcase ( s, tc_exynosVideoBufferRef );
	suite_add_tcase ( s, tc_exynosVideoBufferFree );
	suite_add_tcase ( s, tc_exynosVideoBufferConverting );
	suite_add_tcase ( s, tc_exynosVideoBufferShowing );
	suite_add_tcase ( s, tc_exynosVideoBufferIsConverting );
	suite_add_tcase ( s, tc_exynosVideoBufferIsShowing );
	suite_add_tcase ( s, tc_exynosVideoBufferGetFBID );
	suite_add_tcase ( s, tc_exynosVideoBufferSetCrop );
	suite_add_tcase ( s, tc_exynosVideoBufferGetCrop );
	suite_add_tcase ( s, tc_exynosVideoBufferGetAttr );
	suite_add_tcase ( s, tc_exynosVideoBufferGetStamp );
	suite_add_tcase ( s, tc_exynosVideoBufferGetFourcc );
	suite_add_tcase ( s, tc_exynosVideoBufferCreateInbufZeroCopy );
	suite_add_tcase ( s, tc_exynosVideoBufferCreateInbufRAW );
	suite_add_tcase ( s, tc_exynosVideoBufferCreateInbufRAW_2 );
	suite_add_tcase ( s, tc_exynosVideoBufferModifyOutbufPixmap );
	suite_add_tcase ( s, tc_exynosVideoBufferModifyOutbufPixmap_2 );
	suite_add_tcase ( s, tc_exynosVideoBufferCreateOutbufFB );
	suite_add_tcase ( s, tc_exynosVideoBufferCreateOutbufFB_2 );
	suite_add_tcase ( s, tc_exynosVideoBufferCopyRawData );
	suite_add_tcase ( s, tc_exynosVideoBufferCopyOneChannel );

	suite_add_tcase ( s, tc_valgrind_test );




    SRunner *sr = srunner_create( s );
    srunner_run_all( sr, CK_VERBOSE );

    stat->num_failure=srunner_ntests_failed(sr);
    stat->num_pass=srunner_ntests_run(sr) - srunner_ntests_failed(sr);

	stat->num_tests = srunner_ntests_run( sr );
	stat->num_pass = srunner_ntests_run( sr ) - srunner_ntests_failed( sr );
	stat->num_failure = srunner_ntests_failed( sr );

	stat->num_tested = 25;
	stat->num_nottested = 0;

	srunner_free( sr );

	return 0;
}



