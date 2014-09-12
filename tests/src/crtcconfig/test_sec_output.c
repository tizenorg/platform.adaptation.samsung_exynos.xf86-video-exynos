#include "../main.h"

// include functions from sec.c to test
// we do it because some functions within sec.c are static
#include "../../src/crtcconfig/sec_output.c"

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )

START_TEST( ut__secOutputResumeWbTimeout_workflow_1 )
{
	OsTimerPtr timer = NULL;
	CARD32 now = 0;
	pointer arg = NULL;
	int res = 0;

	res = _secOutputResumeWbTimeout ( timer, now, arg);
	fail_if( res!= 1, "workflow is changed." );
}
END_TEST;

/*======================================================================================================*/

Suite* suite_output( void )
{
	Suite* s = suite_create( "sec_output.c" );

	TCase* tc_function_name = tcase_create( "_secOutputResumeWbTimeout" );

	tcase_add_test( tc_function_name, ut__secOutputResumeWbTimeout_workflow_1 );

	suite_add_tcase( s, tc_function_name );

	return s;
}
