#include "../main.h"

// include functions from sec.c to test
// we do it because some functions within sec.c are static
#include "../../src/crtcconfig/sec_prop.c"

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )

START_TEST( ut__RRModeConvertToDisplayMode_workflow_1 )
{
	ScrnInfoPtr	scrn = NULL;
	RRModePtr   randr_mode = NULL;
    DisplayModePtr	mode = NULL;

	_RRModeConvertToDisplayMode ( scrn, randr_mode, mode);

	/* This foo returns void - correct check must be realized */
	/*fail_if( res!= 1, "work flow is changed." );*/
}
END_TEST;

/*======================================================================================================*/

Suite* suite_prop( void )
{
	Suite* s = suite_create( "sec_prop.c" );

	TCase* tc_function_name = tcase_create( "_RRModeConvertToDisplayMode" );

	tcase_add_test( tc_function_name, ut__RRModeConvertToDisplayMode_workflow_1 );

	suite_add_tcase( s, tc_function_name );

	return s;
}
