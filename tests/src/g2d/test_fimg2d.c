#include "../main.h"

// include functions from sec.c to test
// we do it because some functions within sec.c are static
#include "../../src/g2d/fimg2d.c"

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )

START_TEST( ut__g2d_clear_workflow_1 )
{
	_g2d_clear();

	/* This function returns void - correct check must be realized */
	/*fail_if( res!= 1, "work flow is changed." );*/
}
END_TEST;

/*======================================================================================================*/

Suite* suite_fimg2d( void )
{
	Suite* s = suite_create( "fimg2d.c" );

	TCase* tc_function_name = tcase_create( "_g2d_clear" );

	tcase_add_test( tc_function_name, ut__g2d_clear_workflow_1 );

	suite_add_tcase( s, tc_function_name );

	return s;
}
