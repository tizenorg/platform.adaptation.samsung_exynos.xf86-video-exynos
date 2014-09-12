#include "../main.h"

// include functions from sec.c to test
// we do it because some functions within sec.c are static
#include "../../src/crtcconfig/sec_dummy.c"

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )

START_TEST( ut__dummyFlipPixmapInit_workflow_1 )
{
	xf86CrtcPtr pCrtc = NULL;
	//_dummyFlipPixmapInit (xf86CrtcPtr pCrtc);

	/* This foo returns void - correct check must be realized */
	/*fail_if( res!= 1, "work flow is changed." );*/
}
END_TEST;

/*======================================================================================================*/

Suite* suite_dummy( void )
{
	Suite* s = suite_create( "sec_dummy.c" );

	TCase* tc_function_name = tcase_create( "_dummyFlipPixmapInit" );

	tcase_add_test( tc_function_name, ut__dummyFlipPixmapInit_workflow_1 );

	suite_add_tcase( s, tc_function_name );

	return s;
}
