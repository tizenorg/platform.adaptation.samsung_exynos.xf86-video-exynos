#include "../main.h"

// include functions from sec.c to test
// we do it because some functions within sec.c are static
#include "../../src/ipp/sec_drm_ipp.c"

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )

START_TEST( ut_secDrmIppGetFormatList_workflow_1 )
{
	int res = 0;
	int num = 0;

	res = secDrmIppGetFormatList (&num);
	fail_if( res!= 1, "workflow is changed." );
}
END_TEST;

/*======================================================================================================*/

Suite* suite_drm_ipp( void )
{
	Suite* s = suite_create( "sec_drm_ipp.c" );

	TCase* tc_function_name = tcase_create( "secDrmIppGetFormatList" );

	tcase_add_test( tc_function_name, ut_secDrmIppGetFormatList_workflow_1 );

	suite_add_tcase( s, tc_function_name );

	return s;
}
