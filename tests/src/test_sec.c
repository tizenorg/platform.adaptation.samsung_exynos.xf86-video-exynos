/*
 * test_sec.c
 *
 *  Created on: Aug 20, 2014
 *      Author: s.sizonov
 */

#include "../main.h"

// include functions from sec.c to test
// we do it because some functions within sec.c are static
#include "../../src/sec.c"

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )

START_TEST( ut_SECSetup_workflow_1 )
{
	pointer res = NULL;
	pointer module = NULL;
	pointer opts = NULL;
	int errmaj = 0;
	int errmin = 0;

	res = SECSetup( module, opts, &errmaj, &errmin);
	fail_if( res!= 1, "workflow is changed." );
}
END_TEST;

//========================================================================================================

Suite* suite_sec( void )
{
  Suite* s = suite_create( "sec.c" );

  // test case creation
  TCase* tc_function_name = tcase_create( "SECSetup" );

  tcase_add_test( tc_function_name, ut_SECSetup_workflow_1 );

  suite_add_tcase( s, tc_function_name );

  return s;
}
