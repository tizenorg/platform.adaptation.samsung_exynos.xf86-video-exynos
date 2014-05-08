/*
 * test_crtc.c
 *
 *  Created on: Nov 8, 2013
 *      Author: svoloshynov
 */

#include <pixmap.h>
#include "test_crtc.h"
#include "exynos_driver.h"
#include "mem.h"
#include <list.h>
//#include "prepares.c"

//************************************************* Fake function ***************************************************
#define exynosDisplayModeToKmode fake_exynosDisplayModeToKmode
void fake_exynosDisplayModeToKmode(ScrnInfoPtr pScrn,
                      drmModeModeInfoPtr kmode,
                      DisplayModePtr pMode)
{
    memset (kmode, 0, sizeof (*kmode));

    kmode->clock = pMode->Clock;
    kmode->hdisplay = pMode->HDisplay;
    kmode->hsync_start = pMode->HSyncStart;
    kmode->hsync_end = pMode->HSyncEnd;
    kmode->htotal = pMode->HTotal;
    kmode->hskew = pMode->HSkew;

    kmode->vdisplay = pMode->VDisplay;
    kmode->vsync_start = pMode->VSyncStart;
    kmode->vsync_end = pMode->VSyncEnd;
    kmode->vtotal = pMode->VTotal;
    kmode->vscan = pMode->VScan;
    kmode->vrefresh = xf86ModeVRefresh (pMode);

    kmode->flags = pMode->Flags; //& FLAG_BITS;
}

#define drmModeAddFB (*drmModeAddFB_fake)
#define drmModeSetCrtc (*drmModeSetCrtc_fake)

int (*drmModeAddFB_fake)(int fd, uint32_t width, uint32_t height, uint8_t depth,
        uint8_t bpp, uint32_t pitch, uint32_t bo_handle, uint32_t *buf_id);

int (*drmModeSetCrtc_fake)(int fd, uint32_t crtcId, uint32_t bufferId, uint32_t x,
        uint32_t y, uint32_t *connectors, int count, drmModeModeInfoPtr mode);

#include "./display/exynos_crtc.c"

//Functions and variables which using for test initialization, they are located in tests/prepares.c
extern void initpScrnBySp_num( ScrnInfoPtr pScrn, int n );
extern void destroypScrn( ScrnInfoPtr pScrn );
extern int for_drm_fd;
extern int additonal_params_for_prepare_crtc;

#define PRINTVAL( val ) printf(" %d\n", val);

//****************************************** Functions For Initialization *******************************************
/*
int for_drm_fd = 0;
int additonal_params_for_prepare_crtc = 0;
void prepare_crtc(xf86CrtcPtr pCrtc){

    pCrtc->driver_private=ctrl_calloc(1,sizeof(EXYNOSCrtcPrivRec));
    pCrtc->scrn=ctrl_calloc(1,sizeof(ScrnInfoRec));

    xf86CrtcConfigPtr pXf86CrtcConfig = NULL;
    ScrnInfoPtr pS = pCrtc->scrn;
    pCrtc->scrn->pScreen=ctrl_calloc(1,sizeof(ScreenRec));
    pCrtc->scrn->driverPrivate= ctrl_calloc( 1, sizeof(EXYNOSRec) );

    EXYNOSRec* pExynos = pCrtc->scrn->driverPrivate;

    EXYNOSCrtcPrivPtr pCrtcPriv = pCrtc->driver_private;
    pCrtcPriv->pDispInfo = pExynos->pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );
    pCrtcPriv->pDispInfo->drm_fd = for_drm_fd;

    EXYNOSDispInfoPtr pDI = pCrtcPriv->pDispInfo;
    pDI->mode_res = ctrl_calloc( 1, sizeof( drmModeRes ) );
    pDI->plane_res = ctrl_calloc( 1, sizeof( drmModePlaneRes ) );


    //pDI->mode_res->count_connectors = 2;

    pCrtcPriv->rotate_bo = tbm_bo_alloc( 0, 1, 0);
    pExynos->default_bo = tbm_bo_alloc( 0, 1, 0);
    pExynos->drm_fd=1;
    int i = 0;
    switch ( additonal_params_for_prepare_crtc ) {
		case 101:
	    	xorg_list_init( &pCrtcPriv->link );
	    	pCrtcPriv->xdbg_fpsdebug = ctrl_calloc( 1, sizeof( drmModeCrtc ) );
	    	pCrtcPriv->mode_crtc = drmModeGetCrtc( 0, 0);
			break;

		case 102:
	    	pS->privates = ctrl_calloc( 3, sizeof( DevUnion ) );
	    	pXf86CrtcConfig = ctrl_calloc( 1, sizeof( xf86CrtcConfigRec ) );
	    	pS->privates[1].ptr = pXf86CrtcConfig;
	    	xf86CrtcConfigPtr pXCC = pS->privates[1].ptr;
	    	pXCC->num_output = 2;
	    	pXCC->output = (xf86OutputPtr*) ctrl_calloc( 2, sizeof( xf86OutputPtr ) );
	    	for( i = 0; i < pXCC->num_output; i ++ )
	    		pXCC->output[i] = ( xf86OutputPtr ) ctrl_calloc( 2, sizeof( xf86OutputRec ) );
			break;

		default:
			break;
	}
}

static void free_crtc(xf86CrtcPtr pCrtc){
    free(pCrtc->scrn->driverPrivate);
    ScrnInfoPtr pS = pCrtc->scrn;
    EXYNOSCrtcPrivPtr pCrtcPriv = pCrtc->driver_private;
    free(pCrtcPriv->pDispInfo);
    free(pCrtc->driver_private);

    int i = 0;
    if ( 101 == additonal_params_for_prepare_crtc )
        {
    	ctrl_free( pCrtcPriv->xdbg_fpsdebug );
        }
    else if( 102 == additonal_params_for_prepare_crtc )
    {
    	xf86CrtcConfigPtr pXCC = pS->privates[1].ptr;
    	for( i = 0; i < pXCC->num_output; i ++ )
    		ctrl_free( pXCC->output[i] );
    	ctrl_free( pXCC->output );
    	ctrl_free( pS->privates[1].ptr );
    	ctrl_free( pS->privates );
    	ctrl_free( pCrtcPriv->xdbg_fpsdebug );
    }
    free(pCrtc->scrn);
}
*/
int *ptr,*pp;

int forpScreenDestroyPixmap_check = 0;
Bool forpScreenDestroyPixmap(PixmapPtr pPixmap )
{
	forpScreenDestroyPixmap_check = 1;
	return 1;
}

static PixmapPtr forTest_exynosCrtcGetFlipPixmap( ScrnInfoPtr pScrn, int w, int h, int d, int usage_hint )
{
	if( 1 == usage_hint )
		return NULL;

	PixmapPtr pS = ctrl_calloc( 1, sizeof( PixmapRec ) );

	pS->screen_x = w;
	pS->screen_y = h;
	pS->usage_hint = usage_hint;

	return pS;
}

int drmModeAddFB_ret1(int fd, uint32_t width, uint32_t height, uint8_t depth,
        uint8_t bpp, uint32_t pitch, uint32_t bo_handle, uint32_t *buf_id){

    return 1;
}

int drmModeAddFB_ret0(int fd, uint32_t width, uint32_t height, uint8_t depth,
        uint8_t bpp, uint32_t pitch, uint32_t bo_handle, uint32_t *buf_id){

    return 0;
}

int drmModeAddFB_retNeg1(int fd, uint32_t width, uint32_t height, uint8_t depth,
        uint8_t bpp, uint32_t pitch, uint32_t bo_handle, uint32_t *buf_id){

    return -1;
}

int ck_forTest_gamma_set = 0;
void  forTest_gamma_set(xf86CrtcPtr crtc, CARD16 *red, CARD16 *green, CARD16 *blue, int size)
{
	ck_forTest_gamma_set = 1;
}

int drmModeSetCrtc_ret0(int fd, uint32_t crtcId, uint32_t bufferId, uint32_t x,
        uint32_t y, uint32_t *connectors, int count, drmModeModeInfoPtr mode) {
	return 0;
}

int drmModeSetCrtc_ret1(int fd, uint32_t crtcId, uint32_t bufferId, uint32_t x,
        uint32_t y, uint32_t *connectors, int count, drmModeModeInfoPtr mode) {
	return 1;
}
//********************************************** Unit tests *******************************************************
START_TEST (_EXYNOSCrtcShadowAllocate_Success)
{
    xf86CrtcRec Crtc;
    prepare_crtc(&Crtc);
    drmModeAddFB_fake = drmModeAddFB_ret1;
    void*ptr = EXYNOSCrtcShadowAllocate(&Crtc,100,100);
    fail_if(ptr==0,"should create");
    free_crtc(&Crtc);
}
END_TEST;


START_TEST (_EXYNOSCrtcShadowAllocate_Fail)
{
    void*ptr = EXYNOSCrtcShadowAllocate(0,100,100);
    fail_if(ptr!=0,"should not create");
}
END_TEST;


START_TEST (_EXYNOSCrtcShadowCreate_Success)
{
    xf86CrtcRec Crtc;
    PixmapPtr data=0;
    prepare_crtc(&Crtc);
    drmModeAddFB_fake = drmModeAddFB_ret1;
    void*ptr = EXYNOSCrtcShadowCreate(&Crtc,data,100,100);
    fail_if(ptr==0,"should create");
    free_crtc(&Crtc);
}
END_TEST;


START_TEST (_EXYNOSCrtcShadowCreate_Fail)
{
    PixmapPtr data;
    void*ptr = EXYNOSCrtcShadowCreate(0,&data,100,100);
    fail_if(ptr!=0,"should not create");
}
END_TEST;

//==============================================================================================

//==============================================================================================
/* static xf86CrtcPtr _exynosCrtcGetFromPipe (ScrnInfoPtr pScrn, int pipe) */
START_TEST ( exynosCrtcGetFromPipe_pipesAreNotEqual )
{
	int pipe = 1;
	ScrnInfoRec pScrn;
	initpScrnBySp_num( &pScrn, 1 );
    fail_if( _exynosCrtcGetFromPipe( &pScrn, pipe ) != NULL, "Pipes are not equal, NULL must returned" );
	destroypScrn( &pScrn );
}
END_TEST;

START_TEST ( exynosCrtcGetFromPipe_correctInit )
{
	int pipe = 1;

	ScrnInfoRec pScrn;
	initpScrnBySp_num( &pScrn, 2 );
    fail_if( _exynosCrtcGetFromPipe( &pScrn, pipe ) == NULL, "correct initialization, Crtc must returned" );
	destroypScrn( &pScrn );
}
END_TEST;
//==============================================================================================

//==============================================================================================
/* static void _flipPixmapInit (xf86CrtcPtr pCrtc) */
START_TEST ( flipPixmapInit_initReceivedValue )
{
	xf86CrtcRec pCrtc;
	prepare_crtc( &pCrtc );
    int mem_ck = ctrl_get_cnt_calloc();

    _flipPixmapInit ( &pCrtc );

	EXYNOSCrtcPrivPtr pCP = pCrtc.driver_private;
	fail_if( ctrl_get_cnt_calloc() != mem_ck + 3, "must be equal 1");
	fail_if( pCP->flip_backpixs.lub != -1, "must be equal 2");
	fail_if( pCP->flip_backpixs.num != 2, "must be equal 3");
	fail_if( pCP->flip_backpixs.pix_free == NULL, "must be equal 4");
	fail_if( pCP->flip_backpixs.flip_pixmaps == NULL, "must be equal 5");
	fail_if( pCP->flip_backpixs.flip_draws == NULL, "must be equal 6");
}
END_TEST;
//==============================================================================================

//==============================================================================================
/* static void _flipPixmapDeinit (xf86CrtcPtr pCrtc) */
START_TEST ( flipPixmapDeinit_ )
{
	xf86CrtcRec pCrtc;
	prepare_crtc( &pCrtc );
	int i = 0;
	EXYNOSCrtcPrivPtr pCP = pCrtc.driver_private;
	int flip_backbufs = 3 - 1;

	pCP->flip_backpixs.num = flip_backbufs;
	pCP->flip_backpixs.pix_free = ctrl_calloc (flip_backbufs, sizeof(void*));

	/* PixmapPtr *flip_pixmaps */
	pCP->flip_backpixs.flip_pixmaps = ctrl_calloc (flip_backbufs, sizeof( PixmapPtr ));
	for( i = 0; i < pCP->flip_backpixs.num; i++ )
		pCP->flip_backpixs.flip_pixmaps[i] = ctrl_calloc( flip_backbufs, sizeof( PixmapRec ) );

	pCP->flip_backpixs.flip_draws = ctrl_calloc (flip_backbufs, sizeof(void*));

	pCrtc.scrn->pScreen->DestroyPixmap = forpScreenDestroyPixmap ;

	_flipPixmapDeinit( &pCrtc );

	for( i = 0; i < pCP->flip_backpixs.num; i++ )
		fail_if( pCP->flip_backpixs.pix_free[i] != TRUE, "Must be TRUE");
	fail_if( forpScreenDestroyPixmap_check != 1, "fake variable must be initialized by 1");
	for( i = 0; i < pCP->flip_backpixs.num; i++ )
	{
		fail_if( pCP->flip_backpixs.flip_pixmaps[i] != NULL, "Must be NULL 1");
		fail_if( pCP->flip_backpixs.flip_draws[i] != NULL, "Must be NULL 2");
	}
}
END_TEST;
//==============================================================================================

//==============================================================================================
/* Bool exynosCrtcAllInUseFlipPixmap (ScrnInfoPtr pScrn, int crtc_pipe) */
START_TEST ( exynosCrtcAllInUseFlipPixmap_CrtcIsNull )
{
	int crtc_pipe = 1;
	ScrnInfoRec pScrn;
	initpScrnBySp_num( &pScrn, 1 );
	fail_if( exynosCrtcAllInUseFlipPixmap( &pScrn, crtc_pipe ) != FALSE, "There is free pixmap for flip, FALSE must returned" );
	destroypScrn( &pScrn );
}
END_TEST;

START_TEST ( exynosCrtcAllInUseFlipPixmap_FlipPixmap )
{
	int crtc_pipe = 1;

	ScrnInfoRec pScrn;
	initpScrnBySp_num( &pScrn, 4 );
	fail_if( exynosCrtcAllInUseFlipPixmap( &pScrn, crtc_pipe ) != FALSE, "There is free pixmap for flip, FALSE must returned" );
	destroypScrn( &pScrn );
}
END_TEST;

START_TEST ( exynosCrtcAllInUseFlipPixmap_noFlipPixmap )
{
	int crtc_pipe = 1;
	ScrnInfoRec pScrn;
	initpScrnBySp_num( &pScrn, 2 );
	fail_if( exynosCrtcAllInUseFlipPixmap( &pScrn, crtc_pipe ) != TRUE, "There is no free pixmap for flip, FALSE must returned" );
	destroypScrn( &pScrn );
}
END_TEST;
//==============================================================================================

//==============================================================================================
/* void exynosCrtcExemptAllFlipPixmap (ScrnInfoPtr pScrn, int crtc_pipe) */
START_TEST ( exynosCrtcExemptAllFlipPixmap_pCrtcIsNull )
{
	ScrnInfoRec pScrn;
	int crtc_pipe = 1;
	initpScrnBySp_num( &pScrn, 1 );//Special initialize to make pCrtc NULL
	exynosCrtcExemptAllFlipPixmap( &pScrn, crtc_pipe );
	destroypScrn( &pScrn );
}
END_TEST;

START_TEST ( exynosCrtcExemptAllFlipPixmap_correctInit )
{
	ScrnInfoRec pScrn;
	int crtc_pipe = 1;
	initpScrnBySp_num( &pScrn, 3 );
	exynosCrtcExemptAllFlipPixmap( &pScrn, crtc_pipe );

	xf86CrtcConfigPtr pXCC = NULL;
	xf86CrtcPtr pCrtc = NULL;
	EXYNOSCrtcPrivPtr pCrtcPriv = NULL;
	pXCC = pScrn.privates[1].ptr;
	int i = 0;
	pCrtc = pXCC->crtc[0];
	pCrtcPriv = pCrtc->driver_private;
	for( i = 0; i < 2; i ++ )
		fail_if( pCrtcPriv->flip_backpixs.pix_free[i] != TRUE, "Must be TRUE" );

	destroypScrn( &pScrn );
}
END_TEST;
//==============================================================================================

//==============================================================================================
/* PixmapPtr exynosCrtcGetFlipPixmap (ScrnInfoPtr pScrn, int crtc_pipe, DrawablePtr pDraw, unsigned int usage_hint) */
START_TEST ( exynosCrtcGetFlipPixmap_pCrtcIsNull )
{
	ScrnInfoRec pScrn;
	int crtc_pipe = 1;
	DrawableRec pDraw;
	unsigned int usage_hint = 1;

	initpScrnBySp_num( &pScrn, 1 );

	fail_if( exynosCrtcGetFlipPixmap( &pScrn, crtc_pipe, &pDraw, usage_hint ) != FALSE, "pCrtc is Null, FALSE must be returned" );

	destroypScrn( &pScrn );
}
END_TEST;

START_TEST ( exynosCrtcGetFlipPixmap_flip_countNotZero )
{
	ScrnInfoRec pScrn;
	int crtc_pipe = 1;
	DrawableRec pDraw;
	unsigned int usage_hint = 1;

	initpScrnBySp_num( &pScrn, 3 );
	fail_if( exynosCrtcGetFlipPixmap( &pScrn, crtc_pipe, &pDraw, usage_hint ) != FALSE, "pCrtc is Null, FALSE must be returned" );
	destroypScrn( &pScrn );
}
END_TEST;

START_TEST ( exynosCrtcGetFlipPixmap_correctInit )
{
	ScrnInfoRec pScrn;
	int crtc_pipe = 1;
	DrawableRec pDraw;
	unsigned int usage_hint = 1;

	initpScrnBySp_num( &pScrn, 4 );
	fail_if( exynosCrtcGetFlipPixmap( &pScrn, crtc_pipe, &pDraw, usage_hint ) == NULL, "correctInit, TRUE must be returned" );

	xf86CrtcConfigPtr pXCC = NULL;
	xf86CrtcPtr pCrtc = NULL;
	EXYNOSCrtcPrivPtr pCrtcPriv = NULL;
	pXCC = pScrn.privates[1].ptr;
	int i = 0;
	pCrtc = pXCC->crtc[0];
	pCrtcPriv = pCrtc->driver_private;
	fail_if( pCrtcPriv->flip_backpixs.pix_free[1] != FALSE, "Must be FALSE" );
	fail_if( pCrtcPriv->flip_backpixs.lub != 1, "must be 1" );

	destroypScrn( &pScrn );
}
END_TEST;

START_TEST ( exynosCrtcGetFlipPixmap_pPixmapIsNull )
{
	ScrnInfoRec pScrn;
	int crtc_pipe = 1;
	DrawableRec pDraw;
	unsigned int usage_hint = 1;

	pScrn.pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
	pScrn.pScreen->CreatePixmap = forTest_exynosCrtcGetFlipPixmap;
	initpScrnBySp_num( &pScrn, 5 );
	fail_if( exynosCrtcGetFlipPixmap( &pScrn, crtc_pipe, &pDraw, usage_hint ) != NULL, "pPixmap is NULL, FALSE must be returned" );
	destroypScrn( &pScrn );
}
END_TEST;

START_TEST ( exynosCrtcGetFlipPixmap_correctInit_2 )
{
	ScrnInfoRec pScrn;
	int crtc_pipe = 1;
	DrawableRec pDraw;
    pDraw.width = 1;
    pDraw.height = 2;
    pDraw.depth = 3;

	unsigned int usage_hint = 3;

	PixmapPtr res = NULL;

	pScrn.pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
	pScrn.pScreen->CreatePixmap = forTest_exynosCrtcGetFlipPixmap;
	initpScrnBySp_num( &pScrn, 5 );

	res = exynosCrtcGetFlipPixmap( &pScrn, crtc_pipe, &pDraw, usage_hint );
	fail_if( res == NULL, "correctInit_2, TRUE must be returned" );
	fail_if( res->screen_x != 1, "must be equal 1" );
	fail_if( res->screen_y != 2, "must be equal 2" );
	fail_if( res->usage_hint != 3, "must be equal 3" );

	destroypScrn( &pScrn );
}
END_TEST;
//==============================================================================================

//==============================================================================================
/* void exynosCrtcExemptFlipPixmap (ScrnInfoPtr pScrn, int crtc_pipe, PixmapPtr pPixmap) */
START_TEST ( exynosCrtcExemptFlipPixmap_pCrtcIsNull )
{
	ScrnInfoRec pScrn;
	int crtc_pipe = 2;
	PixmapPtr pPixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );

	initpScrnBySp_num( &pScrn, 2 );

	//Segmaentaion fault must not occur
	exynosCrtcExemptFlipPixmap( &pScrn, crtc_pipe, pPixmap );
	destroypScrn( &pScrn );
}
END_TEST;

START_TEST ( exynosCrtcExemptFlipPixmap_IfNotEqual )
{
	ScrnInfoRec pScrn;
	int crtc_pipe = 1;
	PixmapPtr pPixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );

	initpScrnBySp_num( &pScrn, 3 );

	exynosCrtcExemptFlipPixmap( &pScrn, crtc_pipe, pPixmap );

	xf86CrtcConfigPtr pXCC = NULL;
	xf86CrtcPtr pCrtc = NULL;
	EXYNOSCrtcPrivPtr pCrtcPriv = NULL;
	pXCC = pScrn.privates[1].ptr;
	int i = 0;
	pCrtc = pXCC->crtc[0];
	pCrtcPriv = pCrtc->driver_private;
	for( i = 0; i < 2; i++ )
		fail_if( pCrtcPriv->flip_backpixs.pix_free[i] != FALSE, "Must be FALSE" );
	destroypScrn( &pScrn );
}
END_TEST;

START_TEST ( exynosCrtcExemptFlipPixmap_correctInit )
{
	ScrnInfoRec pScrn;
	int crtc_pipe = 1;
	initpScrnBySp_num( &pScrn, 3 );
	xf86CrtcConfigPtr pXCC = NULL;
	xf86CrtcPtr pCrtc = NULL;
	EXYNOSCrtcPrivPtr pCrtcPriv = NULL;
	pXCC = pScrn.privates[1].ptr;
	pCrtc = pXCC->crtc[0];
	pCrtcPriv = pCrtc->driver_private;

	PixmapPtr pPixmap = pCrtcPriv->flip_backpixs.flip_pixmaps[0];

	exynosCrtcExemptFlipPixmap( &pScrn, crtc_pipe, pPixmap );

	fail_if( pCrtcPriv->flip_backpixs.pix_free[0] != TRUE, "must be TRUE" );
	destroypScrn( &pScrn );
}
END_TEST;
//==============================================================================================



//==============================================================================================
/* static void EXYNOSCrtcShadowDestroy(xf86CrtcPtr pCrtc, PixmapPtr rotate_pixmap,
        void *data) */
START_TEST ( EXYNOSCrtcShadowDestroy_dataIsNull )
{
	xf86CrtcRec pCrtc;
	PixmapPtr rotate_pixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );
	void *data = (void*)0;

	prepare_crtc( &pCrtc );
	extern int test_FreeScratchPixmapHeader;

	EXYNOSCrtcShadowDestroy( &pCrtc, rotate_pixmap, data );

	fail_if( test_FreeScratchPixmapHeader != 1, "must be 1" );
}
END_TEST;

START_TEST ( EXYNOSCrtcShadowDestroy_dataNotNull )
{
	xf86CrtcRec pCrtc;
	PixmapPtr rotate_pixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );
	void *data = (void*)1;
	prepare_crtc( &pCrtc );
	extern int test_FreeScratchPixmapHeader;
	int drm_mem_ck = ctrl_get_cnt_mem_obj( TBM_OBJ );

	EXYNOSCrtcShadowDestroy( &pCrtc, rotate_pixmap, data );

	fail_if( test_FreeScratchPixmapHeader != 1, "must be 1" );
	fail_if(  ctrl_get_cnt_mem_obj( TBM_OBJ ) != drm_mem_ck - 1, "DRM buffer must unref" );
}
END_TEST;
//==============================================================================================

//==============================================================================================
/* static void EXYNOSCrtcDestroy(xf86CrtcPtr pCrtc) */
START_TEST ( EXYNOSCrtcDestroy_memoryManagment )
{
	xf86CrtcRec pCrtc;
	additonal_params_for_prepare_crtc = 101;
	prepare_crtc( &pCrtc );

	int calloc_mem_ck = ctrl_get_cnt_calloc();
	EXYNOSCrtcDestroy( &pCrtc );
	fail_if( pCrtc.driver_private != NULL, "must be NULL" );
	fail_if( ctrl_get_cnt_calloc() != calloc_mem_ck - 1, "Memory must be free" );
	fail_if( ctrl_get_cnt_mem_obj( DRM_OBJ ) != 0, "DRM object must be free" );
}
END_TEST;
//==============================================================================================

//==============================================================================================
/* static Bool EXYNOSCrtcSetModeMajor(xf86CrtcPtr pCrtc, DisplayModePtr pMode,
                    Rotation rotation, int x, int y) */

START_TEST ( EXYNOSCrtcSetModeMajor_1)
{
	xf86CrtcRec pCrtc;

	for_drm_fd = 11;//for initialization ret variable by not zero value
	prepare_crtc( &pCrtc );
	pCrtc.x = 1;
	pCrtc.y = 2;
	DisplayModePtr pMode = ctrl_calloc( 1, sizeof( DisplayModeRec ) );
	pMode->name = NULL;
	Rotation rotation;
	int x = 1;
	int y = 2;
	drmModeAddFB_fake=drmModeAddFB_ret1;
	Bool ret = EXYNOSCrtcSetModeMajor( &pCrtc, pMode, rotation, x, y );
	fail_if( ret == 0, "FAIL must returned" );
	fail_if( pCrtc.x != 0, "Must be equal 1" );
	fail_if( pCrtc.y != 0, "Must be equal 2" );
}
END_TEST;

START_TEST ( EXYNOSCrtcSetModeMajor_0 )
{
    xf86CrtcRec pCrtc;

    for_drm_fd = 11;//for initialization ret variable by not zero value
    additonal_params_for_prepare_crtc = 102;
    prepare_crtc( &pCrtc );

    DisplayModePtr pMode = ctrl_calloc( 1, sizeof( DisplayModeRec ) );
    pMode->name = NULL;
    pMode->Clock = 1;
    pMode->HDisplay = 2;
    pMode->HSyncStart = 3;
    pMode->HSyncEnd = 4;
    pMode->HTotal = 5;
    pMode->HSkew = 6;

    Rotation rotation;
    int x = 1;
    int y = 2;

	pCrtc.funcs = ctrl_calloc( 1, sizeof( xf86CrtcFuncsRec ) );
	pCrtc.gamma_red = ctrl_calloc( 1, sizeof( CARD16 ) );
	pCrtc.gamma_green = ctrl_calloc( 1, sizeof( CARD16 ) );
	pCrtc.gamma_blue = ctrl_calloc( 1, sizeof( CARD16 ) );

	//pCrtc.funcs->gamma_set = forTest_gamma_set;
	xf86CrtcFuncsRec* pf = pCrtc.funcs;
	pf->gamma_set = forTest_gamma_set;

	drmModeSetCrtc_fake = drmModeSetCrtc_ret0;
    drmModeAddFB_fake=drmModeAddFB_ret0;

    Bool ret = EXYNOSCrtcSetModeMajor( &pCrtc, pMode, rotation, x, y );

    fail_if( ret != 1, "TRUE must returned" );
    fail_if( ck_forTest_gamma_set != 1, "Must be initialized by 1" );
	fail_if( pCrtc.x != 1, "Must be equal 1" );
	fail_if( pCrtc.y != 2, "Must be equal 2" );
/*
    kmode->clock = pMode->Clock;
    kmode->hdisplay = pMode->HDisplay;
    kmode->hsync_start = pMode->HSyncStart;
    kmode->hsync_end = pMode->HSyncEnd;
    kmode->htotal = pMode->HTotal;
    kmode->hskew = pMode->HSkew;
*/
    ScrnInfoPtr pScrn = pCrtc.scrn;
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    EXYNOSCrtcPrivPtr pCrtcPriv = pCrtc.driver_private;

    fail_if( pCrtcPriv->kmode.clock != 1, "Must be equal 1" );
    fail_if( pCrtcPriv->kmode.hdisplay != 2, "Must be equal 2" );
    fail_if( pCrtcPriv->kmode.hsync_start != 3, "Must be equal 3" );
    fail_if( pCrtcPriv->kmode.hsync_end != 4, "Must be equal 4" );
    fail_if( pCrtcPriv->kmode.htotal != 5, "Must be equal 5" );
    fail_if( pCrtcPriv->kmode.hskew != 6, "Must be equal 6" );

    free_crtc( &pCrtc );
}
END_TEST;

/*
void ctest(){
    xf86CrtcRec pCrtc;

        for_drm_fd = 11;//for initialization ret variable by not zero value
        prepare_crtc( &pCrtc );
        DisplayModePtr pMode = ctrl_calloc( 1, sizeof( DisplayModeRec ) );
        pMode->name = NULL;
        Rotation rotation;
        int x = 1;
        int y = 2;

        drmModeAddFB_fake=drmModeAddFB_ret0;
        Bool ret = EXYNOSCrtcSetModeMajor( &pCrtc, pMode, rotation, x, y );
}
*/
//==============================================================================================

//==============================================================================================
/* Bool exynosCrtcApply(xf86CrtcPtr pCrtc) */

START_TEST ( exynosCrtcApply_retIsNotZero )
{
	xf86CrtcRec pCrtc;

	additonal_params_for_prepare_crtc = 102;

	prepare_crtc( &pCrtc );
	pCrtc.funcs = ctrl_calloc( 1, sizeof( xf86CrtcFuncsRec ) );
	pCrtc.gamma_red = ctrl_calloc( 1, sizeof( CARD16 ) );
	pCrtc.gamma_green = ctrl_calloc( 1, sizeof( CARD16 ) );
	pCrtc.gamma_blue = ctrl_calloc( 1, sizeof( CARD16 ) );

	//pCrtc.funcs->gamma_set = forTest_gamma_set;
	xf86CrtcFuncsRec* pf = pCrtc.funcs;
	pf->gamma_set = forTest_gamma_set;

	drmModeSetCrtc_fake = drmModeSetCrtc_ret1;

	fail_if( exynosCrtcApply( &pCrtc ), "FALSE must return" );

	fail_if( ck_forTest_gamma_set != 1, "Must be initialized by 1" );

	free_crtc( &pCrtc );
}
END_TEST;

START_TEST ( exynosCrtcApply_corretcInit )
{
	xf86CrtcRec pCrtc;

	additonal_params_for_prepare_crtc = 102;

	prepare_crtc( &pCrtc );
	pCrtc.funcs = ctrl_calloc( 1, sizeof( xf86CrtcFuncsRec ) );
	pCrtc.gamma_red = ctrl_calloc( 1, sizeof( CARD16 ) );
	pCrtc.gamma_green = ctrl_calloc( 1, sizeof( CARD16 ) );
	pCrtc.gamma_blue = ctrl_calloc( 1, sizeof( CARD16 ) );

	//pCrtc.funcs->gamma_set = forTest_gamma_set;
	xf86CrtcFuncsRec* pf = pCrtc.funcs;
	pf->gamma_set = forTest_gamma_set;

	drmModeSetCrtc_fake = drmModeSetCrtc_ret0;

	fail_if( !exynosCrtcApply( &pCrtc ), "FALSE must return" );

	fail_if( ck_forTest_gamma_set != 1, "Must be initialized by 1" );

	free_crtc( &pCrtc );
}
END_TEST;
//==============================================================================================

//==============================================================================================
/* void exynosCrtcInit (ScrnInfoPtr pScrn, EXYNOSDispInfoPtr pDispInfo, int num) */
START_TEST ( exynosCrtcInit_pCrtcIsNull )
{
	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	EXYNOSDispInfoPtr pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );

	initpScrnBySp_num( pScrn, 5 );
	pScrn->numClocks = 101;
	int mem_calloc_ck = ctrl_get_cnt_calloc();

	exynosCrtcInit( pScrn, pDispInfo, 1 );

	fail_if( ctrl_get_cnt_calloc() != mem_calloc_ck, "Memory must free" );

	destroypScrn( pScrn );
}
END_TEST;

START_TEST ( exynosCrtcInit_kcrtcIsNull )
{
	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	initpScrnBySp_num( pScrn, 5 );

	extern int forTest_xf86CrtcDestroy;

	EXYNOSDispInfoPtr pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );
	pDispInfo->mode_res = ctrl_calloc( 1, sizeof( drmModeRes ) );
	pDispInfo->mode_res->crtcs = ctrl_calloc( 1, sizeof( uint32_t ) );
	pDispInfo->drm_fd = 101;

	int mem_calloc_ck = ctrl_get_cnt_calloc();

	exynosCrtcInit( pScrn, pDispInfo, 1 );

	fail_if( forTest_xf86CrtcDestroy != 1, "Must be initialized by 1" );
	fail_if( ctrl_get_cnt_calloc() != mem_calloc_ck + 8, "Memory must free" );

	destroypScrn( pScrn );
}
END_TEST;

START_TEST ( exynosCrtcInit_correctInit )
{
	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	initpScrnBySp_num( pScrn, 1 );

	EXYNOSDispInfoPtr pDispInfo = ctrl_calloc( 1, sizeof( EXYNOSDispInfoRec ) );
	pDispInfo->mode_res = ctrl_calloc( 1, sizeof( drmModeRes ) );
	pDispInfo->mode_res->crtcs = ctrl_calloc( 1, sizeof( uint32_t ) );
	//pDispInfo->crtcs = ctrl_calloc( 1, sizeof( xorg_list ) );

	int mem_calloc_ck = ctrl_get_cnt_calloc();

	xorg_list_init( &pDispInfo->crtcs );

	exynosCrtcInit( pScrn, pDispInfo, 1 );

	fail_if( ctrl_get_cnt_calloc() != mem_calloc_ck + 12, "Memory must allocated" );

	destroypScrn( pScrn );
}
END_TEST;
//==============================================================================================

//==============================================================================================
/* DRI2FrameEventPtr exynosCrtcGetFirstPendingFlip (xf86CrtcPtr pCrtc) */
START_TEST ( exynosCrtcGetFirstPendingFlip_pCrtcIsNull )
{
	fail_if( exynosCrtcGetFirstPendingFlip( NULL ), "pCrtc is null, NULL must returned" );
}
END_TEST;

START_TEST ( exynosCrtcGetFirstPendingFlip_listIsEmpty )
{
	xf86CrtcPtr pCrtc = ctrl_calloc( 1, sizeof( xf86CrtcRec ) );
	pCrtc->driver_private = ctrl_calloc( 1, sizeof( EXYNOSCrtcPrivRec ) );
	EXYNOSCrtcPrivPtr pCC = pCrtc->driver_private;

	xorg_list_init( &pCC->pending_flips );

	fail_if( exynosCrtcGetFirstPendingFlip( pCrtc ), "List is empty, NULL must returned" );
}
END_TEST;

START_TEST ( exynosCrtcGetFirstPendingFlip_corectInit )
{
	xf86CrtcPtr pCrtc = ctrl_calloc( 1, sizeof( xf86CrtcRec ) );
	pCrtc->driver_private = ctrl_calloc( 1, sizeof( EXYNOSCrtcPrivRec ) );
	EXYNOSCrtcPrivPtr pCC = pCrtc->driver_private;

	xorg_list_init( &pCC->pending_flips );
	xorg_list_add(&pCC->link, &pCC->pending_flips);

	fail_if( !exynosCrtcGetFirstPendingFlip( pCrtc ), "CorrectInit, Item must returned" );
}
END_TEST;
//==============================================================================================


//==============================================================================================
/* void exynosCrtcRemovePendingFlip (xf86CrtcPtr pCrtc, DRI2FrameEventPtr pEvent) */
START_TEST ( exynosCrtcRemovePendingFlip_pCrtcIsNull )
{
	//Segmentation fault must not occur
	exynosCrtcRemovePendingFlip( NULL, NULL );
}
END_TEST;

START_TEST ( exynosCrtcRemovePendingFlip_corectInit )
{
	xf86CrtcPtr pCrtc = ctrl_calloc( 1, sizeof( xf86CrtcRec ) );
	pCrtc->driver_private = ctrl_calloc( 1, sizeof( EXYNOSCrtcPrivRec ) );
	EXYNOSCrtcPrivPtr pCC = pCrtc->driver_private;
	DRI2FrameEventPtr pEvent = ctrl_calloc( 1, sizeof( DRI2FrameEventRec ) );


	xorg_list_init( &pCC->pending_flips );
	xorg_list_add( &pEvent->crtc_pending_link, &pCC->pending_flips );

	exynosCrtcRemovePendingFlip( pCrtc, pEvent );

	fail_if( !xorg_list_is_empty( &pCC->pending_flips ) != 0, "list Must be empty" );
}
END_TEST;
//==============================================================================================

//==============================================================================================
/* void secCrtcAddPendingFlip (xf86CrtcPtr pCrtc, DRI2FrameEventPtr pEvent) */
START_TEST ( secCrtcAddPendingFlip_pCrtcIsNull )
{
	//Segmentation fault must not occur
	secCrtcAddPendingFlip( NULL, NULL );
}
END_TEST;

START_TEST ( secCrtcAddPendingFlip_corectInit )
{
	xf86CrtcPtr pCrtc = ctrl_calloc( 1, sizeof( xf86CrtcRec ) );
	pCrtc->driver_private = ctrl_calloc( 1, sizeof( EXYNOSCrtcPrivRec ) );
	EXYNOSCrtcPrivPtr pCC = pCrtc->driver_private;
	DRI2FrameEventPtr pEvent = ctrl_calloc( 1, sizeof( DRI2FrameEventRec ) );


	xorg_list_init( &pCC->pending_flips );
	xorg_list_add( &pEvent->crtc_pending_link, &pCC->pending_flips );

	secCrtcAddPendingFlip( pCrtc, pEvent );

	fail_if( xorg_list_is_empty( &pCC->pending_flips ) != 0, "list Must not be empty" );
}
END_TEST;
//==============================================================================================
int test_crtc_suite(run_stat_data* stat) {

    Suite *s = suite_create("CRTC");
    /* Core test case */
    //ctest();

    TCase *tcrtc_s_core = tcase_create("EXYNOSCrtcShadowAllocate");
    tcase_add_test(tcrtc_s_core, _EXYNOSCrtcShadowAllocate_Success);
    tcase_add_test(tcrtc_s_core, _EXYNOSCrtcShadowAllocate_Fail);
    tcase_add_test(tcrtc_s_core, _EXYNOSCrtcShadowCreate_Success);
    tcase_add_test(tcrtc_s_core, _EXYNOSCrtcShadowCreate_Fail);
    suite_add_tcase(s, tcrtc_s_core);

    //_exynosCrtcGetFromPipe
    TCase *tc_exynosCrtcGetFromPipe = tcase_create("_exynosCrtcGetFromPipe");
    tcase_add_test( tc_exynosCrtcGetFromPipe, exynosCrtcGetFromPipe_pipesAreNotEqual );
    tcase_add_test( tc_exynosCrtcGetFromPipe, exynosCrtcGetFromPipe_correctInit );
    suite_add_tcase(s, tc_exynosCrtcGetFromPipe );

    //_flipPixmapInit
    TCase *tc_flipPixmapInit = tcase_create("_flipPixmapInit");
    tcase_add_test( tc_flipPixmapInit, flipPixmapInit_initReceivedValue );
    suite_add_tcase(s, tc_flipPixmapInit);

    //_flipPixmapDeinit
    TCase *tc_flipPixmapDeinit = tcase_create("_flipPixmapDeinit");
    tcase_add_test( tc_flipPixmapDeinit, flipPixmapDeinit_ );
    suite_add_tcase(s, tc_flipPixmapDeinit );

    //exynosCrtcAllInUseFlipPixmap
    TCase *tc_exynosCrtcAllInUseFlipPixmap = tcase_create("exynosCrtcAllInUseFlipPixmap");
    tcase_add_test( tc_exynosCrtcAllInUseFlipPixmap, exynosCrtcAllInUseFlipPixmap_CrtcIsNull );
    tcase_add_test( tc_exynosCrtcAllInUseFlipPixmap, exynosCrtcAllInUseFlipPixmap_FlipPixmap );
    tcase_add_test( tc_exynosCrtcAllInUseFlipPixmap, exynosCrtcAllInUseFlipPixmap_noFlipPixmap );
    suite_add_tcase(s, tc_exynosCrtcAllInUseFlipPixmap );

    //_flipPixmapDeinit
    TCase *tc_exynosCrtcExemptAllFlipPixmap = tcase_create("exynosCrtcExemptAllFlipPixmap");
    tcase_add_test( tc_exynosCrtcExemptAllFlipPixmap, exynosCrtcExemptAllFlipPixmap_pCrtcIsNull );
    tcase_add_test( tc_exynosCrtcExemptAllFlipPixmap, exynosCrtcExemptAllFlipPixmap_correctInit );
    suite_add_tcase(s, tc_exynosCrtcExemptAllFlipPixmap );

    //_exynosCrtcGetFlipPixmap
    TCase *tc_exynosCrtcGetFlipPixmap = tcase_create("exynosCrtcGetFlipPixmap");
    tcase_add_test( tc_exynosCrtcGetFlipPixmap, exynosCrtcGetFlipPixmap_pCrtcIsNull );
    tcase_add_test( tc_exynosCrtcGetFlipPixmap, exynosCrtcGetFlipPixmap_flip_countNotZero );
    tcase_add_test( tc_exynosCrtcGetFlipPixmap, exynosCrtcGetFlipPixmap_correctInit );
    tcase_add_test( tc_exynosCrtcGetFlipPixmap, exynosCrtcGetFlipPixmap_pPixmapIsNull );
    tcase_add_test( tc_exynosCrtcGetFlipPixmap, exynosCrtcGetFlipPixmap_correctInit_2 );
    suite_add_tcase(s, tc_exynosCrtcGetFlipPixmap );

    //exynosCrtcExemptFlipPixmap
    TCase *tc_exynosCrtcExemptFlipPixmap = tcase_create("exynosCrtcExemptFlipPixmap");
    tcase_add_test( tc_exynosCrtcExemptFlipPixmap, exynosCrtcExemptFlipPixmap_pCrtcIsNull );
    tcase_add_test( tc_exynosCrtcExemptFlipPixmap, exynosCrtcExemptFlipPixmap_IfNotEqual );
    tcase_add_test( tc_exynosCrtcExemptFlipPixmap, exynosCrtcExemptFlipPixmap_correctInit );
    suite_add_tcase(s, tc_exynosCrtcExemptFlipPixmap );

    //EXYNOSCrtcSetModeMajor
    TCase *tc_EXYNOSCrtcSetModeMajor = tcase_create("EXYNOSCrtcSetModeMajor");
    tcase_add_test( tc_EXYNOSCrtcSetModeMajor, EXYNOSCrtcSetModeMajor_1 );
    tcase_add_test( tc_EXYNOSCrtcSetModeMajor, EXYNOSCrtcSetModeMajor_0 );
    suite_add_tcase(s, tc_EXYNOSCrtcSetModeMajor );

    //EXYNOSCrtcShadowDestroy
    TCase *tc_EXYNOSCrtcShadowDestroy = tcase_create("EXYNOSCrtcShadowDestroy");
    tcase_add_test( tc_EXYNOSCrtcShadowDestroy, EXYNOSCrtcShadowDestroy_dataIsNull );
    tcase_add_test( tc_EXYNOSCrtcShadowDestroy, EXYNOSCrtcShadowDestroy_dataNotNull );
    suite_add_tcase(s, tc_EXYNOSCrtcShadowDestroy );

    //EXYNOSCrtcDestroy
    TCase *tc_EXYNOSCrtcDestroy = tcase_create("EXYNOSCrtcDestroy");
    tcase_add_test( tc_EXYNOSCrtcDestroy, EXYNOSCrtcDestroy_memoryManagment );
    suite_add_tcase(s, tc_EXYNOSCrtcDestroy );

    //exynosCrtcApply
    TCase *tc_exynosCrtcApply = tcase_create("exynosCrtcApply");
    tcase_add_test( tc_exynosCrtcApply, exynosCrtcApply_retIsNotZero );
    tcase_add_test( tc_exynosCrtcApply, exynosCrtcApply_corretcInit);
    suite_add_tcase(s, tc_exynosCrtcApply );

    //exynosCrtcInit
    TCase *tc_exynosCrtcInit = tcase_create("exynosCrtcInit");
    tcase_add_test( tc_exynosCrtcInit, exynosCrtcInit_pCrtcIsNull );
    tcase_add_test( tc_exynosCrtcInit, exynosCrtcInit_kcrtcIsNull );
    tcase_add_test( tc_exynosCrtcInit, exynosCrtcInit_correctInit );
    suite_add_tcase(s, tc_exynosCrtcInit );

    //exynosCrtcGetFirstPendingFlip
    TCase *tc_exynosCrtcGetFirstPendingFlip = tcase_create("exynosCrtcGetFirstPendingFlip");
    tcase_add_test( tc_exynosCrtcGetFirstPendingFlip, exynosCrtcGetFirstPendingFlip_pCrtcIsNull );
    tcase_add_test( tc_exynosCrtcGetFirstPendingFlip, exynosCrtcGetFirstPendingFlip_listIsEmpty );
    tcase_add_test( tc_exynosCrtcGetFirstPendingFlip, exynosCrtcGetFirstPendingFlip_corectInit );
    suite_add_tcase(s, tc_exynosCrtcGetFirstPendingFlip );

    //exynosCrtcRemovePendingFlip
    TCase *tc_exynosCrtcRemovePendingFlip = tcase_create("exynosCrtcRemovePendingFlip");
    tcase_add_test( tc_exynosCrtcRemovePendingFlip, exynosCrtcRemovePendingFlip_pCrtcIsNull );
    tcase_add_test( tc_exynosCrtcRemovePendingFlip, exynosCrtcRemovePendingFlip_corectInit );
    suite_add_tcase(s, tc_exynosCrtcRemovePendingFlip );

    //secCrtcAddPendingFlip
    TCase *tc_secCrtcAddPendingFlip = tcase_create("secCrtcAddPendingFlip");
    tcase_add_test( tc_secCrtcAddPendingFlip, secCrtcAddPendingFlip_pCrtcIsNull );
    tcase_add_test( tc_secCrtcAddPendingFlip, secCrtcAddPendingFlip_corectInit );
    suite_add_tcase(s, tc_secCrtcAddPendingFlip );

    SRunner *sr = srunner_create(s);
    //srunner_set_log(sr, "crtc.log");
    srunner_run_all(sr, CK_VERBOSE);

	stat->num_tests = srunner_ntests_run( sr );
	stat->num_pass = srunner_ntests_run( sr ) - srunner_ntests_failed( sr );
	stat->num_failure = srunner_ntests_failed( sr );

    return 0;
}
