#include "../main.h"

// include functions from sec.c to test
// we do it because some functions within sec.c are static
#include "../../src/debug/sec_drmmode_dump.c"

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )

START_TEST( ut_connector_status_str_workflow_1 )
{
	int type = 0;
	int res = 0;

	res = connector_status_str ( type );
	fail_if( res!= 1, "workflow is changed." );
}
END_TEST;

/*======================================================================================================*/

Suite* suite_drmmode_dump( void )
{
	Suite* s = suite_create( "sec_drmmode_dump.c" );

	TCase* tc_function_name = tcase_create( "connector_status_str" );

	tcase_add_test( tc_function_name, ut_connector_status_str_workflow_1 );

	suite_add_tcase( s, tc_function_name );

	return s;
}
