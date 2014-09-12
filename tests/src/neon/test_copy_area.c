#include "../main.h"

// include functions from sec.c to test
// we do it because some functions within sec.c are static
#include "../../src/neon/copy_area.c"

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )

START_TEST( ut_memcpy32_workflow_1 )
{

	unsigned long *dst = NULL;
	unsigned long *src = NULL;
	int size = 0;

	memcpy32 ( dst, src, size);

	/* This function returns void - correct check must be realized */
	/*fail_if( res!= 1, "work flow is changed." );*/
}
END_TEST;

/*======================================================================================================*/

Suite* suite_copy_area( void )
{
	Suite* s = suite_create( "copy_area.c" );

	TCase* tc_function_name = tcase_create( "memcpy32" );

	tcase_add_test( tc_function_name, ut_memcpy32_workflow_1 );

	suite_add_tcase( s, tc_function_name );

	return s;
}
