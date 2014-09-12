#include "../main.h"

// include functions from sec.c to test
// we do it because some functions within sec.c are static
#include "../../src/crtcconfig/sec_display.c"

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )

START_TEST( ut__secDisplaySetDrmEventCtx_workflow_1 )
{
	SECModePtr pSecMode = NULL;

	_secDisplaySetDrmEventCtx( pSecMode );

	/* This foo returns void - correct check must be realized */
	/*fail_if( res!= 1, "work flow is changed." );*/
}
END_TEST;

/*======================================================================================================*/

Suite* suite_display( void )
{
  Suite* s = suite_create( "sec_display.c" );

  TCase* tc_function_name = tcase_create( "_secDisplaySetDrmEventCtx" );

  tcase_add_test( tc_function_name, ut__secDisplaySetDrmEventCtx_workflow_1 );

  suite_add_tcase( s, tc_function_name );

  return s;
}
