#include "../main.h"

// include functions from sec.c to test
// we do it because some functions within sec.c are static
#include "../../src/util/sec_util.c"

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )

START_TEST( ut_secUtilDumpBmp_workflow_1 )
{
	const char * file = NULL;
	const void * data = NULL;
	int width = 0;
	int height = 0;
	int res = 0;

	res =  secUtilDumpBmp ( file, data, width, height);

	fail_if( res!= 1, "workflow is changed." );
}
END_TEST;

/*======================================================================================================*/

Suite* suite_util( void )
{
	Suite* s = suite_create( "sec_util.c" );

	TCase* tc_function_name = tcase_create( "secUtilDumpBmp" );

	tcase_add_test( tc_function_name, ut_secUtilDumpBmp_workflow_1 );

	suite_add_tcase( s, tc_function_name );

	return s;
}
