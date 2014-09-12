#include "../main.h"

// include functions from sec.c to test
// we do it because some functions within sec.c are static
#include "../../src/crtcconfig/sec_crtc.c"

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )

START_TEST( ut__overlayGetXMoveOffset_workflow_1 )
{
	xf86CrtcPtr pCrtc = NULL;
	int x = 0;
	int res = 0;

	/* This function returns void - correct check must be realized */
	/*fail_if( res!= 1, "work flow is changed." );*/

	res = _overlayGetXMoveOffset( pCrtc, x );
	fail_if( res!= 1, "workflow is changed." );
}
END_TEST;

/*======================================================================================================*/

Suite* suite_crtc( void )
{
	Suite* s = suite_create( "sec_crtc.c" );

	TCase* tc_function_name = tcase_create( "_overlayGetXMoveOffset" );

	tcase_add_test( tc_function_name, ut__overlayGetXMoveOffset_workflow_1 );

	suite_add_tcase( s, tc_function_name );

	return s;
}
