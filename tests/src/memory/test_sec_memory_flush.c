#include "../main.h"

// include functions from sec.c to test
// we do it because some functions within sec.c are static
#include "../../src/memory/sec_memory_flush.c"

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )

START_TEST( ut__secMemoryDPMS_workflow_1 )
{
	CallbackListPtr *list = NULL;
	pointer closure = NULL;
	pointer calldata = NULL;

	_secMemoryDPMS ( list, closure, calldata);

	/* This function returns void - correct check must be realized */
	/*fail_if( res!= 1, "work flow is changed." );*/
}
END_TEST;

/*======================================================================================================*/

Suite* suite_memory_flush( void )
{
	Suite* s = suite_create( "sec_memory_flush.c" );

	TCase* tc_function_name = tcase_create( "_secMemoryDPMS" );

	tcase_add_test( tc_function_name, ut__secMemoryDPMS_workflow_1 );

	suite_add_tcase( s, tc_function_name );

	return s;
}
