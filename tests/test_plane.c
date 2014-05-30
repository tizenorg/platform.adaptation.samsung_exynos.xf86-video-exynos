/*
 * test_plane.c
 *
 *  Created on: Oct 23, 2013
 *      Author: sizonov
 */

#include "test_plane.h"
//#include "exynos_util.h"
#include "mem.h"

#define PRINTVAL( val ) printf(" %d \n", val);

// this file contains functions to test
#include "./display/exynos_plane.c" //watch to Makefile in root directory (../../)


//******************************************* Fake functions ************************************************
//********************************************** Fixtures ***************************************************
//********************************************* Unit Tests **************************************************

//===========================================================================================================
/* static Bool _ComparePlaneId (pointer structure, pointer element) */
START_TEST( ComparePlaneId_different )
{
	EXYNOSPlanePtr using_plane = ctrl_calloc( 1, sizeof( EXYNOSPlane ) );
	unsigned int element = 5;
	fail_if( _ComparePlaneId( using_plane, &element ), "Values are different, False must returned"  );
}
END_TEST;

START_TEST( ComparePlaneId_same )
{
	EXYNOSPlanePtr using_plane = ctrl_calloc( 1, sizeof( EXYNOSPlane ) );
	using_plane->plane_id = 5;
	unsigned int element = 5;
	fail_if( !_ComparePlaneId( using_plane, &element ), "Values are same, TRUE must returned"  );
}
END_TEST;
//===========================================================================================================

//===========================================================================================================
/* static Bool _ComparePlaneZpos (pointer structure, pointer element) */
START_TEST( ComparePlaneZpos_different )
{
	EXYNOSPlanePtr using_plane = ctrl_calloc( 1, sizeof( EXYNOSPlane ) );
	unsigned int element = 5;
	fail_if( _ComparePlaneZpos( using_plane, &element ), "Values are different, False must returned"  );
}
END_TEST;

START_TEST( ComparePlaneZpos_same )
{
	EXYNOSPlanePtr using_plane = ctrl_calloc( 1, sizeof( EXYNOSPlane ) );
	using_plane->zpos = 3;
	unsigned int element = 3;
	fail_if( !_ComparePlaneZpos( using_plane, &element ), "Values are same, TRUE must returned"  );
}
END_TEST;
//===========================================================================================================

//===========================================================================================================
/* void exynosPlaneInit (ScrnInfoPtr pScrn, EXYNOSDispInfoPtr pDispInfo, int num) */
/* ScrnInfoPtr pScrn not using in this function */
START_TEST( exynosPlaneInit_pPlanePrivMode_PlaneIsNull )
{
	EXYNOSDispInfoPtr pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );
	pDispInfo->plane_res = ctrl_calloc( 1, sizeof( drmModePlaneRes ) );
	pDispInfo->drm_fd = 101;
	int i = 0;
	drmModePlaneResPtr P = pDispInfo->plane_res;
	P->planes = ctrl_calloc( 4, sizeof( uint32_t ) );
	int num = 1;

	int mem_ck = ctrl_get_cnt_calloc();
	exynosPlaneInit( NULL, pDispInfo, num );
	fail_if( ctrl_get_cnt_calloc() != mem_ck, "Memory must free" );
}
END_TEST;

START_TEST( exynosPlaneInit_correctInit )
{
	EXYNOSDispInfoPtr pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );
	pDispInfo->plane_res = ctrl_calloc( 1, sizeof( drmModePlaneRes ) );
	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );

    xorg_list_init ( &pDispInfo->planes );

	int i = 0;
	drmModePlaneResPtr P = pDispInfo->plane_res;
	P->planes = ctrl_calloc( 4, sizeof( uint32_t ) );
	int num = 1;

	int mem_ck = ctrl_get_cnt_calloc();
	exynosPlaneInit( pScrn, pDispInfo, num );
	fail_if( ctrl_get_cnt_calloc() != mem_ck + 1, "Memory must allocate" );
}
END_TEST;
//===========================================================================================================

//===========================================================================================================
/* void exynosPlaneDeinit (EXYNOSPlanePrivPtr pPlanePriv) */
START_TEST( exynosPlaneDeinit_freeMemory )
{
	EXYNOSPlanePrivPtr pPlanePriv = ctrl_calloc( 1, sizeof( EXYNOSPlanePrivRec ) );
	pPlanePriv->mode_plane = drmModeGetPlane( 0, 9 );
	xorg_list_init ( &pPlanePriv->link );

	int mem_ck = ctrl_get_cnt_calloc();
	int mem_drm_ck = ctrl_get_cnt_mem_obj( DRM_OBJ );

	exynosPlaneDeinit( pPlanePriv );

	fail_if( ctrl_get_cnt_calloc() != mem_ck - 1, "Memory must free" );
	fail_if( ctrl_get_cnt_mem_obj( DRM_OBJ ) != mem_drm_ck - 1, "DRM memory must free" );
}
END_TEST;
//===========================================================================================================

//===========================================================================================================
/* unsigned int exynosPlaneGetAvailable (ScrnInfoPtr pScrn) */
START_TEST( exynosPlaneGetAvailable_pScrnIsNull )
{
	//Segmentation fault must not occur
	fail_if( exynosPlaneGetAvailable( NULL ) != 0, "pScrn is NULL, 0 must returned");
}
END_TEST;

START_TEST( exynosPlaneGetAvailable_pExynosIsNull )
{
	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->driverPrivate = NULL;

	fail_if( exynosPlaneGetAvailable( pScrn ) != 0, "pExynos is NULL, 0 must returned");
}
END_TEST;

START_TEST( exynosPlaneGetAvailable_pDispInfoIsNull )
{
	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );

	fail_if( exynosPlaneGetAvailable( pScrn ) != 0, "pDispInfo is NULL, 0 must returned");
}
END_TEST;

START_TEST( exynosPlaneGetAvailable_IfCount_PlanesIsZero )
{
	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	EXYNOSPtr PdP = pScrn->driverPrivate;
	PdP->pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );
	PdP->pDispInfo->plane_res = ctrl_calloc( 1, sizeof( drmModePlaneRes ) );
	PdP->pDispInfo->plane_res->count_planes = 0;
	PdP->pDispInfo->plane_res->planes = ctrl_calloc( 1, sizeof( uint32_t ) );

	fail_if( exynosPlaneGetAvailable( pScrn ) != 0, "Count_Plane is 0, 0 must returned");
}
END_TEST;

START_TEST( exynosPlaneGetAvailable_correctInit )
{
	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	EXYNOSPtr PdP = pScrn->driverPrivate;
	PdP->pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );
	PdP->pDispInfo->plane_res = ctrl_calloc( 1, sizeof( drmModePlaneRes ) );
	PdP->pDispInfo->plane_res->count_planes = 1;
	PdP->pDispInfo->plane_res->planes = ctrl_calloc( 1, sizeof( uint32_t ) );

	fail_if( exynosPlaneGetAvailable( pScrn ) != 0, "pDispInfo is NULL, 0 must returned");
}
END_TEST;
//===========================================================================================================

//===========================================================================================================
/* static unsigned int _FindAvailableZpos(EXYNOSDispInfoPtr pDispInfo) */
START_TEST( FindAvailableZpos_pDispInfoIsNull )
{
	fail_if( _FindAvailableZpos( NULL ) != 0, "pDispInfo is NULL, 0 must returned" );
}
END_TEST;

START_TEST( FindAvailableZpos_corretcInit )
{
	EXYNOSDispInfoPtr pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );
	fail_if( !_FindAvailableZpos( pDispInfo ), "correct initialization, 1 must returned" );
}
END_TEST;
//===========================================================================================================

//===========================================================================================================
/* Bool exynosPlaneDraw (ScrnInfoPtr pScrn, xRectangle fixed_box, xRectangle crtc_box,
                 unsigned int fb_id, unsigned int plane_id, unsigned int crtc_id, unsigned int zpos) */
START_TEST( exynosPlaneDraw_pSrnNull )
{
	xRectangle fixed_box = { 0, };
	xRectangle crtc_box = { 0, };

	fail_if( exynosPlaneDraw( NULL, fixed_box, crtc_box, 0, 0, 0, 0) != FALSE, "pScrn is Null, FALSE must returned" );
}
END_TEST;

START_TEST( exynosPlaneDraw_plane_idIsZero )
{
	xRectangle fixed_box = { 0, };
	xRectangle crtc_box = { 0, };

	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );


	fail_if( exynosPlaneDraw( pScrn, fixed_box, crtc_box, 0, 0, 0, 0) != FALSE, "plane_id is 0, FALSE must returned" );
}
END_TEST;

START_TEST( exynosPlaneDraw_fb_idIsZero )
{
	xRectangle fixed_box = { 0, };
	xRectangle crtc_box = { 0, };
	unsigned int plane_id = 1;


	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );

	fail_if( exynosPlaneDraw( pScrn, fixed_box, crtc_box, 0, plane_id, 0, 0) != FALSE, "fb_id is 0, FALSE must returned" );
}
END_TEST;

START_TEST( exynosPlaneDraw_crtc_idIsZero )
{
	xRectangle fixed_box = { 0, };
	xRectangle crtc_box = { 0, };
	unsigned int fb_id = 1;
	unsigned int plane_id = 1;

	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );

	fail_if( exynosPlaneDraw( pScrn, fixed_box, crtc_box, fb_id, plane_id, 0, 0) != FALSE, "crtc_id is 0, FALSE must returned" );
}
END_TEST;

START_TEST( exynosPlaneDraw_zposLessthenLAYER_MAX )
{
	xRectangle fixed_box = { 0, };
	xRectangle crtc_box = { 0, };
	unsigned int fb_id = 1;
	unsigned int plane_id = 1;
	unsigned int crtc_id = 1;

	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );

	fail_if( exynosPlaneDraw( pScrn, fixed_box, crtc_box, fb_id, plane_id, crtc_id, -1) != FALSE, "zpos < LAYER_MAX, FALSE must returned" );

}
END_TEST;

START_TEST( exynosPlaneDraw_pExynosIsNull )
{
	xRectangle fixed_box = { 0, };
	xRectangle crtc_box = { 0, };
	unsigned int fb_id = 1;
	unsigned int plane_id = 1;
	unsigned int crtc_id = 1;
	unsigned int zpos = 1;

	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );

	fail_if( exynosPlaneDraw( pScrn, fixed_box, crtc_box, fb_id, plane_id, crtc_id, zpos) != FALSE, "pExynos is Null, FALSE must returned" );
}
END_TEST;

START_TEST( exynosPlaneDraw_pDispInfoIsNull )
{
	xRectangle fixed_box = { 0, };
	xRectangle crtc_box = { 0, };
	unsigned int fb_id = 1;
	unsigned int plane_id = 1;
	unsigned int crtc_id = 1;
	unsigned int zpos = 1;

	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );


	fail_if( exynosPlaneDraw( pScrn, fixed_box, crtc_box, fb_id, plane_id, crtc_id, zpos) != FALSE, "pDispInfo is Null, FALSE must returned" );
}
END_TEST;

START_TEST( exynosPlaneDraw_ifexynosUtilSetDrmPropertyFailAndUsing_PlaneNull )
{
	xRectangle fixed_box = { 0, };
	xRectangle crtc_box = { 0, };
	unsigned int fb_id = 1;
	unsigned int plane_id = 1;
	unsigned int crtc_id = 1;
	unsigned int zpos = LAYER_NONE;

	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	EXYNOSPtr PdP = pScrn->driverPrivate;
	PdP->drm_fd = 101;
	PdP->pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );

	fail_if( exynosPlaneDraw( pScrn, fixed_box, crtc_box, fb_id, plane_id, crtc_id, zpos) != FALSE, "FALSE must returned" );
}
END_TEST;

START_TEST( exynosPlaneDraw_ifexynosUtilSetDrmPropertyFailAndUsing_PlaneNotNull )
{
	xRectangle fixed_box = { 0, };
	xRectangle crtc_box = { 0, };
	unsigned int fb_id = 1;
	unsigned int plane_id = 1;
	unsigned int crtc_id = 1;
	unsigned int zpos = LAYER_NONE;

	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	EXYNOSPtr PdP = pScrn->driverPrivate;
	PdP->pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );
	EXYNOSDispInfoPtr ppD = PdP->pDispInfo;
	ppD->using_planes = exynosUtilStorageInit();
	PdP->drm_fd = 101;

	EXYNOSPlanePtr data = ctrl_calloc( 1, sizeof( EXYNOSPlane ) );
	data->plane_id = 1;

	int ddata1 = ctrl_calloc( 1, sizeof( int ) );
	if( !exynosUtilStorageAdd ( ppD->using_planes , data ) )
		return;

	fail_if( exynosPlaneDraw( pScrn, fixed_box, crtc_box, fb_id, plane_id, crtc_id, zpos) != FALSE, "FALSE must returned" );
}
END_TEST;

START_TEST( exynosPlaneDraw_failOnexynosPlaneDraw )
{
	xRectangle fixed_box = { 0, };
	fixed_box.x = 101;//watch drm.c
	xRectangle crtc_box = { 0, };
	unsigned int fb_id = 1;
	unsigned int plane_id = 1;
	unsigned int crtc_id = 1;
	unsigned int zpos = LAYER_NONE;

	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	EXYNOSPtr PdP = pScrn->driverPrivate;
	PdP->pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );
	EXYNOSDispInfoPtr ppD = PdP->pDispInfo;
	ppD->using_planes = exynosUtilStorageInit();

	EXYNOSPlanePtr data = ctrl_calloc( 1, sizeof( EXYNOSPlane ) );
	data->plane_id = 1;

	int ddata1 = ctrl_calloc( 1, sizeof( int ) );
	if( !exynosUtilStorageAdd ( ppD->using_planes , data ) )
		return;

	extern int cnt_drmModeSetPlane;

	fail_if( exynosPlaneDraw( pScrn, fixed_box, crtc_box, fb_id, plane_id, crtc_id, zpos) != FALSE, "FALSE must returned" );
	fail_if( !cnt_drmModeSetPlane , "must be 1");
}
END_TEST;


extern Fixed_test fixed_t;
extern Crtc_test crtc_t;

START_TEST( exynosPlaneDraw_correctInit )
{
	xRectangle fixed_box = { 1, 2, 3, 4, };
	xRectangle crtc_box = { 5, 6, 7 , 8, };
	unsigned int fb_id = 1;
	unsigned int plane_id = 1;
	unsigned int crtc_id = 1;
	unsigned int zpos = LAYER_NONE;

	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	EXYNOSPtr PdP = pScrn->driverPrivate;
	PdP->pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );
	EXYNOSDispInfoPtr ppD = PdP->pDispInfo;
	ppD->using_planes = exynosUtilStorageInit();

	EXYNOSPlanePtr data = ctrl_calloc( 1, sizeof( EXYNOSPlane ) );
	data->plane_id = 1;

	int ddata1 = ctrl_calloc( 1, sizeof( int ) );
	if( !exynosUtilStorageAdd ( ppD->using_planes , data ) )
		return;

	extern int cnt_drmModeSetPlane;

	fail_if( !exynosPlaneDraw( pScrn, fixed_box, crtc_box, fb_id, plane_id, crtc_id, zpos), "TRUE must returned" );

	fail_if( !cnt_drmModeSetPlane , "must be 1");

	fail_if( fixed_t.x != ((unsigned int) fixed_box.x) << 16, "must be equal 1" );
	fail_if( fixed_t.y != ((unsigned int) fixed_box.y) << 16, "must be equal 2" );
	fail_if( fixed_t.w != ((unsigned int) fixed_box.width) << 16, "must be equal 3" );
	fail_if( fixed_t.h != ((unsigned int) fixed_box.height) << 16, "must be equal 4" );

	fail_if( crtc_t.x != crtc_box.x & ~1 , "must be equal 5" );
	fail_if( crtc_t.y != crtc_box.y , "must be equal 6" );
	fail_if( crtc_t.w != crtc_box.width, "must be equal 7" );
	fail_if( crtc_t.h != crtc_box.height, "must be equal 8" );
}
END_TEST;
//===========================================================================================================

//===========================================================================================================
/* unsigned int exynosPlaneGetCrtcId (ScrnInfoPtr pScrn, unsigned int plane_id) */
START_TEST( exynosPlaneGetCrtcId_pScrnIsNull )
{
	unsigned int plane_id = 0;

	fail_if( exynosPlaneGetCrtcId( NULL, plane_id ), "pScrn is Null, 0 must returned" );
}
END_TEST;

START_TEST( exynosPlaneGetCrtcId_pExynosIsNull )
{
	unsigned int plane_id = 0;
	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );

	fail_if( exynosPlaneGetCrtcId( pScrn, plane_id ), "pExynos is Null, 0 must returned" );
}
END_TEST;

START_TEST( exynosPlaneGetCrtcId_pDispInfoIsNull )
{
	unsigned int plane_id = 0;
	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );

	fail_if( exynosPlaneGetCrtcId( pScrn, plane_id ), "pScrn is Null, 0 must returned" );
}
END_TEST;

START_TEST( exynosPlaneGetCrtcId_mode_planeIsNull )
{
	unsigned int plane_id = 0;
	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	EXYNOSPtr pdP =	pScrn->driverPrivate;
	pdP->pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );
	EXYNOSDispInfoPtr ppDI = pdP->pDispInfo;
	ppDI->plane_res = ctrl_calloc( 1, sizeof( drmModePlaneRes ) );
	ppDI->plane_res->planes = ctrl_calloc( 4, sizeof( uint32_t ) );
	ppDI->plane_res->count_planes = 2;

	fail_if( exynosPlaneGetCrtcId( pScrn, plane_id ), "mode_plane is Null, 0 must returned" );
}
END_TEST;

START_TEST( exynosPlaneGetCrtcId_corretcInit )
{
	unsigned int plane_id = 0;
	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	EXYNOSPtr pdP =	pScrn->driverPrivate;
	pdP->pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );
	pdP->pDispInfo->drm_fd = 121;
	EXYNOSDispInfoPtr ppDI = pdP->pDispInfo;
	ppDI->plane_res = ctrl_calloc( 1, sizeof( drmModePlaneRes ) );
	ppDI->plane_res->planes = ctrl_calloc( 4, sizeof( uint32_t ) );
	ppDI->plane_res->count_planes = 2;

	int mem_ck = ctrl_get_cnt_calloc();

	fail_if( !exynosPlaneGetCrtcId( pScrn, plane_id ), "correct initialization, 1 must returned" );
	fail_if( ctrl_get_cnt_calloc() != mem_ck, "Memory must free" );
}
END_TEST;

//===========================================================================================================
/* Bool exynosPlaneHide (ScrnInfoPtr pScrn, unsigned int plane_id) */
START_TEST( exynosPlaneHide_pScrnIsNull )
{
	unsigned int plane_id = 0;

	fail_if( exynosPlaneHide( NULL, plane_id ), "pScrn is Null, 0 must returned" );
}
END_TEST;

START_TEST( exynosPlaneHide_pExynosIsNull )
{
	unsigned int plane_id = 0;
	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );

	fail_if( exynosPlaneHide( pScrn, plane_id ), "pExynos is Null, 0 must returned" );
}
END_TEST;

START_TEST( exynosPlaneHide_pDispInfoIsNull )
{
	unsigned int plane_id = 0;
	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );

	fail_if( exynosPlaneHide( pScrn, plane_id ), "pScrn is Null, 0 must returned" );
}
END_TEST;

START_TEST( exynosPlaneHide_crtc_idIsZero )
{
	unsigned int plane_id = 0;
	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	EXYNOSPtr pdP =	pScrn->driverPrivate;
	pdP->pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );
	EXYNOSDispInfoPtr ppDI = pdP->pDispInfo;
	ppDI->plane_res = ctrl_calloc( 1, sizeof( drmModePlaneRes ) );
	ppDI->plane_res->planes = ctrl_calloc( 4, sizeof( uint32_t ) );
	ppDI->plane_res->count_planes = 2;

	fail_if( !exynosPlaneHide( pScrn, plane_id ), "crtc_id is 0, TRUE must returned" );
}
END_TEST;

START_TEST( exynosPlaneHide_failOndrmModeSetPlane )
{
	unsigned int plane_id = 0;
	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	EXYNOSPtr pdP =	pScrn->driverPrivate;
	pdP->drm_fd = 141;
	pdP->pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );
	pdP->pDispInfo->drm_fd = 121;
	EXYNOSDispInfoPtr ppDI = pdP->pDispInfo;
	ppDI->plane_res = ctrl_calloc( 1, sizeof( drmModePlaneRes ) );
	ppDI->plane_res->planes = ctrl_calloc( 4, sizeof( uint32_t ) );
	ppDI->plane_res->count_planes = 2;

	extern int cnt_drmModeSetPlane;

	fail_if( exynosPlaneHide( pScrn, plane_id ), "FALSE must returned" );
	fail_if( cnt_drmModeSetPlane != 1, "must be 1" );
}
END_TEST;

START_TEST( exynosPlaneHide_correctInit )
{
	unsigned int plane_id = 0;
	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	EXYNOSPtr pdP =	pScrn->driverPrivate;
	pdP->pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );
	pdP->pDispInfo->drm_fd = 121;
	EXYNOSDispInfoPtr ppDI = pdP->pDispInfo;
	ppDI->plane_res = ctrl_calloc( 1, sizeof( drmModePlaneRes ) );
	ppDI->plane_res->planes = ctrl_calloc( 4, sizeof( uint32_t ) );
	ppDI->plane_res->count_planes = 2;

	extern int cnt_drmModeSetPlane;

	fail_if( !exynosPlaneHide( pScrn, plane_id ), ", TRUE must returned" );
	fail_if( cnt_drmModeSetPlane != 1, "must be 1" );
}
END_TEST;

//===========================================================================================================


//***********************************************************************************************************
int plane_suite( run_stat_data* stat  )
{
	Suite* s = suite_create( "plane.c" );

	// create test case 'tc_plane_init'
	TCase* tc_ComparePlaneId          = tcase_create( "_ComparePlaneId" );
	TCase* tc_ComparePlaneZpos        = tcase_create( "ComparePlaneZpos" );
	TCase* tc_exynosPlaneInit         = tcase_create( "exynosPlaneInit" );
	TCase* tc_exynosPlaneDeinit       = tcase_create( "exynosPlaneDeinit" );
	TCase* tc_exynosPlaneGetAvailable = tcase_create( "exynosPlaneGetAvailable" );
	TCase* tc_FindAvailableZpos       = tcase_create( "_FindAvailableZpos" );
	TCase* tc_exynosPlaneDraw         = tcase_create( "exynosPlaneDraw" );
	TCase* tc_exynosPlaneGetCrtcId    = tcase_create( "exynosPlaneGetCrtcId" );
	TCase* tc_exynosPlaneHide         = tcase_create( "exynosPlaneHide" );


	//_ComparePlaneId
	tcase_add_test( tc_ComparePlaneId, ComparePlaneId_different );
	tcase_add_test( tc_ComparePlaneId, ComparePlaneId_same);

	//ComparePlaneZpos
	tcase_add_test( tc_ComparePlaneZpos, ComparePlaneZpos_different );
	tcase_add_test( tc_ComparePlaneZpos, ComparePlaneZpos_same );

	//exynosPlaneInit
	//tcase_add_test( tc_exynosPlaneInit, exynosPlaneInit_pPlanePrivMode_PlaneIsNull );
	tcase_add_test( tc_exynosPlaneInit, exynosPlaneInit_correctInit );

	//exynosPlaneDeinit
	tcase_add_test( tc_exynosPlaneDeinit, exynosPlaneDeinit_freeMemory );

	//exynosPlaneGetAvailable
	tcase_add_test( tc_exynosPlaneGetAvailable, exynosPlaneGetAvailable_pScrnIsNull );
	tcase_add_test( tc_exynosPlaneGetAvailable, exynosPlaneGetAvailable_pExynosIsNull );
	tcase_add_test( tc_exynosPlaneGetAvailable, exynosPlaneGetAvailable_pDispInfoIsNull );
	tcase_add_test( tc_exynosPlaneGetAvailable, exynosPlaneGetAvailable_IfCount_PlanesIsZero );
	tcase_add_test( tc_exynosPlaneGetAvailable, exynosPlaneGetAvailable_correctInit );

	//_FindAvailableZpos
	tcase_add_test( tc_FindAvailableZpos, FindAvailableZpos_pDispInfoIsNull );
	tcase_add_test( tc_FindAvailableZpos, FindAvailableZpos_corretcInit );

	//exynosPlaneDraw
	tcase_add_test( tc_exynosPlaneDraw, exynosPlaneDraw_pSrnNull );
	tcase_add_test( tc_exynosPlaneDraw, exynosPlaneDraw_plane_idIsZero );
	tcase_add_test( tc_exynosPlaneDraw, exynosPlaneDraw_fb_idIsZero );
	tcase_add_test( tc_exynosPlaneDraw, exynosPlaneDraw_crtc_idIsZero );
	tcase_add_test( tc_exynosPlaneDraw, exynosPlaneDraw_zposLessthenLAYER_MAX );
	tcase_add_test( tc_exynosPlaneDraw, exynosPlaneDraw_pExynosIsNull );
	tcase_add_test( tc_exynosPlaneDraw, exynosPlaneDraw_pDispInfoIsNull );
	tcase_add_test( tc_exynosPlaneDraw, exynosPlaneDraw_ifexynosUtilSetDrmPropertyFailAndUsing_PlaneNull );
	tcase_add_test( tc_exynosPlaneDraw, exynosPlaneDraw_ifexynosUtilSetDrmPropertyFailAndUsing_PlaneNotNull );
	tcase_add_test( tc_exynosPlaneDraw, exynosPlaneDraw_failOnexynosPlaneDraw );
	tcase_add_test( tc_exynosPlaneDraw, exynosPlaneDraw_correctInit );

	//exynosPlaneGetCrtcId
	tcase_add_test( tc_exynosPlaneGetCrtcId, exynosPlaneGetCrtcId_pScrnIsNull );
	tcase_add_test( tc_exynosPlaneGetCrtcId, exynosPlaneGetCrtcId_pExynosIsNull );
	tcase_add_test( tc_exynosPlaneGetCrtcId, exynosPlaneGetCrtcId_pDispInfoIsNull );
	tcase_add_test( tc_exynosPlaneGetCrtcId, exynosPlaneGetCrtcId_mode_planeIsNull );
	tcase_add_test( tc_exynosPlaneGetCrtcId, exynosPlaneGetCrtcId_corretcInit );

	//exynosPlaneHide
	tcase_add_test( tc_exynosPlaneHide, exynosPlaneHide_pScrnIsNull );
	tcase_add_test( tc_exynosPlaneHide, exynosPlaneHide_pExynosIsNull );
	tcase_add_test( tc_exynosPlaneHide, exynosPlaneHide_pDispInfoIsNull );
	tcase_add_test( tc_exynosPlaneHide, exynosPlaneHide_crtc_idIsZero );
	tcase_add_test( tc_exynosPlaneHide, exynosPlaneHide_failOndrmModeSetPlane );
	tcase_add_test( tc_exynosPlaneHide, exynosPlaneHide_correctInit );


	suite_add_tcase( s, tc_ComparePlaneId );
	suite_add_tcase( s, tc_ComparePlaneZpos );
	suite_add_tcase( s, tc_exynosPlaneInit );
	suite_add_tcase( s, tc_exynosPlaneDeinit );
	suite_add_tcase( s, tc_FindAvailableZpos );
	suite_add_tcase( s, tc_exynosPlaneDraw );
	suite_add_tcase( s, tc_exynosPlaneGetCrtcId );
	suite_add_tcase( s, tc_exynosPlaneHide );


	SRunner *sr = srunner_create( s );
	srunner_run_all( sr, CK_VERBOSE );

	stat->num_tests = srunner_ntests_run( sr );
	stat->num_pass = srunner_ntests_run( sr ) - srunner_ntests_failed( sr );
	stat->num_failure = srunner_ntests_failed( sr );

	return 0;
}
