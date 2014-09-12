#include "../main.h"

#define PRINTF ( printf( "\nDEBUG\n\n" ); )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )
#define PRINTNUM( A ) ( printf("%d\n", A) )

#define _secUtilIsVbufValid(v,__FUNCTION__) fake_secUtilIsVbufValid(v,__FUNCTION__)
#define secUtilRemoveFreeVideoBufferFunc fake_secUtilRemoveFreeVideoBufferFunc
#define util_g2d_copy_with_scale fake_util_g2d_copy_with_scale
#define g2d_exec fake_g2d_exec

typedef struct stsp
{
	int v_secUtilRemoveFreeVideoBufferFunc;
	int v_util_g2d_copy_with_scale;
	int v_g2d_exec;
} sTSecPlane;

sTSecPlane tsp = { 0, 0, 0, 0, };

#define VBUF_IS_VALID(v) ((v) ? TRUE : FALSE)

// include functions from sec.c to test
// we do it because some functions within sec.c are static
#include "../../src/crtcconfig/sec_plane.c"

extern unsigned ctrl_get_cnt_calloc();

void fake_util_g2d_copy_with_scale(G2dImage* src, G2dImage* dst,
            int src_x, int src_y, unsigned int src_w, unsigned int src_h,
            int dst_x, int dst_y, unsigned int dst_w, unsigned int dst_h,
            int negative)
{
	tsp.v_util_g2d_copy_with_scale++;
}

void fake_g2d_exec( void )
{
	tsp.v_g2d_exec++;
}

Bool fake_secUtilIsVbufValid( SECVideoBuf *v, const char *func )
{
	if ( 1 == v->fb_id )
		return TRUE;
	return FALSE;
}

void
fake_secUtilRemoveFreeVideoBufferFunc (SECVideoBuf *vbuf, FreeVideoBufFunc func, void *data)
{
	tsp.v_secUtilRemoveFreeVideoBufferFunc++;
}


/* _secPlaneFreeVbuf */
START_TEST( ut__secPlaneFreeVbuf_notRelevantPlaneId )
{
	SECVideoBuf *vbuf = NULL;
	void *data = -1;

	/* SEGFAULT error must not occurs */
	_secPlaneFreeVbuf( vbuf, data);
}
END_TEST;

START_TEST( ut__secPlaneFreeVbuf_tableIsNULL )
{
	SECVideoBuf *vbuf = NULL;
	void *data = 5;

	/* SEGFAULT error must not occurs */
	_secPlaneFreeVbuf( vbuf, data);
}
END_TEST;

START_TEST( ut__secPlaneFreeVbuf_fbIsNULL )
{
	SECPlaneTable stable;
	stable.plane_id = 1;
	SECVideoBuf vbuf;
	vbuf.fb_id = 2;
	void *data = 1;
	plane_table_size = 1;
	plane_table = &stable;

	xorg_list_init( &stable.fbs );

	/* SEGFAULT error must not occurs */
	_secPlaneFreeVbuf( &vbuf, data);
}
END_TEST;

START_TEST( ut__secPlaneFreeVbuf_MemMangment)
{
	SECPlaneTable stable;
	stable.plane_id = 1;
	SECVideoBuf vbuf;
	vbuf.fb_id = 2;
	void *data = 1;
	plane_table_size = 1;
	plane_table = &stable;

	xorg_list_init( &stable.fbs );
	SECPlaneFb *fb = ctrl_calloc( 1, sizeof( SECPlaneFb ) );
	fb->id = 2;
	xorg_list_add( &fb->link, &stable.fbs );

	int mm_calloc = ctrl_get_cnt_calloc();

	/* Memory must be freed */
	_secPlaneFreeVbuf( &vbuf, data);

	fail_if( mm_calloc - 1 != ctrl_get_cnt_calloc(), "Memory was not freed" );
}
END_TEST;

/* _secPlaneTableFindPos */
START_TEST( ut__secPlaneTableFindPos_notRelevantCrtcId )
{
	int crtc_id = -1;
	int zpos = 0;

	SECPlaneTable *res = _secPlaneTableFindPos ( crtc_id , zpos );

	fail_if( res != NULL, "not relevant id, NULL must be returned" );
}
END_TEST;

START_TEST( ut__secPlaneTableFindPos_correctWorkFlow )
{
	SECPlaneTable table;
	table.crtc_id = 1;
	table.zpos = 2;

	int crtc_id = 1;
	int zpos = 2;

	plane_table_size = 1;
	plane_table = &table;


	SECPlaneTable *res = _secPlaneTableFindPos ( crtc_id , zpos );

	fail_if( res == NULL, "Correct work flow, NULL must not be returned" );
}
END_TEST;

/* _secPlaneTableFind */
START_TEST( ut__secPlaneTableFind_notRelevantId )
{
	int plane_id = -1;

	SECPlaneTable *res = _secPlaneTableFind ( plane_id );

	fail_if( res != NULL, "Not relevant id, NULL must be returned" );
}
END_TEST;

START_TEST( ut__secPlaneTableFind_correctWorkFlow )
{
	SECPlaneTable table;
	table.plane_id = 1;

	int plane_id = 1;

	plane_table_size = 1;
	plane_table = &table;


	SECPlaneTable *res = _secPlaneTableFind ( plane_id );

	fail_if( res == NULL, "Correct work flow, NULL must not be returned" );
}
END_TEST;

/* _secPlaneTableFindEmpty */
START_TEST( ut__secPlaneTableFindEmpty_noFreeTables )
{
	SECPlaneTable table;
	table.in_use = 1;

	plane_table_size = 1;
	plane_table = &table;

	SECPlaneTable* res = _secPlaneTableFindEmpty();
	fail_if( res != NULL, "There are no free tables, NULL must be returned" );
}
END_TEST;

START_TEST( ut__secPlaneTableFindEmpty_correctWorkFlow )
{
	SECPlaneTable table;
	table.in_use = 0;

	plane_table_size = 1;
	plane_table = &table;

	SECPlaneTable* res = _secPlaneTableFindEmpty();
	fail_if( res == NULL, "Correct work flow, NULL must not be returned" );
}
END_TEST;

/* _secPlaneTableFindBuffer */
START_TEST( ut__secPlaneTableFindBuffer_noBuffers )
{
	SECPlaneTable table;
	int fb_id = 0;

	xorg_list_init( &table.fbs );

	SECPlaneFb* res = _secPlaneTableFindBuffer ( &table, fb_id, NULL, NULL );
	fail_if( res != NULL, "There are no free buffers, NULL must be returned" );
}
END_TEST;

START_TEST( ut__secPlaneTableFindBuffer_returnBufferById )
{
	SECPlaneTable table;
	int fb_id = 1;

	xorg_list_init( &table.fbs );
	SECPlaneFb fb;
	fb.id = 1;
	xorg_list_add( &fb.link, &table.fbs );

	SECPlaneFb* res = _secPlaneTableFindBuffer ( &table, fb_id, NULL, NULL );
	fail_if( res == NULL, "correct work flow, NULL must not be returned" );
}
END_TEST;

START_TEST( ut__secPlaneTableFindBuffer_returnBufferByBo )
{
	SECPlaneTable table;
	int fb_id = -1;
	tbm_bo bo;

	xorg_list_init( &table.fbs );
	SECPlaneFb fb;
	fb.id = 2;
	fb.type = PLANE_FB_TYPE_BO;
	fb.buffer.bo = &bo;

	xorg_list_add( &fb.link, &table.fbs );

	SECPlaneFb* res = _secPlaneTableFindBuffer ( &table, fb_id, &bo, NULL );
	fail_if( res == NULL, "correct work flow, NULL must not be returned" );
}
END_TEST;

START_TEST( ut__secPlaneTableFindBuffer_notValidVbuf )
{
	SECPlaneTable table;
	SECVideoBuf vbuf;
	vbuf.fb_id = 0;
	int fb_id = -1;
	tbm_bo bo;

	xorg_list_init( &table.fbs );
	SECPlaneFb fb;
	fb.id = 2;
	fb.buffer.bo = &bo;

	xorg_list_add( &fb.link, &table.fbs );

	SECPlaneFb* res = _secPlaneTableFindBuffer ( &table, fb_id, NULL, &vbuf );
	fail_if( res != NULL, "vbuf is NULL, NULL must be returned" );
}
END_TEST;

START_TEST( ut__secPlaneTableFindBuffer_returnBufferWhenVbufIsValid )
{
	SECPlaneTable table;
	SECVideoBuf vbuf;
	vbuf.fb_id = 1;
	vbuf.stamp = 2;

	int fb_id = -1;
	tbm_bo bo;

	xorg_list_init( &table.fbs );
	SECPlaneFb fb;
	fb.id = 3;
	fb.type = PLANE_FB_TYPE_DEFAULT;
	fb.buffer.vbuf = &vbuf;
	xorg_list_add( &fb.link, &table.fbs );

	SECPlaneFb* res = _secPlaneTableFindBuffer ( &table, fb_id, NULL, &vbuf );
	fail_if( res == NULL, "correct work flow, NULL must not be returned" );
}
END_TEST;

/* _secPlaneTableFreeBuffer */
START_TEST( ut__secPlaneTableFreeBuffer_fbIsEqualToTableFb )
{
	SECPlaneTable table;
	SECPlaneFb *fb = ctrl_calloc( 1, sizeof( SECPlaneFb ) );
	int mm_calloc = ctrl_get_cnt_calloc();

	table.cur_fb = fb;

	_secPlaneTableFreeBuffer ( &table, fb );

	fail_if( mm_calloc != ctrl_get_cnt_calloc(), "Memory must not be freed" );
	ctrl_free( fb );
}
END_TEST;

START_TEST( ut__secPlaneTableFreeBuffer_boUnref )
{
	SECPlaneTable table;
	SECPlaneFb *fb = ctrl_calloc( 1, sizeof( SECPlaneFb ) );

	fb->buffer.bo = tbm_bo_alloc( 0, 1, 0 );
	fb->type = PLANE_FB_TYPE_BO;
	tbm_bo_ref( fb->buffer.bo );

	xorg_list_init( &table.fbs );
	xorg_list_add( &fb->link, &table.fbs );

	int mm_tbm = tbm_get_ref( fb->buffer.bo );
	int mm_calloc = ctrl_get_cnt_calloc();

	_secPlaneTableFreeBuffer ( &table, fb );

	fail_if( mm_tbm - 1 != tbm_get_ref( fb->buffer.bo ), "Bo must be unref" );
	fail_if( mm_calloc - 1 != ctrl_get_cnt_calloc(), "Memory must not be freed" );
}
END_TEST;

START_TEST( ut__secPlaneTableFreeBuffer_removeVbuf )
{
	SECVideoBuf vbuf;
	SECPlaneTable table;
	SECPlaneFb *fb = ctrl_calloc( 1, sizeof( SECPlaneFb ) );
	fb->buffer_gone = FALSE;
	fb->buffer.vbuf = &vbuf;

	xorg_list_init( &table.fbs );
	xorg_list_add( &fb->link, &table.fbs );

	int mm_calloc = ctrl_get_cnt_calloc();

	_secPlaneTableFreeBuffer ( &table, fb );

	fail_if( tsp.v_secUtilRemoveFreeVideoBufferFunc != 1, "Local variable must be changed" );
	fail_if( mm_calloc - 1 != ctrl_get_cnt_calloc(), "Memory must not be freed" );
}
END_TEST;

/* _secPlaneTableEnsure */
START_TEST( ut__secPlaneTableEnsure_countPlanesLessThenOne )
{
	ScrnInfoPtr pScrn = NULL;
	int count_planes = 0;

	Bool res = _secPlaneTableEnsure ( pScrn, count_planes );

	fail_if( res != FALSE, "There are no planes, FALSE must be returned" );
}
END_TEST;

START_TEST( ut__secPlaneTableEnsure_planeTableIsInitialize )
{
	ScrnInfoPtr pScrn = NULL;
	int count_planes = 4;

	plane_table = ctrl_calloc( 1, sizeof( SECPlaneTable  ) );
	Bool res = _secPlaneTableEnsure ( pScrn, count_planes );

	fail_if( res != TRUE, "plane_table is initialize, TRUE must be returned" );
	ctrl_free( plane_table );
}
END_TEST;

START_TEST( ut__secPlaneTableEnsure_memoryManagement )
{
	ScrnInfoPtr pScrn = NULL;
	int count_planes = 4;


	Bool res = _secPlaneTableEnsure ( pScrn, count_planes );

	fail_if( 1 != ctrl_get_cnt_calloc(), "Memory must be allocated" );
	fail_if( res != TRUE, "plane_table is initialize, TRUE must be returned" );
}
END_TEST;

/* _secPlaneExecAccessibility */
START_TEST( ut__secPlaneExecAccessibility_srcImgIsNULL )
{
	xRectangle sr;
	xRectangle dr;
	tbm_bo src_bo = tbm_bo_alloc( 0, 1, 0 );
	tbm_bo dst_bo = tbm_bo_alloc( 0, 1, 0 );
	int sw = 1;
	int sh = 2;
    int dw = 3;
    int dh = 4;
    Bool bNegative = FALSE;

	_secPlaneExecAccessibility ( src_bo, sw, sh, &sr, dst_bo, dw, dh, &dr, bNegative);

	fail_if( 1 != tsp.v_util_g2d_copy_with_scale, "Local variable must be changed" );
	fail_if( 1 != tsp.v_g2d_exec, "Local variable must be changed" );
}
END_TEST;

/* _check_hw_restriction */
START_TEST( ut__check_hw_restriction_crtcIsNULL )
{
	xf86CrtcConfigPrivateIndex = 0;
	ScrnInfoRec Scrn;
	xf86CrtcRec Crtc;
	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &Crtc;
	int crtc_id = 0;
	int buf_w;
	int buf_h;
	xRectanglePtr src = NULL;
	xRectanglePtr dst = NULL;
	xRectanglePtr aligned_src = NULL;
	xRectanglePtr aligned_dst = NULL;

	Bool res = _check_hw_restriction ( &Scrn,  crtc_id,  buf_w,  buf_h,
	                       src, dst, aligned_src, aligned_dst);

	fail_if( res != FALSE, "There is no crtc, FALSE must be returned" );
}
END_TEST;

START_TEST( ut__check_hw_restriction_crtcIsNotEnabledOrNotActive )
{
	xf86CrtcConfigPrivateIndex = 0;
	ScrnInfoRec Scrn;

	drmModeCrtc mode;
	mode.crtc_id = 1;

	SECCrtcPrivRec CrtcPriv;
	CrtcPriv.mode_crtc = &mode;

	xf86CrtcPtr crtc = ctrl_calloc(2 , sizeof( xf86CrtcPtr ));
	crtc[0].driver_private = &CrtcPriv;
	crtc[0].enabled = 0;
	crtc[0].active = 0;

	xf86CrtcConfigRec ConfCrtc;
	ConfCrtc.crtc = &crtc;
	ConfCrtc.num_crtc = 1;

 	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &ConfCrtc;

	int crtc_id = 1;
	int buf_w = 0;
	int buf_h = 0;
	xRectanglePtr src = NULL;
	xRectanglePtr dst = NULL;
	xRectanglePtr aligned_src = NULL;
	xRectanglePtr aligned_dst = NULL;

	Bool res = _check_hw_restriction ( &Scrn,  crtc_id,  buf_w,  buf_h,
	                       src, dst, aligned_src, aligned_dst);

	fail_if( res != FALSE, "Crtc is not enable or active, FALSE must be returned" );
}
END_TEST;

START_TEST( ut__check_hw_restriction_bufWIsLessThenMinWith )
{
	xf86CrtcConfigPrivateIndex = 0;
	ScrnInfoRec Scrn;

	drmModeCrtc mode;
	mode.crtc_id = 1;

	SECCrtcPrivRec CrtcPriv;
	CrtcPriv.mode_crtc = &mode;

	xf86CrtcPtr crtc = ctrl_calloc(2 , sizeof( xf86CrtcPtr ));
	crtc[0].driver_private = &CrtcPriv;
	crtc[0].enabled = 1;
	crtc[0].active = 1;

	xf86CrtcConfigRec ConfCrtc;
	ConfCrtc.crtc = &crtc;
	ConfCrtc.num_crtc = 1;

 	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &ConfCrtc;

	int crtc_id = 1;
	int buf_w = MIN_WIDTH - 1;
	int buf_h = MIN_HEIGHT + 1;

	xRectanglePtr src = NULL;
	xRectanglePtr dst = NULL;
	xRectanglePtr aligned_src = NULL;
	xRectanglePtr aligned_dst = NULL;

	Bool res = _check_hw_restriction ( &Scrn,  crtc_id,  buf_w,  buf_h,
	                       src, dst, aligned_src, aligned_dst);

	fail_if( res != FALSE, "buf_w is less then MIN_WITH, FALSE must be returned" );
}
END_TEST;

START_TEST( ut__check_hw_restriction_bufWIsNotDividendByTwo )
{
	xf86CrtcConfigPrivateIndex = 0;
	ScrnInfoRec Scrn;

	drmModeCrtc mode;
	mode.crtc_id = 1;

	SECCrtcPrivRec CrtcPriv;
	CrtcPriv.mode_crtc = &mode;

	xf86CrtcPtr crtc = ctrl_calloc(2 , sizeof( xf86CrtcPtr ));
	crtc[0].driver_private = &CrtcPriv;
	crtc[0].enabled = 1;
	crtc[0].active = 1;

	xf86CrtcConfigRec ConfCrtc;
	ConfCrtc.crtc = &crtc;
	ConfCrtc.num_crtc = 1;

 	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &ConfCrtc;

	int crtc_id = 1;
	int buf_w = 33;
	int buf_h = MIN_HEIGHT + 1;

	xRectanglePtr src = NULL;
	xRectanglePtr dst = NULL;
	xRectanglePtr aligned_src = NULL;
	xRectanglePtr aligned_dst = NULL;

	Bool res = _check_hw_restriction ( &Scrn,  crtc_id,  buf_w,  buf_h,
	                       src, dst, aligned_src, aligned_dst);

	fail_if( res != FALSE, "buf_w is not divided by two without residue, FALSE must be returned" );
}
END_TEST;


START_TEST( ut__check_hw_restriction_bufHIsLessThenMinWith )
{
	xf86CrtcConfigPrivateIndex = 0;
	ScrnInfoRec Scrn;

	drmModeCrtc mode;
	mode.crtc_id = 1;

	SECCrtcPrivRec CrtcPriv;
	CrtcPriv.mode_crtc = &mode;

	xf86CrtcPtr crtc = ctrl_calloc(2 , sizeof( xf86CrtcPtr ));
	crtc[0].driver_private = &CrtcPriv;
	crtc[0].enabled = 1;
	crtc[0].active = 1;

	xf86CrtcConfigRec ConfCrtc;
	ConfCrtc.crtc = &crtc;
	ConfCrtc.num_crtc = 1;

 	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &ConfCrtc;

	int crtc_id = 1;
	int buf_w = 34;
	int buf_h = MIN_HEIGHT - 1;

	xRectanglePtr src = NULL;
	xRectanglePtr dst = NULL;
	xRectanglePtr aligned_src = NULL;
	xRectanglePtr aligned_dst = NULL;

	Bool res = _check_hw_restriction ( &Scrn,  crtc_id,  buf_w,  buf_h,
	                       src, dst, aligned_src, aligned_dst);

	fail_if( res != FALSE, "buf_w is less then MIN_WITH, FALSE must be returned" );
}
END_TEST;

START_TEST( ut__check_hw_restriction_bufHIsNotDividendByTwo )
{
	xf86CrtcConfigPrivateIndex = 0;
	ScrnInfoRec Scrn;

	drmModeCrtc mode;
	mode.crtc_id = 1;

	SECCrtcPrivRec CrtcPriv;
	CrtcPriv.mode_crtc = &mode;

	xf86CrtcPtr crtc = ctrl_calloc(2 , sizeof( xf86CrtcPtr ));
	crtc[0].driver_private = &CrtcPriv;
	crtc[0].enabled = 1;
	crtc[0].active = 1;

	xf86CrtcConfigRec ConfCrtc;
	ConfCrtc.crtc = &crtc;
	ConfCrtc.num_crtc = 1;

 	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &ConfCrtc;

	int crtc_id = 1;
	int buf_w = 34;
	int buf_h = 33;

	xRectanglePtr src = NULL;
	xRectanglePtr dst = NULL;
	xRectanglePtr aligned_src = NULL;
	xRectanglePtr aligned_dst = NULL;

	Bool res = _check_hw_restriction ( &Scrn,  crtc_id,  buf_w,  buf_h,
	                       src, dst, aligned_src, aligned_dst);

	fail_if( res != FALSE, "buf_w is not divided by two without residue, FALSE must be returned" );
}
END_TEST;

START_TEST( ut__check_hw_restriction_alignedDstWithMinusAlignedXLessThenMinWidth )
{
	xf86CrtcConfigPrivateIndex = 0;
	ScrnInfoRec Scrn;
	Scrn.virtualX = 20;
	Scrn.virtualY = 20;

	drmModeCrtc mode;
	mode.crtc_id = 1;

	SECCrtcPrivRec CrtcPriv;
	CrtcPriv.mode_crtc = &mode;

	xf86CrtcPtr crtc = ctrl_calloc(2 , sizeof( xf86CrtcPtr ));
	crtc[0].driver_private = &CrtcPriv;
	crtc[0].enabled = 1;
	crtc[0].active = 1;

	xf86CrtcConfigRec ConfCrtc;
	ConfCrtc.crtc = &crtc;
	ConfCrtc.num_crtc = 1;

 	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &ConfCrtc;

	int crtc_id = 1;
	int buf_w = 34;
	int buf_h = 34;

	xRectangle src;
	src.x = 1;
	src.y = 2;
	src.width = 34;
	src.height = 34;

	xRectangle dst;
	dst.x = 1;
	dst.y = 2;
	dst.width = 10;
	dst.height = 10;

	xRectangle aligned_src;
	aligned_src.x = 1;
	aligned_src.y = 2;
	aligned_src.width = 34;
	aligned_src.height = 34;

	xRectangle aligned_dst;
	aligned_dst.x = 1;
	aligned_dst.y = 2;
	aligned_dst.width = 34;
	aligned_dst.height = 34;

	Bool res = _check_hw_restriction ( &Scrn,  crtc_id,  buf_w,  buf_h,
	                       &src, &dst, &aligned_src, &aligned_dst);

	fail_if( res != FALSE, "align_dst less then MIN_WIDTH, FALSE must be returned" );
}
END_TEST;

START_TEST( ut__check_hw_restriction_alignedDstHeightMinusAlignedYLessThenMinHeight )
{
	xf86CrtcConfigPrivateIndex = 0;
	ScrnInfoRec Scrn;
	Scrn.virtualX = 40;
	Scrn.virtualY = 40;

	drmModeCrtc mode;
	mode.crtc_id = 1;

	SECCrtcPrivRec CrtcPriv;
	CrtcPriv.mode_crtc = &mode;

	xf86CrtcPtr crtc = ctrl_calloc(2 , sizeof( xf86CrtcPtr ));
	crtc[0].driver_private = &CrtcPriv;
	crtc[0].enabled = 1;
	crtc[0].active = 1;

	xf86CrtcConfigRec ConfCrtc;
	ConfCrtc.crtc = &crtc;
	ConfCrtc.num_crtc = 1;

 	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &ConfCrtc;

	int crtc_id = 1;
	int buf_w = 34;
	int buf_h = 34;

	xRectangle src;
	src.x = 1;
	src.y = 2;
	src.width = 34;
	src.height = 34;

	xRectangle dst;
	dst.x = 1;
	dst.y = 2;
	dst.width = 50;
	dst.height = 10;

	xRectangle aligned_src;
	aligned_src.x = 1;
	aligned_src.y = 2;
	aligned_src.width = 34;
	aligned_src.height = 34;

	xRectangle aligned_dst;
	aligned_dst.x = 1;
	aligned_dst.y = 2;
	aligned_dst.width = 34;
	aligned_dst.height = 34;

	Bool res = _check_hw_restriction ( &Scrn,  crtc_id,  buf_w,  buf_h,
	                       &src, &dst, &aligned_src, &aligned_dst);

	fail_if( res != FALSE, "align_dst less then MIN_HEIGHT, FALSE must be returned" );
}
END_TEST;

START_TEST( ut__check_hw_restriction_alignedSrcWidthMinusAlignedSrcXLessThenMinWidth )
{
	xf86CrtcConfigPrivateIndex = 0;
	ScrnInfoRec Scrn;
	Scrn.virtualX = 40;
	Scrn.virtualY = 40;

	drmModeCrtc mode;
	mode.crtc_id = 1;

	SECCrtcPrivRec CrtcPriv;
	CrtcPriv.mode_crtc = &mode;

	xf86CrtcPtr crtc = ctrl_calloc(2 , sizeof( xf86CrtcPtr ));
	crtc[0].driver_private = &CrtcPriv;
	crtc[0].enabled = 1;
	crtc[0].active = 1;

	xf86CrtcConfigRec ConfCrtc;
	ConfCrtc.crtc = &crtc;
	ConfCrtc.num_crtc = 1;

 	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &ConfCrtc;

	int crtc_id = 1;
	int buf_w = 34;
	int buf_h = 34;

	xRectangle src;
	src.x = 1;
	src.y = 2;
	src.width = 10;
	src.height = 34;

	xRectangle dst;
	dst.x = 1;
	dst.y = 2;
	dst.width = 50;
	dst.height = 50;

	xRectangle aligned_src;
	aligned_src.x = 1;
	aligned_src.y = 2;
	aligned_src.width = 34;
	aligned_src.height = 34;

	xRectangle aligned_dst;
	aligned_dst.x = 1;
	aligned_dst.y = 2;
	aligned_dst.width = 34;
	aligned_dst.height = 34;

	Bool res = _check_hw_restriction ( &Scrn,  crtc_id,  buf_w,  buf_h,
	                       &src, &dst, &aligned_src, &aligned_dst);

	fail_if( res != FALSE, "align_src less then MIN_WIDTH, FALSE must be returned" );
}
END_TEST;

START_TEST( ut__check_hw_restriction_alignedSrcHeightMinusAlignedSrcYLessThenMinHeight )
{
	xf86CrtcConfigPrivateIndex = 0;
	ScrnInfoRec Scrn;
	Scrn.virtualX = 40;
	Scrn.virtualY = 40;

	drmModeCrtc mode;
	mode.crtc_id = 1;

	SECCrtcPrivRec CrtcPriv;
	CrtcPriv.mode_crtc = &mode;

	xf86CrtcPtr crtc = ctrl_calloc(2 , sizeof( xf86CrtcPtr ));
	crtc[0].driver_private = &CrtcPriv;
	crtc[0].enabled = 1;
	crtc[0].active = 1;

	xf86CrtcConfigRec ConfCrtc;
	ConfCrtc.crtc = &crtc;
	ConfCrtc.num_crtc = 1;

 	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &ConfCrtc;

	int crtc_id = 1;
	int buf_w = 34;
	int buf_h = 34;

	xRectangle src;
	src.x = 1;
	src.y = 2;
	src.width = 50;
	src.height = 10;

	xRectangle dst;
	dst.x = 1;
	dst.y = 2;
	dst.width = 50;
	dst.height = 50;

	xRectangle aligned_src;
	aligned_src.x = 1;
	aligned_src.y = 2;
	aligned_src.width = 34;
	aligned_src.height = 34;

	xRectangle aligned_dst;
	aligned_dst.x = 1;
	aligned_dst.y = 2;
	aligned_dst.width = 34;
	aligned_dst.height = 34;

	Bool res = _check_hw_restriction ( &Scrn,  crtc_id,  buf_w,  buf_h,
	                       &src, &dst, &aligned_src, &aligned_dst);

	fail_if( res != FALSE, "align_src less then MIN_HEIGHT, FALSE must be returned" );
}
END_TEST;


START_TEST( ut__check_hw_restriction_corretWorkFlow )
{
	xf86CrtcConfigPrivateIndex = 0;
	ScrnInfoRec Scrn;
	Scrn.virtualX = 40;
	Scrn.virtualY = 40;

	drmModeCrtc mode;
	mode.crtc_id = 1;

	SECCrtcPrivRec CrtcPriv;
	CrtcPriv.mode_crtc = &mode;

	xf86CrtcPtr crtc = ctrl_calloc(2 , sizeof( xf86CrtcPtr ));
	crtc[0].driver_private = &CrtcPriv;
	crtc[0].enabled = 1;
	crtc[0].active = 1;

	xf86CrtcConfigRec ConfCrtc;
	ConfCrtc.crtc = &crtc;
	ConfCrtc.num_crtc = 1;

 	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &ConfCrtc;

	int crtc_id = 1;
	int buf_w = 34;
	int buf_h = 34;

	xRectangle src;
	src.x = 1;
	src.y = 2;
	src.width = 50;
	src.height = 50;

	xRectangle dst;
	dst.x = 1;
	dst.y = 2;
	dst.width = 50;
	dst.height = 50;

	xRectangle aligned_src;
	aligned_src.x = 1;
	aligned_src.y = 2;
	aligned_src.width = 34;
	aligned_src.height = 34;

	xRectangle aligned_dst;
	aligned_dst.x = 1;
	aligned_dst.y = 2;
	aligned_dst.width = 34;
	aligned_dst.height = 34;

	Bool res = _check_hw_restriction ( &Scrn,  crtc_id,  buf_w,  buf_h,
	                       &src, &dst, &aligned_src, &aligned_dst);

	fail_if( res != TRUE, "Correct work flow, TRUE must be returned" );
}
END_TEST;

/* _secPlaneShowInternal */
START_TEST( ut__secPlaneShowInternal_lcdIsOff )
{
	SECModeRec SecMode;
	SECRec Sec;
	ScrnInfoRec Scrn;
	Scrn.driverPrivate = &Sec;
	SECPlanePrivRec PlanePriv;
	PlanePriv.pScrn = &Scrn;
	PlanePriv.pSecMode = &SecMode;
	SECPlaneTable table;
	table.pPlanePriv = &PlanePriv;
	SECPlaneFb old_fb;
	SECPlaneFb new_fb;
    xRectangle new_src;
    xRectangle new_dst;
    int new_zpos = 0;
    Bool need_set_plane = FALSE;

	Sec.isLcdOff = TRUE;

	Bool res = _secPlaneShowInternal ( &table, &old_fb, &new_fb, &new_src,
									  &new_dst, new_zpos, need_set_plane);

	fail_if( res != FALSE, "LCD is off, FALSE must be returned" );

}
END_TEST;

START_TEST( ut__secPlaneShowInternal_tableIsOff )
{
	SECModeRec SecMode;
	SECRec Sec;
	ScrnInfoRec Scrn;
	Scrn.driverPrivate = &Sec;
	SECPlanePrivRec PlanePriv;
	PlanePriv.pScrn = &Scrn;
	PlanePriv.pSecMode = &SecMode;
	SECPlaneTable table;
	table.pPlanePriv = &PlanePriv;
	SECPlaneFb old_fb;
	SECPlaneFb new_fb;
    xRectangle new_src;
    xRectangle new_dst;
    int new_zpos = 0;
    Bool need_set_plane = FALSE;

	Sec.isLcdOff = FALSE;
	table.onoff = FALSE;
	Bool res = _secPlaneShowInternal ( &table, &old_fb, &new_fb, &new_src,
									  &new_dst, new_zpos, need_set_plane);

	fail_if( res != FALSE, "table is off, FALSE must be returned" );
}
END_TEST;

START_TEST( ut__secPlaneShowInternal_newZPosAndOldZPosAreNotEqual )
{
	SECModeRec SecMode;
	SecMode.fd = 100;
	SECRec Sec;
	ScrnInfoRec Scrn;
	Scrn.driverPrivate = &Sec;
	SECPlanePrivRec PlanePriv;
	PlanePriv.pScrn = &Scrn;
	PlanePriv.pSecMode = &SecMode;
	SECPlaneTable table;
	table.pPlanePriv = &PlanePriv;
	SECPlaneFb old_fb;
	SECPlaneFb new_fb;
    xRectangle new_src;
    xRectangle new_dst;
    int new_zpos = 1;
    Bool need_set_plane = FALSE;

	Sec.isLcdOff = FALSE;
	table.onoff = TRUE;
	table.zpos = 2;

	Bool res = _secPlaneShowInternal ( &table, &old_fb, &new_fb, &new_src,
									  &new_dst, new_zpos, need_set_plane);

	fail_if( res != FALSE, "New zpos and old zpos are not equal, FALSE must be returned" );
}
END_TEST;

START_TEST( ut__secPlaneShowInternal_lastIfIsFalse )
{
	SECModeRec SecMode;
	SECRec Sec;
	ScrnInfoRec Scrn;
	Scrn.driverPrivate = &Sec;
	SECPlanePrivRec PlanePriv;
	PlanePriv.pScrn = &Scrn;
	PlanePriv.pSecMode = &SecMode;
	SECPlaneTable table;
	table.pPlanePriv = &PlanePriv;
	SECPlaneFb old_fb;
	SECPlaneFb new_fb;
    xRectangle new_src;
    xRectangle new_dst;
    xRectangle t_src;
    xRectangle t_dst;
    int new_zpos = 1;
    Bool need_set_plane = FALSE;

	Sec.isLcdOff = FALSE;
	table.onoff = TRUE;
	table.zpos = 1;
	table.visible = 1;
	table.src = new_src;
    table.dst = new_dst;

	Bool res = _secPlaneShowInternal ( &table, &table.src, &table.src, &new_src,
									  &new_dst, new_zpos, need_set_plane);

	fail_if( res != TRUE, "Last if is FALSE, FALSE must be returned" );
}
END_TEST;

START_TEST( ut__secPlaneShowInternal_pCrtcPrivIsNULL )
{
	SECModeRec SecMode;
	SECRec Sec;
	xf86CrtcConfigRec ConfCrtc;
	ScrnInfoRec Scrn;
	Scrn.driverPrivate = &Sec;
	xf86CrtcConfigPrivateIndex = 0;
	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &ConfCrtc;

	SECPlanePrivRec PlanePriv;
	PlanePriv.pScrn = &Scrn;
	PlanePriv.pSecMode = &SecMode;
	SECPlaneTable table;
	table.pPlanePriv = &PlanePriv;
	SECPlaneFb old_fb;
	SECPlaneFb new_fb;
    xRectangle new_src;
    xRectangle new_dst;
    xRectangle t_src;
    xRectangle t_dst;
    int new_zpos = 1;
    Bool need_set_plane = TRUE;

	Sec.isLcdOff = FALSE;
	table.onoff = TRUE;
	table.zpos = 1;
	table.visible = 0;

	Bool res = _secPlaneShowInternal ( &table, &old_fb, &new_fb, &new_src,
									  &new_dst, new_zpos, need_set_plane);

	fail_if( res != FALSE, "Last if is FALSE, FALSE must be returned" );
}
END_TEST;

START_TEST( ut__secPlaneShowInternal__check_hw_restrictionReturnsFalse )
{
	SECModeRec SecMode;
	SECRec Sec;
	drmModeCrtcPtr pModeCrtc = ctrl_calloc( 1, sizeof( 	drmModeCrtc ) );
	SECCrtcPrivPtr pCrtcPriv = ctrl_calloc( 1, sizeof( SECCrtcPrivRec ) );
	pCrtcPriv->mode_crtc = pModeCrtc;
	pModeCrtc->crtc_id = 5;
	xf86CrtcPtr pCrtc = ctrl_calloc( 2, sizeof( xf86CrtcRec ) );
	pCrtc->driver_private = pCrtcPriv;
 	xf86CrtcConfigRec ConfCrtc;
	ConfCrtc.num_crtc  = 1;
	ConfCrtc.crtc = &pCrtc;
	ConfCrtc.crtc[0]->desiredX = 2;
	ScrnInfoRec Scrn;
	Scrn.driverPrivate = &Sec;
	xf86CrtcConfigPrivateIndex = 0;
	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &ConfCrtc;

	SECPlanePrivRec PlanePriv;
	PlanePriv.pScrn = &Scrn;
	PlanePriv.pSecMode = &SecMode;
	SECPlaneFb ffb;
	ffb.width = 100;
	ffb.height = 100;
	SECPlaneTable table;
	table.pPlanePriv = &PlanePriv;
	table.crtc_id = 5;
	table.cur_fb = &ffb;
	SECPlaneFb old_fb;
	SECPlaneFb new_fb;
	SECVideoBuf vbuf;
	vbuf.id = 5;
	new_fb.buffer.vbuf = &vbuf;
    xRectangle new_src;
    xRectangle new_dst;
    xRectangle t_src;
    xRectangle t_dst;
    int new_zpos = 1;
    Bool need_set_plane = TRUE;

	Sec.isLcdOff = FALSE;
	table.onoff = TRUE;
	table.zpos = 1;
	table.visible = 0;

	Bool res = _secPlaneShowInternal ( &table, &old_fb, &new_fb, &new_src,
									  &new_dst, new_zpos, need_set_plane);

	fail_if( res != TRUE, "_check_hw_restriction return TRUE, TRUE must be returned" );
}
END_TEST;


START_TEST( ut__secPlaneShowInternal_drmModeSetPlaneReturnsTrue )
{
	SECRec Sec;
	drmModeCrtcPtr pModeCrtc = ctrl_calloc( 1, sizeof( 	drmModeCrtc ) );
	SECCrtcPrivPtr pCrtcPriv = ctrl_calloc( 1, sizeof( SECCrtcPrivRec ) );
	pCrtcPriv->mode_crtc = pModeCrtc;
	pModeCrtc->crtc_id = 5;
	xf86CrtcPtr pCrtc = ctrl_calloc( 2, sizeof( xf86CrtcRec ) );
	pCrtc->driver_private = pCrtcPriv;
	pCrtc->active = 1;
	pCrtc->enabled = 1;
 	xf86CrtcConfigRec ConfCrtc;
	ConfCrtc.num_crtc = 1;
	ConfCrtc.crtc = &pCrtc;
	ConfCrtc.crtc[0]->desiredX = 2;
	ScrnInfoRec Scrn;
	Scrn.driverPrivate = &Sec;
	Scrn.virtualX = 1000;
	Scrn.virtualY = 1000;
	xf86CrtcConfigPrivateIndex = 0;
	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &ConfCrtc;
	SECModeRec SecMode;
	SecMode.fd = 100;
	SECPlanePrivRec PlanePriv;
	PlanePriv.pScrn = &Scrn;
	PlanePriv.pSecMode = &SecMode;
	SECPlaneFb ffb;
	ffb.width = 100;
	ffb.height = 100;
	SECPlaneTable table;
	table.pPlanePriv = &PlanePriv;
	table.crtc_id = 5;
	table.cur_fb = &ffb;
	SECPlaneFb old_fb;
	SECPlaneFb new_fb;
	SECVideoBuf vbuf;
	vbuf.id = 5;
	new_fb.buffer.vbuf = &vbuf;
    xRectangle new_src;
    new_src.width = 100;
    new_src.height = 100;
    xRectangle new_dst;
    new_dst.width = 100;
    new_dst.height = 100;
    xRectangle t_src;
    xRectangle t_dst;
    int new_zpos = 1;
    Bool need_set_plane = TRUE;

	Sec.isLcdOff = FALSE;
	table.onoff = TRUE;
	table.zpos = 1;
	table.visible = 0;

	Bool res = _secPlaneShowInternal ( &table, &old_fb, &new_fb, &new_src,
									  &new_dst, new_zpos, need_set_plane);

	fail_if( res != FALSE, "drmModeSetPlane returned TRUE, FALSE must be returned" );
}
END_TEST;

START_TEST( ut__secPlaneShowInternal_corrcetcWorkFlowTrueByMainIf )
{
	SECRec Sec;
	drmModeCrtcPtr pModeCrtc = ctrl_calloc( 1, sizeof( 	drmModeCrtc ) );
	SECCrtcPrivPtr pCrtcPriv = ctrl_calloc( 1, sizeof( SECCrtcPrivRec ) );
	pCrtcPriv->mode_crtc = pModeCrtc;
	pModeCrtc->crtc_id = 5;
	xf86CrtcPtr pCrtc = ctrl_calloc( 2, sizeof( xf86CrtcRec ) );
	pCrtc->driver_private = pCrtcPriv;
	pCrtc->active = 1;
	pCrtc->enabled = 1;
 	xf86CrtcConfigRec ConfCrtc;
	ConfCrtc.num_crtc = 1;
	ConfCrtc.crtc = &pCrtc;
	ConfCrtc.crtc[0]->desiredX = 2;
	ScrnInfoRec Scrn;
	Scrn.driverPrivate = &Sec;
	Scrn.virtualX = 1000;
	Scrn.virtualY = 1000;
	xf86CrtcConfigPrivateIndex = 0;
	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &ConfCrtc;
	SECModeRec SecMode;
	SecMode.fd = 1;
	SECPlanePrivRec PlanePriv;
	PlanePriv.pScrn = &Scrn;
	PlanePriv.pSecMode = &SecMode;
	SECPlaneFb ffb;
	ffb.width = 100;
	ffb.height = 100;
	SECPlaneTable table;
	table.pPlanePriv = &PlanePriv;
	table.crtc_id = 5;
	table.cur_fb = &ffb;
	table.visible = 0;
	SECPlaneFb old_fb;
	SECPlaneFb new_fb;
	SECVideoBuf vbuf;
	vbuf.id = 5;
	new_fb.buffer.vbuf = &vbuf;
    xRectangle new_src;
    new_src.width = 100;
    new_src.height = 100;
    xRectangle new_dst;
    new_dst.width = 100;
    new_dst.height = 100;
    xRectangle t_src;
    xRectangle t_dst;
    int new_zpos = 1;
    Bool need_set_plane = TRUE;

	Sec.isLcdOff = FALSE;
	table.onoff = TRUE;
	table.zpos = 1;
	table.visible = 0;

	Bool res = _secPlaneShowInternal ( &table, &old_fb, &new_fb, &new_src,
									  &new_dst, new_zpos, need_set_plane);

	fail_if( res != TRUE, "correct work flow TRUE by main if, TRUE must be returned" );
	fail_if( table.visible != 1, "table->visible must be changed");
}
END_TEST;

START_TEST( ut__secPlaneShowInternal_heightsWidthsAreZeros )
{
	SECRec Sec;
	drmModeCrtcPtr pModeCrtc = ctrl_calloc( 1, sizeof( 	drmModeCrtc ) );
	pModeCrtc->crtc_id = 5;
	SECCrtcPrivPtr pCrtcPriv = ctrl_calloc( 1, sizeof( SECCrtcPrivRec ) );
	pCrtcPriv->mode_crtc = pModeCrtc;
	pCrtcPriv->bAccessibility = TRUE;
	xf86CrtcPtr pCrtc = ctrl_calloc( 2, sizeof( xf86CrtcRec ) );
	pCrtc->driver_private = pCrtcPriv;
	pCrtc->active = 1;
	pCrtc->enabled = 1;
 	xf86CrtcConfigRec ConfCrtc;
	ConfCrtc.num_crtc = 1;
	ConfCrtc.crtc = &pCrtc;
	ConfCrtc.crtc[0]->desiredX = 2;
	ScrnInfoRec Scrn;
	Scrn.driverPrivate = &Sec;
	Scrn.virtualX = 1000;
	Scrn.virtualY = 1000;
	xf86CrtcConfigPrivateIndex = 0;
	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &ConfCrtc;
	SECModeRec SecMode;
	SecMode.fd = 1;
	SECPlanePrivRec PlanePriv;
	PlanePriv.pScrn = &Scrn;
	PlanePriv.pSecMode = &SecMode;
	SECPlaneFb ffb;
	ffb.width = 100;
	ffb.height = 100;
	SECPlaneTable table;
	table.pPlanePriv = &PlanePriv;
	table.crtc_id = 5;
	table.cur_fb = &ffb;
	table.visible = 0;
	SECPlaneFb old_fb;
	SECPlaneFb new_fb;
	SECVideoBuf vbuf;
	vbuf.id = 5;
	new_fb.buffer.vbuf = &vbuf;
    xRectangle new_src;
    xRectangle new_dst;
    xRectangle t_src;
    xRectangle t_dst;
    int new_zpos = 1;
    Bool need_set_plane = TRUE;

	Sec.isLcdOff = FALSE;
	table.onoff = TRUE;
	table.zpos = 1;
	table.visible = 0;

	Bool res = _secPlaneShowInternal ( &table, &old_fb, &new_fb, &new_src,
									  &new_dst, new_zpos, need_set_plane);

	fail_if( res != TRUE,
			"fb_src.width fb_src.height access.width access.height are zero, TRUE must be returned" );
}
END_TEST;

START_TEST( ut__secPlaneShowInternal_drmAddFBFails )
{
	SECRec Sec;
	drmModeCrtcPtr pModeCrtc = ctrl_calloc( 1, sizeof( 	drmModeCrtc ) );
	pModeCrtc->crtc_id = 5;
	SECCrtcPrivPtr pCrtcPriv = ctrl_calloc( 1, sizeof( SECCrtcPrivRec ) );
	pCrtcPriv->mode_crtc = pModeCrtc;
	pCrtcPriv->bAccessibility = TRUE;
	xf86CrtcPtr pCrtc = ctrl_calloc( 2, sizeof( xf86CrtcRec ) );
	pCrtc->driver_private = pCrtcPriv;
	pCrtc->active = 1;
	pCrtc->enabled = 1;
 	xf86CrtcConfigRec ConfCrtc;
	ConfCrtc.num_crtc = 1;
	ConfCrtc.crtc = &pCrtc;
	ConfCrtc.crtc[0]->desiredX = 2;
	ScrnInfoRec Scrn;
	Scrn.driverPrivate = &Sec;
	Scrn.virtualX = 1000;
	Scrn.virtualY = 1000;
	xf86CrtcConfigPrivateIndex = 0;
	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &ConfCrtc;
	SECModeRec SecMode;
	SecMode.fd = 100;
	SECPlanePrivRec PlanePriv;
	PlanePriv.pScrn = &Scrn;
	PlanePriv.pSecMode = &SecMode;
	SECPlaneFb ffb;
	ffb.width = 100;
	ffb.height = 100;
	SECPlaneTable table;
	table.pPlanePriv = &PlanePriv;
	table.crtc_id = 5;
	table.cur_fb = &ffb;
	table.visible = 0;
	SECPlaneFb old_fb;
	SECPlaneFb new_fb;
	SECVideoBuf vbuf;
	vbuf.id = 5;
	new_fb.buffer.vbuf = &vbuf;
    xRectangle new_src;
    new_src.width = 100;
    new_src.height = 100;
    xRectangle new_dst;
    new_dst.width = 100;
    new_dst.height = 100;
    xRectangle t_src;
    xRectangle t_dst;
    int new_zpos = 1;
    Bool need_set_plane = TRUE;

	Sec.isLcdOff = FALSE;
	table.onoff = TRUE;
	table.zpos = 1;
	table.visible = 0;

	Bool res = _secPlaneShowInternal ( &table, &old_fb, &new_fb, &new_src,
									  &new_dst, new_zpos, need_set_plane);

	fail_if( res != FALSE, "drmAddFb failed, FALSE must be returned");
	fail_if( 1 != ctrl_get_cnt_mem_obj( TBM_OBJ ), "TBM_OBG must be allocated");
}
END_TEST;

START_TEST( ut__secPlaneShowInternal_access_fb_idIsNotRelevant )
{
	SECRec Sec;
	drmModeCrtcPtr pModeCrtc = ctrl_calloc( 1, sizeof( 	drmModeCrtc ) );
	pModeCrtc->crtc_id = 5;
	SECCrtcPrivPtr pCrtcPriv = ctrl_calloc( 1, sizeof( SECCrtcPrivRec ) );
	pCrtcPriv->mode_crtc = pModeCrtc;
	pCrtcPriv->bAccessibility = TRUE;
	xf86CrtcPtr pCrtc = ctrl_calloc( 2, sizeof( xf86CrtcRec ) );
	pCrtc->driver_private = pCrtcPriv;
	pCrtc->active = 1;
	pCrtc->enabled = 1;
 	xf86CrtcConfigRec ConfCrtc;
	ConfCrtc.num_crtc = 1;
	ConfCrtc.crtc = &pCrtc;
	ConfCrtc.crtc[0]->desiredX = 2;
	ScrnInfoRec Scrn;
	Scrn.driverPrivate = &Sec;
	Scrn.virtualX = 1000;
	Scrn.virtualY = 1000;
	xf86CrtcConfigPrivateIndex = 0;
	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &ConfCrtc;
	SECModeRec SecMode;
	SecMode.fd = 1;
	SECPlanePrivRec PlanePriv;
	PlanePriv.pScrn = &Scrn;
	PlanePriv.pSecMode = &SecMode;
	SECPlaneFb ffb;
	ffb.width = 100;
	ffb.height = 100;
	SECPlaneTable table;
	table.pPlanePriv = &PlanePriv;
	table.crtc_id = 5;
	table.cur_fb = &ffb;
	table.visible = 0;
	SECPlaneFb old_fb;
	SECPlaneFb new_fb;
	SECVideoBuf vbuf;
	vbuf.id = 5;
	new_fb.buffer.vbuf = &vbuf;
    xRectangle new_src;
    new_src.width = 100;
    new_src.height = 100;
    xRectangle new_dst;
    new_dst.width = 100;
    new_dst.height = 100;
    xRectangle t_src;
    xRectangle t_dst;
    int new_zpos = 1;
    Bool need_set_plane = TRUE;

	Sec.isLcdOff = FALSE;
	table.onoff = TRUE;
	table.zpos = 1;
	table.visible = 0;

	Bool res = _secPlaneShowInternal ( &table, &old_fb, &new_fb, &new_src,
									  &new_dst, new_zpos, need_set_plane);

	fail_if( res != FALSE, "access->fb_id <= 0, FALSE must be returned");
	fail_if( 1 != ctrl_get_cnt_mem_obj( TBM_OBJ ), "TBM_OBG must be allocated");
}
END_TEST;

START_TEST( ut__secPlaneShowInternal_src_boIsNull )
{
	SECRec Sec;
	drmModeCrtcPtr pModeCrtc = ctrl_calloc( 1, sizeof( 	drmModeCrtc ) );
	pModeCrtc->crtc_id = 5;
	SECCrtcPrivPtr pCrtcPriv = ctrl_calloc( 1, sizeof( SECCrtcPrivRec ) );
	pCrtcPriv->mode_crtc = pModeCrtc;
	pCrtcPriv->bAccessibility = TRUE;
	xf86CrtcPtr pCrtc = ctrl_calloc( 2, sizeof( xf86CrtcRec ) );
	pCrtc->driver_private = pCrtcPriv;
	pCrtc->active = 1;
	pCrtc->enabled = 1;
 	xf86CrtcConfigRec ConfCrtc;
	ConfCrtc.num_crtc = 1;
	ConfCrtc.crtc = &pCrtc;
	ConfCrtc.crtc[0]->desiredX = 2;
	ScrnInfoRec Scrn;
	Scrn.driverPrivate = &Sec;
	Scrn.virtualX = 1000;
	Scrn.virtualY = 1000;
	xf86CrtcConfigPrivateIndex = 0;
	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &ConfCrtc;
	SECModeRec SecMode;
	SecMode.fd = 1;
	SECPlanePrivRec PlanePriv;
	PlanePriv.pScrn = &Scrn;
	PlanePriv.pSecMode = &SecMode;
	SECPlaneFb ffb;
	ffb.width = 100;
	ffb.height = 100;
	SECPlaneTable table;
	table.pPlanePriv = &PlanePriv;
	table.crtc_id = 5;
	table.cur_fb = &ffb;
	table.visible = 0;
	SECPlaneFb old_fb;
	SECPlaneFb new_fb;
	SECVideoBuf vbuf;
	vbuf.id = 5;
	new_fb.buffer.vbuf = &vbuf;
    xRectangle new_src;
    new_src.width = 100;
    new_src.height = 100;
    xRectangle new_dst;
    new_dst.width = 100;
    new_dst.height = 100;
    xRectangle t_src;
    xRectangle t_dst;
    int new_zpos = 1;
    Bool need_set_plane = TRUE;

	Sec.isLcdOff = FALSE;
	table.onoff = TRUE;
	table.zpos = 1;
	table.visible = 0;

	Bool res = _secPlaneShowInternal ( &table, &old_fb, &new_fb, &new_src,
									  &new_dst, new_zpos, need_set_plane);

	fail_if( res != FALSE, "access->fb_id <= 0, FALSE must be returned");
	fail_if( 1 != ctrl_get_cnt_mem_obj( TBM_OBJ ), "TBM_OBG must be allocated");
}
END_TEST;

START_TEST( ut__secPlaneShowInternal__check_hw_restrictionReturnsFalse2 )
{
	SECRec Sec;
	drmModeCrtcPtr pModeCrtc = ctrl_calloc( 1, sizeof( 	drmModeCrtc ) );
	pModeCrtc->crtc_id = 5;
	SECCrtcPrivPtr pCrtcPriv = ctrl_calloc( 1, sizeof( SECCrtcPrivRec ) );
	pCrtcPriv->mode_crtc = pModeCrtc;
	pCrtcPriv->bAccessibility = TRUE;
	xf86CrtcPtr pCrtc = ctrl_calloc( 2, sizeof( xf86CrtcRec ) );
	pCrtc->driver_private = pCrtcPriv;
	pCrtc->active = 1;
	pCrtc->enabled = 1;
 	xf86CrtcConfigRec ConfCrtc;
	ConfCrtc.num_crtc = 1;
	ConfCrtc.crtc = &pCrtc;
	ConfCrtc.crtc[0]->desiredX = 2;
	ScrnInfoRec Scrn;
	Scrn.driverPrivate = &Sec;
	Scrn.virtualX = 0;
	Scrn.virtualY = 0;
	xf86CrtcConfigPrivateIndex = 0;
	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &ConfCrtc;
	SECModeRec SecMode;
	SecMode.fd = 101;
	SECPlanePrivRec PlanePriv;
	PlanePriv.pScrn = &Scrn;
	PlanePriv.pSecMode = &SecMode;
	SECPlaneFb ffb;
	ffb.width = 0;
	ffb.height = 0;
	SECPlaneTable table;
	table.pPlanePriv = &PlanePriv;
	table.crtc_id = 5;
	table.cur_fb = &ffb;
	table.visible = 0;
	SECPlaneFb old_fb;
	SECPlaneFb new_fb;
	SECVideoBuf vbuf;
	vbuf.id = 5;
	new_fb.buffer.vbuf = &vbuf;
	new_fb.buffer.bo = tbm_bo_alloc( 0, 1, 0 );
     xRectangle new_src;
    new_src.width = 100;
    new_src.height = 100;
    xRectangle new_dst;
    new_dst.width = 100;
    new_dst.height = 100;
    xRectangle t_src;
    xRectangle t_dst;
    int new_zpos = 1;
    Bool need_set_plane = TRUE;

	Sec.isLcdOff = FALSE;
	table.onoff = TRUE;
	table.zpos = 1;
	table.visible = 0;

	Bool res = _secPlaneShowInternal ( &table, &old_fb, &new_fb, &new_src,
									  &new_dst, new_zpos, need_set_plane);

	fail_if( res != TRUE, "_check_hw_restriction return TRUE, TRUE must be returned" );
	fail_if( 2 != ctrl_get_cnt_mem_obj( TBM_OBJ ), "TBM_OBG must be allocated");
}
END_TEST;

START_TEST( ut__secPlaneShowInternal_drmModeSetPlaneReturnsTrue2 )
{
	SECRec Sec;
	drmModeCrtcPtr pModeCrtc = ctrl_calloc( 1, sizeof( 	drmModeCrtc ) );
	pModeCrtc->crtc_id = 5;
	SECCrtcPrivPtr pCrtcPriv = ctrl_calloc( 1, sizeof( SECCrtcPrivRec ) );
	pCrtcPriv->mode_crtc = pModeCrtc;
	pCrtcPriv->bAccessibility = TRUE;
	xf86CrtcPtr pCrtc = ctrl_calloc( 2, sizeof( xf86CrtcRec ) );
	pCrtc->driver_private = pCrtcPriv;
	pCrtc->active = 1;
	pCrtc->enabled = 1;
 	xf86CrtcConfigRec ConfCrtc;
	ConfCrtc.num_crtc = 1;
	ConfCrtc.crtc = &pCrtc;
	ConfCrtc.crtc[0]->desiredX = 2;
	ScrnInfoRec Scrn;
	Scrn.driverPrivate = &Sec;
	Scrn.virtualX = 500;
	Scrn.virtualY = 500;
	xf86CrtcConfigPrivateIndex = 0;
	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &ConfCrtc;
	SECModeRec SecMode;
	SecMode.fd = 101;
	SECPlanePrivRec PlanePriv;
	PlanePriv.pScrn = &Scrn;
	PlanePriv.pSecMode = &SecMode;
	SECPlaneFb ffb;
	ffb.width = 100;
	ffb.height = 100;
	SECPlaneTable table;
	table.pPlanePriv = &PlanePriv;
	table.crtc_id = 5;
	table.cur_fb = &ffb;
	table.visible = 0;
	SECPlaneFb old_fb;
	SECPlaneFb new_fb;
	SECVideoBuf vbuf;
	vbuf.id = 5;
	new_fb.buffer.vbuf = &vbuf;
	new_fb.buffer.bo = tbm_bo_alloc( 0, 1, 0 );
     xRectangle new_src;
    new_src.width = 100;
    new_src.height = 100;
    xRectangle new_dst;
    new_dst.width = 100;
    new_dst.height = 100;
    xRectangle t_src;
    xRectangle t_dst;
    int new_zpos = 1;
    Bool need_set_plane = TRUE;

	Sec.isLcdOff = FALSE;
	table.onoff = TRUE;
	table.zpos = 1;
	table.visible = 0;

	Bool res = _secPlaneShowInternal ( &table, &old_fb, &new_fb, &new_src,
									  &new_dst, new_zpos, need_set_plane);

	fail_if( res != FALSE, "drmModeSetPlane returned TRUE, FALSE must be returned" );
	fail_if( 2 != ctrl_get_cnt_mem_obj( TBM_OBJ ), "TBM_OBG must be allocated");
}
END_TEST;

START_TEST( ut__secPlaneShowInternal_corrcetcWorkFlowFalseByMainIf )
{
	SECRec Sec;
	drmModeCrtcPtr pModeCrtc = ctrl_calloc( 1, sizeof( 	drmModeCrtc ) );
	pModeCrtc->crtc_id = 5;
	SECCrtcPrivPtr pCrtcPriv = ctrl_calloc( 1, sizeof( SECCrtcPrivRec ) );
	pCrtcPriv->mode_crtc = pModeCrtc;
	pCrtcPriv->bAccessibility = TRUE;
	xf86CrtcPtr pCrtc = ctrl_calloc( 2, sizeof( xf86CrtcRec ) );
	pCrtc->driver_private = pCrtcPriv;
	pCrtc->active = 1;
	pCrtc->enabled = 1;
 	xf86CrtcConfigRec ConfCrtc;
	ConfCrtc.num_crtc = 1;
	ConfCrtc.crtc = &pCrtc;
	ConfCrtc.crtc[0]->desiredX = 2;
	ScrnInfoRec Scrn;
	Scrn.driverPrivate = &Sec;
	Scrn.virtualX = 500;
	Scrn.virtualY = 500;
	xf86CrtcConfigPrivateIndex = 0;
	DevUnion *privates = ctrl_calloc( 2, sizeof(DevUnion) );
	Scrn.privates = privates;
	Scrn.privates[xf86CrtcConfigPrivateIndex].ptr = &ConfCrtc;
	SECModeRec SecMode;
	SecMode.fd = 102;
	SECPlanePrivRec PlanePriv;
	PlanePriv.pScrn = &Scrn;
	PlanePriv.pSecMode = &SecMode;
	SECPlaneFb ffb;
	ffb.width = 100;
	ffb.height = 100;
	SECPlaneTable table;
	table.pPlanePriv = &PlanePriv;
	table.crtc_id = 5;
	table.cur_fb = &ffb;
	table.visible = 0;
	SECPlaneFb old_fb;
	SECPlaneFb new_fb;
	SECVideoBuf vbuf;
	vbuf.id = 5;
	new_fb.buffer.vbuf = &vbuf;
	new_fb.buffer.bo = tbm_bo_alloc( 0, 1, 0 );
     xRectangle new_src;
    new_src.width = 100;
    new_src.height = 100;
    xRectangle new_dst;
    new_dst.width = 100;
    new_dst.height = 100;
    xRectangle t_src;
    xRectangle t_dst;
    int new_zpos = 1;
    Bool need_set_plane = TRUE;

	Sec.isLcdOff = FALSE;
	table.onoff = TRUE;
	table.zpos = 1;
	table.visible = 0;

	Bool res = _secPlaneShowInternal ( &table, &old_fb, &new_fb, &new_src,
									  &new_dst, new_zpos, need_set_plane);

	fail_if( res != TRUE, "correct work flow TRUE by main if, TRUE must be returned" );
	fail_if( table.visible != 1, "table->visible must be changed");
	fail_if( 2 != ctrl_get_cnt_mem_obj( TBM_OBJ ), "TBM_OBG must be allocated");
}
END_TEST;

/* _secPlaneHideInternal */
START_TEST( ut__secPlaneHideInternal_tableIsNULL )
{
	SECPlaneTable table;

	Bool ret = _secPlaneHideInternal ( NULL );

	fail_if( ret != FALSE, "table is NULL, FALSE must be returned" );
}
END_TEST;

START_TEST( ut___secPlaneHideInternal_tableIsNotVisible )
{
	SECPlaneTable table;
	table.visible = FALSE;
	table.crtc_id = -1;

	Bool ret = _secPlaneHideInternal ( &table );

	fail_if( ret != TRUE, "table is not visible, TRUE must be returned" );
}
END_TEST;

START_TEST( ut___secPlaneHideInternal_crtc_idIsNotRelevant )
{
	SECPlaneTable table;
	table.visible = TRUE;
	table.crtc_id = -1;

	Bool ret = _secPlaneHideInternal ( &table );

	fail_if( ret != FALSE, "crtc_id is not relevant, FALSE must be returned" );
}
END_TEST;

START_TEST( ut___secPlaneHideInternal_drmSetPlaneFails )
{
	SECRec sec;
	sec.isLcdOff = FALSE;
	SECModeRec secMode;
	secMode.fd = 100;
	ScrnInfoRec scrn;
	scrn.driverPrivate = &sec;
	SECPlanePrivRec planePriv;
	planePriv.pScrn = &scrn;
	planePriv.pSecMode = &secMode;
	SECPlaneFb planeFb;
	SECPlaneTable table;
	table.pPlanePriv = &planePriv;
	table.visible = TRUE;
	table.crtc_id = 1;
	table.onoff = 1;
	table.cur_fb = &planeFb;

	Bool ret = _secPlaneHideInternal ( &table );

	fail_if( ret != FALSE, "drmSetPlane return True, FALSE must be returned" );
}
END_TEST;

START_TEST( ut___secPlaneHideInternal_correctWorkFlow )
{
	SECRec sec;
	sec.isLcdOff = FALSE;
	SECModeRec secMode;
	secMode.fd = 1;
	ScrnInfoRec scrn;
	scrn.driverPrivate = &sec;
	SECPlanePrivRec planePriv;
	planePriv.pScrn = &scrn;
	planePriv.pSecMode = &secMode;
	SECPlaneFb planeFb;
	SECPlaneTable table;
	table.pPlanePriv = &planePriv;
	table.visible = TRUE;
	table.crtc_id = 1;
	table.onoff = 1;
	table.cur_fb = &planeFb;
	table.visible = TRUE;

	Bool ret = _secPlaneHideInternal ( &table );

	fail_if( table.visible != FALSE, "table.visible wasn't changed");
	fail_if( ret != TRUE, "correct work flow, FALSE must be returned" );
}
END_TEST;

/* secPlaneInit */
START_TEST( ut_secPlaneInit_pScrnIsNull )
{
	int num = 0;

	/* Segmentation fault must not occurs */
	secPlaneInit( NULL , NULL, num);
}
END_TEST;

START_TEST( ut_secPlaneInit_pSecModeIsNull )
{
	ScrnInfoRec scrn;
	int num = 0;

	/* Segmentation fault must not occurs */
	secPlaneInit( &scrn, NULL, num);
}
END_TEST;

START_TEST( ut_secPlaneInit_plane_resAreNull )
{
	ScrnInfoRec scrn;
	SECModeRec secMode;
	secMode.plane_res = NULL;

	int num = 0;

	/* Segmentation fault must not occurs */
	secPlaneInit( &scrn , &secMode, num);
}
END_TEST;

START_TEST( ut_secPlaneInit_plane_resCountPlanesIsZero )
{
	ScrnInfoRec scrn;
	drmModePlaneRes plane_res;
	plane_res.count_planes = 0;
	SECModeRec secMode;
	secMode.plane_res = &plane_res;
	int num = 0;

	/* Segmentation fault must not occurs */
	secPlaneInit( &scrn , &secMode, num);
}
END_TEST;

START_TEST( ut_secPlaneInit__mode_planeIsNull)
{
	ScrnInfoRec scrn;
	drmModePlaneRes plane_res;
	plane_res.count_planes = 1;
	plane_res.planes = ctrl_calloc( 2, sizeof( uint32_t ) );
	SECModeRec secMode;
	secMode.plane_res = &plane_res;
	secMode.fd = 100;
	int num = 0;

	/* Segmentation fault must not occurs */
	secPlaneInit( &scrn , &secMode, num);

	ctrl_free( plane_res.planes );

	/*pPlanePriv should be freed*/
	fail_if( ctrl_get_cnt_calloc() - 1, "Memory should be freed" );
}
END_TEST;

START_TEST( ut_secPlaneInit_correctWorkFlow )
{
	ScrnInfoRec scrn;
	drmModePlaneRes plane_res;
	plane_res.count_planes = 1;
	plane_res.planes = ctrl_calloc( 2, sizeof( uint32_t ) );
	SECModeRec secMode;
	secMode.plane_res = &plane_res;
	secMode.fd = 0;
	int num = 0;

	xorg_list_init( &secMode.planes );

	/* Segmentation fault must not occurs */
	secPlaneInit( &scrn , &secMode, num );

	ctrl_free( plane_res.planes );

	/*pPlanePriv should be freed*/
	fail_if( 2 != ctrl_get_cnt_calloc(), "Memory should be freed" );
}
END_TEST;

/* secPlaneDeinit */
START_TEST( ut_secPlaneDeinit_pScrnIsNull )
{
	/* Segmentation fault must not occurs */
	secPlaneDeinit ( NULL, NULL );
}
END_TEST;

START_TEST( ut_secPlaneDeinit_pPlanePrivIsNull )
{
	ScrnInfoRec scrn;

	/* Segmentation fault must not occurs */
	secPlaneDeinit ( &scrn, NULL );
}
END_TEST;

START_TEST( ut_secPlaneDeinit_memoryMenegment )
{
	plane_table = ctrl_calloc( 2, sizeof( SECPlaneTable ) );
	plane_table[0].plane_id = 1;
	plane_table[1].plane_id = -1;

	plane_table_size = 2;

	ScrnInfoPtr pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	SECPlanePrivPtr pPlanePriv = ctrl_calloc( 1, sizeof( SECPlanePrivRec ) );
	pPlanePriv->plane_id = 1;

	xorg_list_init( &pPlanePriv->link );
	xorg_list_init( &plane_table[0].fbs );
	xorg_list_init( &plane_table[1].fbs );

	secPlaneDeinit ( pScrn, pPlanePriv );

	ctrl_free( pScrn );

	fail_if( 0 != ctrl_get_cnt_calloc(), "Memory should be freed" );
}
END_TEST;

/* secPlaneGetID */
START_TEST( ut_secPlaneGetID_idIsNotAvailable )
{
	plane_table = ctrl_calloc( 2, sizeof( SECPlaneTable ) );
	plane_table[0].in_use = 1;
	plane_table[1].in_use = 1;

	plane_table_size = 2;
	int ret = secPlaneGetID();

	fail_if( -1 != ret, "Not available id, -1 should be returned" );
}
END_TEST;

START_TEST( ut_secPlaneGetID_correctWorkFlow )
{
	plane_table = ctrl_calloc( 2, sizeof( SECPlaneTable ) );
	plane_table[0].in_use = 1;
	plane_table[1].in_use = 0;
	plane_table[1].plane_id = 1;

	plane_table_size = 2;
	int ret = secPlaneGetID();

	fail_if( 1 != ret, "Not available id, -1 should be returned" );
}
END_TEST;

/* secPlaneFreeId */
START_TEST( ut_secPlaneFreeId_tableIsNull )
{
	int plane_id = -1;
	secPlaneFreeId ( plane_id );
}
END_TEST;

START_TEST( ut_secPlaneFreeId_CorrectWorkFlow )
{
	plane_table_size = 2;

	plane_table = ctrl_calloc( 3, sizeof( SECPlaneTable ) );
	plane_table[1].plane_id = 1;

	xorg_list_init( &plane_table[0].fbs );
	xorg_list_init( &plane_table[1].fbs );

	SECModeRec secMode;
	SECPlanePrivRec planePriv;
	planePriv.pSecMode = &secMode;

	plane_table[1].access = ctrl_calloc( 1, sizeof( SECPlaneAccess ) );
	plane_table[1].access->fb_id = 1;
	plane_table[1].access->bo = tbm_bo_alloc(0,1,0);
	plane_table[1].pPlanePriv = &planePriv;
	plane_table[1].in_use = TRUE;
	plane_table[1].onoff = FALSE;

	int plane_id = 1;
	secPlaneFreeId ( plane_id );

	ctrl_free( plane_table );

	fail_if( 0 != ctrl_get_cnt_calloc(), "Memory should be freed" );
	fail_if( plane_table[1].in_use != FALSE, "Variable should be changed" );
	fail_if( plane_table[1].onoff != TRUE, "Variable should be changed" );
}
END_TEST;

/* secPlaneTrun */
START_TEST( ut_secPlaneTrun_tableIsNull )
{
	int plane_id = -1;
	Bool onoff = FALSE;
	Bool user = FALSE;

	Bool ret = secPlaneTrun ( plane_id, onoff, user );

	fail_if( 0 != ctrl_get_cnt_calloc(), "Table is null, FALSE should be returned" );
}
END_TEST;

START_TEST( ut_secPlaneTrun_LCDIsOff )
{
	plane_table_size = 2;

	plane_table = ctrl_calloc( 3, sizeof( SECPlaneTable ) );
	plane_table[1].plane_id = 1;


	SECRec sec;
	sec.isLcdOff = TRUE;
	ScrnInfoRec scrn;
	scrn.driverPrivate = &sec;
	SECPlanePrivRec planePriv;
	planePriv.pScrn = &scrn;
	plane_table[1].pPlanePriv = &planePriv;

	int plane_id = 1;
	Bool onoff = FALSE;
	Bool user = FALSE;

	Bool ret = secPlaneTrun ( plane_id, onoff, user );

	fail_if( ret != TRUE, "LCD is off, TRUE should be returned" );
}
END_TEST;

START_TEST( ut_secPlaneTrun_tableonofAndOnoffAreEqual )
{
	plane_table_size = 2;

	plane_table = ctrl_calloc( 3, sizeof( SECPlaneTable ) );
	plane_table[1].plane_id = 1;


	SECRec sec;
	sec.isLcdOff = FALSE;
	ScrnInfoRec scrn;
	scrn.driverPrivate = &sec;
	SECPlanePrivRec planePriv;
	planePriv.pScrn = &scrn;
	plane_table[1].pPlanePriv = &planePriv;

	int plane_id = 1;
	Bool onoff = FALSE;
	Bool user = FALSE;

	Bool ret = secPlaneTrun ( plane_id, onoff, user );

	fail_if( ret != TRUE, "onoff is off, TRUE should be returned" );
}
END_TEST;

START_TEST( ut_secPlaneTrun_correctWorkFlow )
{
	plane_table_size = 2;

	plane_table = ctrl_calloc( 3, sizeof( SECPlaneTable ) );
	plane_table[1].plane_id = 1;

	SECRec sec;
	sec.isLcdOff = FALSE;
	ScrnInfoRec scrn;
	scrn.driverPrivate = &sec;
	SECPlanePrivRec planePriv;
	planePriv.pScrn = &scrn;
	plane_table[1].pPlanePriv = &planePriv;
	plane_table[1].onoff = FALSE;
	plane_table[1].visible = TRUE;

	int plane_id = 1;
	Bool onoff = TRUE;
	Bool user = FALSE;

	Bool ret = secPlaneTrun ( plane_id, onoff, user );

	fail_if( ret != TRUE, "correct work flow, TRUE had to be returned" );
	fail_if( plane_table[1].onoff != TRUE, "plane_table[1].onoff had to be changed" );
}
END_TEST;

START_TEST( ut_secPlaneTrun_correctWorkFlow2 )
{
	plane_table_size = 2;

	plane_table = ctrl_calloc( 3, sizeof( SECPlaneTable ) );
	plane_table[1].plane_id = 1;

	SECRec sec;
	sec.isLcdOff = FALSE;
	ScrnInfoRec scrn;
	scrn.driverPrivate = &sec;
	SECPlanePrivRec planePriv;
	planePriv.pScrn = &scrn;
	plane_table[1].pPlanePriv = &planePriv;
	plane_table[1].onoff = TRUE;
	plane_table[1].visible = TRUE;

	int plane_id = 1;
	Bool onoff = FALSE;
	Bool user = FALSE;

	Bool ret = secPlaneTrun ( plane_id, onoff, user );

	fail_if( ret != TRUE, "correct work flow 2, TRUE had to be returned" );
	fail_if( plane_table[1].onoff != FALSE, "plane_table[1].onoff had to be changed" );

	free( plane_table );
}
END_TEST;



/* secPlaneTrunStatus */
START_TEST( ut_secPlaneTrunStatus_idIsNotRelevant )
{
	int plane_id = 1;

	Bool ret = secPlaneTrunStatus ( plane_id );

	fail_if( ret != FALSE, "id is not relevant, False had to be returned" );
}
END_TEST;

START_TEST( ut_secPlaneTrunStatus_correctWorkFlow )
{
	plane_table_size = 2;

	plane_table = ctrl_calloc( 2, sizeof( SECPlaneTable ) );
	plane_table[1].plane_id = 1;
	plane_table[1].onoff = TRUE;

	int plane_id = 1;

	Bool ret = secPlaneTrunStatus ( plane_id );

	fail_if( ret != plane_table[1].onoff, "TRUE had to be returned" );

	free( plane_table );
}
END_TEST;

/* secPlaneFreezeUpdate */
START_TEST( ut_secPlaneFreezeUpdate_tableIsNull )
{
	int plane_id = 0;
	Bool enable = TRUE;

	/* Segmentation fault must not occurs */
	secPlaneFreezeUpdate ( plane_id, enable );
}
END_TEST;

START_TEST( ut_secPlaneFreezeUpdate_correctWorkFlow )
{
	plane_table_size = 2;

	plane_table = ctrl_calloc( 2, sizeof( SECPlaneTable ) );
	plane_table[1].plane_id = 1;
	plane_table[1].onoff = TRUE;
	plane_table[1].freeze_update = FALSE;

	int plane_id = 1;
	Bool enable = TRUE;

	/* Segmentation fault must not occurs */
	secPlaneFreezeUpdate ( plane_id, enable );

	fail_if( plane_table[1].freeze_update != enable, "freeze_update had to be changed" );

	free( plane_table );
}
END_TEST;


/* secPlaneRemoveBuffer */
START_TEST( ut_secPlaneRemoveBuffer_fbIdIsNotRelevant )
{
	int plane_id = 0;
	int fb_id = 0;

	Bool ret = secPlaneRemoveBuffer ( plane_id, fb_id );

	fail_if( ret != FALSE, "fb_id is not relevant, FALSE had to be returned" );
}
END_TEST;

START_TEST( ut_secPlaneRemoveBuffer_planeIdIsNotRelevant )
{
	int plane_id = 0;
	int fb_id = 1;

	Bool ret = secPlaneRemoveBuffer ( plane_id, fb_id );

	fail_if( ret != FALSE, "plane_id is not relevant, FALSE had to be returned" );
}
END_TEST;

START_TEST( ut_secPlaneRemoveBuffer_fbIsNull )
{
	plane_table_size = 2;

	plane_table = ctrl_calloc( 2, sizeof( SECPlaneTable ) );
	plane_table[1].plane_id = 1;
	plane_table[1].onoff = TRUE;
	plane_table[1].freeze_update = FALSE;

	xorg_list_init( &plane_table[1].fbs );

	int plane_id = 1;
	int fb_id = 1;

	Bool ret = secPlaneRemoveBuffer ( plane_id, fb_id );

	fail_if( ret != FALSE, "fb is NULL, FALSE had to be returned" );

	free( plane_table );
}
END_TEST;

START_TEST( ut_secPlaneRemoveBuffer_correctWorkFlow )
{
	plane_table_size = 2;

	plane_table = ctrl_calloc( 2, sizeof( SECPlaneTable ) );
	plane_table[1].plane_id = 1;
	plane_table[1].onoff = TRUE;
	plane_table[1].freeze_update = FALSE;

	SECPlaneFb *fb = ctrl_calloc( 1, sizeof( SECPlaneFb ) );
	fb->id = 2;

	xorg_list_init( &plane_table[1].fbs );
	xorg_list_add( &fb->link, &plane_table[1].fbs );

	int plane_id = 1;
	int fb_id = 2;

	Bool ret = secPlaneRemoveBuffer ( plane_id, fb_id );

	free( plane_table );
	fail_if( ret != TRUE, "Correct work flow, TRUE had to be returned" );
	fail_if( ctrl_get_cnt_calloc() != 0, "Memory had to be freed" );
}
END_TEST;

/* secPlaneAddBo */
START_TEST( ut_secPlaneAddBo_boIsNull )
{
	int plane_id = 0;
	tbm_bo bo = NULL;

	int ret = secPlaneAddBo ( plane_id, bo );

	fail_if( ret != 0, "bo is NULL, 0 had to be returned" );
}
END_TEST;

START_TEST( ut_secPlaneAddBo_tableIsNull )
{
	int plane_id = -1;
	tbm_bo bo = tbm_bo_alloc( 0, 1, 0 );

	int ret = secPlaneAddBo ( plane_id, bo );

	fail_if( ret != 0, "table is NULL, 0 had to be returned" );
}
END_TEST;

START_TEST( ut_secPlaneAddBo_fbIsNotNull )
{
	plane_table_size = 2;

	plane_table = ctrl_calloc( 2, sizeof( SECPlaneTable ) );
	plane_table[1].plane_id = 1;

	int plane_id = 1;
	tbm_bo bo = tbm_bo_alloc( 0, 1, 0 );

	SECPlaneFb *fb = ctrl_calloc( 1, sizeof( SECPlaneFb ) );
	fb->id = 1;
	fb->type = PLANE_FB_TYPE_BO;
	fb->buffer.bo = bo;

	xorg_list_init( &plane_table[1].fbs );
	xorg_list_add( &fb->link, &plane_table[1].fbs );

	int ret = secPlaneAddBo ( plane_id, bo );

	fail_if( ret != 0, "fb is not NULL, 0 had to be returned" );

	ctrl_free( plane_table );
	ctrl_free( fb );
}
END_TEST;

START_TEST( ut_secPlaneAddBo_bo_dataIsNull )
{
	plane_table_size = 2;

	plane_table = ctrl_calloc( 2, sizeof( SECPlaneTable ) );
	plane_table[1].plane_id = 1;

	int plane_id = 1;
	tbm_bo bo = tbm_bo_alloc( 0, 1, 0 );

	SECPlaneFb *fb = ctrl_calloc( 1, sizeof( SECPlaneFb ) );
	fb->id = 1;

	xorg_list_init( &plane_table[1].fbs );
	xorg_list_add( &fb->link, &plane_table[1].fbs );

	int ret = secPlaneAddBo ( plane_id, bo );

	fail_if( ret != 0, "bo_data is NULL, 0 had to be returned" );

	ctrl_free( plane_table );
	ctrl_free( fb );
}
END_TEST;

START_TEST( ut_secPlaneAddBo_bo_data_fb_idIsNotRelevant )
{
	plane_table_size = 2;

	plane_table = ctrl_calloc( 2, sizeof( SECPlaneTable ) );
	plane_table[1].plane_id = 1;

	int plane_id = 1;
	tbm_bo bo = tbm_bo_alloc( 0, 1, 0 );

	SECPlaneFb *fb = ctrl_calloc( 1, sizeof( SECPlaneFb ) );
	fb->id = 1;

	SECFbBoDataRec bo_data;
	bo_data.fb_id = 0;

	tbm_bo_add_user_data( bo, 1, &bo_data );

	xorg_list_init( &plane_table[1].fbs );
	xorg_list_add( &fb->link, &plane_table[1].fbs );

	int ret = secPlaneAddBo ( plane_id, bo );

	fail_if( ret != 0, "bo_data->fb_id is NULL, 0 had to be returned" );

	ctrl_free( plane_table );
	ctrl_free( fb );
}
END_TEST;

START_TEST( ut_secPlaneAddBo_widthIsNegative )
{
	plane_table_size = 2;

	plane_table = ctrl_calloc( 2, sizeof( SECPlaneTable ) );
	plane_table[1].plane_id = 1;

	int plane_id = 1;
	tbm_bo bo = tbm_bo_alloc( 0, 1, 0 );

	SECPlaneFb *fb = ctrl_calloc( 1, sizeof( SECPlaneFb ) );
	fb->id = 1;

	SECFbBoDataRec bo_data;
	bo_data.fb_id = 2;
	bo_data.pos.x1 = 5;
	bo_data.pos.x2 = 1;

	tbm_bo_add_user_data( bo, 1, &bo_data );

	xorg_list_init( &plane_table[1].fbs );
	xorg_list_add( &fb->link, &plane_table[1].fbs );

	int ret = secPlaneAddBo ( plane_id, bo );

	fail_if( ret != 0, "width is NULL, 0 had to be returned" );

	ctrl_free( plane_table );
	ctrl_free( fb );
}
END_TEST;

START_TEST( ut_secPlaneAddBo_heightIsNegative )
{
	plane_table_size = 2;

	plane_table = ctrl_calloc( 2, sizeof( SECPlaneTable ) );
	plane_table[1].plane_id = 1;

	int plane_id = 1;
	tbm_bo bo = tbm_bo_alloc( 0, 1, 0 );

	SECPlaneFb *fb = ctrl_calloc( 1, sizeof( SECPlaneFb ) );
	fb->id = 1;

	SECFbBoDataRec bo_data;
	bo_data.fb_id = 2;
	bo_data.pos.x1 = 1;
	bo_data.pos.x2 = 5;
	bo_data.pos.y1 = 5;
	bo_data.pos.y2 = 1;

	tbm_bo_add_user_data( bo, 1, &bo_data );

	xorg_list_init( &plane_table[1].fbs );
	xorg_list_add( &fb->link, &plane_table[1].fbs );

	int ret = secPlaneAddBo ( plane_id, bo );

	fail_if( ret != 0, "height is NULL, 0 had to be returned" );

	ctrl_free( plane_table );
	ctrl_free( fb );
}
END_TEST;

START_TEST( ut_secPlaneAddBo_correctWorkFlow )
{
	plane_table_size = 2;

	plane_table = ctrl_calloc( 2, sizeof( SECPlaneTable ) );
	plane_table[1].plane_id = 1;

	int plane_id = 1;
	tbm_bo bo = tbm_bo_alloc( 0, 1, 0 );

	SECPlaneFb *fb = ctrl_calloc( 1, sizeof( SECPlaneFb ) );
	fb->id = 1;

	SECFbBoDataRec bo_data;
	bo_data.fb_id = 2;
	bo_data.pos.x1 = 1;
	bo_data.pos.x2 = 5;
	bo_data.pos.y1 = 1;
	bo_data.pos.y2 = 5;

	tbm_bo_add_user_data( bo, 1, &bo_data );

	xorg_list_init( &plane_table[1].fbs );
	xorg_list_add( &fb->link, &plane_table[1].fbs );

	int ret = secPlaneAddBo ( plane_id, bo );

	SECPlaneFb *fbs = NULL, *fb_next = NULL;
	xorg_list_for_each_entry_safe (fbs, fb_next, &plane_table[1].fbs, link)
	{
		if( 2 == fbs->id )
		{
			ctrl_free(fbs);
		}
	}

	ctrl_free( plane_table );
	ctrl_free( fb );

	fail_if( ret != 2, "Correct work flow ,correct id had to be returned" );
	fail_if( ctrl_get_cnt_calloc() != 0, "Memory had to be allocated" );
	fail_if( tbm_get_ref(bo)!= 2, "bo had to be referenced" );
}
END_TEST;

/* secPlaneAddBuffer */
START_TEST( ut_secPlaneAddBuffer_vbufIsNull )
{
	int plane_id = 0;

	int ret = secPlaneAddBuffer ( plane_id, NULL );

	fail_if( ret != 0, "vbuf is NULL , 0 had to be returned" );
}
END_TEST;

START_TEST( ut_secPlaneAddBuffer_vbuf_buf_idIsNotRelevant )
{
	int plane_id = 0;
	SECVideoBuf vbuf;
	vbuf.fb_id = 0;

	int ret = secPlaneAddBuffer ( plane_id, &vbuf );

	fail_if( ret != 0, "vbuf->buf_id is not relevant, 0 had to be returned" );
}
END_TEST;

START_TEST( ut_secPlaneAddBuffer_vbuf_widthIsNegative )
{
	int plane_id = 0;
	SECVideoBuf vbuf;
	vbuf.fb_id = 1;
	vbuf.width = 0;

	int ret = secPlaneAddBuffer ( plane_id, &vbuf );

	fail_if( ret != 0, "vbuf->width is negative, 0 had to be returned" );
}
END_TEST;

START_TEST( ut_secPlaneAddBuffer_vbuf_heightIsNegative )
{
	int plane_id = 0;
	SECVideoBuf vbuf;
	vbuf.fb_id = 1;
	vbuf.width = 2;
	vbuf.height = 0;

	int ret = secPlaneAddBuffer ( plane_id, &vbuf );

	fail_if( ret != 0, "vbuf->height is negative, 0 had to be returned" );
}
END_TEST;

START_TEST( ut_secPlaneAddBuffer_tableIsNull )
{
	int plane_id = 0;
	SECVideoBuf vbuf;
	vbuf.fb_id = 1;
	vbuf.width = 2;
	vbuf.height = 3;

	int ret = secPlaneAddBuffer ( plane_id, &vbuf );

	fail_if( ret != 0, "table is NULL, 0 had to be returned" );
}
END_TEST;

START_TEST( ut_secPlaneAddBuffer_fbIsNotNull )
{
	int plane_id = 1;
	SECVideoBuf vbuf;
	vbuf.fb_id = 1;
	vbuf.width = 2;
	vbuf.height = 3;
	vbuf.stamp = 1;

	plane_table_size = 2;

	plane_table = ctrl_calloc( 2, sizeof( SECPlaneTable ) );
	plane_table[1].plane_id = 1;

	SECPlaneFb *fb = ctrl_calloc( 1, sizeof( SECPlaneFb ) );
	fb->id = 1;
	fb->type = PLANE_FB_TYPE_DEFAULT;
	fb->buffer.vbuf = &vbuf;

	xorg_list_init( &plane_table[1].fbs );
	xorg_list_add( &fb->link, &plane_table[1].fbs );

	int ret = secPlaneAddBuffer ( plane_id, &vbuf );

	fail_if( ret != 0, "fb is not NULL, 0 had to be returned" );

	ctrl_free( plane_table );
	ctrl_free( fb );
}
END_TEST;

START_TEST( ut_secPlaneAddBuffer_correctWorkFlow )
{
	int plane_id = 1;
	SECVideoBuf vbuf;
	vbuf.fb_id = 2;
	vbuf.width = 2;
	vbuf.height = 3;
	vbuf.stamp = 1;

	plane_table_size = 2;

	plane_table = ctrl_calloc( 2, sizeof( SECPlaneTable ) );
	plane_table[1].plane_id = 1;

	SECPlaneFb *fb = ctrl_calloc( 1, sizeof( SECPlaneFb ) );
	fb->id = 1;

	xorg_list_init( &plane_table[1].fbs );
	xorg_list_add( &fb->link, &plane_table[1].fbs );

	int ret = secPlaneAddBuffer ( plane_id, &vbuf );

	ctrl_free( plane_table );
	ctrl_free( fb );

	fail_if( ret != vbuf.fb_id, "Correct work flow, 1 had to be returned" );
	fail_if( ctrl_get_cnt_calloc() != 1, "Memory had to be allocated" );

	SECPlaneFb *fbs = NULL, *fb_next = NULL;
	xorg_list_for_each_entry_safe (fbs, fb_next, &plane_table[1].fbs, link)
	{
		if( 2 == fbs->id )
		{
			ctrl_free(fbs);
		}
	}
}
END_TEST;

/* secPlaneGetBuffer */
START_TEST( ut_secPlaneGetBuffer_tableIsNull )
{
	int plane_id = 0;
	tbm_bo bo = NULL;
	SECVideoBuf vbuf;

	int ret = secPlaneGetBuffer ( plane_id, bo, &vbuf );

	fail_if( ret != 0, "table is NULL, 0 had to be returned" );
}
END_TEST;

START_TEST( ut_secPlaneGetBuffer_fbIsNull )
{
	int plane_id = 1;
	tbm_bo bo = tbm_bo_alloc( 0, 1, 0 );
	SECVideoBuf vbuf;

	plane_table_size = 2;

	plane_table = ctrl_calloc( 2, sizeof( SECPlaneTable ) );
	plane_table[1].plane_id = 1;

	SECPlaneFb *fb = ctrl_calloc( 1, sizeof( SECPlaneFb ) );
	fb->id = 1;

	xorg_list_init( &plane_table[1].fbs );
	xorg_list_add( &fb->link, &plane_table[1].fbs );

	int ret = secPlaneGetBuffer ( plane_id, bo, &vbuf );

	ctrl_free( plane_table );
	ctrl_free( fb );

	fail_if( ret != 0, "fb is NULL, 0 had to be returned" );
}
END_TEST;

START_TEST( ut_secPlaneGetBuffer_correctWorkFlow )
{
	int plane_id = 1;
	tbm_bo bo = tbm_bo_alloc( 0, 1, 0 );
	SECVideoBuf vbuf;
	vbuf.fb_id = 1;

	plane_table_size = 2;

	plane_table = ctrl_calloc( 2, sizeof( SECPlaneTable ) );
	plane_table[1].plane_id = 1;

	SECPlaneFb *fb = ctrl_calloc( 1, sizeof( SECPlaneFb ) );
	fb->id = 1;
	fb->type = PLANE_FB_TYPE_BO;
	fb->buffer.bo = bo;

	xorg_list_init( &plane_table[1].fbs );
	xorg_list_add( &fb->link, &plane_table[1].fbs );

	int ret = secPlaneGetBuffer ( plane_id, bo, &vbuf );

	ctrl_free( plane_table );
	ctrl_free( fb );
	tbm_bo_unref( bo );

	fail_if( ret != vbuf.fb_id, "Correct work flow, fb_id had to be returned" );
}
END_TEST;


/*======================================================================================================*/

Suite* suite_plane( void )
{
	Suite* s = suite_create( "sec_plane.c" );


	TCase* tc__secPlaneFreeVbuf = tcase_create( "_secPlaneFreeVbuf" );
	tcase_add_test( tc__secPlaneFreeVbuf, ut__secPlaneFreeVbuf_notRelevantPlaneId );
	tcase_add_test( tc__secPlaneFreeVbuf, ut__secPlaneFreeVbuf_tableIsNULL );
	tcase_add_test( tc__secPlaneFreeVbuf, ut__secPlaneFreeVbuf_fbIsNULL );
	tcase_add_test( tc__secPlaneFreeVbuf, ut__secPlaneFreeVbuf_MemMangment );

	TCase* tc__secPlaneTableFindPos = tcase_create( "_secPlaneTableFindPos" );
	tcase_add_test( tc__secPlaneTableFindPos, ut__secPlaneTableFindPos_notRelevantCrtcId );
	tcase_add_test( tc__secPlaneTableFindPos, ut__secPlaneTableFindPos_correctWorkFlow );

	TCase* tc__secPlaneTableFind = tcase_create( "_secPlaneTableFind" );
	tcase_add_test( tc__secPlaneTableFind, ut__secPlaneTableFind_notRelevantId );
	tcase_add_test( tc__secPlaneTableFind, ut__secPlaneTableFind_correctWorkFlow );

	TCase* tc__secPlaneTableFindEmpty = tcase_create( "_secPlaneTableFindEmpty" );
	tcase_add_test( tc__secPlaneTableFindEmpty, ut__secPlaneTableFindEmpty_noFreeTables );
	tcase_add_test( tc__secPlaneTableFindEmpty, ut__secPlaneTableFindEmpty_correctWorkFlow );

	TCase* tc__secPlaneTableFindBuffer = tcase_create( "_secPlaneTableFindBuffer" );
	tcase_add_test( tc__secPlaneTableFindBuffer, ut__secPlaneTableFindBuffer_noBuffers );
	tcase_add_test( tc__secPlaneTableFindBuffer, ut__secPlaneTableFindBuffer_returnBufferById );
	tcase_add_test( tc__secPlaneTableFindBuffer, ut__secPlaneTableFindBuffer_returnBufferByBo );
	tcase_add_test( tc__secPlaneTableFindBuffer, ut__secPlaneTableFindBuffer_notValidVbuf );
	tcase_add_test( tc__secPlaneTableFindBuffer,
			ut__secPlaneTableFindBuffer_returnBufferWhenVbufIsValid );

	TCase* tc__secPlaneTableFreeBuffer = tcase_create( "_secPlaneTableFreeBuffer" );
	tcase_add_test( tc__secPlaneTableFreeBuffer, ut__secPlaneTableFreeBuffer_fbIsEqualToTableFb );
	tcase_add_test( tc__secPlaneTableFreeBuffer, ut__secPlaneTableFreeBuffer_boUnref );
	tcase_add_test( tc__secPlaneTableFreeBuffer, ut__secPlaneTableFreeBuffer_removeVbuf );

	TCase* tc__secPlaneTableEnsure = tcase_create( "_secPlaneTableEnsure" );
	tcase_add_test( tc__secPlaneTableEnsure, ut__secPlaneTableEnsure_countPlanesLessThenOne );
	tcase_add_test( tc__secPlaneTableEnsure, ut__secPlaneTableEnsure_planeTableIsInitialize );
	tcase_add_test( tc__secPlaneTableEnsure, ut__secPlaneTableEnsure_memoryManagement );

	TCase* tc__secPlaneExecAccessibility = tcase_create( "_secPlaneExecAccessibility" );
	tcase_add_test( tc__secPlaneExecAccessibility, ut__secPlaneExecAccessibility_srcImgIsNULL );

	TCase* tc__check_hw_restriction = tcase_create( "_check_hw_restriction" );
	tcase_add_test( tc__check_hw_restriction, ut__check_hw_restriction_crtcIsNULL );
	tcase_add_test( tc__check_hw_restriction, ut__check_hw_restriction_crtcIsNotEnabledOrNotActive );
	tcase_add_test( tc__check_hw_restriction, ut__check_hw_restriction_bufWIsLessThenMinWith );
	tcase_add_test( tc__check_hw_restriction, ut__check_hw_restriction_bufWIsNotDividendByTwo );
	tcase_add_test( tc__check_hw_restriction, ut__check_hw_restriction_bufHIsLessThenMinWith );
	tcase_add_test( tc__check_hw_restriction, ut__check_hw_restriction_bufHIsNotDividendByTwo );
	tcase_add_test( tc__check_hw_restriction,
			ut__check_hw_restriction_alignedDstWithMinusAlignedXLessThenMinWidth );
	tcase_add_test( tc__check_hw_restriction,
			ut__check_hw_restriction_alignedDstHeightMinusAlignedYLessThenMinHeight );
	tcase_add_test( tc__check_hw_restriction,
			ut__check_hw_restriction_alignedSrcWidthMinusAlignedSrcXLessThenMinWidth );
	tcase_add_test( tc__check_hw_restriction,
			ut__check_hw_restriction_alignedSrcHeightMinusAlignedSrcYLessThenMinHeight);
	tcase_add_test( tc__check_hw_restriction,
			ut__check_hw_restriction_corretWorkFlow );

	TCase* tc__secPlaneShowInternal = tcase_create( "_secPlaneShowInternal" );

	tcase_add_test( tc__secPlaneShowInternal, ut__secPlaneShowInternal_lcdIsOff );
	tcase_add_test( tc__secPlaneShowInternal, ut__secPlaneShowInternal_tableIsOff );

	tcase_add_test( tc__secPlaneShowInternal,
			ut__secPlaneShowInternal_newZPosAndOldZPosAreNotEqual );

	tcase_add_test( tc__secPlaneShowInternal, ut__secPlaneShowInternal_lastIfIsFalse );
	tcase_add_test( tc__secPlaneShowInternal, ut__secPlaneShowInternal_pCrtcPrivIsNULL );
	tcase_add_test( tc__secPlaneShowInternal,
			ut__secPlaneShowInternal__check_hw_restrictionReturnsFalse );
	tcase_add_test( tc__secPlaneShowInternal,
			ut__secPlaneShowInternal_drmModeSetPlaneReturnsTrue );
	tcase_add_test( tc__secPlaneShowInternal,
			ut__secPlaneShowInternal_corrcetcWorkFlowTrueByMainIf );
	tcase_add_test( tc__secPlaneShowInternal,
			ut__secPlaneShowInternal_heightsWidthsAreZeros );
	tcase_add_test( tc__secPlaneShowInternal, ut__secPlaneShowInternal_drmAddFBFails );
	tcase_add_test( tc__secPlaneShowInternal,
			ut__secPlaneShowInternal_access_fb_idIsNotRelevant );
	tcase_add_test( tc__secPlaneShowInternal, ut__secPlaneShowInternal_src_boIsNull );
	tcase_add_test( tc__secPlaneShowInternal,
			ut__secPlaneShowInternal__check_hw_restrictionReturnsFalse2 );
	tcase_add_test( tc__secPlaneShowInternal,
			ut__secPlaneShowInternal_drmModeSetPlaneReturnsTrue2 );
	tcase_add_test( tc__secPlaneShowInternal,
			ut__secPlaneShowInternal_corrcetcWorkFlowFalseByMainIf );


	TCase* tc__secPlaneHideInternal = tcase_create( "_secPlaneHideInternal" );
	tcase_add_test( tc__secPlaneHideInternal, ut__secPlaneHideInternal_tableIsNULL );
	tcase_add_test( tc__secPlaneHideInternal, ut___secPlaneHideInternal_tableIsNotVisible );
	tcase_add_test( tc__secPlaneHideInternal, ut___secPlaneHideInternal_crtc_idIsNotRelevant );
	tcase_add_test( tc__secPlaneHideInternal, ut___secPlaneHideInternal_drmSetPlaneFails );
	tcase_add_test( tc__secPlaneHideInternal, ut___secPlaneHideInternal_correctWorkFlow );

	TCase* tc_secPlaneInit = tcase_create( "secPlaneInit" );

	tcase_add_test( tc_secPlaneInit, ut_secPlaneInit_pScrnIsNull );
	tcase_add_test( tc_secPlaneInit, ut_secPlaneInit_pSecModeIsNull );
	tcase_add_test( tc_secPlaneInit, ut_secPlaneInit_plane_resAreNull );
	tcase_add_test( tc_secPlaneInit, ut_secPlaneInit_plane_resCountPlanesIsZero );
	tcase_add_test( tc_secPlaneInit, ut_secPlaneInit__mode_planeIsNull );

	tcase_add_test( tc_secPlaneInit, ut_secPlaneInit_correctWorkFlow );

	TCase* tc_secPlaneDeinit = tcase_create( "secPlaneDeinit" );
	tcase_add_test( tc_secPlaneDeinit, ut_secPlaneDeinit_pScrnIsNull );
	tcase_add_test( tc_secPlaneDeinit, ut_secPlaneDeinit_pPlanePrivIsNull );
	tcase_add_test( tc_secPlaneDeinit, ut_secPlaneDeinit_memoryMenegment );

	TCase* tc_secPlaneGetID = tcase_create( "secPlaneGetID" );
	tcase_add_test( tc_secPlaneGetID, ut_secPlaneGetID_idIsNotAvailable );
	tcase_add_test( tc_secPlaneGetID, ut_secPlaneGetID_correctWorkFlow );

	TCase* tc_secPlaneFreeId = tcase_create( "secPlaneFreeId" );
	tcase_add_test( tc_secPlaneFreeId, ut_secPlaneFreeId_tableIsNull );
	tcase_add_test( tc_secPlaneFreeId, ut_secPlaneFreeId_CorrectWorkFlow );

	TCase* tc_secPlaneTrun = tcase_create( "secPlaneTrun" );
	tcase_add_test( tc_secPlaneTrun, ut_secPlaneTrun_tableIsNull );
	tcase_add_test( tc_secPlaneTrun, ut_secPlaneTrun_LCDIsOff );
	tcase_add_test( tc_secPlaneTrun, ut_secPlaneTrun_tableonofAndOnoffAreEqual );
	tcase_add_test( tc_secPlaneTrun, ut_secPlaneTrun_correctWorkFlow );
	tcase_add_test( tc_secPlaneTrun, ut_secPlaneTrun_correctWorkFlow2 );

	TCase* tc_secPlaneTrunStatus = tcase_create( "secPlaneTrunStatus" );
	tcase_add_test( tc_secPlaneTrunStatus, ut_secPlaneTrunStatus_idIsNotRelevant );
	tcase_add_test( tc_secPlaneTrunStatus, ut_secPlaneTrunStatus_correctWorkFlow );

	TCase* tc_secPlaneFreezeUpdate = tcase_create( "secPlaneFreezeUpdate" );
	tcase_add_test( tc_secPlaneFreezeUpdate, ut_secPlaneFreezeUpdate_tableIsNull );
	tcase_add_test( tc_secPlaneFreezeUpdate, ut_secPlaneFreezeUpdate_correctWorkFlow );

	TCase* tc_secPlaneRemoveBuffer = tcase_create( "secPlaneRemoveBuffer" );
	tcase_add_test( tc_secPlaneRemoveBuffer, ut_secPlaneRemoveBuffer_fbIdIsNotRelevant );
	tcase_add_test( tc_secPlaneRemoveBuffer, ut_secPlaneRemoveBuffer_planeIdIsNotRelevant );
	tcase_add_test( tc_secPlaneRemoveBuffer, ut_secPlaneRemoveBuffer_fbIsNull );
	tcase_add_test( tc_secPlaneRemoveBuffer, ut_secPlaneRemoveBuffer_correctWorkFlow );

	TCase* tc_secPlaneAddBo = tcase_create( "secPlaneAddBo" );
	tcase_add_test( tc_secPlaneAddBo, ut_secPlaneAddBo_boIsNull );
	tcase_add_test( tc_secPlaneAddBo, ut_secPlaneAddBo_tableIsNull );
	tcase_add_test( tc_secPlaneAddBo, ut_secPlaneAddBo_fbIsNotNull );
	tcase_add_test( tc_secPlaneAddBo, ut_secPlaneAddBo_bo_dataIsNull );
	tcase_add_test( tc_secPlaneAddBo, ut_secPlaneAddBo_bo_data_fb_idIsNotRelevant );
	tcase_add_test( tc_secPlaneAddBo, ut_secPlaneAddBo_widthIsNegative );
	tcase_add_test( tc_secPlaneAddBo, ut_secPlaneAddBo_heightIsNegative );
	tcase_add_test( tc_secPlaneAddBo, ut_secPlaneAddBo_correctWorkFlow );

	TCase* tc_secPlaneAddBuffer = tcase_create( "secPlaneAddBuffer" );
	tcase_add_test( tc_secPlaneAddBuffer, ut_secPlaneAddBuffer_vbufIsNull );
	tcase_add_test( tc_secPlaneAddBuffer, ut_secPlaneAddBuffer_vbuf_buf_idIsNotRelevant );
	tcase_add_test( tc_secPlaneAddBuffer, ut_secPlaneAddBuffer_vbuf_widthIsNegative );
	tcase_add_test( tc_secPlaneAddBuffer, ut_secPlaneAddBuffer_vbuf_heightIsNegative );
	tcase_add_test( tc_secPlaneAddBuffer, ut_secPlaneAddBuffer_tableIsNull );
	tcase_add_test( tc_secPlaneAddBuffer, ut_secPlaneAddBuffer_fbIsNotNull );
	tcase_add_test( tc_secPlaneAddBuffer, ut_secPlaneAddBuffer_correctWorkFlow );

	TCase* tc_secPlaneGetBuffer = tcase_create( "secPlaneGetBuffer" );
	tcase_add_test( tc_secPlaneGetBuffer, ut_secPlaneGetBuffer_tableIsNull );
	tcase_add_test( tc_secPlaneGetBuffer, ut_secPlaneGetBuffer_fbIsNull );
	tcase_add_test( tc_secPlaneGetBuffer, ut_secPlaneGetBuffer_correctWorkFlow );


	suite_add_tcase( s, tc__secPlaneFreeVbuf );
	suite_add_tcase( s, tc__secPlaneTableFindPos );
	suite_add_tcase( s, tc__secPlaneTableFind );
	suite_add_tcase( s, tc__secPlaneTableFindEmpty );
	suite_add_tcase( s, tc__secPlaneTableFindBuffer );
	suite_add_tcase( s, tc__secPlaneTableFreeBuffer );
	suite_add_tcase( s, tc__secPlaneTableEnsure );
	suite_add_tcase( s, tc__secPlaneExecAccessibility );
	suite_add_tcase( s, tc__check_hw_restriction );
	suite_add_tcase( s, tc__secPlaneShowInternal );
	suite_add_tcase( s, tc__secPlaneHideInternal );
	suite_add_tcase( s, tc_secPlaneInit );
	suite_add_tcase( s, tc_secPlaneDeinit );
	suite_add_tcase( s, tc_secPlaneGetID );
	suite_add_tcase( s, tc_secPlaneFreeId );
	suite_add_tcase( s, tc_secPlaneTrun );
	suite_add_tcase( s, tc_secPlaneTrunStatus );
	suite_add_tcase( s, tc_secPlaneFreezeUpdate );
	suite_add_tcase( s, tc_secPlaneRemoveBuffer );
	suite_add_tcase( s, tc_secPlaneAddBo );
	suite_add_tcase( s, tc_secPlaneAddBuffer );
	suite_add_tcase( s, tc_secPlaneGetBuffer );


	return s;
}
