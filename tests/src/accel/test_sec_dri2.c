#include "../../main.h"

#define PRINTF  printf( "DEBUG\n" );
#define PRINTF_ADR( adr ) ( printf( "%s = %p\n", #adr, adr ) )

// include functions from sec_dri2.c to test
// we do it because some functions within sec_dri2.c are static
#include "../../src/accel/sec_dri2.c"

//===================================== FAXES FUNCTION (local) ==============================================

PixmapPtr CreatePixmap_fake( ScreenPtr pScreen, int width, int height, int depth, unsigned usage_hint )
{
  if( !width ) return NULL;

  return ctrl_calloc( 1, sizeof( PixmapRec ) );
}

//============================================== FIXTURES ===================================================

// ---------------------------------------- _initBackBufPixmap ----------------------------------------------

// MUST be not-changed, except within setup_page_flip() function call
DRI2BufferPtr       pBackBuf_gl     = NULL;
DrawablePtr         pDraw_gl        = NULL;
ScreenPtr           pScreen_gl      = NULL;
ScrnInfoPtr         pScrn_gl        = NULL;
SECPtr              pSec_gl         = NULL;
SECExaPrivPtr       pExaPriv_gl     = NULL;
DRI2BufferPrivPtr   pBackBufPriv_gl = NULL;
PixmapPtr           p_ret           = NULL;

// definition checked fixture '_initBackBufPixmap' for 'tc__initBackBufPixmap' test case
void setup_page__initBackBufPixmap( void )
{
  pBackBuf_gl = ctrl_calloc( 1, sizeof( DRI2BufferRec ) );
  pBackBuf_gl->driverPrivate = ctrl_calloc( 1, sizeof( DRI2BufferPrivRec ) );
  pBackBufPriv_gl = pBackBuf_gl->driverPrivate;

  pDraw_gl = ctrl_calloc( 1, sizeof( DrawableRec ) );
  pDraw_gl->pScreen = ctrl_calloc( 1, sizeof( ScreenRec ) );
  pScreen_gl = pDraw_gl->pScreen;
  pScreen_gl->CreatePixmap = CreatePixmap_fake;

  xf86Screens = ctrl_calloc( 1, sizeof( ScrnInfoPtr ) );
  xf86Screens[0] = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );

  pScreen_gl->myNum = 0;
  pScrn_gl = xf86Screens[pScreen_gl->myNum];
  pScrn_gl->driverPrivate = ctrl_calloc( 1, sizeof( SECRec ) );


  pSec_gl = SECPTR( pScrn_gl );
  pSec_gl->pExaPriv = ctrl_calloc( 1, sizeof( SECExaPriv ) );

  pExaPriv_gl = SECEXAPTR( pSec_gl );
}

void teardown_page__initBackBufPixmap( void )
{
  // ctrl_free make NULL pointer check
  // we don't need to call ctrl_free here, because of each unit tests and setup_* function
  // (in witch we have called ctrl_calloc) executed as different process, so memory will be freed by OS automatically.
}

//========================================= UNIT TESTS ===================================================

//==================================== _getName ==========================================================
START_TEST( ut__getName_deref_null_ptr_1 )
{
	_getName( NULL );
}
END_TEST;

START_TEST( ut__getName_deref_null_ptr_2 )
{
  PixmapPtr pPix = ctrl_calloc( 1, sizeof( PixmapRec ) );

  // pPix->devKind != 9, so exaGetPixmapDriverPrivate will return NULL
  _getName( pPix );
}
END_TEST;

START_TEST( ut__getName_pPix_null )
{
	int ret = _getName( NULL );

	fail_if( ret != 0, "wrong workflow" );
}
END_TEST;

START_TEST( ut__getName_pExaPixPriv_null )
{
  PixmapPtr pPix = ctrl_calloc( 1, sizeof( PixmapRec ) );

  int ret = _getName( pPix );

  // pPix->devKind != 9, so exaGetPixmapDriverPrivate will return NULL and _getName must return 0
  fail_if( ret != 0, "wrong workflow" );
  fail_if( pPix->refcnt != 11, "wrong workflow" );
}
END_TEST;

START_TEST( ut__getName_bo_no_null )
{
  PixmapPtr pPix = ctrl_calloc( 1, sizeof( PixmapRec ) );
  int ret;

  pPix->devKind = 9;
  ret = _getName( pPix );

  // pPix->usage_hint != 25, so _getName must return 11
  fail_if( ret != 11, "wrong workflow" );
  fail_if( pPix->refcnt != 11, "wrong workflow" );
}
END_TEST;

START_TEST( ut__getName_no_isFrameBuffer )
{
  PixmapPtr pPix = ctrl_calloc( 1, sizeof( PixmapRec ) );
  int ret;

  pPix->devKind = 9;
  pPix->usage_hint = 25;
  ret = _getName( pPix );

  // pPix->devPrivates != (void*)1, so _getName must return 0
  fail_if( ret != 0, "wrong workflow" );
  fail_if( pPix->refcnt != 11, "wrong workflow" );
}
END_TEST;

START_TEST( ut__getName_yes_isFrameBuffer )
{
  PixmapPtr pPix = ctrl_calloc( 1, sizeof( PixmapRec ) );
  int ret;

  pPix->devKind = 9;
  pPix->usage_hint = 25;
  pPix->devPrivates = (void*)1;
  ret = _getName( pPix );

  fail_if( ret != (unsigned int)ROOT_FB_ADDR, "wrong workflow" );
  fail_if( pPix->refcnt != 11, "wrong workflow" );
}
END_TEST;

//======================================== _initBackBufPixmap ============================================
START_TEST( ut__initBackBufPixmap_no_flip_no_pixmap )
{
  p_ret = _initBackBufPixmap( pBackBuf_gl, pDraw_gl, 0 );

  fail_if( p_ret != NULL, "pPixmap must be NULL" );
}
END_TEST;

START_TEST( ut__initBackBufPixmap_no_flip_yes_pixmap_1 )
{
  pDraw_gl->width = 11;
  p_ret = _initBackBufPixmap( pBackBuf_gl, pDraw_gl, 0 );

  fail_if( p_ret == NULL, "pPixmap mustn't be NULL" );
}
END_TEST;

START_TEST( ut__initBackBufPixmap_no_flip_yes_pixmap_2 )
{
  pDraw_gl->width = 11;
  p_ret = _initBackBufPixmap( pBackBuf_gl, pDraw_gl, 0 );

  fail_if( pBackBufPriv_gl->num_buf != 1, "pBackBufPriv_gl->num_buf(%d) must be 1",
           pBackBufPriv_gl->num_buf );
}
END_TEST;

START_TEST( ut__initBackBufPixmap_no_flip_yes_pixmap_3 )
{
  pDraw_gl->width = 11;
  p_ret = _initBackBufPixmap( pBackBuf_gl, pDraw_gl, 0 );

  fail_if( pBackBufPriv_gl->pBackPixmaps == NULL, "pBackBufPriv_gl->num_buf(%p) mustn't be NULL",
           pBackBufPriv_gl->pBackPixmaps);
}
END_TEST;

START_TEST( ut__initBackBufPixmap_no_flip_yes_pixmap_4 )
{
  pDraw_gl->width = 11;
  p_ret = _initBackBufPixmap( pBackBuf_gl, pDraw_gl, 0 );

  fail_if( pBackBufPriv_gl->pBackPixmaps[0] != p_ret, "pBackBufPriv_gl->pBackPixmaps[0](%p) must be equal p_ret(%p)",
           pBackBufPriv_gl->pBackPixmaps[0], p_ret );
}
END_TEST;

START_TEST( ut__initBackBufPixmap_no_flip_yes_pixmap_5 )
{
  pDraw_gl->width = 11;
  p_ret = _initBackBufPixmap( pBackBuf_gl, pDraw_gl, 0 );

  fail_if( pBackBufPriv_gl->canFlip != 0, "pBackBufPriv_gl->canFlip(%d) must be equal 0",
           pBackBufPriv_gl->canFlip );
}
END_TEST;

START_TEST( ut__initBackBufPixmap_no_flip_yes_pixmap_6 )
{
  pDraw_gl->width = 11;
  p_ret = _initBackBufPixmap( pBackBuf_gl, pDraw_gl, 0 );

  fail_if( pBackBufPriv_gl->avail_idx != 0, "pBackBufPriv_gl->avail_idx(%d) must be equal0",
           pBackBufPriv_gl->avail_idx );
}
END_TEST;

START_TEST( ut__initBackBufPixmap_no_flip_yes_pixmap_7 )
{
  pDraw_gl->width = 11;
  p_ret = _initBackBufPixmap( pBackBuf_gl, pDraw_gl, 0 );

  fail_if( pBackBufPriv_gl->free_idx != 0, "pBackBufPriv_gl->free_idx(%d) must be equal 0",
           pBackBufPriv_gl->free_idx );
}
END_TEST;

START_TEST( ut__initBackBufPixmap_no_flip_yes_pixmap_8 )
{
  pDraw_gl->width = 11;
  p_ret = _initBackBufPixmap( pBackBuf_gl, pDraw_gl, 0 );

  fail_if( pBackBufPriv_gl->cur_idx != 0, "pBackBufPriv_gl->cur_idx(%d) must be equal 0",
           pBackBufPriv_gl->cur_idx );
}
END_TEST;

START_TEST( ut__initBackBufPixmap_no_flip_yes_pixmap_9 )
{
  pDraw_gl->width = 11;
  p_ret = _initBackBufPixmap( pBackBuf_gl, pDraw_gl, 0 );

  fail_if( pBackBufPriv_gl->pipe != -1, "pBackBufPriv_gl->pipe(%d) must be equal -1",
           pBackBufPriv_gl->pipe );
}
END_TEST;

START_TEST( ut__initBackBufPixmap_no_flip_yes_pixmap_10 )
{
  pDraw_gl->width = 11;
  p_ret = _initBackBufPixmap( pBackBuf_gl, pDraw_gl, 0 );

  //fail_if( p_ret != , "pBackBufPriv_gl->pipe(%d) must be equal -1",
  //         pBackBufPriv_gl->pipe );
}
END_TEST;


//========================================================================================================
//========================================================================================================
//========================================================================================================

Suite* suite_dri2( void )
{
  // suite for file to test creation
  Suite* s = suite_create( "sec_dri2.c" );

  // test case for each function creation
  TCase* tc__getName = tcase_create( "_getName" );
  TCase* tc__initBackBufPixmap = tcase_create( "_initBackBufPixmap" );

  // push unit tests to test case tc_getName
  tcase_add_test( tc__getName, ut__getName_deref_null_ptr_1 );
  tcase_add_test( tc__getName, ut__getName_deref_null_ptr_2 );
  tcase_add_test( tc__getName, ut__getName_pPix_null );
  tcase_add_test( tc__getName, ut__getName_pExaPixPriv_null );
  tcase_add_test( tc__getName, ut__getName_bo_no_null );
  tcase_add_test( tc__getName, ut__getName_no_isFrameBuffer );
  tcase_add_test( tc__getName, ut__getName_yes_isFrameBuffer );

  // functions setup_page__initBackBufPixmap and teardown_page__initBackBufPixmap will be executed before and after
  // each unit tests in tc__initBackBufPixmap respectively
  tcase_add_checked_fixture( tc__initBackBufPixmap, setup_page__initBackBufPixmap, teardown_page__initBackBufPixmap );

  // push unit tests to test case tc_initBackBufPixmap
  tcase_add_test( tc__initBackBufPixmap, ut__initBackBufPixmap_no_flip_no_pixmap );
  tcase_add_test( tc__initBackBufPixmap, ut__initBackBufPixmap_no_flip_yes_pixmap_1 );
  tcase_add_test( tc__initBackBufPixmap, ut__initBackBufPixmap_no_flip_yes_pixmap_2 );
  tcase_add_test( tc__initBackBufPixmap, ut__initBackBufPixmap_no_flip_yes_pixmap_3 );
  tcase_add_test( tc__initBackBufPixmap, ut__initBackBufPixmap_no_flip_yes_pixmap_4 );
  tcase_add_test( tc__initBackBufPixmap, ut__initBackBufPixmap_no_flip_yes_pixmap_5 );
  tcase_add_test( tc__initBackBufPixmap, ut__initBackBufPixmap_no_flip_yes_pixmap_6 );
  tcase_add_test( tc__initBackBufPixmap, ut__initBackBufPixmap_no_flip_yes_pixmap_7 );
  tcase_add_test( tc__initBackBufPixmap, ut__initBackBufPixmap_no_flip_yes_pixmap_8 );
  tcase_add_test( tc__initBackBufPixmap, ut__initBackBufPixmap_no_flip_yes_pixmap_9 );
  tcase_add_test( tc__initBackBufPixmap, ut__initBackBufPixmap_no_flip_yes_pixmap_10 );

  // push test cases to suite
  suite_add_tcase( s, tc__getName );
  suite_add_tcase( s, tc__initBackBufPixmap );

  return s;
}
