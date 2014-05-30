/*
 * test_exa.c
 *
 *  Created on: Oct 23, 2013
 *      Author: sizonov
 */

#include "test_exa.h"
#include "mem.h"

#define PRINTVAL( v ) printf( "\n %d \n", v );

// this file contains functions to test
#include "./accel/exynos_exa.c"

//******************************************** Unit tests ***************************************************
//===========================================================================================================
/* static char *_getStrFromHint (int usage_hint) */
START_TEST( getStrFromHint_unsupportedUsage_Hint )
{
	int usage_hint = 21;//unsupported hint

	fail_if( _getStrFromHint( usage_hint ) != NULL, "unsupported hint, NULL must returned" );
}
END_TEST;

START_TEST( getStrFromHint_correctInit )
{
	int usage_hint = CREATE_PIXMAP_USAGE_NORMAL;//unsupported hint

	fail_if( _getStrFromHint( usage_hint ) != "NORMAL", "correct initialization, string must returned" );
}
END_TEST;

START_TEST( getStrFromHint_correctInitMinusOne )
{
	int usage_hint = -1;//unsupported hint

	fail_if( _getStrFromHint( usage_hint ) != NULL, "correct initialization by -1, NULL must returned" );
}
END_TEST;
//===========================================================================================================

//===========================================================================================================
/* Bool EXYNOSExaPrepareAccess (PixmapPtr pPix, int index) */
START_TEST( EXYNOSExaPrepareAccess_pPixNull )
{
	PixmapPtr pPix = ctrl_calloc( 1, sizeof( PixmapRec ) );
	int index = 1;

	pPix->refcnt = 101;//special initialization where with it help pExaPixPriv will be NULL

	//Segmentation fault must not occur
	fail_if( EXYNOSExaPrepareAccess ( pPix, index ) != FALSE, "FALSE must returned");
}
END_TEST;

START_TEST( EXYNOSExaPrepareAccess_ifpExaPixPrivTboAllocate )
{
	PixmapPtr pPix = ctrl_calloc( 1, sizeof( PixmapRec ) );
	int index = 1;

	pPix->refcnt = 102;

	extern EXYNOSExaPixPrivPtr pExaPixPrivForTest;

	fail_if( EXYNOSExaPrepareAccess ( pPix, index ) != TRUE, "FALSE must returned");
	fail_if( cnt_map_bo( pExaPixPrivForTest->tbo ) != 1, "FAIL" );//Check that tbo was mapped
	fail_if( !pPix->devPrivate.ptr, "Must not be NULL" );
}
END_TEST;

START_TEST( EXYNOSExaPrepareAccess_pExaPixPriv_pPixDataNotNull )
{
	PixmapPtr pPix = ctrl_calloc( 1, sizeof( PixmapRec ) );
	int index = 1;

	pPix->refcnt = 103;

	extern EXYNOSExaPixPrivPtr pExaPixPrivForTest;

	fail_if( EXYNOSExaPrepareAccess ( pPix, index ) != TRUE, "FALSE must returned");
	fail_if( !pPix->devPrivate.ptr, "Must not be NULL" );
}
END_TEST;
//===========================================================================================================

//===========================================================================================================
/* void EXYNOSExaFinishAccess (PixmapPtr pPix, int index) */
START_TEST( EXYNOSExaFinishAccess_pPixNull )
{
	int index = 1;

	//Segmentation fault must not occur
	EXYNOSExaFinishAccess ( NULL ,index );
}
END_TEST;

START_TEST( EXYNOSExaFinishAccess_ifPExaPixPrivNull )
{
	PixmapPtr pPix = ctrl_calloc( 1, sizeof( PixmapRec ) );
	int index = 1;

	pPix->refcnt = 101;//with it help pExaPixPriv will be NULL

	//Segmentation fault must not occur
	EXYNOSExaFinishAccess ( pPix ,index );
}
END_TEST;

START_TEST( EXYNOSExaFinishAccess_ifpExaPixPriv_tboAllocate )
{
	PixmapPtr pPix = ctrl_calloc( 1, sizeof( PixmapRec ) );
	int index = 1;
	extern EXYNOSExaPixPrivPtr pExaPixPrivForTest;

	pPix->refcnt = 102;

	EXYNOSExaFinishAccess ( pPix ,index );
	fail_if( cnt_map_bo( pExaPixPrivForTest->tbo ) != -1, "pExaPixPrivForTest->tbo must unmap" );
}
END_TEST;

START_TEST( EXYNOSExaFinishAccess_correctInit )
{
	PixmapPtr pPix = ctrl_calloc( 1, sizeof( PixmapRec ) );
	int index = 1;
	extern EXYNOSExaPixPrivPtr pExaPixPrivForTest;

	pPix->refcnt = 104;

	EXYNOSExaFinishAccess ( pPix ,index );
	fail_if( cnt_map_bo( pExaPixPrivForTest->tbo ) != -1, "pExaPixPrivForTest->tbo must unmap" );
	fail_if( pExaPixPrivForTest->pPixData, "pExaPixPrivForTest->pPixData must be NULL" );
}
END_TEST;
//===========================================================================================================

//===========================================================================================================
/* static void * EXYNOSExaCreatePixmap (ScreenPtr pScreen, int size, int align) */
START_TEST( EXYNOSExaCreatePixmap_checkReturnValue )
{
	ScreenPtr pScreen = NULL;
	int size = 1;
	int align = 1;
	fail_if( !EXYNOSExaCreatePixmap( pScreen, size, align ), "NULL must not be return");
	fail_if( ctrl_get_cnt_calloc() != 1, "Memory must allocate" );
}
END_TEST;
//===========================================================================================================

//===========================================================================================================
/* static void EXYNOSExaDestroyPixmap (ScreenPtr pScreen, void *driverPriv) */
START_TEST( EXYNOSExaDestroyPixmap_driverPrivNull )
{
	ScreenPtr pScreen = NULL;

	//Segmentation fault must not occur
	EXYNOSExaDestroyPixmap( pScreen, NULL );
}
END_TEST;

START_TEST( EXYNOSExaDestroyPixmap_errorByDefault )
{
	ScreenPtr pScreen = NULL;
	EXYNOSExaPixPrivPtr driverPriv = ctrl_calloc( 1, sizeof( EXYNOSExaPixPrivRec ) );
	driverPriv->usage_hint = 1234;

	EXYNOSExaDestroyPixmap( pScreen, driverPriv );
	fail_if( ctrl_get_cnt_calloc() != 0, "Memory must free" );
}
END_TEST;

START_TEST( EXYNOSExaDestroyPixmap_freeTBO )
{
	ScreenPtr pScreen = NULL;
	EXYNOSExaPixPrivPtr driverPriv = ctrl_calloc( 1, sizeof( EXYNOSExaPixPrivRec ) );
	driverPriv->usage_hint = CREATE_PIXMAP_USAGE_DRI2_BACK;
	driverPriv->tbo = tbm_bo_alloc( 0, 1, 0 );

	int pre_ck = ctrl_get_cnt_mem_obj( TBM_OBJ );

	EXYNOSExaDestroyPixmap( pScreen, driverPriv );
	fail_if( ctrl_get_cnt_calloc() != 0, "Memory must free" );
	fail_if( driverPriv->tbo != NULL, "Must be equal");
}
END_TEST;

//===========================================================================================================

//===========================================================================================================
/* static Bool EXYNOSExaModifyPixmapHeader (PixmapPtr pPixmap, int width, int height,
                             int depth, int bitsPerPixel, int devKind, pointer pPixData) */
START_TEST( EXYNOSExaModifyPixmapHeader_pPixmapNull )
{
	EXYNOSExaModifyPixmapHeader( NULL, 1, 1, 1, 1, 1, NULL );
}
END_TEST;

static PixmapPtr prepare_pixmap(){
    PixmapPtr pPixmap_t = ctrl_calloc( 1, sizeof( PixmapRec ) );
    pPixmap_t->drawable.pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
	pPixmap_t->drawable.pScreen->myNum = 1;
    return pPixmap_t;
}

static void prepare_screens() {
    int num_screens = 4;

    xf86Screens = (ScrnInfoPtr*) ctrl_calloc(num_screens, sizeof(ScrnInfoPtr));
    int i = 0;
    for (; i < num_screens; i++) {
        xf86Screens[i] = ctrl_calloc(1, sizeof(ScrnInfoRec));
        xf86Screens[i]->driverPrivate = ctrl_calloc(1, sizeof(EXYNOSRec));
    }

}


START_TEST( EXYNOSExaModifyPixmapHeader_ifpPixDataIsROOT_FB_ADDR )
{
    PixmapPtr pPixmap_t = prepare_pixmap();
	int width = 1;
	int height = 1;
	int depth = 1;
	int bitsPerPixel = 1;
	int devKind = 1;
	pointer pPixData = (void*)ROOT_FB_ADDR;

	Bool res;
	prepare_screens();
	//Segmentation fault must not occur
	res = EXYNOSExaModifyPixmapHeader( pPixmap_t, width, height, depth, bitsPerPixel, devKind, pPixData);

	fail_if( res != TRUE, "TRUE must return" );
	fail_if( pPixmap_t->usage_hint != CREATE_PIXMAP_USAGE_SCANOUT , "must be equal");
}
END_TEST;

START_TEST( EXYNOSExaModifyPixmapHeader_ifpPixDataNotNull )
{
    PixmapPtr pPixmap_t = prepare_pixmap();
	int width = 1;
	int height = 1;
	int depth = 1;
	int bitsPerPixel = 1;
	int devKind = 1;
	pointer pPixData = (void*)100;

	Bool res;

	pPixmap_t->refcnt = 102;

	prepare_screens();

	//Segmentation fault must not occur
	res = EXYNOSExaModifyPixmapHeader( pPixmap_t, width, height, depth, bitsPerPixel, devKind, pPixData);

	fail_if( res != TRUE, "TRUE must return" );

}
END_TEST;

START_TEST( EXYNOSExaModifyPixmapHeader_ifpPixDataNotNullNoHint )
{
	PixmapPtr pPixmap_t = ctrl_calloc( 1, sizeof( PixmapPtr ) );
	int width = 1;
	int height = 1;
	int depth = 1;
	int bitsPerPixel = 1;
	int devKind = 1;
	pointer pPixData = NULL;

	Bool res;

	pPixmap_t->refcnt = 102;
	pPixmap_t->drawable.pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
	pPixmap_t->drawable.pScreen->myNum = 1;

	xf86Screens = (ScrnInfoPtr*)ctrl_calloc( 1, sizeof( ScrnInfoPtr ) );
	int i = 0;
	for( ; i < 4; i++ )
	{
		xf86Screens[i] = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
		xf86Screens[i]->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	}

	//Segmentation fault must not occur
	res = EXYNOSExaModifyPixmapHeader( pPixmap_t, width, height, depth, bitsPerPixel, devKind, pPixData);

	fail_if( res != FALSE, "FALSE must return" );
}
END_TEST;

START_TEST( EXYNOSExaModifyPixmapHeader_inSwitchCREATE_PIXMAP_USAGE_GLYPH_PICTURE )
{
	PixmapRec pPixmap;
	int width = 1;
	int height = 1;
	int depth = 1;
	int bitsPerPixel = 1;
	int devKind = 1;
	pointer pPixData = (void *)0;

	Bool res;

	pPixmap.usage_hint = CREATE_PIXMAP_USAGE_GLYPH_PICTURE;
	pPixmap.devKind = 2;
	pPixmap.drawable.height = 2;

	pPixmap.drawable.pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
	pPixmap.drawable.pScreen->myNum = 1;

	xf86Screens = (ScrnInfoPtr*)ctrl_calloc( 1, sizeof( ScrnInfoPtr ) );
	int i = 0;
	for( ; i < 4; i++ )
	{
		xf86Screens[i] = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
		xf86Screens[i]->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	}

	//Segmentation fault must not occur
	res = EXYNOSExaModifyPixmapHeader( &pPixmap, width, height, depth, bitsPerPixel, devKind, pPixData);

	fail_if( res != TRUE, "TRUE must return" );
	fail_if( ctrl_get_cnt_mem_obj( TBM_OBJ ) != 1, "TBM_OBJ must allocate" );
}
END_TEST;

START_TEST( EXYNOSExaModifyPixmapHeader_inSwitchCREATE_PIXMAP_USAGE_SCANOUT_DRI2_BACK )
{
	PixmapRec pPixmap;
	int width = 1;
	int height = 1;
	int depth = 1;
	int bitsPerPixel = 1;
	int devKind = 1;
	pointer pPixData = (void *)0;

	Bool res;

	pPixmap.usage_hint = CREATE_PIXMAP_USAGE_SCANOUT_DRI2_BACK;
	pPixmap.devKind = 2;
	pPixmap.drawable.height = 2;

	pPixmap.drawable.pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
	pPixmap.drawable.pScreen->myNum = 1;

	xf86Screens = (ScrnInfoPtr*)ctrl_calloc( 1, sizeof( ScrnInfoPtr ) );
	int i = 0;
	for( ; i < 4; i++ )
	{
		xf86Screens[i] = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
		xf86Screens[i]->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	}

	//Segmentation fault must not occur
	res = EXYNOSExaModifyPixmapHeader( &pPixmap, width, height, depth, bitsPerPixel, devKind, pPixData);

	fail_if( res != TRUE, "TRUE must return" );
	fail_if( ctrl_get_cnt_mem_obj( TBM_OBJ ) != 1, "TBM_OBJ must allocate" );
}
END_TEST;
//===========================================================================================================

//===========================================================================================================
/* void exynosPixmapSetFrameCount (PixmapPtr pPix, unsigned int xid, CARD64 sbc) */
START_TEST( exynosPixmapSetFrameCount_equivalence )
{
	PixmapPtr pPix = ctrl_calloc( 1, sizeof( PixmapRec ) );

	CARD64 sbc = 1;
	unsigned int xid = 2;

	extern EXYNOSExaPixPrivPtr pExaPixPrivForTest;

	exynosPixmapSetFrameCount( pPix, xid, sbc );

	fail_if( pExaPixPrivForTest->owner != xid, "must be equal 1" );
	fail_if( pExaPixPrivForTest->sbc != sbc, "must be equal 2" );
}
END_TEST;
//===========================================================================================================

//===========================================================================================================
/* void exynosPixmapGetFrameCount (PixmapPtr pPix, unsigned int *xid, CARD64 *sbc) */
START_TEST( exynosPixmapGetFrameCount_equivalence )
{
	PixmapPtr pPix = ctrl_calloc( 1, sizeof( PixmapRec ) );
	pPix->refcnt = 105;

	CARD64 sbc = 0;
	unsigned int xid = 0;

	exynosPixmapGetFrameCount( pPix, &xid, &sbc );

	fail_if( xid != 1, "must be equal 1" );
	fail_if( sbc != 2, "must be equal 2" );
}
END_TEST;
//===========================================================================================================

//===========================================================================================================
/* tbm_bo exynosExaPixmapGetBo (PixmapPtr pPix) */
START_TEST( exynosExaPixmapGetBo_pPixNull )
{
	fail_if( exynosExaPixmapGetBo( NULL ), "NULL must returned" );
}
END_TEST;

START_TEST( exynosExaPixmapGetBo_returnValue )
{
	PixmapPtr pPix = ctrl_calloc( 1, sizeof( PixmapRec ) );
	pPix->refcnt = 102;

	tbm_bo res;

	res = exynosExaPixmapGetBo( pPix );

	fail_if( res == NULL, "NULL must not be returned" );
}
END_TEST;
//===========================================================================================================

//===========================================================================================================
/* void exynosExaPixmapSetBo (PixmapPtr pPix, tbm_bo tbo) */
START_TEST( exynosExaPixmapSetBo_pPixNull )
{
	tbm_bo arg;
	exynosExaPixmapSetBo( NULL, arg );
}
END_TEST;

START_TEST( exynosExaPixmapSetBo_tboNull )
{
	PixmapPtr pPix = ctrl_calloc( 1, sizeof( PixmapRec ) );
	pPix->refcnt = 103;

	tbm_bo arg;
	exynosExaPixmapSetBo( pPix, arg );
}
END_TEST;

START_TEST( exynosExaPixmapSetBo_correctInit )
{

}
END_TEST;

//===========================================================================================================
/* Bool exynosExaInit (ScreenPtr pScreen) */
START_TEST( exynosExaInit_ifexaDriverInitGivesFalse )
{
	ScreenPtr t_pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
	t_pScreen->myNum = 0;
	t_pScreen->numDepths = 1;

	xf86Screens = (ScrnInfoPtr*)ctrl_calloc( 4, sizeof( ScrnInfoPtr ) );
	int i = 0;
	for( ; i < 4; i++ )
	{
		xf86Screens[i] = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
		xf86Screens[i]->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
		EXYNOSPtr pE = xf86Screens[i]->driverPrivate;
		pE->is_sw_exa = TRUE;
		pE->pAccelInfo = ctrl_calloc( 1, sizeof( EXYNOSAccelInfoRec ) );
	}

	int mem_calloc_ck = ctrl_get_cnt_calloc();
	fail_if( exynosExaInit ( t_pScreen ), "FALSE must returned" );
	fail_if( mem_calloc_ck != mem_calloc_ck, "must be equal" );
}
END_TEST

START_TEST( exynosExaInit_correctInit )
{
	ScreenPtr t_pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
	t_pScreen->myNum = 0;

	xf86Screens = (ScrnInfoPtr*)ctrl_calloc( 4, sizeof( ScrnInfoPtr ) );
	int i = 0;
	for( ; i < 4; i++ )
	{
		xf86Screens[i] = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
		xf86Screens[i]->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
		EXYNOSPtr pE = xf86Screens[i]->driverPrivate;
		pE->is_sw_exa = TRUE;
		pE->pAccelInfo = ctrl_calloc( 1, sizeof( EXYNOSAccelInfoRec ) );
	}

	int mem_calloc_ck = ctrl_get_cnt_calloc();
	fail_if( !exynosExaInit ( t_pScreen ), "TRUE must returned" );
	fail_if( ctrl_get_cnt_calloc() != mem_calloc_ck + 1, "must be equal" );
}
END_TEST
//===========================================================================================================

//***********************************************************************************************************
int exa_suite( run_stat_data* stat  )
{
	Suite* s = suite_create( "EXA.C" );

	TCase* tc_getStrFromHint                = tcase_create( "_getStrFromHint" );
	TCase* tc_EXYNOSExaPrepareAccess        = tcase_create( "EXYNOSExaPrepareAccess" );
	TCase* tc_EXYNOSExaFinishAccess         = tcase_create( "EXYNOSExaFinishAccess" );
	TCase* tc_EXYNOSExaCreatePixmap         = tcase_create( "EXYNOSExaCreatePixmap" );
	TCase* tc_EXYNOSExaDestroyPixmap        = tcase_create( "EXYNOSExaDestroyPixmap" );
	TCase* tc_EXYNOSExaModifyPixmapHeader   = tcase_create( "EXYNOSExaModifyPixmapHeader" );
	TCase* tc_EXYNOSExaModifyPixmapHeader_2 = tcase_create( "EXYNOSExaModifyPixmapHeader" );
	TCase* tc_exynosPixmapSetFrameCount     = tcase_create( "exynosPixmapSetFrameCount" );
	TCase* tc_exynosPixmapGetFrameCount     = tcase_create( "exynosPixmapGetFrameCount" );
	TCase* tc_exynosExaPixmapGetBo          = tcase_create( "exynosExaPixmapGetBo" );
	TCase* tc_exynosExaPixmapSetBo          = tcase_create( "exynosExaPixmapSetBo" );
	TCase* tc_exynosExaInit				    = tcase_create( "exynosExaInit" );


	//_getStrFromHint
	tcase_add_test( tc_getStrFromHint, getStrFromHint_unsupportedUsage_Hint );
	tcase_add_test( tc_getStrFromHint, getStrFromHint_correctInit );
	tcase_add_test( tc_getStrFromHint, getStrFromHint_correctInitMinusOne );

	//EXYNOSExaPrepareAccess
	tcase_add_test( tc_EXYNOSExaPrepareAccess, EXYNOSExaPrepareAccess_pPixNull );
	tcase_add_test( tc_EXYNOSExaPrepareAccess, EXYNOSExaPrepareAccess_ifpExaPixPrivTboAllocate );
	tcase_add_test( tc_EXYNOSExaPrepareAccess, EXYNOSExaPrepareAccess_pExaPixPriv_pPixDataNotNull );

	//EXYNOSExaFinishAccess
	tcase_add_test( tc_EXYNOSExaFinishAccess, EXYNOSExaFinishAccess_pPixNull );
	tcase_add_test( tc_EXYNOSExaFinishAccess, EXYNOSExaFinishAccess_ifPExaPixPrivNull );
	tcase_add_test( tc_EXYNOSExaFinishAccess, EXYNOSExaFinishAccess_ifpExaPixPriv_tboAllocate );
	tcase_add_test( tc_EXYNOSExaFinishAccess, EXYNOSExaFinishAccess_correctInit );

	//EXYNOSExaCreatePixmap
	tcase_add_test( tc_EXYNOSExaCreatePixmap, EXYNOSExaCreatePixmap_checkReturnValue );

	//EXYNOSExaDestroyPixmap
	tcase_add_test( tc_EXYNOSExaDestroyPixmap, EXYNOSExaDestroyPixmap_driverPrivNull );
	tcase_add_test( tc_EXYNOSExaDestroyPixmap, EXYNOSExaDestroyPixmap_errorByDefault );
	//tcase_add_test( tc_EXYNOSExaDestroyPixmap, EXYNOSExaDestroyPixmap_freeTBO );

	//EXYNOSExaModifyPixmapHeader
	tcase_add_test( tc_EXYNOSExaModifyPixmapHeader, EXYNOSExaModifyPixmapHeader_pPixmapNull );
	tcase_add_test( tc_EXYNOSExaModifyPixmapHeader, EXYNOSExaModifyPixmapHeader_ifpPixDataIsROOT_FB_ADDR );
	tcase_add_test( tc_EXYNOSExaModifyPixmapHeader, EXYNOSExaModifyPixmapHeader_ifpPixDataNotNull );
/*
	tcase_add_test( tc_EXYNOSExaModifyPixmapHeader_2, EXYNOSExaModifyPixmapHeader_ifpPixDataNotNullNoHint );
	tcase_add_test( tc_EXYNOSExaModifyPixmapHeader_2, EXYNOSExaModifyPixmapHeader_inSwitchCREATE_PIXMAP_USAGE_GLYPH_PICTURE );
	tcase_add_test( tc_EXYNOSExaModifyPixmapHeader_2, EXYNOSExaModifyPixmapHeader_inSwitchCREATE_PIXMAP_USAGE_SCANOUT_DRI2_BACK );
*/
	//exynosPixmapSetFrameCount
	tcase_add_test( tc_exynosPixmapSetFrameCount, exynosPixmapSetFrameCount_equivalence );

	//exynosPixmapGetFrameCount
	tcase_add_test( tc_exynosPixmapGetFrameCount, exynosPixmapGetFrameCount_equivalence );

	//exynosExaPixmapGetBo
	tcase_add_test( tc_exynosExaPixmapGetBo, exynosExaPixmapGetBo_pPixNull );
	tcase_add_test( tc_exynosExaPixmapGetBo, exynosExaPixmapGetBo_returnValue );

	//exynosExaPixmapSetBo
	tcase_add_test( tc_exynosExaPixmapSetBo, exynosExaPixmapSetBo_pPixNull );
	tcase_add_test( tc_exynosExaPixmapSetBo, exynosExaPixmapSetBo_tboNull );
	tcase_add_test( tc_exynosExaPixmapSetBo, exynosExaPixmapSetBo_correctInit );

	//exynosExaInit
	tcase_add_test( tc_exynosExaInit, exynosExaInit_ifexaDriverInitGivesFalse );
	tcase_add_test( tc_exynosExaInit, exynosExaInit_correctInit );





	// add test case 'tc_get_str_from_hint' to 's' suite
	suite_add_tcase( s, tc_getStrFromHint );
	suite_add_tcase( s, tc_EXYNOSExaPrepareAccess );
	suite_add_tcase( s, tc_EXYNOSExaFinishAccess );
	suite_add_tcase( s, tc_EXYNOSExaCreatePixmap );
	suite_add_tcase( s, tc_EXYNOSExaDestroyPixmap);
	suite_add_tcase( s, tc_EXYNOSExaModifyPixmapHeader );
	suite_add_tcase( s, tc_EXYNOSExaModifyPixmapHeader_2 );
	suite_add_tcase( s, tc_exynosPixmapSetFrameCount );
	suite_add_tcase( s, tc_exynosPixmapGetFrameCount );
	suite_add_tcase( s, tc_exynosExaPixmapGetBo );
	suite_add_tcase( s, tc_exynosExaPixmapSetBo );
	suite_add_tcase( s, tc_exynosExaInit);


	SRunner *sr = srunner_create( s );
	srunner_run_all( sr, CK_VERBOSE );

	stat->num_tests = srunner_ntests_run( sr );
	stat->num_pass = srunner_ntests_run( sr ) - srunner_ntests_failed( sr );
	stat->num_failure = srunner_ntests_failed( sr );


	return 0;
}

