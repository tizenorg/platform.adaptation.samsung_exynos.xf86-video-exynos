#include "../main.h"

// include functions from sec.c to test
// we do it because some functions within sec.c are static
#include "../../src/xv/sec_video_tvout.c"

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )

START_TEST( ut__secVieoTvCalSize_workflow_1 )
{
	SECVideoTv* tv = NULL;
	int src_w = 0;
	int src_h = 0;
	int dst_w = 0;
	int dst_h = 0;
	int res = 0;

//	res = _secVieoTvCalSize ( tv, src_w, src_h, dst_w, dst_h);
	fail_if( res!= 1, "workflow is changed." );
}
END_TEST;

/*======================================================================================================*/

Suite* suite_video_tvout( void )
{
	Suite* s = suite_create( "sec_video_tvout.c" );

	TCase* tc_function_name = tcase_create( "_getPixmap" );

	tcase_add_test( tc_function_name, ut__secVieoTvCalSize_workflow_1 );

	suite_add_tcase( s, tc_function_name );

	return s;
}
