#include <stdlib.h>

#include <check.h>
#include <stdarg.h>

#include "test_display.h"
#include "test_clone.h"
#include "test_video_capture.h"
#include "test_video_buffer.h"
#include "test_mem_pool.h"
#include "test_dri2.h"
#include "test_crtc.h"

#include "exynos_driver.h"
#include "test_stat.h"
#define FILES    19

run_stat_data statistic[FILES];


//#define TEST_VIEW

extern void calloc_init( void );


// entry point in C/C++ application
//==========================================================================================
int main( void )
{
    int total_tests = 0;
    int number_failed = 0;

    calloc_init();

	//test_util_g2d_suite(&statistic[0]);
	//test_display(&statistic[1]);//display.log OFF
	//test_video_buffer_suite(&statistic[2]);//delete valgrind test from it( valgrind_test )
	//test_video_ipp_suite(&statistic[3]);
	//test_fimg2d_suite(&statistic[4]);
	//accel_suite(&statistic[5]);
	//clone_suite(&statistic[6]);
	//test_crtc_suite(&statistic[7]);//crtc.log OFF
	//test_video_capture_suite( &statistic[8] );
	//test_mem_pool(&statistic[9]);
 	//exa_suite(&statistic[10]);
	//test_output_suite(&statistic[11]);
    //test_video_converter_suite( &statistic[12] );
    plane_suite( &statistic[13] );
	//test_test();

    int am_tests = 0;
    int am_tests_p = 0;
    int am_tests_f = 0;
    int am_func_t = 0;
    int am_func_nt = 0;
    int i = 0;
    for( ; i < FILES; i++ )
    {
    	am_tests   += statistic[i].num_tests;
    	am_tests_p += statistic[i].num_pass;
    	am_tests_f += statistic[i].num_failure;
    	am_func_t  += statistic[i].num_tested;
        am_func_nt += statistic[i].num_nottested;
    }
    printf("\n %d %d %d %d %d %d\n", am_tests, am_tests_p, am_tests_f,
    		am_func_t + am_func_nt , am_func_t, am_func_nt);


    //out summary stat
}
