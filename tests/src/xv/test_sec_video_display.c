#include "../main.h"

// include functions from sec.c to test
// we do it because some functions within sec.c are static
#include "../../src/xv/sec_video_display.c"

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )

START_TEST( ut__getPixmap_workflow_1 )
{
	DrawablePtr pDraw = NULL;
	int res = 0;

	res = _getPixmap ( pDraw );
	fail_if( res!= 1, "workflow is changed." );
}
END_TEST;

/*======================================================================================================*/

Suite* suite_video_display( void )
{
	Suite* s = suite_create( "sec_video_display.c" );

	TCase* tc_function_name = tcase_create( "_getPixmap" );

	tcase_add_test( tc_function_name, ut__getPixmap_workflow_1 );

	suite_add_tcase( s, tc_function_name );

	return s;
}
