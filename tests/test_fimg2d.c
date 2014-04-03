/*
 * test_fimg2d.c
 *
 *  Created on: Oct 21, 2013
 *      Author: svoloshynov
 */
#include "test_fimg2d.h"
#include "mem.h"
#include "./g2d/fimg2d.c"


#define PRINTVAL( v ) printf( "\n %d \n", v );

void setup_initgCtx( void )
{
	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );
}

void teardown_freegCtx( void )
{
	free( gCtx );
}

//=====================================================================================================
/* static void _g2d_clear(void) */
START_TEST ( _g2d_clear_test )
{
	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );

    gCtx->cmd_nr = 1;
    gCtx->cmd_gem_nr = 1;

    _g2d_clear();

    fail_if( gCtx->cmd_nr != 0, " gCtx->cmd_nr must be 0 " );
    fail_if( gCtx->cmd_gem_nr == 1, " gCtx->cmd_gem_nr must be 0 " );

    gCtx = NULL;
}
END_TEST
//=====================================================================================================

//=====================================================================================================
/* int g2d_init (int fd) */
START_TEST ( g2d_init_gCtxAlreadyInit )
{
	int fd = 1;
	int res;

	gCtx = ctrl_calloc(1, sizeof(*gCtx));//Memory for gCtx allocated, function must returned FALSE

	fail_if( g2d_init ( fd ) != FALSE, "gCtx already initialized, FALSE must returned" );
}
END_TEST

START_TEST ( g2d_init_drmIoctlReturnNegativeValue )
{
	int fd = -1;
	int res;

	fail_if( g2d_init ( fd ) != FALSE, "FALSE must returned, fd negative" );
	fail_if( ctrl_get_cnt_mem_obj( CALLOC_OBJ ) != 0, "Memory must free" );
}
END_TEST

START_TEST ( g2d_init_correctInit )
{
	int fd = 1;
	int res;

	fail_if( g2d_init ( fd ) != TRUE, "TRUE must returned, correct initialization" );

	fail_if( gCtx->major != 12, "gCtx->major wrong initialization" );
    fail_if( gCtx->minor != 34, "gCtx->major wrong initialization" );

    fail_if( ctrl_get_cnt_mem_obj( CALLOC_OBJ ) != 1, "Memory must allocated" );
}
END_TEST
//=====================================================================================================

//=====================================================================================================
/* void g2d_fini (void) */
START_TEST ( g2d_fini_memoryManagement )
{
	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );//pre initialization

	fail_if( ctrl_get_cnt_mem_obj( CALLOC_OBJ ) != 1, "Memory must allocated" );// pre check

	g2d_fini();

	fail_if( ctrl_get_cnt_mem_obj( CALLOC_OBJ ) != 0, "Memory must free" );
}
END_TEST
//=====================================================================================================

//=====================================================================================================
/* int g2d_add_cmd(unsigned int cmd, unsigned int value) */
START_TEST ( g2d_add_cmd_overflowCmd_GemSize )
{
	int cmd = SRC_BASE_ADDR_REG;
	unsigned int value = 1;
	int res;

	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );

	gCtx->cmd_gem_nr = 65;//Using for broke the function

	res = g2d_add_cmd( cmd, value );

	fail_if( res != FALSE, " gCtx->cmd_gem_nr >= 64( SRC_BASE_ADDR_REG ) " );
}
END_TEST

START_TEST ( g2d_add_cmd_correctInit )
{
	int cmd = SRC_PLANE2_BASE_ADDR_REG;
	unsigned int value = 1;
	int res;

	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );

	gCtx->cmd_gem_nr = 1;
	int tst = gCtx->cmd_gem_nr;

	res = g2d_add_cmd( cmd, value );

	fail_if( res != TRUE, "TRUE must returned" );
/*
    gCtx->cmd_gem[gCtx->cmd_gem_nr].offset = cmd;
    gCtx->cmd_gem[gCtx->cmd_gem_nr].data = value;
    gCtx->cmd_gem_nr++;
*/
	fail_if( gCtx->cmd_gem[gCtx->cmd_gem_nr - 1].offset != SRC_PLANE2_BASE_ADDR_REG, "must be equal 1" );
	fail_if( gCtx->cmd_gem[gCtx->cmd_gem_nr - 1].data != value, "must be equal 2" );
	fail_if( gCtx->cmd_gem_nr != tst + 1, "gCtx->cmd_gem_nr must increase by 1");
}
END_TEST

START_TEST ( g2d_add_cmd_goToDefaultOverflowCmd_GemSize )
{
	int cmd = 12;
	unsigned int value = 1;
	int res;

	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );

	gCtx->cmd_nr = G2D_MAX_CMD + 1; //Using for broke the function

	res = g2d_add_cmd( cmd, value );

	fail_if( res != FALSE, "gCtx->cmd_nr >= 64( G2D_MAX_CMD )" );
}
END_TEST

START_TEST ( g2d_add_cmd_goToDefaultCorrectInit )
{
	int cmd = 12;
	unsigned int value = 1;
	int res;

	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );

	gCtx->cmd_nr = 1;
	int tst = gCtx->cmd_nr;

	res = g2d_add_cmd( cmd, value );

	fail_if( res != TRUE, "TRUE must returned" );
/*
    gCtx->cmd[gCtx->cmd_nr].offset = cmd;
    gCtx->cmd[gCtx->cmd_nr].data = value;
    gCtx->cmd_nr++;
*/
	fail_if( gCtx->cmd[gCtx->cmd_nr - 1].offset != 12, "must be equal 1" );
	fail_if( gCtx->cmd[gCtx->cmd_nr - 1].data != value, "must be equal 2" );
	fail_if( gCtx->cmd_nr != tst + 1, "gCtx->cmd_gem_nr must increase by 1");
}
END_TEST
//=====================================================================================================

//=====================================================================================================
/* void g2d_reset (unsigned int clear_reg) */
START_TEST ( g2d_reset_clear_RegZero )
{
	unsigned int clear_reg = 1;

	gCtx = ctrl_calloc( 1, sizeof( *gCtx ) );

	g2d_reset( clear_reg );
/*
	gCtx->cmd[gCtx->cmd_nr].offset = cmd;
	gCtx->cmd[gCtx->cmd_nr].data = value;
	gCtx->cmd_nr++;
*/
	fail_if( gCtx->cmd[gCtx->cmd_nr - 1].offset != SOFT_RESET_REG, "must be equal 1" );
	fail_if( gCtx->cmd[gCtx->cmd_nr - 1].data != 0x01, "must be equal 2" );
	fail_if( gCtx->cmd_nr != 1, "gCtx->cmd_gem_nr must increase by 1");
}
END_TEST
//=====================================================================================================

//=====================================================================================================
/* int g2d_exec (void) */
START_TEST ( g2d_exec_cmdlist_NrIsZero )
{
	int res;
	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );

	gCtx->cmdlist_nr = 1;

	res = g2d_exec();

	fail_if( res != TRUE, "TRUE must returned" );
}
END_TEST

START_TEST ( g2d_exec_ioctlGivesNegativeValue )
{
	int res;
	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );

	gCtx->cmdlist_nr = 1;
	gCtx->drm_fd = -1;

	res = g2d_exec();

	fail_if( res != FALSE, "FALSE must returned, ioctl return value < 0" );
}
END_TEST

START_TEST ( g2d_exec_correctInit )
{
	int res;
	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );

	gCtx->cmdlist_nr = 1;
	gCtx->drm_fd = 1;

	res = g2d_exec();

	fail_if( res != TRUE, "TRUE must returned" );
	fail_if( gCtx->cmdlist_nr != 0, "must be gCtx->cmdlist_nr == 0" );
}
END_TEST
//=====================================================================================================

//=====================================================================================================
/* int g2d_flush (void) */
START_TEST ( g2d_flush_Cmd_NrAndCmd_Gem_NrAreZero )
{
	int res;
	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );

	gCtx->cmd_nr = 0;
	res = g2d_flush();
	fail_if( res != TRUE, "If gCtx->cmd_nr = 0, TRUE must returned" );

	gCtx->cmd_gem_nr = 0;
	res = g2d_flush();
	fail_if( res != TRUE, "If gCtx->cmd_gem_nr = 0, TRUE must returned" );
}
END_TEST

START_TEST ( g2d_flush_drm_FdNegative )
{
	int res;
	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );

	gCtx->drm_fd = -1;
	gCtx->cmd_nr = 1;
	gCtx->cmd_gem_nr = 1;
	gCtx->cmdlist_nr = 0;

	res = g2d_flush();
	fail_if( res != FALSE, "False must returned, gCtx->drm_fd is negative");

	fail_if( gCtx->cmd_nr != 0, "gCtx->cmd_nr must be 0" );
	fail_if( gCtx->cmd_gem_nr != 0, "gCtx->cmd_gem_nr must be 0" );
}
END_TEST

START_TEST ( g2d_flush_correctInit )
{
	int res;
	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );

	extern struct drm_exynos_g2d_set_cmdlist store_cmdlist;

	gCtx->drm_fd = 1;
	gCtx->cmd_nr = 1;
	gCtx->cmd_gem_nr = 1;
	gCtx->cmdlist_nr = 0;

	res = g2d_flush();
	fail_if( res != TRUE, "TRUE must returned, correct initialization");

	fail_if( gCtx->cmd_nr != 0, "gCtx->cmd_nr must be 0" );
	fail_if( gCtx->cmd_gem_nr != 0, "gCtx->cmd_gem_nr must be 0" );
/*
    cmdlist.cmd_nr = gCtx->cmd_nr;
    cmdlist.cmd_gem_nr = gCtx->cmd_gem_nr;
    cmdlist.event_type = G2D_EVENT_NOT;
    cmdlist.user_data = 0;
*/
	fail_if( store_cmdlist.cmd_nr     != 1, "must be equal 1");
	fail_if( store_cmdlist.cmd_gem_nr != 1, "must be equal 2");
	fail_if( store_cmdlist.event_type != G2D_EVENT_NOT, "must be equal 3");
	fail_if( store_cmdlist.user_data  != 0, "must be equal 4");

	fail_if( store_cmdlist.cmd_nr !=  1, "must be equal 5");

}
END_TEST
//=====================================================================================================

//=====================================================================================================
/* G2dImage* g2d_image_new(void) */
START_TEST ( g2d_image_new_initReturnedValue )
{
	G2dImage* res_img = NULL;
	res_img = g2d_image_new();
/*
    img->repeat_mode = G2D_REPEAT_MODE_NONE;
    img->scale_mode = G2D_SCALE_MODE_NONE;
    img->xscale = G2D_FIXED_1;
    img->yscale = G2D_FIXED_1;
*/
	fail_if( res_img == NULL, "img must returned" );

	fail_if( res_img->repeat_mode != G2D_REPEAT_MODE_NONE, "must be equal 1" );
	fail_if( res_img->scale_mode != G2D_SCALE_MODE_NONE, "must be equal 2" );
	fail_if( res_img->xscale != G2D_FIXED_1, "must be equal 3" );
	fail_if( res_img->yscale != G2D_FIXED_1, "must be equal 4" );
}
END_TEST
//=====================================================================================================

//=====================================================================================================
/* G2dImage* g2d_image_create_solid (unsigned int color) */
START_TEST ( g2d_image_create_solid_initReturnedValue )
{
	G2dImage* res_img = NULL;
	unsigned int color = 1;

	res_img = g2d_image_create_solid( color );
/*
    img->select_mode = G2D_SELECT_MODE_FGCOLOR;
    img->color_mode = G2D_COLOR_FMT_ARGB8888|G2D_ORDER_AXRGB;
    img->data.color = color;
    img->width = -1;
    img->height = -1;
*/
	fail_if( res_img == NULL, "img must returned" );

	fail_if( res_img->select_mode != G2D_SELECT_MODE_FGCOLOR, "must be equal 1" );
	fail_if( res_img->color_mode  != G2D_COLOR_FMT_ARGB8888|G2D_ORDER_AXRGB, "must be equal 2" );
	fail_if( res_img->data.color  != color, "must be equal 3" );
	fail_if( res_img->width  != -1, "must be equal 4" );
	fail_if( res_img->height != -1, "must be equal 5" );
}
END_TEST
//=====================================================================================================

//=====================================================================================================
/* G2dImage* g2d_image_create_bo(G2dColorMode format, unsigned int width, unsigned int height,
                                            unsigned int bo, unsigned int stride) */
START_TEST ( g2d_image_create_bo_boNotNegative )
{
	G2dColorMode format;
	unsigned int width = 12;
	unsigned int height = 34;
	unsigned int bo = 1;
	unsigned int stride = 2;

	G2dImage* res_img = NULL;
	res_img = g2d_image_create_bo( format, width, height, bo, stride );

	fail_if( res_img == NULL, "NULL does not must returned" );
/*
	img->select_mode = G2D_SELECT_MODE_NORMAL;
	img->color_mode = format;
	img->width = width;
	img->height = height;
*/
	fail_if( res_img->select_mode != G2D_SELECT_MODE_NORMAL, "must be equal 1");
	fail_if( res_img->color_mode != format, "must be equal 2");
	fail_if( res_img->width != width, "must be equal 3");
	fail_if( res_img->height != height, "must be equal 4");
/*
	Initialization inside if(bo)
    img->data.bo[0] = bo;
    img->stride = stride;
*/
	fail_if( res_img->data.bo[0] != bo, "must be equal 5");
	fail_if( res_img->stride != stride, "must be equal 6");
}
END_TEST

START_TEST ( g2d_image_create_bo_unsupportedFormat )
{
	G2dColorMode format;
	unsigned int width = 12;
	unsigned int height = 34;
	unsigned int bo = 0;
	unsigned int stride = 2;

	G2dImage* res_img = NULL;
	res_img = g2d_image_create_bo( format, width, height, bo, stride );

	fail_if( res_img != NULL, "NULL does not must returned" );
}
END_TEST

START_TEST ( g2d_image_create_bo_failOndrmCommandWriteRead )
{
	G2dColorMode format = G2D_COLOR_FMT_XRGB8888;
	unsigned int width = 12;
	unsigned int height = 34;
	unsigned int bo = 0;
	unsigned int stride = 2;

	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );
	gCtx->drm_fd = -1;

	G2dImage* res_img = NULL;
	res_img = g2d_image_create_bo( format, width, height, bo, stride );

	fail_if( res_img != NULL, "NULL must returned, fail with OndrmCommandWriteRead" );
}
END_TEST

START_TEST ( g2d_image_create_bo_correctInit )
{
	G2dColorMode format = G2D_COLOR_FMT_XRGB8888;
	unsigned int width = 12;
	unsigned int height = 34;
	unsigned int bo = 0;
	unsigned int stride = 2;

	extern struct drm_exynos_gem_create stored_arg;

	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );
	gCtx->drm_fd = 1;

	G2dImage* res_img = NULL;
	res_img = g2d_image_create_bo( format, width, height, bo, stride );

	fail_if( res_img == NULL, "TRUE must returned, correct initialization" );
/*
	img->stride = stride;
	img->data.bo[0] = arg.handle;
	img->need_free = 1;
*/
	fail_if( res_img->stride != width*4, "must be equal 1" );
	fail_if( res_img->data.bo[0] != stored_arg.handle, "must be equal 2" );
	fail_if( res_img->need_free != 1, "must be equal 3" );
}
END_TEST
//=====================================================================================================

//=====================================================================================================
/* G2dImage* g2d_image_create_bo2(G2dColorMode format,
                      unsigned int width, unsigned int height,
                      unsigned int bo1, unsigned int bo2, unsigned int stride) */
START_TEST ( g2d_image_create_bo2_Bo1Zero )
{
	G2dColorMode format;
    unsigned int width;
    unsigned int height;
    unsigned int bo1 = 0;
    unsigned int bo2;
    unsigned int stride;

    G2dImage* res_img = NULL;

    res_img = g2d_image_create_bo2( format, width, height, bo1, bo2, stride );

    fail_if( res_img != NULL, "NULL must returned" );

}
END_TEST

START_TEST ( g2d_image_create_bo2_Bo2Zero )
{
	G2dColorMode format = 0x100;
    unsigned int width;
    unsigned int height;
    unsigned int bo1 = 1;
    unsigned int bo2 = 0;
    unsigned int stride;

    G2dImage* res_img = NULL;

    res_img = g2d_image_create_bo2( format, width, height, bo1, bo2, stride );

    fail_if( res_img != NULL, "NULL must returned" );

}
END_TEST

START_TEST ( g2d_image_create_bo2_correctInit )
{
	G2dColorMode format = 0x100;
    unsigned int width = 1;
    unsigned int height = 2;
    unsigned int bo1 = 3;
    unsigned int bo2 = 4;
    unsigned int stride = 5;

    G2dImage* res_img = NULL;

    res_img = g2d_image_create_bo2( format, width, height, bo1, bo2, stride );

    fail_if( res_img == NULL, "NULL must returned" );
/*
    img->select_mode = G2D_SELECT_MODE_NORMAL;
    img->color_mode = format;
    img->width = width;
    img->height = height;

    img->data.bo[0] = bo1;
    img->data.bo[1] = bo2;
    img->stride = stride;
*/
    fail_if( res_img->select_mode != G2D_SELECT_MODE_NORMAL, "must be equal 1" );
    fail_if( res_img->color_mode != format, "must be equal 2" );
    fail_if( res_img->width != width, "must be equal 3" );
    fail_if( res_img->height != height, "must be equal 4" );

    fail_if( res_img->data.bo[0] != bo1, "must be equal 5" );
    fail_if( res_img->data.bo[1] != bo2, "must be equal 6" );
    fail_if( res_img->stride != stride, "must be equal 7" );
}
END_TEST
//=====================================================================================================

//=====================================================================================================
/* G2dImage* g2d_image_create_data (G2dColorMode format, unsigned int width, unsigned int height,
                                            void* data, unsigned int stride) */
START_TEST ( g2d_image_create_data_dataNotZeroFailOndrmCommandWriteRead )
{
	G2dColorMode format;
	unsigned int width;
	unsigned int height;
	int data = 1;
	unsigned int stride;

	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );
	gCtx->drm_fd = -1;//This initialization using for breaking drmCommandWriteRead function

	G2dImage* res_img;

	res_img = g2d_image_create_data ( format, width, height, &data, stride);

	fail_if( res_img != NULL, "NULL must returned, fail in drmCommandWriteRead" );
	fail_if( ctrl_get_cnt_calloc() != 1, "Memory must free" );
}
END_TEST

START_TEST ( g2d_image_create_data_dataNotZero )
{
	G2dColorMode format;
	unsigned int width;
	unsigned int height;
	int data = 1;
	unsigned int stride;

	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );
	gCtx->drm_fd = 1;

	extern struct drm_exynos_gem_userptr stored_userptr;

	G2dImage* res_img;

	res_img = g2d_image_create_data ( format, width, height, &data, stride);

	fail_if( res_img == NULL, "NULL must returned, fail in drmCommandWriteRead" );
	fail_if( ctrl_get_cnt_calloc() != 2, "Memory must allocate" );
/*
    img->select_mode = G2D_SELECT_MODE_NORMAL;
    img->color_mode = format;
    img->width = width;
    img->height = height;
*/
	fail_if( res_img->select_mode != G2D_SELECT_MODE_NORMAL, "must be equal 1" );
	fail_if( res_img->color_mode != format, "must be equal 2" );
	fail_if( res_img->width != width, "must be equal 3" );
	fail_if( res_img->height != height, "must be equal 4" );
/*
    img->data.bo[0] = userptr.handle;
    img->need_free = 1;
*/
	fail_if( res_img->data.bo[0] != stored_userptr.handle, "Must be equal 5" );
	fail_if( res_img->need_free != 1, "Must be equal 6" );
}
END_TEST

START_TEST ( g2d_image_create_data_dataZeroFailInSwitch)
{
	G2dColorMode format = 0x54;//for receive fail in switch
	unsigned int width;
	unsigned int height;
	int data = 0;//for go under if( data )
	unsigned int stride;

	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );
	gCtx->drm_fd = 1;

	extern struct drm_exynos_gem_userptr stored_userptr;

	G2dImage* res_img;

	res_img = g2d_image_create_data ( format, width, height, data, stride );

	fail_if( res_img != NULL, "NULL must returned, fail in drmCommandWriteRead" );
	fail_if( ctrl_get_cnt_calloc() != 1, "Memory must allocate" );
}
END_TEST

START_TEST ( g2d_image_create_data_correctInit)
{
	G2dColorMode format = G2D_COLOR_FMT_A1;
	unsigned int width = 2;
	unsigned int height;
	int data = 0;
	unsigned int stride = 1;

	extern struct drm_exynos_gem_mmap stored_arg_map;
	extern struct drm_exynos_gem_create stored_arg;

	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );
	gCtx->drm_fd = 1;

	G2dImage* res_img;

	res_img = g2d_image_create_data ( format, width, height, data, stride);

	fail_if( res_img == NULL, "NULL must returned, fail in drmCommandWriteRead" );
	fail_if( ctrl_get_cnt_calloc() != 2, "Memory must allocate" );
/*
	img->mapped_ptr[0] = (void*)(unsigned long)arg_map.mapped;
*/
	fail_if( res_img->mapped_ptr[0] != stored_arg_map.mapped, "Must be equal 1" );
/*
	img->stride = stride;
    img->data.bo[0] = arg.handle;
    img->need_free = 1;
*/
	fail_if( res_img->stride != (width+7) / 8, "Must be equal 2" );
	fail_if( res_img->data.bo[0] != stored_arg.handle, "Must be equal 3" );
	fail_if( res_img->need_free != 1, "Must be equal 4" );
}
END_TEST
//=====================================================================================================

//=====================================================================================================
/* void g2d_image_free (G2dImage* img) */
START_TEST ( g2d_image_free_ImgNeed_FreeNotZerofailInIoctl )
{
	G2dImage* res_img = ctrl_calloc( 1, sizeof( G2dImage ) );
	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );

	int mem_cnt = ctrl_get_cnt_calloc();

	res_img->need_free = 1;
	gCtx->drm_fd = -1;

	g2d_image_free( res_img );
}
END_TEST

START_TEST ( g2d_image_free_ImgNeed_FreeNotZero )
{
	G2dImage* res_img = ctrl_calloc( 1, sizeof( G2dImage ) );
	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );

	extern struct drm_gem_close stored_arg_close;

	int mem_cnt = ctrl_get_cnt_calloc();

	res_img->need_free = 1;
	res_img->data.bo[0] = 2;
	gCtx->drm_fd = 1;

	int for_ck = res_img->data.bo[0] = 2;

	g2d_image_free( res_img );

	fail_if( ctrl_get_cnt_calloc() != 1, "Memory must free" );
	fail_if( stored_arg_close.handle != for_ck, "Must be equal" );
}
END_TEST
//=====================================================================================================

//=====================================================================================================
/* int g2d_set_src(G2dImage* img) */
START_TEST ( g2d_set_src_imgNULL )
{
	int res;

	res = g2d_set_src( NULL );

	fail_if( res != FALSE, "FALSE must returned" );
}
END_TEST

START_TEST ( g2d_set_src_img_Select_ModeZero )
{
	G2dImage* img = ctrl_calloc( 1, sizeof( G2dImage ) );
	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );
	int res;

	img->select_mode = -1;//Not supported value for switch

	res = g2d_set_src( img );

	fail_if( res != FALSE, "FALSE must returned" );
}
END_TEST

START_TEST ( g2d_set_src_img_DataBo2Zero )
{
	G2dImage* img = ctrl_calloc( 1, sizeof( G2dImage ) );
	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );
	int res;

	img->select_mode = G2D_SELECT_MODE_NORMAL;
	img->color_mode = 0x100;
	img->data.bo[1] = 0;

	res = g2d_set_src( img );
}
END_TEST

START_TEST ( g2d_set_src_img_correctInit )
{
	G2dImage* img = ctrl_calloc( 1, sizeof( G2dImage ) );
	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );
	int res;

	img->select_mode = G2D_SELECT_MODE_FGCOLOR;
	img->color_mode = 1;

	res = g2d_set_src( img );

	fail_if( res != TRUE, "TRUE must returned" );
}
END_TEST
//=====================================================================================================

//=====================================================================================================
/* int g2d_set_mask(G2dImage* img) */
START_TEST ( g2d_set_mask_imgNull )
{
	int res;

	res = g2d_set_mask( NULL );

	fail_if( res != FALSE, "FALSE must returned" );
}
END_TEST

START_TEST ( g2d_set_mask_imgSelect_ModeIsNotG2D_SELECT_MODE_NORMAL )
{
	G2dImage* img = ctrl_calloc( 1, sizeof( G2dImage ) );
	img->select_mode = -1;
	int res;

	res = g2d_set_mask( img );

	fail_if( res != FALSE, "FALSE must returned" );
}
END_TEST

START_TEST ( g2d_set_mask_ )
{
	G2dImage* img = ctrl_calloc( 1, sizeof( G2dImage ) );
	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );
	int res;

	img->select_mode = G2D_SELECT_MODE_NORMAL;
	img->color_mode = G2D_COLOR_FMT_A1;

	res = g2d_set_mask( img );

	fail_if( res != TRUE, "TRUE must returned" );
}
END_TEST
//=====================================================================================================

//=====================================================================================================
/* int g2d_set_dst(G2dImage* img) */
START_TEST ( g2d_set_dst_imgNull )
{
	int res;

	res = g2d_set_dst( NULL );

	fail_if( res != FALSE, "FALSE must returned" );
}
END_TEST

START_TEST ( g2d_set_dst_unsupportedImg_Select_ModeFailInSwitch )
{
	G2dImage* img = ctrl_calloc( 1, sizeof( G2dImage ) );
	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );
	int res;

	img->select_mode = -1;//unsupported value for switch

	res = g2d_set_dst( img );

	fail_if( res != FALSE, "FALSE must returned" );
}
END_TEST

START_TEST ( g2d_set_dst_correctInit )
{
	G2dImage* img = ctrl_calloc( 1, sizeof( G2dImage ) );
	gCtx = ctrl_calloc( 1, sizeof( G2dContext ) );
	int res;

	img->select_mode = G2D_SELECT_MODE_BGCOLOR;

	res = g2d_set_dst( img );

	fail_if( res != TRUE, "TRUE must returned" );
}
END_TEST
//=====================================================================================================

//=====================================================================================================
/* unsigned int g2d_get_blend_op(G2dOp op) */
START_TEST ( g2d_get_blend_op_unsupportedOp )
{
	int res;
	res = g2d_get_blend_op( -1 );//G2D_OP_CLEAR initialized 8:11 bits by 0x1

	fail_if( res != 0x00000100, "wrong initialization by default" );
}
END_TEST

START_TEST ( g2d_get_blend_op_correctInit )
{
	int res;
	res = g2d_get_blend_op( G2D_OP_CLEAR );//G2D_OP_CLEAR initialized 0:3 bits and 8:11 bits by 0x1

	fail_if( res != 0x00000101, "wrong initialization" );
}
END_TEST
//=====================================================================================================
//*****************************************************************************************************
int test_fimg2d_suite ( run_stat_data* stat )
{
	Suite *s = suite_create ( "FIMG2D.C" );
	/* Core test case */
	TCase *tc_g2d_clear              = tcase_create ("_g2d_clear");
	TCase *tc_g2d_init               = tcase_create ("g2d_init");
	TCase *tc_g2d_fini               = tcase_create ("g2d_fini");
	TCase *tc_g2d_add_cmd            = tcase_create ("g2d_add_cmd");
	TCase *tc_g2d_reset              = tcase_create ("g2d_reset");
	TCase *tc_g2d_exec               = tcase_create ("g2d_exec");
	TCase *tc_g2d_flush              = tcase_create ("g2d_flush");
	TCase *tc_g2d_image_new          = tcase_create ("g2d_flush");
	TCase *tc_g2d_image_create_solid = tcase_create ("g2d_flush");
	TCase *tc_g2d_image_create_bo    = tcase_create ("g2d_image_create_bo");
	TCase *tc_g2d_image_create_bo2   = tcase_create ("g2d_image_create_bo2");
	TCase *tc_g2d_image_create_data  = tcase_create ("g2d_image_create_data");
	TCase *tc_g2d_image_free         = tcase_create ("g2d_image_free");
	TCase *tc_g2d_set_src            = tcase_create ("g2d_set_src");
	TCase *tc_g2d_set_mask           = tcase_create ("g2d_set_mask");
	TCase *tc_g2d_set_dst            = tcase_create ("g2d_set_dst");
	TCase *tc_g2d_get_blend_op       = tcase_create ("g2d_get_blend_op");


	//_g2d_clear
  	tcase_add_test( tc_g2d_clear, _g2d_clear_test );

  	//g2d_init
  	tcase_add_test( tc_g2d_init, g2d_init_gCtxAlreadyInit );
  	tcase_add_test( tc_g2d_init, g2d_init_drmIoctlReturnNegativeValue );
  	tcase_add_test( tc_g2d_init, g2d_init_correctInit );

  	//g2d_fini
  	tcase_add_test( tc_g2d_fini, g2d_fini_memoryManagement );

  	//g2d_add_cmd
  	tcase_add_test( tc_g2d_add_cmd, g2d_add_cmd_overflowCmd_GemSize );
  	tcase_add_test( tc_g2d_add_cmd, g2d_add_cmd_correctInit );
  	tcase_add_test( tc_g2d_add_cmd, g2d_add_cmd_goToDefaultOverflowCmd_GemSize );
  	tcase_add_test( tc_g2d_add_cmd, g2d_add_cmd_goToDefaultCorrectInit );

	//g2d_reset
	tcase_add_test( tc_g2d_reset, g2d_reset_clear_RegZero );

	//g2d_exec
	tcase_add_test( tc_g2d_exec, g2d_exec_cmdlist_NrIsZero );
	tcase_add_test( tc_g2d_exec, g2d_exec_ioctlGivesNegativeValue );
	tcase_add_test( tc_g2d_exec, g2d_exec_correctInit );

	//g2d_flush
	tcase_add_test( tc_g2d_flush, g2d_flush_Cmd_NrAndCmd_Gem_NrAreZero );
	tcase_add_test( tc_g2d_flush, g2d_flush_drm_FdNegative );
	tcase_add_test( tc_g2d_flush, g2d_flush_correctInit );

	//g2d_image_new
	tcase_add_test( tc_g2d_image_new, g2d_image_new_initReturnedValue );

	//g2d_image_create_solid
	tcase_add_test( tc_g2d_image_create_solid, g2d_image_create_solid_initReturnedValue );

	//g2d_image_create_bo
	tcase_add_test( tc_g2d_image_create_bo, g2d_image_create_bo_boNotNegative );
	tcase_add_test( tc_g2d_image_create_bo, g2d_image_create_bo_unsupportedFormat );
	tcase_add_test( tc_g2d_image_create_bo, g2d_image_create_bo_failOndrmCommandWriteRead );
	tcase_add_test( tc_g2d_image_create_bo, g2d_image_create_bo_correctInit );

	//g2d_image_create_bo2
	tcase_add_test( tc_g2d_image_create_bo2, g2d_image_create_bo2_Bo1Zero );
	tcase_add_test( tc_g2d_image_create_bo2, g2d_image_create_bo2_Bo2Zero );
	tcase_add_test( tc_g2d_image_create_bo2, g2d_image_create_bo2_correctInit );

	//g2d_image_create_data
	tcase_add_test( tc_g2d_image_create_data, g2d_image_create_data_dataNotZeroFailOndrmCommandWriteRead );
	tcase_add_test( tc_g2d_image_create_data, g2d_image_create_data_dataNotZero );
	tcase_add_test( tc_g2d_image_create_data, g2d_image_create_data_dataZeroFailInSwitch );
	tcase_add_test( tc_g2d_image_create_data, g2d_image_create_data_correctInit );

	//g2d_image_free
	tcase_add_test( tc_g2d_image_free, g2d_image_free_ImgNeed_FreeNotZerofailInIoctl );
	tcase_add_test( tc_g2d_image_free, g2d_image_free_ImgNeed_FreeNotZero );

	//g2d_set_src
	tcase_add_test( tc_g2d_set_src, g2d_set_src_imgNULL );
	tcase_add_test( tc_g2d_set_src, g2d_set_src_img_Select_ModeZero );
	tcase_add_test( tc_g2d_set_src, g2d_set_src_img_DataBo2Zero );
	tcase_add_test( tc_g2d_set_src, g2d_set_src_img_correctInit );

	//g2d_set_mask
	tcase_add_test( tc_g2d_set_mask, g2d_set_mask_imgNull );
	tcase_add_test( tc_g2d_set_mask, g2d_set_mask_imgSelect_ModeIsNotG2D_SELECT_MODE_NORMAL );
	tcase_add_test( tc_g2d_set_mask, g2d_set_mask_ );

	//g2d_set_dst
	tcase_add_test( tc_g2d_set_dst, g2d_set_dst_imgNull);
	tcase_add_test( tc_g2d_set_dst, g2d_set_dst_unsupportedImg_Select_ModeFailInSwitch );
	tcase_add_test( tc_g2d_set_dst, g2d_set_dst_correctInit );

	//g2d_get_blend_op
	tcase_add_test( tc_g2d_get_blend_op, g2d_get_blend_op_unsupportedOp );
	tcase_add_test( tc_g2d_get_blend_op, g2d_get_blend_op_correctInit );


  	suite_add_tcase( s, tc_g2d_clear );
  	suite_add_tcase( s, tc_g2d_init );
  	suite_add_tcase( s, tc_g2d_fini );
  	suite_add_tcase( s, tc_g2d_add_cmd );
  	suite_add_tcase( s, tc_g2d_reset );
  	suite_add_tcase( s, tc_g2d_exec );
  	suite_add_tcase( s, tc_g2d_flush );
  	suite_add_tcase( s, tc_g2d_image_new );
  	suite_add_tcase( s, tc_g2d_image_create_solid );
  	suite_add_tcase( s, tc_g2d_image_create_bo );
  	suite_add_tcase( s, tc_g2d_image_create_bo2 );
  	suite_add_tcase( s, tc_g2d_image_create_data );
  	suite_add_tcase( s, tc_g2d_image_free );
  	suite_add_tcase( s, tc_g2d_set_src );
  	suite_add_tcase( s, tc_g2d_set_mask );
  	suite_add_tcase( s, tc_g2d_set_dst );
  	suite_add_tcase( s, tc_g2d_get_blend_op );


	SRunner *sr = srunner_create( s );
	srunner_run_all( sr, CK_VERBOSE );

	stat->num_tests = srunner_ntests_run( sr );
	stat->num_pass = srunner_ntests_run( sr ) - srunner_ntests_failed( sr );
	stat->num_failure = srunner_ntests_failed( sr );

	stat->num_tested = 17;
	stat->num_nottested = 0;

	return s;
}
