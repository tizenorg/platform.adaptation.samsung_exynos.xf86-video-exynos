/*
 * test_util_g2d.c
 *
 *  Created on: Oct 21, 2013
 *      Author: svoloshynov
 */
#include "test_util_g2d.h"

#define g2d_reset fake_g2d_reset
#define g2d_flush fake_g2d_flush

int tst_g2d_reset = 0;
void g2d_reset (unsigned int clear_reg)
{
	tst_g2d_reset = 1;
}

int tst_g2d_flush = 0;
int g2d_flush (void)
{
	tst_g2d_flush = 1;
	return 1;
}


#include "./g2d/util_g2d.c"

#define PRINTVAL( v ) printf( "\n %d \n", v );

//************************************************ Fixtures **************************************************************
//*********************************************** Unit tests *************************************************************
//========================================================================================================================
/* static int _util_get_clip(G2dImage* img, int x, int y, int width, int height, G2dPointVal *lt, G2dPointVal *rb) */
START_TEST ( util_get_clip_setModeG2D_SELECT_MODE_FGCOLOR )
{
	G2dImage img;
	int x;
	int y;
	int width;
	int height;
	G2dPointVal lt;
	G2dPointVal rb;

	int res;

	img.select_mode = G2D_SELECT_MODE_FGCOLOR;
	//function must rewrite these four variables
	lt.data.x = 1;
	lt.data.y = 1;
	rb.data.x = 2;
	rb.data.y = 2;

	res = _util_get_clip( &img, x, y, width, height, &lt, &rb);

	fail_if( res != 1, "1 must returned" );

	fail_if( lt.data.x != 0, "must be equal 1" );
	fail_if( lt.data.y != 0, "must be equal 2" );
	fail_if( rb.data.x != 1, "must be equal 3" );
	fail_if( rb.data.y != 1, "must be equal 4" );
}
END_TEST;

START_TEST ( util_get_clip_xAndyAreNegative )
{
	G2dImage img;
	int x = -1;
	int y = -2;
	int width = 5;
	int height = 5;
	G2dPointVal lt;
	G2dPointVal rb;

	int res;

	img.select_mode = G2D_SELECT_MODE_NORMAL;

	res = _util_get_clip( &img, x, y, width, height, &lt, &rb);

	fail_if( res != 1, "1 must returned" );

	fail_if( lt.data.x != 0, "must be equal 1" );
	fail_if( lt.data.y != 0, "must be equal 2" );
	fail_if( rb.data.x != 4, "must be equal 3" );
	fail_if( rb.data.y != 3, "must be equal 4" );
}
END_TEST;

START_TEST ( util_get_clip_XPlusWYplusHAreBiggerThenIwIh )
{
	G2dImage img;
	int x = 5;
	int y = 5;
	int width = 5;
	int height = 5;
	G2dPointVal lt;
	G2dPointVal rb;

	int res;

	img.select_mode = G2D_SELECT_MODE_NORMAL;

	img.width = 8;
	img.height = 8;

	res = _util_get_clip( &img, x, y, width, height, &lt, &rb);

	fail_if( res != 1, "1 must returned" );

	fail_if( lt.data.x != 5, "must be equal 1" );
	fail_if( lt.data.y != 5, "must be equal 2" );
	fail_if( rb.data.x != 8, "must be equal 3" );
	fail_if( rb.data.y != 8, "must be equal 4" );
}
END_TEST;

START_TEST ( util_get_clip_WidthorHeightNegativeAndRepeatModeG2D_REPEAT_MODE_NONE )
{
	G2dImage img;
	int x = 1;
	int y = 1;
	int width = 0;
	int height = 0;
	G2dPointVal lt;
	G2dPointVal rb;

	int res;

	img.select_mode = G2D_SELECT_MODE_NORMAL;
	img.repeat_mode = G2D_REPEAT_MODE_NONE;

	res = _util_get_clip( &img, x, y, width, height, &lt, &rb);

	fail_if( res != 0, "0 must returned" );
}
END_TEST;

START_TEST ( util_get_clip_WidthorHeightNegativeAndRepeatModeIsNotG2D_REPEAT_MODE_NONE )
{
	G2dImage img;
	int x = 1;
	int y = 1;
	int width = 0;
	int height = 0;
	G2dPointVal lt;
	G2dPointVal rb;

	int res;

	img.select_mode = G2D_SELECT_MODE_NORMAL;
	img.repeat_mode = G2D_REPEAT_MODE_PAD;

	res = _util_get_clip( &img, x, y, width, height, &lt, &rb);

	fail_if( res != 1, "1 must returned" );
}
END_TEST;
//========================================================================================================================

//========================================================================================================================
/* void util_g2d_fill(G2dImage* img, int x, int y, unsigned int w, unsigned int h, unsigned int color) */
START_TEST ( util_g2d_fill_failOnUtil_Get_Clip )
{
	G2dImage img;
	int x;
	int y;
	unsigned int w = -1;
	unsigned int h = -1;
	unsigned int color;

	img.select_mode = G2D_SELECT_MODE_NORMAL;
	img.repeat_mode = G2D_REPEAT_MODE_NONE;

	util_g2d_fill( &img, x, y, w, h, color );

	fail_if( tst_g2d_reset != 1, " fake variable must initialized by 0" );
	fail_if( tst_g2d_flush != 0, " fake variable must initialized by 1" );
}
END_TEST;

START_TEST ( util_g2d_fill_normalWorkFlow )
{
	G2dImage img;
	int x;
	int y;
	unsigned int w = 100;
	unsigned int h = 100;
	unsigned int color;

	img.select_mode = G2D_SELECT_MODE_NORMAL;

	util_g2d_fill( &img, x, y, w, h, color );

	fail_if( tst_g2d_reset != 0, " fake variable must initialized by 1" );
	fail_if( tst_g2d_flush != 1, " fake variable must initialized by 0" );
}
END_TEST;
//========================================================================================================================

//========================================================================================================================
/* void util_g2d_fill_alu(G2dImage* img, int x, int y, unsigned int w, unsigned int h,
            unsigned int color, G2dAlu alu) */
START_TEST ( util_g2d_fill_alu_failOnUtil_Get_Clip )
{
	G2dImage img;
	int x;
	int y;
	unsigned int w = -1;
	unsigned int h = -1;
	unsigned int color;

    G2dAlu alu;

    img.select_mode = G2D_SELECT_MODE_NORMAL;
   	img.repeat_mode = G2D_REPEAT_MODE_NONE;

    util_g2d_fill_alu( &img, x, y, w, h, color, alu);

	fail_if( tst_g2d_reset != 1, " fake variable must initialized by 0" );
	fail_if( tst_g2d_flush != 0, " fake variable must initialized by 1" );
}
END_TEST;

START_TEST ( util_g2d_fill_workFlow )
{
	G2dImage img;
	int x;
	int y;
	unsigned int w = 100;
	unsigned int h = 100;
	unsigned int color;

    G2dAlu alu;

    img.select_mode = G2D_SELECT_MODE_NORMAL;

    util_g2d_fill_alu( &img, x, y, w, h, color, alu);

	fail_if( tst_g2d_reset != 0, " fake variable must initialized by 0" );
	fail_if( tst_g2d_flush != 1, " fake variable must initialized by 1" );
}
END_TEST;
//========================================================================================================================



//********************************************** Tests runner ************************************************************

int test_util_g2d_suite ( run_stat_data* stat )
{
	//These two extern initialized cGtx
	extern void setup_initgCtx( void );
	extern void teardown_freegCtx( void );


	Suite *s = suite_create ("UTIL_G2D.C");
	/* Core test case */
	TCase *tc_util_get_clip     = tcase_create ( "_util_get_clip" );
	TCase *tc_util_g2d_fill     = tcase_create ( "util_g2d_fill" );
	TCase *tc_util_g2d_fill_alu = tcase_create ( "util_g2d_fill_alu" );


	//_util_get_clip
	tcase_add_test ( tc_util_get_clip, util_get_clip_setModeG2D_SELECT_MODE_FGCOLOR );
	tcase_add_test ( tc_util_get_clip, util_get_clip_xAndyAreNegative );
	tcase_add_test ( tc_util_get_clip, util_get_clip_XPlusWYplusHAreBiggerThenIwIh );
	tcase_add_test ( tc_util_get_clip, util_get_clip_WidthorHeightNegativeAndRepeatModeG2D_REPEAT_MODE_NONE );
	tcase_add_test ( tc_util_get_clip, util_get_clip_WidthorHeightNegativeAndRepeatModeIsNotG2D_REPEAT_MODE_NONE );

	//util_g2d_fill
	tcase_add_checked_fixture( tc_util_g2d_fill, setup_initgCtx, teardown_freegCtx );
	tcase_add_test ( tc_util_g2d_fill, util_g2d_fill_failOnUtil_Get_Clip );
	tcase_add_test ( tc_util_g2d_fill, util_g2d_fill_normalWorkFlow );

	//util_g2d_fill_alu
	tcase_add_checked_fixture( tc_util_g2d_fill_alu, setup_initgCtx, teardown_freegCtx );
	tcase_add_test ( tc_util_g2d_fill_alu, util_g2d_fill_alu_failOnUtil_Get_Clip );
	tcase_add_test ( tc_util_g2d_fill_alu, util_g2d_fill_workFlow );


	suite_add_tcase ( s, tc_util_get_clip );
	suite_add_tcase ( s, tc_util_g2d_fill );
	suite_add_tcase ( s, tc_util_g2d_fill_alu );


	SRunner *sr = srunner_create( s );
	srunner_run_all( sr, CK_VERBOSE );

	stat->num_tests = srunner_ntests_run( sr );
	stat->num_pass = srunner_ntests_run( sr ) - srunner_ntests_failed( sr );
	stat->num_failure = srunner_ntests_failed( sr );

	return 0;
}
