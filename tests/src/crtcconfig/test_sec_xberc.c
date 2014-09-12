#include "../main.h"

// include functions from sec.c to test
// we do it because some functions within sec.c are static
#include "../../src/crtcconfig/sec_xberc.c"

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )

START_TEST( ut_SECXbercSetTvoutMode_workflow_1 )
{
	int argc = 0;
	char ** argv = NULL;
	RRPropertyValuePtr value = NULL;
	ScrnInfoPtr scrn = NULL;
	int res = 0;

	res = SECXbercSetTvoutMode (argc, argv, value, scrn);
	fail_if( res!= 1, "workflow is changed." );
}
END_TEST;

/*======================================================================================================*/

Suite* suite_xberc( void )
{
	Suite* s = suite_create( "sec_xberc.c" );

	TCase* tc_function_name = tcase_create( "SECXbercSetTvoutMode" );

	tcase_add_test( tc_function_name, ut_SECXbercSetTvoutMode_workflow_1 );

	suite_add_tcase( s, tc_function_name );

	return s;
}
