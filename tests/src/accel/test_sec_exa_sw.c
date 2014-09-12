#include "../../main.h"

// include functions from sec.c to test
// we do it because some functions within sec.c are static
#include "../../src/accel/sec_exa_sw.c"

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )

START_TEST( ut__swBoxAdd_workflow_1 )
{
	struct xorg_list *l = NULL;
	BoxPtr b1 = NULL;
	BoxPtr b2 = NULL;
	int res;

	res = _swBoxAdd (l, b1,b2);
	fail_if( res!= 1, "work flow is changed." );
}
END_TEST;

//========================================================================================================

Suite* suite_exa_sw( void )
{
	Suite* s = suite_create( "sec_exa_sw.c" );

	// test case creation
	TCase* tc_function_name = tcase_create( "_swBoxAdd" );

	tcase_add_test( tc_function_name, ut__swBoxAdd_workflow_1 );

	suite_add_tcase( s, tc_function_name );

	return s;
}
