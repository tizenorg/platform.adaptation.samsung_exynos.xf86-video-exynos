/*
 * test_utils.c
 *
 *  Created on: Nov 18, 2013
 *      Author: svoloshynov
 */

#include "exynos_driver.h"

// allocates and fills EXYNOSOutputPrivPtr structure for further using
// type - connector type, crtc_d == 7
//-----------------------------------------------------------------------------------------------------------
static EXYNOSOutputPrivPtr create_output( uint32_t type )
{
  EXYNOSOutputPrivPtr out = calloc( 1, sizeof(EXYNOSOutputPrivRec) );

  out->mode_output = calloc( 1, sizeof(drmModeConnector) );
  out->mode_output->connector_type = type;
  out->mode_encoder = calloc( 1, sizeof(drmModeEncoder) );
  out->mode_encoder->crtc_id = 7;

  return out;
}

// this function prepare ScrnInfoPtr struct for further using
// till only for 'test_clone_start_stop' unit-test
//-----------------------------------------------------------------------------------------------------------
void prepare( ScrnInfoPtr pScr )
{
  //EXYNOSOutputPrivPtr pOutputPriv = calloc (1, sizeof (EXYNOSOutputPrivRec));
  pScr->driverPrivate = calloc( 1, sizeof(EXYNOSRec) );

  EXYNOSRec* pExynos = pScr->driverPrivate;

  pExynos->pDispInfo = calloc( 1, sizeof(EXYNOSDispInfoRec) );
  pExynos->pDispInfo->conn_mode = DISPLAY_CONN_MODE_HDMI;
  pExynos->pDispInfo->main_lcd_mode.hdisplay = 1024;
  pExynos->pDispInfo->main_lcd_mode.vdisplay = 768;
  pExynos->pDispInfo->ext_connector_mode.hdisplay = 100;
  pExynos->pDispInfo->ext_connector_mode.vdisplay = 100;

  pExynos->pDispInfo->mode_res = calloc( 1, sizeof(drmModeRes) );
  pExynos->pDispInfo->mode_res->count_connectors = 1;
  pExynos->pDispInfo->mode_res->count_crtcs = 1;
  pExynos->pDispInfo->mode_res->count_encoders = 1;
  pExynos->drm_fd=1;

  xorg_list_init( &pExynos->pDispInfo->crtcs );
  xorg_list_init( &pExynos->pDispInfo->outputs );
  xorg_list_init( &pExynos->pDispInfo->planes );

  EXYNOSOutputPrivPtr o1 = create_output( DRM_MODE_CONNECTOR_HDMIA );
  xorg_list_add( &o1->link, &pExynos->pDispInfo->outputs );
  EXYNOSOutputPrivPtr o2 = create_output( DRM_MODE_CONNECTOR_LVDS );
  xorg_list_add( &o2->link, &pExynos->pDispInfo->outputs );
}

// this function prepare ScrnInfoPtr( pCrtc ) struct for test_crtc.c
int sp_num = 0;//By this variable pScrn will get different initialization
int destruction_flag = 0;//Flag for destroypScrn function

void initpScrnBySp_num( ScrnInfoPtr pScrn, int n )
{
	xf86CrtcConfigPtr pXf86CrtcConfig = NULL;
	xf86CrtcConfigPtr pXCC = NULL;
	xf86CrtcPtr pCrtc = NULL;
	EXYNOSCrtcPrivPtr pCrtcPriv = NULL;
    int i = 0;
    int j = 0;
    int flip_backbufs = 3 - 1;

	pScrn->privates = ctrl_calloc( 3, sizeof( DevUnion ) );
	pXf86CrtcConfig = ctrl_calloc( 1, sizeof( xf86CrtcConfigRec ) );
	pScrn->privates[1].ptr = pXf86CrtcConfig;
	pXCC = pScrn->privates[1].ptr;
	pXCC->num_output = 2;
    pXCC->crtc = ctrl_calloc( 3, sizeof( xf86CrtcPtr ) );

	switch ( n ) {
		case 1:
		    for( i = 0; i < 3; i++  )
		    {
		    	pXCC->crtc[i] = ctrl_calloc( 1, sizeof( xf86CrtcRec ) );
		    	pCrtc = pXCC->crtc[i];
		        pCrtc->driver_private = ctrl_calloc(1,sizeof(EXYNOSCrtcPrivRec));
		    }

		    destruction_flag = 1;
		    break;

		case 2:
		    for( i = 0; i < 3; i++  )
		    {
		    	pXCC->crtc[i] = ctrl_calloc( 1, sizeof( xf86CrtcRec ) );
		    	pCrtc = pXCC->crtc[i];
		        pCrtc->driver_private = ctrl_calloc(1,sizeof(EXYNOSCrtcPrivRec));
		        pCrtcPriv = pCrtc->driver_private;
		        pCrtcPriv->pipe = 1;
		    }

		    destruction_flag = 2;
			break;

		case 3:
		    for( i = 0; i < 3; i++  )
		    {
		    	pXCC->crtc[i] = ctrl_calloc( 1, sizeof( xf86CrtcRec ) );
		    	pCrtc = pXCC->crtc[i];
		        pCrtc->driver_private = ctrl_calloc(1,sizeof(EXYNOSCrtcPrivRec));
		        pCrtcPriv = pCrtc->driver_private;
		        pCrtcPriv->pipe = 1;
		        pCrtcPriv->flip_backpixs.num = 2;
		        pCrtcPriv->flip_backpixs.pix_free = ctrl_calloc( 3, sizeof( Bool ));
		        pCrtcPriv->flip_backpixs.pix_free[0] = FALSE;
		        pCrtcPriv->flip_backpixs.pix_free[1] = FALSE;
		        pCrtcPriv->flip_backpixs.pix_free[2] = FALSE;
			    pCrtcPriv->flip_backpixs.flip_pixmaps = ctrl_calloc( 3, sizeof( PixmapPtr ) );
			    pCrtcPriv->flip_backpixs.flip_draws = ctrl_calloc( 3, sizeof( DrawablePtr ) );
			    for( j = 0; j < 3; j++ )
			    {
			    	pCrtcPriv->flip_backpixs.flip_pixmaps[j] = ctrl_calloc( 1, sizeof( PixmapRec ) );
			    	pCrtcPriv->flip_backpixs.flip_draws[j] = ctrl_calloc( 1, sizeof( DrawableRec ) );
			    }
			    pCrtcPriv->flip_count = 1;
		    }

		    destruction_flag = 3;
			break;

		case 4:
		    for( i = 0; i < 3; i++  )
		    {
		    	pXCC->crtc[i] = ctrl_calloc( 1, sizeof( xf86CrtcRec ) );
		    	pCrtc = pXCC->crtc[i];
		        pCrtc->driver_private = ctrl_calloc(1,sizeof(EXYNOSCrtcPrivRec));
		        pCrtcPriv = pCrtc->driver_private;
		        pCrtcPriv->pipe = 1;
		        pCrtcPriv->flip_backpixs.num = 2;
		        pCrtcPriv->flip_backpixs.pix_free = ctrl_calloc( 3, sizeof( Bool ));
		        pCrtcPriv->flip_backpixs.pix_free[0] = TRUE;
		        pCrtcPriv->flip_backpixs.pix_free[1] = TRUE;
		        pCrtcPriv->flip_backpixs.pix_free[2] = TRUE;
			    pCrtcPriv->flip_backpixs.flip_pixmaps = ctrl_calloc( 3, sizeof( PixmapPtr ) );
			    pCrtcPriv->flip_backpixs.flip_draws = ctrl_calloc( 3, sizeof( DrawablePtr ) );
			    for( j = 0; j < 3; j++ )
			    {
			    	pCrtcPriv->flip_backpixs.flip_pixmaps[j] = ctrl_calloc( 1, sizeof( PixmapRec ) );
			    	pCrtcPriv->flip_backpixs.flip_draws[j] = ctrl_calloc( 1, sizeof( DrawableRec ) );
			    }
			    pCrtcPriv->flip_count = 0;
		    }

		    destruction_flag = 4;
			break;

		case 5:
		    for( i = 0; i < 3; i++  )
		    {
		    	pXCC->crtc[i] = ctrl_calloc( 1, sizeof( xf86CrtcRec ) );
		    	pCrtc = pXCC->crtc[i];
		        pCrtc->driver_private = ctrl_calloc(1,sizeof(EXYNOSCrtcPrivRec));
		        pCrtcPriv = pCrtc->driver_private;
		        pCrtcPriv->pipe = 1;
		        pCrtcPriv->flip_backpixs.num = 2;
		        pCrtcPriv->flip_backpixs.pix_free = ctrl_calloc( 3, sizeof( Bool ));
		        pCrtcPriv->flip_backpixs.pix_free[0] = TRUE;
		        pCrtcPriv->flip_backpixs.pix_free[1] = TRUE;
		        pCrtcPriv->flip_backpixs.pix_free[2] = TRUE;
			    pCrtcPriv->flip_backpixs.flip_pixmaps = ctrl_calloc( 3, sizeof( PixmapPtr ) );
			    pCrtcPriv->flip_backpixs.flip_draws = ctrl_calloc( 3, sizeof( DrawablePtr ) );
			    for( j = 0; j < 3; j++ )
			    {
			    	pCrtcPriv->flip_backpixs.flip_pixmaps[j] = NULL;
			    	pCrtcPriv->flip_backpixs.flip_draws[j] = ctrl_calloc( 1, sizeof( DrawableRec ) );
			    }
			    pCrtcPriv->flip_count = 0;
		    }

		    destruction_flag = 5;
			break;

		default:
			printf("used undefined n, ERROR");
			break;
	}
}

//This function frees memory of ScrnInfoPtr which was allocated by initpScrnBySp_num()
void destroypScrn( ScrnInfoPtr pScrn )
{
	xf86CrtcConfigPtr pXf86CrtcConfig = NULL;
	xf86CrtcConfigPtr pXCC = NULL;
	xf86CrtcPtr pCrtc = NULL;
	EXYNOSCrtcPrivPtr pCrtcPriv = NULL;
	pXCC = pScrn->privates[1].ptr;
	int i = 0;
	int j = 0;
	switch ( destruction_flag )
	{
	case 1:
	    for( i = 0; i < 3; i++  )
	    {
	    	pCrtc = pXCC->crtc[i];
	    	ctrl_free( pCrtc->driver_private );
	    	ctrl_free( pXCC->crtc[i] );
	    }
		break;

	case 2:
		for( i = 0; i < 3; i++  )
		{
			pCrtc = pXCC->crtc[i];
			ctrl_free( pCrtc->driver_private );
			ctrl_free( pXCC->crtc[i] );
		}
		break;

	case 3:
	case 4:
		pCrtc = pXCC->crtc[i];
		pCrtcPriv = pCrtc->driver_private;

		for( i = 0; i < 3; i++  )
		{
			pCrtc = pXCC->crtc[i];
			EXYNOSCrtcPrivPtr pC = pCrtc->driver_private;
			ctrl_free( pC->flip_backpixs.pix_free );
			for( j = 0; j < 3; j++)
			{
				ctrl_free( pC->flip_backpixs.flip_pixmaps[j] );
				ctrl_free( pC->flip_backpixs.flip_draws[j] );
			}
			ctrl_free( pC->flip_backpixs.flip_pixmaps );
			ctrl_free( pC->flip_backpixs.flip_draws );
			ctrl_free( pCrtc->driver_private );
			ctrl_free( pXCC->crtc[i] );
		}
		break;
	case 5:
		pCrtc = pXCC->crtc[i];
		pCrtcPriv = pCrtc->driver_private;

		for( i = 0; i < 3; i++  )
		{
			pCrtc = pXCC->crtc[i];
			EXYNOSCrtcPrivPtr pC = pCrtc->driver_private;
			ctrl_free( pC->flip_backpixs.pix_free );
			for( j = 0; j < 3; j++)
			{
				ctrl_free( pC->flip_backpixs.flip_draws[j] );
			}
			ctrl_free( pC->flip_backpixs.flip_pixmaps );
			ctrl_free( pC->flip_backpixs.flip_draws );
			ctrl_free( pCrtc->driver_private );
			ctrl_free( pXCC->crtc[i] );
		}
		break;
	default:
		printf("Attempt to free undefined pScrn");
		break;
	}
    ctrl_free( pXCC->crtc );
    ctrl_free( pScrn->privates[1].ptr );
    ctrl_free( pScrn->privates );
}

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

void free_crtc(xf86CrtcPtr pCrtc){
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
