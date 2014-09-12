#include "../main.h"

// include functions from sec.c to test
// we do it because some functions within sec.c are static
#include "../../src/ipp/sec_wb.c"

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )

START_TEST( ut__secWbCountPrint_workflow_1 )
{
	OsTimerPtr timer = NULL;
	CARD32 now = NULL;
	pointer arg = NULL;
	int res = 0;

	res = _secWbCountPrint ( timer, now, arg);
	fail_if( res!= 1, "workflow is changed." );
}
END_TEST;

/*======================================================================================================*/

Suite* suite_wb( void )
{
	Suite* s = suite_create( "sec_wb.c" );

	TCase* tc_function_name = tcase_create( "_secWbCountPrint" );

	tcase_add_test( tc_function_name, ut__secWbCountPrint_workflow_1 );

	suite_add_tcase( s, tc_function_name );

	return s;
}
