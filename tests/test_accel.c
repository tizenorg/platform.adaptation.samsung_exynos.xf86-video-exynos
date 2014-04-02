/*
 * test_accel.c
 *
 *  Created on: Oct 21, 2013
 *      Author: sizonov
 */

#include "test_accel.h"


// this file contains functions to test
#include "./accel/exynos_accel.c"

#define PRINTVAL( v ) printf( "\n %d \n", v );

//****************************************** Redefined funcions *****************************************************
//*********************************************** Fixtures **********************************************************
void setupInitxf86Screens( void )
{
	xf86Screens = (ScrnInfoPtr*)ctrl_calloc( 4, sizeof( ScrnInfoRec ) );
	int i = 0;
	for( ; i < 4; i++ )
	{
		xf86Screens[i] = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
		xf86Screens[i]->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	}
}

void teardownDestroyxf86Screens( void )
{
	int i = 0;
	for( ; i < 4; i++ )
		xf86Screens[i]->driverPrivate = NULL;
		xf86Screens[i] = NULL;
	xf86Screens = NULL;
}
//*********************************************** Unit tests **********************************************************
//=====================================================================================================================
/* Bool exynosAccelInit (ScreenPtr pScreen) */
START_TEST( exynosAccelInit_correctInit )
{
	ScreenPtr pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
	pScreen->myNum = 1;

	EXYNOSPtr pdP = xf86Screens[1]->driverPrivate;
	pdP->is_exa = TRUE;

	Bool res;
	res = exynosAccelInit( pScreen );

	fail_if( res != TRUE, "TRUE must returned" );
}
END_TEST;

START_TEST( exynosAccelInit_correctInit_2 )
{
	ScreenPtr pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
	pScreen->myNum = 1;

	EXYNOSPtr pdP = xf86Screens[1]->driverPrivate;
	pdP->is_exa = TRUE;
	pdP->is_dri2 = TRUE;

	Bool res;
	res = exynosAccelInit( pScreen );

	fail_if( res != TRUE, "TRUE must returned" );
}
END_TEST;
//=====================================================================================================================

//=====================================================================================================================
/* void exynosAccelDeinit (ScreenPtr pScreen) */
START_TEST( exynosAccelDeinit_ )
{
	ScreenPtr pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
	pScreen->myNum = 1;

	extern int ck_DRI2CloseScreen;

	exynosAccelDeinit( pScreen );

	fail_if( ck_DRI2CloseScreen != 1, "fake variable must be 1" );
}
END_TEST;
//=====================================================================================================================

//*********************************************************************************************************************
int accel_suite( run_stat_data* stat )
{
	Suite* s = suite_create( "ACCEL.C" );

	TCase* tc_exynosAccelInit   = tcase_create( "exynosAccelInit" );
	TCase* tc_exynosAccelDeinit = tcase_create( "exynosAccelInit" );


	//exynosAccelInit
	tcase_add_checked_fixture( tc_exynosAccelInit, setupInitxf86Screens, teardownDestroyxf86Screens );
	tcase_add_test( tc_exynosAccelInit, exynosAccelInit_correctInit );
	tcase_add_test( tc_exynosAccelInit, exynosAccelInit_correctInit_2 );

	//exynosAccelDeinit
	tcase_add_checked_fixture( tc_exynosAccelDeinit, setupInitxf86Screens, teardownDestroyxf86Screens );
	tcase_add_test( tc_exynosAccelDeinit, exynosAccelDeinit_ );


	suite_add_tcase( s, tc_exynosAccelInit );
	suite_add_tcase( s, tc_exynosAccelDeinit );


	SRunner *sr = srunner_create( s );
	srunner_run_all( sr, CK_VERBOSE );

	stat->num_tests = srunner_ntests_run( sr );
	stat->num_pass = srunner_ntests_run( sr ) - srunner_ntests_failed( sr );
	stat->num_failure = srunner_ntests_failed( sr );

	stat->num_tested = 2;
	stat->num_nottested = 0;

	return 0;
}
