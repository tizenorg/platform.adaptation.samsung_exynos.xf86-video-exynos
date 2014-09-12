#ifndef MAIN_H_
#define MAIN_H_

#include <check.h>

#include "stdio.h"

#include "fake_mem.h"
#include "fake_drm.h"

#include "sec_util.h"

#define calloc ctrl_calloc
#define malloc ctrl_malloc
#define free ctrl_free
#define strdup ctrl_strdup

typedef struct sf
{
	Suite* (*suite_func)(void);
} SuitFunctions;
//typedef Suite* (*suite_func_t)(void);

typedef enum { UT_RUN_ALL,     UT_SEC,            UT_DRI2,
			   UT_EXA_G2D,     UT_EXA_SW,         UT_EXA,
			   UT_CRTC,        UT_DISPLAY,        UT_DUMMY,
			   UT_LAYER,       UT_OUTPUT,         UT_PLANE,
			   UT_PROP,        UT_XBERC,          UT_DRMMODE_DUMP,
			   UT_FIMG2D,      UT_UTIL_G2D,       UT_CONVERTER,
			   UT_DRM_IPP,     UT_WB,             UT_MEMORY_FLUSH,
			   UT_COPY_AREA,   UT_UTIL,           UT_VIDEO_DISPLAY,
			   UT_VIDEO_TVOUT, UT_VIDEO_VIRTUAL , UT_VIDEO } SuiteSelection;

void runner( SuiteSelection pick );
void run_all( void );
void run_picked( Suite* (*suite_func)(void) );
//void run_picked( suite_func_t suite_func );

/* Driver's suite creation functions */
/*----------------------------------*/

/* ./src */
Suite* suite_sec( void );

/* ./src/accel */
Suite* suite_dri2( void );
Suite* suite_exa_g2d( void );
Suite* suite_exa_sw( void );
Suite* suite_exa( void );

/* ./src/crtcconfig */
Suite* suite_crtc( void );
Suite* suite_display( void );
Suite* suite_dummy( void );
Suite* suite_layer( void );
Suite* suite_output( void );
Suite* suite_plane( void );
Suite* suite_prop( void );
Suite* suite_xberc( void );

/* ./src/debug */
Suite* suite_drmmode_dump( void );

/* ./src/g2d */
Suite* suite_fimg2d( void );
Suite* suite_util_g2d( void );

/* ./src/ipp */
Suite* suite_converter( void );
Suite* suite_drm_ipp( void );
Suite* suite_wb( void );

/* ./src/memory */
Suite* suite_memory_flush( void );

/* ./src/neon */
Suite* suite_copy_area( void );

/* ./src/util */
Suite* suite_util( void );

/* ./src/xv */
Suite* suite_video_display( void );
Suite* suite_video_tvout( void );
Suite* suite_video_virtual( void );
Suite* suite_video( void );


#endif /* MAIN_H_ */
