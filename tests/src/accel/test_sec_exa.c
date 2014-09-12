#include "../../main.h"

// include functions from sec.c to test
// we do it because some functions within sec.c are static
#include "../../src/accel/sec_exa.c"

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )

START_TEST( ut__setScreenRotationProperty_workflow_1 )
{
	ScrnInfoPtr pScrn = NULL;
	int res;

	_setScreenRotationProperty( pScrn );

	/* This foo returns void - correct check must be realized */
	/*fail_if( res!= 1, "work flow is changed." );*/
}
END_TEST;

//========================================================================================================

Suite* suite_exa( void )
{
	Suite* s = suite_create( "sec_exa.c" );

	// test case creation
	TCase* tc_function_name = tcase_create( "_setScreenRotationProperty" );

	tcase_add_test( tc_function_name, ut__setScreenRotationProperty_workflow_1 );

	suite_add_tcase( s, tc_function_name );

	return s;
}
