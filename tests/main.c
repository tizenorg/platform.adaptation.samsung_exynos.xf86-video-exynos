
#include "main.h"

static int ret = 0;

// if you want to run all available unit tests, simple run application (./test)
// if you want to run available unit tests only for specified file, run application
// with argument, that specific this file
// (`./test 1` -- will run unit tests for sec.c (UT_SEC); `./test 11` -- will run unit tests for sec_plane.c (UT_PLANE) )
// (`./test 1` equal to run_picked( suite_sec )           `./test 11` equal to run_picked( suite_plane ))
//===========================================================================================
int main( int argc, char** argv )
{
	int file_to_test = 0;

	calloc_init();

	// if no arguments - run all tests
	if( argc < 2 )
	  runner( UT_RUN_ALL );
	else
	{
	  sscanf( argv[1], "%d", &file_to_test );
	  runner( (SuiteSelection)file_to_test );
	}

	return ret;
}

// this function picks which file(s) to test
//===========================================================================================
void runner( SuiteSelection pick )
{
	switch ( pick )
	{
		case UT_RUN_ALL:
			run_all();
			break;

		case UT_SEC:
			run_picked( suite_sec );
			break;

		case UT_DRI2:
			run_picked( suite_dri2 );
			break;

		case UT_EXA_G2D:
			run_picked( suite_exa_g2d );
			break;

		case UT_EXA_SW:
			run_picked( suite_exa_sw );
			break;

		case UT_EXA:
			run_picked( suite_exa );
			break;

		case UT_CRTC:
			run_picked( suite_crtc );
			break;

		case UT_DISPLAY:
			run_picked( suite_display );
			break;

		case UT_DUMMY:
			run_picked( suite_dummy );
			break;

		case UT_LAYER:
			run_picked( suite_layer );
			break;

		case UT_OUTPUT:
			run_picked( suite_output );
			break;

		case UT_PLANE:
			run_picked( suite_plane );
			break;

		case UT_PROP:
			run_picked( suite_prop );
			break;

		case UT_XBERC:
			run_picked( suite_xberc );
			break;

		case UT_DRMMODE_DUMP:
			run_picked( suite_drmmode_dump );
			break;

		case UT_CONVERTER:
			run_picked( suite_converter );
			break;

		case UT_DRM_IPP:
			run_picked( suite_drm_ipp );
			break;

		case UT_WB:
			run_picked( suite_wb );
			break;

		case UT_MEMORY_FLUSH:
		//	run_picked( suite_memory_flush );
			break;

		case UT_COPY_AREA:
			run_picked( suite_copy_area );
			break;

		case UT_UTIL:
			run_picked( suite_util );
			break;

		case UT_VIDEO_DISPLAY:
			run_picked( suite_video_display );
			break;

		case UT_VIDEO_TVOUT:
			run_picked( suite_video_tvout );
			break;

		case UT_VIDEO_VIRTUAL:
			run_picked( suite_video_virtual );
			break;
		case UT_VIDEO:
			run_picked( suite_video );
			break;

		default:
			printf( "not correct input argument!!!\n" );
			break;
	}
}

// this function runs all tests
//===========================================================================================
void run_all( void )
{
	SRunner* sr;
	Suite* s;
	int number_failed;

/*
	SuitFunctions funcs[] = {
			 { suite_sec },           { suite_dri2 },          { suite_exa_g2d },
			 { suite_exa_sw },        { suite_exa },           { suite_crtc },
			 { suite_display }, 	  { suite_dummy },         { suite_layer },
			 { suite_output },        { suite_prop },
			 { suite_xberc },         { suite_drmmode_dump },  { suite_fimg2d },
			 { suite_util_g2d },      { suite_converter },     { suite_drm_ipp },
			 { suite_wb },            { suite_memory_flush },  { suite_copy_area },
			 { suite_util },          { suite_video_display }, { suite_video_tvout },
			 { suite_video_virtual }, { suite_video }, { suite_plane }
	};
*/

	SuitFunctions funcs[] = {
	             { suite_video_virtual },           { suite_plane },          { suite_dri2 }
	};
	sr = srunner_create( funcs[0].suite_func() );

	int i = 1;
	for( ; i < 3; i++ )
		srunner_add_suite( sr, funcs[i].suite_func() );

	srunner_run_all( sr, CK_VERBOSE );

	number_failed = srunner_ntests_failed( sr );

	srunner_free( sr );

	ret = (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

// this function runs tests for suite_func's file only
//===========================================================================================
void run_picked( Suite* (*suite_func)(void) )
{
	int number_failed;

	Suite* s;
	SRunner* sr;

	s = (*suite_func)();
	sr = srunner_create( s );

	srunner_set_fork_status (sr, CK_FORK); //CK_NOFORK

	srunner_run_all( sr, CK_VERBOSE );

	number_failed = srunner_ntests_failed( sr );

	srunner_free( sr );

	ret = (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
