#include "../main.h"

// include functions from sec.c to test
// we do it because some functions within sec.c are static
#include "../../src/g2d/util_g2d.c"

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )

START_TEST( ut__util_get_clip_workflow_1 )
{
	G2dImage* img = NULL;
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;
	G2dPointVal *lt = NULL;
	G2dPointVal *rb = NULL;
	int res = 0;

	res = _util_get_clip( img, x, y, width, height, lt, rb);
	fail_if( res!= 1, "workflow is changed." );
}
END_TEST;

/*======================================================================================================*/

Suite* suite_util_g2d( void )
{
	Suite* s = suite_create( "util_g2d.c" );

	TCase* tc_function_name = tcase_create( "_util_get_clip" );

	tcase_add_test( tc_function_name, ut__util_get_clip_workflow_1 );

	suite_add_tcase( s, tc_function_name );

	return s;
}
