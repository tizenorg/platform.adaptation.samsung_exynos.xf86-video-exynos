/*
 * test_video_capture.c
 *
 *  Created on: Oct 21, 2013
 *      Author: svoloshynov
 */
#include "test_video_converter.h"
#include "exynos_driver.h"
//#include "exynos_util.c"

#define PRINTVAL( val ) printf("\n %d \n", val);

//********************************************** Fake functions ********************************************************
#define exynosVideoIppQueueBuf fake_exynosVideoIppQueueBuf
//#define exynosUtilStorageFindData fake_exynosUtilStorageFindData

struct drm_exynos_ipp_queue_buf check_exynosVideoIppQueueBufBuf;//Variable for check corret initialization
Bool fake_exynosVideoIppQueueBuf (int fd, struct drm_exynos_ipp_queue_buf *buf)
{
    XDBG_RETURN_VAL_IF_FAIL (fd >= 0, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (buf != NULL, FALSE);
    XDBG_RETURN_VAL_IF_FAIL (buf->handle[0] != 0, FALSE);

    check_exynosVideoIppQueueBufBuf.prop_id   = buf->prop_id;
    check_exynosVideoIppQueueBufBuf.ops_id    = buf->ops_id;
    check_exynosVideoIppQueueBufBuf.buf_type  = buf->buf_type;
    check_exynosVideoIppQueueBufBuf.buf_id    =  buf->buf_id;
    check_exynosVideoIppQueueBufBuf.user_data = buf->user_data;

    return TRUE;
}

#include "./xv/exynos_video_converter.c"


//************************************************* Fixtures ***********************************************************
//************************************************ Unit tests **********************************************************
//======================================================================================================================
/* static void _exynosVideoConverterSetupConfig (EXYNOSCvtType type, EXYNOSIppProp *prop,
                                  struct drm_exynos_ipp_config *config) */
START_TEST ( exynosVideoConverterSetupConfig_fillConfigValuesByPropValues )
{
	EXYNOSCvtType type;
	EXYNOSIppProp prop = { 0, };
    struct drm_exynos_ipp_config config;

    prop.id = FOURCC_RGB565;
    prop.width = 1;
    prop.height = 2;
    prop.crop.x = 3;
    prop.crop.y = 4;
    prop.crop.width = 5;
    prop.crop.height = 6;

	_exynosVideoConverterSetupConfig( CVT_TYPE_SRC, &prop, &config );

	fail_if( config.ops_id != EXYNOS_DRM_OPS_SRC, "must be equal 1" );
	fail_if( config.fmt != DRM_FORMAT_RGB565, "must be equal 2" );
	fail_if( config.sz.hsize != 1, "must be equal 3" );
	fail_if( config.sz.vsize != 2, "must be equal 4" );
	fail_if( config.pos.x != 3, "must be equal 5" );
	fail_if( config.pos.y != 4, "must be equal 6" );
	fail_if( config.pos.w != 5, "must be equal 7" );
	fail_if( config.pos.h != 6, "must be equal 8" );
}
END_TEST;

START_TEST ( exynosVideoConverterSetupConfig_fillConfigValuesByPropValues_2 )
{
	EXYNOSCvtType type;
	EXYNOSIppProp prop = { 0, };
    struct drm_exynos_ipp_config config;

    prop.degree = 450;

	_exynosVideoConverterSetupConfig( CVT_TYPE_DST, &prop, &config );

	fail_if( config.ops_id != EXYNOS_DRM_OPS_DST, "must be equal 1" );
	fail_if( config.degree != EXYNOS_DRM_DEGREE_90, "must be equal 2" );
}
END_TEST;
//======================================================================================================================
/* static Bool _exynosVideoConverterCompareIppDataStamp (pointer structure, pointer element) */
START_TEST ( exynosVideoConverterCompareIppDataStamp_different )
{
	CARD32 element = 1;
	EXYNOSIpp pIpp_data = { 0, };;

	pIpp_data.stamp = 2;

	fail_if( _exynosVideoConverterCompareIppDataStamp( &pIpp_data, &element ) != FALSE, "variables are different, FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterCompareIppDataStamp_same )
{
	CARD32 element = 1;
	EXYNOSIpp pIpp_data = { 0, };;

	pIpp_data.stamp = 1;

	fail_if( _exynosVideoConverterCompareIppDataStamp( &pIpp_data, &element ) != TRUE, "variables are same, TRUE must returned" );
}
END_TEST;
//======================================================================================================================

//======================================================================================================================
/* static Bool _exynosVideoConverterCompareIppBufIndex (pointer structure, pointer element) */
START_TEST ( exynosVideoConverterCompareIppBufIndex_different )
{
    unsigned int index = 1;
    EXYNOSIppBuf buf = { 0, };

    buf.index = 2;

	fail_if( _exynosVideoConverterCompareIppBufIndex( &buf, &index ) != FALSE, "variables are different, FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterCompareIppBufIndex_same )
{
    unsigned int index = 1;
    EXYNOSIppBuf buf = { 0, };;

    buf.index = 1;

	fail_if( _exynosVideoConverterCompareIppBufIndex( &buf, &index ) != TRUE, "variables are same, TRUE must returned" );
}
END_TEST;
//======================================================================================================================

/* static Bool _exynosVideoConverterIppQueueBuf (IMAGEPortInfoPtr pPort, EXYNOSIppPtr pIpp_data,
 										 EXYNOSIppBuf *pIpp_buf) */
START_TEST ( exynosVideoConverterIppQueueBuf_pIpp_DataNull )
{
	//pIpp_data NULL segmentation fault must not occur
	fail_if( _exynosVideoConverterIppQueueBuf( NULL, NULL, NULL ) != FALSE, "pIpp_buf NULL, FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterIppQueueBuf_pIpp_BufNull )
{
	EXYNOSIppPtr pIpp_data = ctrl_calloc( 1, sizeof( EXYNOSIpp ) );
	//pIpp_buf NULL segmentation fault must not occur
	fail_if( _exynosVideoConverterIppQueueBuf( NULL, pIpp_data, NULL ) != FALSE, "pIpp_buf NULL, FALSE must returned" );
	ctrl_free( pIpp_data );
}
END_TEST;

START_TEST ( exynosVideoConverterIppQueueBuf_pIpp_DataProp_IdIsZero )
{
	EXYNOSIppPtr pIpp_data = ctrl_calloc( 1, sizeof( EXYNOSIpp ) );
	EXYNOSIppBuf *pIpp_buf = ctrl_calloc( 1, sizeof( EXYNOSIppBuf ) );
	pIpp_data->prop_id = -1;

	fail_if( _exynosVideoConverterIppQueueBuf( NULL, pIpp_data, pIpp_buf ) != FALSE, "pIpp_buf->prop_id < 0, FALSE must returned" );
	ctrl_free( pIpp_data );
	ctrl_free( pIpp_buf );
}
END_TEST;

START_TEST ( exynosVideoConverterIppQueueBuf_bufHandlesAreNull )
{
	EXYNOSIppPtr pIpp_data = ctrl_calloc( 1, sizeof( EXYNOSIpp ) );
	EXYNOSIppBuf *pIpp_buf = ctrl_calloc( 1, sizeof( EXYNOSIppBuf ) );
	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo ) );
	pPort->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	ScrnInfoPtr P = pPort->pScrn;
	P->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );

	pIpp_data->prop_id = 1;

	fail_if( _exynosVideoConverterIppQueueBuf( pPort, pIpp_data, pIpp_buf ) != FALSE, "buf->handle[] == NULL, FALSE must returned" );

	ctrl_free( pIpp_data );
	ctrl_free( pIpp_buf );
	ctrl_free( P->driverPrivate );
	ctrl_free( pPort->pScrn );
	ctrl_free( pPort );
}
END_TEST;

START_TEST ( exynosVideoConverterIppQueueBuf_correctInit )
{
	EXYNOSIppPtr pIpp_data = ctrl_calloc( 1, sizeof( EXYNOSIpp ) );

	EXYNOSIppBuf *pIpp_buf = ctrl_calloc( 1, sizeof( EXYNOSIppBuf ) );
	pIpp_buf->type = CVT_TYPE_SRC;
	int i = 0;
	for( i = 0; i < PLANE_CNT; i++ )
		pIpp_buf->handles[i].u32 = 1;


	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo ) );
	pPort->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	ScrnInfoPtr P = pPort->pScrn;
	P->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );

	pIpp_data->prop_id = 1;
	pIpp_buf->index = 2;

/*
    buf.prop_id = pIpp_data->prop_id;
    buf.ops_id = (pIpp_buf->type == CVT_TYPE_SRC) ? EXYNOS_DRM_OPS_SRC : EXYNOS_DRM_OPS_DST;
    buf.buf_type = IPP_BUF_ENQUEUE;
    buf.buf_id = pIpp_buf->index;
    buf.user_data = (__u64) pPort;
*/

	fail_if( _exynosVideoConverterIppQueueBuf( pPort, pIpp_data, pIpp_buf ) != TRUE, "corret initialization, TRUE must returned" );

	fail_if( check_exynosVideoIppQueueBufBuf.prop_id != 1, "must be equal 1" );
	fail_if( check_exynosVideoIppQueueBufBuf.ops_id != EXYNOS_DRM_OPS_SRC, "must be equal 2" );
	fail_if( check_exynosVideoIppQueueBufBuf.buf_type != IPP_BUF_ENQUEUE, "must be equal 3" );
	fail_if( check_exynosVideoIppQueueBufBuf.buf_id != 2, "must be equal 4" );
	fail_if( !check_exynosVideoIppQueueBufBuf.user_data, "must be equal 5" );

	ctrl_free( pIpp_data );
	ctrl_free( pIpp_buf );
	ctrl_free( P->driverPrivate );
	ctrl_free( pPort->pScrn );
	ctrl_free( pPort );
}
END_TEST;

START_TEST ( exynosVideoConverterIppQueueBuf_correctInit_2 )
{
	EXYNOSIppPtr pIpp_data = ctrl_calloc( 1, sizeof( EXYNOSIpp ) );

	EXYNOSIppBuf *pIpp_buf = ctrl_calloc( 1, sizeof( EXYNOSIppBuf ) );
	pIpp_buf->type = CVT_TYPE_DST;
	int i = 0;
	for( i = 0; i < PLANE_CNT; i++ )
		pIpp_buf->handles[i].u32 = 1;


	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo ) );
	pPort->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	ScrnInfoPtr P = pPort->pScrn;
	P->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );

	fail_if( _exynosVideoConverterIppQueueBuf( pPort, pIpp_data, pIpp_buf ) != TRUE, "corret initialization, TRUE must returned" );
	fail_if( check_exynosVideoIppQueueBufBuf.ops_id != EXYNOS_DRM_OPS_DST, "must be equal 1" );

	ctrl_free( pIpp_data );
	ctrl_free( pIpp_buf );
	ctrl_free( P->driverPrivate );
	ctrl_free( pPort->pScrn );
	ctrl_free( pPort );
}
END_TEST;
//======================================================================================================================

//======================================================================================================================
/* Bool exynosVideoConverterIppDequeueBuf (IMAGEPortInfoPtr pPort, EXYNOSIppPtr pIpp_data, EXYNOSIppBuf *pIpp_buf) */
START_TEST ( exynosVideoConverterIppDequeueBuf_pIpp_DataNull )
{
	//pIpp_data NULL segmentation fault must not occur
	fail_if( exynosVideoConverterIppDequeueBuf( NULL, NULL, NULL ) != FALSE, "pIpp_buf NULL, FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterIppDequeueBuf_pIpp_BufNull )
{
	EXYNOSIppPtr pIpp_data = ctrl_calloc( 1, sizeof( EXYNOSIpp ) );
	//pIpp_buf NULL segmentation fault must not occur
	fail_if( exynosVideoConverterIppDequeueBuf( NULL, pIpp_data, NULL ) != FALSE, "pIpp_buf NULL, FALSE must returned" );
	ctrl_free( pIpp_data );
}
END_TEST;

START_TEST ( exynosVideoConverterIppDequeueBuf_pIpp_DataProp_IdIsZero )
{
	EXYNOSIppPtr pIpp_data = ctrl_calloc( 1, sizeof( EXYNOSIpp ) );
	EXYNOSIppBuf *pIpp_buf = ctrl_calloc( 1, sizeof( EXYNOSIppBuf ) );
	pIpp_data->prop_id = -1;

	fail_if( exynosVideoConverterIppDequeueBuf( NULL, pIpp_data, pIpp_buf ) != FALSE, "pIpp_buf->prop_id < 0, FALSE must returned" );
	ctrl_free( pIpp_data );
	ctrl_free( pIpp_buf );
}
END_TEST;

START_TEST ( exynosVideoConverterIppDequeueBuf_ifExynosUtilStorageFindDataReturnNull )
{
	EXYNOSIppPtr pIpp_data = ctrl_calloc( 1, sizeof( EXYNOSIpp ) );
	EXYNOSIppBuf *pIpp_buf = ctrl_calloc( 1, sizeof( EXYNOSIppBuf ) );
	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo ) );
	pPort->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	ScrnInfoPtr P = pPort->pScrn;
	P->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );

	pIpp_data->prop_id = 1;

	fail_if( exynosVideoConverterIppDequeueBuf( pPort, pIpp_data, pIpp_buf ) != TRUE, " TRUE must returned" );

	ctrl_free( pIpp_data );
	ctrl_free( pIpp_buf );
	ctrl_free( P->driverPrivate );
	ctrl_free( pPort->pScrn );
	ctrl_free( pPort );
}
END_TEST;

START_TEST ( exynosVideoConverterIppDequeueBuf_bufHandlesAreNull )
{
	EXYNOSIppPtr pIpp_data = ctrl_calloc( 1, sizeof( EXYNOSIpp ) );

	pIpp_data->list_of_inbuf = exynosUtilStorageInit();
	int ddata1 = ctrl_calloc( 1, sizeof( int ) );
	if( !exynosUtilStorageAdd ( pIpp_data->list_of_inbuf , ddata1 ) )
		return;

	EXYNOSIppBuf *pIpp_buf = ctrl_calloc( 1, sizeof( EXYNOSIppBuf ) );
	pIpp_buf->type = CVT_TYPE_SRC;

	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo ) );
	pPort->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	ScrnInfoPtr P = pPort->pScrn;
	P->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );

	pIpp_data->prop_id = 1;

	fail_if( exynosVideoConverterIppDequeueBuf( pPort, pIpp_data, pIpp_buf ) != FALSE, "buf->handle[] == NULL, FALSE must returned" );

	ctrl_free( pIpp_data );
	ctrl_free( pIpp_buf );
	ctrl_free( P->driverPrivate );
	ctrl_free( pPort->pScrn );
	ctrl_free( pPort );
}
END_TEST;

START_TEST ( exynosVideoConverterIppDequeueBuf_correctInit )
{
	EXYNOSIppPtr pIpp_data = ctrl_calloc( 1, sizeof( EXYNOSIpp ) );

	pIpp_data->list_of_inbuf = exynosUtilStorageInit();
	int ddata1 = ctrl_calloc( 1, sizeof( int ) );
	if( !exynosUtilStorageAdd ( pIpp_data->list_of_inbuf , ddata1 ) )
		return;

	EXYNOSIppBuf *pIpp_buf = ctrl_calloc( 1, sizeof( EXYNOSIppBuf ) );
	pIpp_buf->type = CVT_TYPE_SRC;
	int i = 0;
	for( i = 0; i < PLANE_CNT; i++ )
		pIpp_buf->handles[i].u32 = 1;


	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo ) );
	pPort->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	ScrnInfoPtr P = pPort->pScrn;
	P->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );

	pIpp_data->prop_id = 1;
	//pIpp_buf->index = 2;
/*
    buf.prop_id = pIpp_data->prop_id;
    buf.ops_id = (pIpp_buf->type == CVT_TYPE_SRC) ? EXYNOS_DRM_OPS_SRC : EXYNOS_DRM_OPS_DST;
    buf.buf_type = IPP_BUF_ENQUEUE;
    buf.buf_id = pIpp_buf->index;
    buf.user_data = (__u64) pPort;
*/
	fail_if( exynosVideoConverterIppDequeueBuf( pPort, pIpp_data, pIpp_buf ) != TRUE, "corret initialization, TRUE must returned" );
	fail_if( check_exynosVideoIppQueueBufBuf.prop_id != 1, "must be equal 1" );
	fail_if( check_exynosVideoIppQueueBufBuf.ops_id != EXYNOS_DRM_OPS_SRC, "must be equal 2" );
	fail_if( check_exynosVideoIppQueueBufBuf.buf_type != IPP_BUF_DEQUEUE, "must be equal 3" );
	fail_if( check_exynosVideoIppQueueBufBuf.buf_id != 0, "must be equal 4" );
	fail_if( !check_exynosVideoIppQueueBufBuf.user_data, "must be equal 5" );

	ctrl_free( pIpp_data );
	ctrl_free( pIpp_buf );
	ctrl_free( P->driverPrivate );
	ctrl_free( pPort->pScrn );
	ctrl_free( pPort );
}
END_TEST;
//======================================================================================================================

//======================================================================================================================
/* static void _exynosVideoConverterDequeueAllBuf (IMAGEPortInfoPtr pPort, EXYNOSIppPtr pIpp_data) */
START_TEST ( exynosVideoConverterDequeueAllBuf_pIpp_DataNull )
{
	//pIpp_data NULL Segmentation fault must not occur
	_exynosVideoConverterDequeueAllBuf( NULL, NULL );
}
END_TEST;

START_TEST ( exynosVideoConverterDequeueAllBuf_pPotrNull )
{
	EXYNOSIppPtr pIpp_data = ctrl_calloc( 1, sizeof( EXYNOSIpp ) );
	//pIpp_data NULL Segmentation fault must not occur
	_exynosVideoConverterDequeueAllBuf( NULL, pIpp_data );
}
END_TEST;

START_TEST ( exynosVideoConverterDequeueAllBuf_ )
{
	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo ) );
	pPort->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	ScrnInfoPtr P = pPort->pScrn;
	P->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );

	pPort->last_ipp_data = ctrl_calloc( 1, sizeof( EXYNOSIpp ) );
	pPort->last_ipp_data->list_of_inbuf  = exynosUtilStorageInit();
	pPort->last_ipp_data->list_of_outbuf = exynosUtilStorageInit();

	int ddata1 = ctrl_calloc( 1, sizeof( int ) );
	int ddata2 = ctrl_calloc( 1, sizeof( int ) );

	if( !exynosUtilStorageAdd ( pPort->last_ipp_data->list_of_inbuf , ddata1 ) )
		return;
	if( !exynosUtilStorageAdd ( pPort->last_ipp_data->list_of_outbuf , ddata2 ) )
		return;

	EXYNOSIppPtr pIpp_data = ctrl_calloc( 1, sizeof( EXYNOSIpp ) );
	int *ddata3 = ctrl_calloc( 1, sizeof( int ) );
	pIpp_data->list_of_outbuf = exynosUtilStorageInit();
	if( !exynosUtilStorageAdd ( pIpp_data->list_of_outbuf , ddata3 ) )
		return;

	int mem_ck = ctrl_get_cnt_calloc();
	//pIpp_data NULL Segmentation fault must not occur
	_exynosVideoConverterDequeueAllBuf( pPort, pIpp_data );

	fail_if( ctrl_get_cnt_calloc() != mem_ck - 6, "Memory must free" );
	fail_if( pPort->last_ipp_data->list_of_inbuf != NULL, "Must be NULL" );
	fail_if( pPort->last_ipp_data->list_of_outbuf != NULL, "Must be NULL" );
}
END_TEST;
//======================================================================================================================

//======================================================================================================================
/* static EXYNOSIpp * _exynosVideoConverterSetupIppData (IMAGEPortInfoPtr pPort, PixmapPtr src, PixmapPtr dst) */
START_TEST ( exynosVideoConverterSetupIppData_srcNull )
{
	fail_if( _exynosVideoConverterSetupIppData( NULL, NULL, NULL ) ,"src is NULL, NULL must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterSetupIppData_pPortNull )
{
	PixmapPtr src = ctrl_calloc( 1, sizeof( PixmapRec ) );
	fail_if( _exynosVideoConverterSetupIppData( NULL, src, NULL ) ,"pPort is NULL, NULL must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterSetupIppData_dstNull )
{
	PixmapPtr src = ctrl_calloc( 1, sizeof( PixmapRec ) );
	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo ) );
	fail_if( _exynosVideoConverterSetupIppData( pPort, src, NULL ) ,"dst is NULL, NULL must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterSetupIppData_correctInit )
{
	PixmapPtr src = ctrl_calloc( 1, sizeof( PixmapRec ) );
	src->refcnt = 106;
	src->drawable.width = 1;
	src->drawable.height = 2;

	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo ) );
	pPort->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pPort->in_crop.x = 3;
	pPort->in_crop.y = 4;
	pPort->out_crop.x = 7;
	pPort->out_crop.y = 8;
	pPort->hw_rotate = 9;
	pPort->vflip = 10;
	pPort->hflip = 11;

	PixmapPtr dst = ctrl_calloc( 1, sizeof( PixmapRec ) );
	dst->refcnt = 106;
	dst->drawable.width = 5;
	dst->drawable.height = 6;


	EXYNOSIppPtr res_pIpp_data = _exynosVideoConverterSetupIppData( pPort, src, dst );

	fail_if( res_pIpp_data == NULL ,"correct initialization, NULL must not returned" );

    /* SRC */
/*
    pIpp_data->props[CVT_TYPE_SRC].id = exynosVideoBufferGetFourcc (src);
    pIpp_data->props[CVT_TYPE_SRC].width = src->drawable.width;
    pIpp_data->props[CVT_TYPE_SRC].height = src->drawable.height;
    pIpp_data->props[CVT_TYPE_SRC].crop = pPort->in_crop;
*/
	fail_if( res_pIpp_data->props[CVT_TYPE_SRC].id != 0, "Must be equal 1" );
	fail_if( res_pIpp_data->props[CVT_TYPE_SRC].width  != 1, "Must be equal 2" );
	fail_if( res_pIpp_data->props[CVT_TYPE_SRC].height != 2, "Must be equal 3" );
	fail_if( res_pIpp_data->props[CVT_TYPE_SRC].crop.x != 3, "Must be equal 4" );
	fail_if( res_pIpp_data->props[CVT_TYPE_SRC].crop.y != 4, "Must be equal 5" );

    /* DST */
/*
    pIpp_data->props[CVT_TYPE_DST].id = exynosVideoBufferGetFourcc (dst);
    pIpp_data->props[CVT_TYPE_DST].width = dst->drawable.width;
    pIpp_data->props[CVT_TYPE_DST].height = dst->drawable.height;
    pIpp_data->props[CVT_TYPE_DST].crop = pPort->out_crop;
    pIpp_data->props[CVT_TYPE_DST].degree = pPort->hw_rotate;
    pIpp_data->props[CVT_TYPE_DST].hflip = pPort->hflip;
    pIpp_data->props[CVT_TYPE_DST].vflip = pPort->vflip;
*/
	fail_if( res_pIpp_data->props[CVT_TYPE_DST].id != 0, "Must be equal 6" );
	fail_if( res_pIpp_data->props[CVT_TYPE_DST].width!= 5, "Must be equal 7" );
	fail_if( res_pIpp_data->props[CVT_TYPE_DST].height != 6, "Must be equal 8" );
	fail_if( res_pIpp_data->props[CVT_TYPE_DST].crop.x != 7, "Must be equal 9" );
	fail_if( res_pIpp_data->props[CVT_TYPE_DST].crop.y != 8, "Must be equal 10" );
	fail_if( res_pIpp_data->props[CVT_TYPE_DST].degree != 9, "Must be equal 11" );
	fail_if( res_pIpp_data->props[CVT_TYPE_DST].vflip != 10, "Must be equal 12" );
	fail_if( res_pIpp_data->props[CVT_TYPE_DST].hflip != 11, "Must be equal 13" );
}
END_TEST;
//======================================================================================================================

//======================================================================================================================
/* static Bool _exynosVideoConverterHasKeys (int id) */
START_TEST ( exynosVideoConverterHasKeys_unsupportedKey )
{
	fail_if( _exynosVideoConverterHasKeys( FOURCC_RGB565 ) != FALSE, "unsupported Key, FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterHasKeys_unsupportedKey_2 )
{
	//If argument is random value
	fail_if( _exynosVideoConverterHasKeys( 100 ) != FALSE, "unsupported Key, FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterHasKeys_correctInit )
{
	fail_if( _exynosVideoConverterHasKeys( FOURCC_SR16 ) != TRUE, "correct Key, TRUE must returned" );
}
END_TEST;
//======================================================================================================================

//======================================================================================================================
/* static Bool _exynosVideoConverterSetProperty (IMAGEPortInfoPtr pPort, PixmapPtr src, PixmapPtr dst) */
START_TEST ( exynosVideoConverterSetProperty_pPortNull )
{
	fail_if( _exynosVideoConverterSetProperty( NULL, NULL, NULL ) != FALSE, "pPort is Null, FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterSetProperty_srcNull )
{
	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo) );
	fail_if( _exynosVideoConverterSetProperty( pPort, NULL, NULL ) != FALSE, "src is Null, FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterSetProperty_dstNull )
{
	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo) );
	PixmapPtr src = ctrl_calloc( 1, sizeof( PixmapRec ) );
	fail_if( _exynosVideoConverterSetProperty( pPort, src, NULL ) != FALSE, "dst is Null, FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterSetProperty_pPortLast_Ipp_DataIsNotNull )
{
	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo) );
	pPort->pScrn  = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pPort->pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	pPort->last_ipp_data = ctrl_calloc( 1, sizeof( EXYNOSIpp ) );

	PixmapPtr src = ctrl_calloc( 1, sizeof( PixmapRec ) );
	PixmapPtr dst = ctrl_calloc( 1, sizeof( PixmapRec ) );

	fail_if( _exynosVideoConverterSetProperty( pPort, src, dst ) != TRUE, "TRUE, FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterSetProperty_pIpp_dataProp_IdNegative )
{
	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo) );
	pPort->pScrn  = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pPort->pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	EXYNOSPtr P = pPort->pScrn->driverPrivate;
	P->drm_fd = -1;

	PixmapPtr src = ctrl_calloc( 1, sizeof( PixmapRec ) );
	src->refcnt = 106;
	PixmapPtr dst = ctrl_calloc( 1, sizeof( PixmapRec ) );
	dst->refcnt = 106;

	fail_if( _exynosVideoConverterSetProperty( pPort, src, dst ) != FALSE, "pIpp_data->prop_id < 0, FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterSetProperty_correctInit )
{
	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo) );
	pPort->pScrn  = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pPort->pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	EXYNOSPtr P = pPort->pScrn->driverPrivate;
	P->drm_fd = 1;

	PixmapPtr src = ctrl_calloc( 1, sizeof( PixmapRec ) );
	src->refcnt = 106;
	PixmapPtr dst = ctrl_calloc( 1, sizeof( PixmapRec ) );
	dst->refcnt = 106;

	fail_if( _exynosVideoConverterSetProperty( pPort, src, dst ) != TRUE, "correct initialization, TRUE must returned" );
}
END_TEST;
//======================================================================================================================

//======================================================================================================================
/* static Bool _exynosVideoConverterSetBuf (IMAGEPortInfoPtr pPort, PixmapPtr src, PixmapPtr dst) */
START_TEST ( exynosVideoConverterSetBuf_srcNull )
{
	fail_if( _exynosVideoConverterSetBuf( NULL, NULL, NULL ), "src is Null, False must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterSetBuf_dstNull )
{
	PixmapPtr src = ctrl_calloc( 1, sizeof( PixmapRec ) );
	fail_if( _exynosVideoConverterSetBuf( NULL, src, NULL ), "dst is Null, False must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterSetBuf_pPortLast_Ppp_DataIsNull )
{
	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo) );
	pPort->last_ipp_data = NULL;

	PixmapPtr src = ctrl_calloc( 1, sizeof( PixmapRec ) );
	PixmapPtr dst = ctrl_calloc( 1, sizeof( PixmapRec ) );

	fail_if( _exynosVideoConverterSetBuf( pPort, src, dst ), "pPort->last_ipp_data is Null, False must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterSetBuf_bufHandlesAreNull )
{
	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo) );
	pPort->pScrn  = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pPort->pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	pPort->last_ipp_data = ctrl_calloc( 1, sizeof( EXYNOSIpp ) );

	PixmapPtr src = ctrl_calloc( 1, sizeof( PixmapRec ) );
	src->refcnt = 106;
	PixmapPtr dst = ctrl_calloc( 1, sizeof( PixmapRec ) );
	dst->refcnt = 106;

	fail_if( _exynosVideoConverterSetBuf( pPort, src, dst ), "pPort->last_ipp_data is Null, False must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterSetBuf_correctInit )
{
	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo) );
	pPort->pScrn  = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pPort->pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	pPort->last_ipp_data = ctrl_calloc( 1, sizeof( EXYNOSIpp ) );

	extern int imp_val;

	PixmapPtr src = ctrl_calloc( 1, sizeof( PixmapRec ) );
	src->refcnt = 106;
	PixmapPtr dst = ctrl_calloc( 1, sizeof( PixmapRec ) );
	dst->refcnt = 106;

	imp_val = 107;

	fail_if( !_exynosVideoConverterSetBuf( pPort, src, dst ), "correct initialization, TRUE must returned" );
}
END_TEST;
//======================================================================================================================

//======================================================================================================================
/* void exynosVideoConverterClose(IMAGEPortInfoPtr pPort) */
START_TEST ( exynosVideoConverterClose_pPortNull )
{
	//Segmentation fault must not occur
	exynosVideoConverterClose( NULL );
}
END_TEST;

START_TEST ( exynosVideoConverterClose_pPortLast_Ipp_DataIsNull )
{
	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo ) );

	exynosVideoConverterClose( pPort );
}
END_TEST;

START_TEST ( exynosVideoConverterClose_ )
{
	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo ) );
	pPort->last_ipp_data = ctrl_calloc( 1, sizeof( EXYNOSIpp ) );
	pPort->pScrn  = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	pPort->pScrn->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	pPort->last_ipp_data = ctrl_calloc( 1, sizeof( EXYNOSIpp ) );

	int mem_ck = ctrl_get_cnt_calloc();

	exynosVideoConverterClose( pPort );

	fail_if( ctrl_get_cnt_calloc() != mem_ck - 1, "Memory must free" );
	fail_if( pPort->last_ipp_data != NULL, "must be NULL" );
}
END_TEST;
//======================================================================================================================

//======================================================================================================================
/* Bool exynosVideoConverterAdaptSize (EXYNOSIppProp *src, EXYNOSIppProp *dst) */
START_TEST ( exynosVideoConverterAdaptSize_unsupportedWithAndHeight )
{
	EXYNOSIppProp *src = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );
	EXYNOSIppProp *dst = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );

	src->width = 12;
	fail_if( exynosVideoConverterAdaptSize( src, dst ), "FALSE must returned" );

	src->width = 18;
	src->height = 7;
	fail_if( exynosVideoConverterAdaptSize( src, dst ), "FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterAdaptSize_unsupportedCropWidthAndCropHeight )
{
	EXYNOSIppProp *src = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );
	EXYNOSIppProp *dst = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );

	src->width = 18;
	src->height = 9;

	src->crop.width = 14;
	fail_if( exynosVideoConverterAdaptSize( src, dst ), "FALSE must returned" );

	src->crop.width = 18;
	src->crop.height = 7;
	fail_if( exynosVideoConverterAdaptSize( src, dst ), "FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterAdaptSize_cropXAndCropYAreLesThenSrcWidthAndSrcHeight )
{
	EXYNOSIppProp *src = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );
	EXYNOSIppProp *dst = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );

	src->width = 18;
	src->height = 9;
	src->crop.width = 18;
	src->crop.height = 9;

	src->crop.x = 19;
	fail_if( exynosVideoConverterAdaptSize( src, dst ), "FALSE must returned" );

	src->crop.x = 2;
	src->crop.y = 19;
	fail_if( exynosVideoConverterAdaptSize( src, dst ), "FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterAdaptSize_ifSrcCropXBiggerThenSrcWidth )
{
	EXYNOSIppProp *src = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );
	EXYNOSIppProp *dst = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );

	src->width = 18;
	src->height = 9;
	src->crop.width = 50;
	src->crop.height = 9;
	src->crop.x = -30;
	src->crop.y = 0;
	src->id = 100;

	fail_if( exynosVideoConverterAdaptSize( src, dst ), "FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterAdaptSize_ifSrcCropYBiggerThenSrcHeight )
{
	EXYNOSIppProp *src = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );
	EXYNOSIppProp *dst = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );

	src->width = 18;
	src->height = 9;
	src->crop.width = 19;
	src->crop.height = 50;
	src->crop.x = 0;
	src->crop.y = -30;
	src->id = 100;

	fail_if( exynosVideoConverterAdaptSize( src, dst ), "FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterAdaptSize_unsupportedDstWithAndHeight )
{
	EXYNOSIppProp *src = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );
	EXYNOSIppProp *dst = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );

	src->width = 18;
	src->height = 9;
	src->crop.width = 18;
	src->crop.height = 19;
	src->crop.x = 1;
	src->crop.y = 1;
	src->id = 100;

	dst->width = 12;
	fail_if( exynosVideoConverterAdaptSize( src, dst ), "FALSE must returned" );

	dst->width = 18;
	dst->height = 7;
	fail_if( exynosVideoConverterAdaptSize( src, dst ), "FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterAdaptSize_unsupportedDstCropWidthAndDstCropHeight )
{
	EXYNOSIppProp *src = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );
	EXYNOSIppProp *dst = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );

	src->width = 18;
	src->height = 9;
	src->crop.width = 18;
	src->crop.height = 19;
	src->crop.x = 1;
	src->crop.y = 1;
	src->id = 100;

	dst->width = 18;
	dst->height = 9;

	dst->crop.width = 14;
	fail_if( exynosVideoConverterAdaptSize( src, dst ), "FALSE must returned" );

	dst->crop.width = 18;
	dst->crop.height = 4;
	//fail_if( exynosVideoConverterAdaptSize( src, dst ), "FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterAdaptSize_cropXAndCropYAreLesThenDstWidthAndSrcHeight )
{
	EXYNOSIppProp *src = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );
	EXYNOSIppProp *dst = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );

	src->width = 18;
	src->height = 9;
	src->crop.width = 18;
	src->crop.height = 19;
	src->crop.x = 1;
	src->crop.y = 1;
	src->id = 100;

	dst->width = 18;
	dst->height = 9;
	dst->crop.width = 18;
	dst->crop.height = 9;

	dst->crop.x = 19;
	fail_if( exynosVideoConverterAdaptSize( src, dst ), "FALSE must returned" );

	dst->crop.x = 2;
	dst->crop.y = 19;
	fail_if( exynosVideoConverterAdaptSize( src, dst ), "FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterAdaptSize_ifSrcCropXBiggerThenDstWidth )
{
	EXYNOSIppProp *src = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );
	EXYNOSIppProp *dst = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );

	src->width = 18;
	src->height = 9;
	src->crop.width = 18;
	src->crop.height = 19;
	src->crop.x = 1;
	src->crop.y = 1;
	src->id = 100;


	dst->width = 18;
	dst->height = 9;
	dst->crop.width = 50;
	dst->crop.height = 9;
	dst->crop.x = -30;
	dst->crop.y = 0;
	dst->id = 100;

	fail_if( exynosVideoConverterAdaptSize( src, dst ), "FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterAdaptSize_ifSrcCropYBiggerThenDstHeight )
{
	EXYNOSIppProp *src = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );
	EXYNOSIppProp *dst = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );

	src->width = 18;
	src->height = 9;
	src->crop.width = 18;
	src->crop.height = 19;
	src->crop.x = 1;
	src->crop.y = 1;
	src->id = 100;

	dst->width = 18;
	dst->height = 9;
	dst->crop.width = 19;
	dst->crop.height = 50;
	dst->crop.x = 0;
	dst->crop.y = -30;
	dst->id = 100;

	fail_if( exynosVideoConverterAdaptSize( src, dst ), "FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterAdaptSize_correctInit )
{
	EXYNOSIppProp *src = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );
	EXYNOSIppProp *dst = ctrl_calloc( 1, sizeof( EXYNOSIppProp ) );

	src->width = 18;
	src->height = 9;
	src->crop.width = 18;
	src->crop.height = 19;
	src->crop.x = 1;
	src->crop.y = 1;
	src->id = 100;

	dst->width = 18;
	dst->height = 9;
	dst->crop.width = 18;
	dst->crop.height = 19;
	dst->crop.x = 1;
	dst->crop.y = 1;
	dst->id = 100;

	fail_if( !exynosVideoConverterAdaptSize( src, dst ), "TRUE must returned" );
}
END_TEST;
//======================================================================================================================

//======================================================================================================================
/* Bool exynosVideoConverterOpen (IMAGEPortInfoPtr pPort, PixmapPtr src, PixmapPtr dst) */
START_TEST ( exynosVideoConverterOpen_Prop_IdIsNrgative )
{
	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo ) );
	pPort->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	ScrnInfoPtr P = pPort->pScrn;
	P->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	EXYNOSPtr PP = P->driverPrivate;
	PP->drm_fd = -1;

	extern int imp_val;
	imp_val = 106;

	PixmapPtr src = ctrl_calloc( 1, sizeof( PixmapRec ) );
	src->refcnt = 106;
	PixmapPtr dst = ctrl_calloc( 1, sizeof( PixmapRec ) );
	dst->refcnt = 106;

	fail_if ( exynosVideoConverterOpen( pPort, src, dst ), "prod_id < 0, FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterOpen_IfexynosVideoConverterSetBufFail )
{
	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo ) );
	pPort->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	ScrnInfoPtr P = pPort->pScrn;
	P->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	EXYNOSPtr PP = P->driverPrivate;
	PP->drm_fd = 1;

	extern int imp_val;
	imp_val = 106;

	PixmapPtr src = ctrl_calloc( 1, sizeof( PixmapRec ) );
	src->refcnt = 106;
	PixmapPtr dst = ctrl_calloc( 1, sizeof( PixmapRec ) );
	dst->refcnt = 106;

	fail_if ( exynosVideoConverterOpen( pPort, src, dst ), "prod_id < 0, FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoConverterOpen_correctInitifpPortLast_Ipp_DataStarted )
{
	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo ) );
	pPort->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	ScrnInfoPtr P = pPort->pScrn;
	P->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	EXYNOSPtr PP = P->driverPrivate;
	PP->drm_fd = 1;

	pPort->last_ipp_data = ctrl_calloc( 1, sizeof( EXYNOSIpp ) );
	pPort->last_ipp_data->started = TRUE;

	extern int imp_val;
	imp_val = 107;

	PixmapPtr src = ctrl_calloc( 1, sizeof( PixmapRec ) );
	src->refcnt = 106;
	PixmapPtr dst = ctrl_calloc( 1, sizeof( PixmapRec ) );
	dst->refcnt = 106;

	fail_if ( !exynosVideoConverterOpen( pPort, src, dst ), "correct initialization, FALSE must returned" );
}
END_TEST;


START_TEST ( exynosVideoConverterOpen_correctInit )
{
	IMAGEPortInfoPtr pPort = ctrl_calloc( 1, sizeof( IMAGEPortInfo ) );
	pPort->pScrn = ctrl_calloc( 1, sizeof( ScrnInfoRec ) );
	ScrnInfoPtr P = pPort->pScrn;
	P->driverPrivate = ctrl_calloc( 1, sizeof( EXYNOSRec ) );
	EXYNOSPtr PP = P->driverPrivate;
	PP->drm_fd = 1;

	extern int imp_val;
	imp_val = 107;

	PixmapPtr src = ctrl_calloc( 1, sizeof( PixmapRec ) );
	src->refcnt = 106;
	PixmapPtr dst = ctrl_calloc( 1, sizeof( PixmapRec ) );
	dst->refcnt = 106;

	fail_if( !exynosVideoConverterOpen( pPort, src, dst ), "TRUE must returned" );
	fail_if( pPort->last_ipp_data->started != TRUE, "Must be initilalized by TRUE");
}
END_TEST;
//======================================================================================================================

//**********************************************************************************************************************
int test_video_converter_suite ( run_stat_data* stat )
{
	Suite *s = suite_create ( "VIDEO_CONVERTER.C" );
	/* Core test case */
	TCase *tc_exynosVideoConverterSetupConfig         = tcase_create ("_exynosVideoConverterSetupConfig");
	TCase *tc_exynosVideoConverterCompareIppDataStamp = tcase_create ("_exynosVideoConverterCompareIppDataStamp");
	TCase *tc_exynosVideoConverterCompareIppBufIndex  = tcase_create ("_exynosVideoConverterCompareIppBufIndex");
	TCase *tc_exynosVideoConverterIppQueueBuf         = tcase_create ("_exynosVideoConverterIppQueueBuf");
	TCase *tc_exynosVideoConverterIppDequeueBuf       = tcase_create ("exynosVideoConverterIppDequeueBuf");
	TCase *tc_exynosVideoConverterDequeueAllBuf       = tcase_create ("_exynosVideoConverterDequeueAllBuf");
	TCase *tc_exynosVideoConverterSetupIppData        = tcase_create ("_exynosVideoConverterSetupIppData");
	TCase *tc_exynosVideoConverterHasKeys             = tcase_create ("_exynosVideoConverterHasKeys");
	TCase *tc_exynosVideoConverterSetProperty         = tcase_create ("_exynosVideoConverterSetProperty");
	TCase *tc_exynosVideoConverterSetBuf              = tcase_create ("_exynosVideoConverterSetBuf");
	TCase *tc_exynosVideoConverterClose               = tcase_create ("exynosVideoConverterClose");
	TCase *tc_exynosVideoConverterAdaptSize           = tcase_create ("exynosVideoConverterAdaptSize");
	TCase *tc_exynosVideoConverterOpen                = tcase_create ("exynosVideoConverterOpen");


	//_exynosVideoConverterSetupConfig
	tcase_add_test( tc_exynosVideoConverterSetupConfig, exynosVideoConverterSetupConfig_fillConfigValuesByPropValues );
	tcase_add_test( tc_exynosVideoConverterSetupConfig, exynosVideoConverterSetupConfig_fillConfigValuesByPropValues_2 );

	//_exynosVideoConverterCompareIppDataStamp
	tcase_add_test( tc_exynosVideoConverterCompareIppDataStamp , exynosVideoConverterCompareIppDataStamp_different );
	tcase_add_test( tc_exynosVideoConverterCompareIppDataStamp , exynosVideoConverterCompareIppDataStamp_same );

	//_exynosVideoConverterCompareIppBufIndex
	tcase_add_test( tc_exynosVideoConverterCompareIppBufIndex , exynosVideoConverterCompareIppBufIndex_different );
	tcase_add_test( tc_exynosVideoConverterCompareIppBufIndex , exynosVideoConverterCompareIppBufIndex_same );

	//_exynosVideoConverterIppQueueBuf
	tcase_add_test( tc_exynosVideoConverterIppQueueBuf , exynosVideoConverterIppQueueBuf_pIpp_DataNull );
	tcase_add_test( tc_exynosVideoConverterIppQueueBuf , exynosVideoConverterIppQueueBuf_pIpp_BufNull );
	tcase_add_test( tc_exynosVideoConverterIppQueueBuf , exynosVideoConverterIppQueueBuf_pIpp_DataProp_IdIsZero );
	tcase_add_test( tc_exynosVideoConverterIppQueueBuf , exynosVideoConverterIppQueueBuf_bufHandlesAreNull );
	tcase_add_test( tc_exynosVideoConverterIppQueueBuf , exynosVideoConverterIppQueueBuf_correctInit );
	tcase_add_test( tc_exynosVideoConverterIppQueueBuf , exynosVideoConverterIppQueueBuf_correctInit_2 );

	//exynosVideoConverterIppDequeueBuf
	tcase_add_test( tc_exynosVideoConverterIppDequeueBuf, exynosVideoConverterIppDequeueBuf_pIpp_DataNull );
	tcase_add_test( tc_exynosVideoConverterIppDequeueBuf, exynosVideoConverterIppDequeueBuf_pIpp_BufNull );
	tcase_add_test( tc_exynosVideoConverterIppDequeueBuf, exynosVideoConverterIppDequeueBuf_pIpp_DataProp_IdIsZero );
	tcase_add_test( tc_exynosVideoConverterIppDequeueBuf, exynosVideoConverterIppDequeueBuf_ifExynosUtilStorageFindDataReturnNull );
	tcase_add_test( tc_exynosVideoConverterIppDequeueBuf, exynosVideoConverterIppDequeueBuf_bufHandlesAreNull );
	tcase_add_test( tc_exynosVideoConverterIppDequeueBuf, exynosVideoConverterIppDequeueBuf_correctInit );

	//exynosVideoConverterDequeueAllBuf
	tcase_add_test( tc_exynosVideoConverterDequeueAllBuf, exynosVideoConverterDequeueAllBuf_pIpp_DataNull );
	tcase_add_test( tc_exynosVideoConverterDequeueAllBuf, exynosVideoConverterDequeueAllBuf_pPotrNull );
	tcase_add_test( tc_exynosVideoConverterDequeueAllBuf, exynosVideoConverterDequeueAllBuf_ );

	//_exynosVideoConverterSetupIppData
	tcase_add_test( tc_exynosVideoConverterSetupIppData, exynosVideoConverterSetupIppData_srcNull );
	tcase_add_test( tc_exynosVideoConverterSetupIppData, exynosVideoConverterSetupIppData_pPortNull );
	tcase_add_test( tc_exynosVideoConverterSetupIppData, exynosVideoConverterSetupIppData_dstNull );
	tcase_add_test( tc_exynosVideoConverterSetupIppData, exynosVideoConverterSetupIppData_correctInit );

	//_exynosVideoConverterHasKeys
	tcase_add_test( tc_exynosVideoConverterHasKeys, exynosVideoConverterHasKeys_unsupportedKey );
	tcase_add_test( tc_exynosVideoConverterHasKeys, exynosVideoConverterHasKeys_unsupportedKey_2 );
	tcase_add_test( tc_exynosVideoConverterHasKeys, exynosVideoConverterHasKeys_correctInit );

	//_exynosVideoConverterSetProperty
	tcase_add_test( tc_exynosVideoConverterSetProperty, exynosVideoConverterSetProperty_pPortNull );
	tcase_add_test( tc_exynosVideoConverterSetProperty, exynosVideoConverterSetProperty_srcNull );
	tcase_add_test( tc_exynosVideoConverterSetProperty, exynosVideoConverterSetProperty_dstNull );
	tcase_add_test( tc_exynosVideoConverterSetProperty, exynosVideoConverterSetProperty_pPortLast_Ipp_DataIsNotNull );
	tcase_add_test( tc_exynosVideoConverterSetProperty, exynosVideoConverterSetProperty_pIpp_dataProp_IdNegative );
	tcase_add_test( tc_exynosVideoConverterSetProperty, exynosVideoConverterSetProperty_correctInit );

	//_exynosVideoConverterSetBuf
	tcase_add_test( tc_exynosVideoConverterSetBuf, exynosVideoConverterSetBuf_srcNull );
	tcase_add_test( tc_exynosVideoConverterSetBuf, exynosVideoConverterSetBuf_dstNull );
	tcase_add_test( tc_exynosVideoConverterSetBuf, exynosVideoConverterSetBuf_pPortLast_Ppp_DataIsNull );
	tcase_add_test( tc_exynosVideoConverterSetBuf, exynosVideoConverterSetBuf_bufHandlesAreNull );
	tcase_add_test( tc_exynosVideoConverterSetBuf, exynosVideoConverterSetBuf_correctInit );

	//exynosVideoConverterClose
	tcase_add_test( tc_exynosVideoConverterClose, exynosVideoConverterClose_pPortNull );
	tcase_add_test( tc_exynosVideoConverterClose, exynosVideoConverterClose_pPortLast_Ipp_DataIsNull );
	tcase_add_test( tc_exynosVideoConverterClose, exynosVideoConverterClose_ );

	//exynosVideoConverterAdaptSize
	tcase_add_test( tc_exynosVideoConverterAdaptSize, exynosVideoConverterAdaptSize_unsupportedWithAndHeight );
	tcase_add_test( tc_exynosVideoConverterAdaptSize, exynosVideoConverterAdaptSize_unsupportedCropWidthAndCropHeight );
	tcase_add_test( tc_exynosVideoConverterAdaptSize, exynosVideoConverterAdaptSize_cropXAndCropYAreLesThenSrcWidthAndSrcHeight );
	tcase_add_test( tc_exynosVideoConverterAdaptSize, exynosVideoConverterAdaptSize_ifSrcCropXBiggerThenSrcWidth );
	tcase_add_test( tc_exynosVideoConverterAdaptSize, exynosVideoConverterAdaptSize_ifSrcCropYBiggerThenSrcHeight );
	tcase_add_test( tc_exynosVideoConverterAdaptSize, exynosVideoConverterAdaptSize_unsupportedDstWithAndHeight );
	tcase_add_test( tc_exynosVideoConverterAdaptSize, exynosVideoConverterAdaptSize_unsupportedDstCropWidthAndDstCropHeight );
	tcase_add_test( tc_exynosVideoConverterAdaptSize, exynosVideoConverterAdaptSize_cropXAndCropYAreLesThenDstWidthAndSrcHeight );
	tcase_add_test( tc_exynosVideoConverterAdaptSize, exynosVideoConverterAdaptSize_ifSrcCropXBiggerThenDstWidth );
	tcase_add_test( tc_exynosVideoConverterAdaptSize, exynosVideoConverterAdaptSize_ifSrcCropYBiggerThenDstHeight );
	tcase_add_test( tc_exynosVideoConverterAdaptSize, exynosVideoConverterAdaptSize_correctInit );

	//exynosVideoConverterOpen
	tcase_add_test( tc_exynosVideoConverterOpen, exynosVideoConverterOpen_Prop_IdIsNrgative );
	tcase_add_test( tc_exynosVideoConverterOpen, exynosVideoConverterOpen_IfexynosVideoConverterSetBufFail );
	tcase_add_test( tc_exynosVideoConverterOpen, exynosVideoConverterOpen_correctInitifpPortLast_Ipp_DataStarted );
	tcase_add_test( tc_exynosVideoConverterOpen, exynosVideoConverterOpen_correctInit );


	suite_add_tcase ( s, tc_exynosVideoConverterSetupConfig );
	suite_add_tcase ( s, tc_exynosVideoConverterCompareIppDataStamp );
	suite_add_tcase ( s, tc_exynosVideoConverterCompareIppBufIndex );
	suite_add_tcase ( s, tc_exynosVideoConverterIppQueueBuf );
	suite_add_tcase ( s, tc_exynosVideoConverterIppDequeueBuf );
	suite_add_tcase ( s, tc_exynosVideoConverterDequeueAllBuf );
	suite_add_tcase ( s, tc_exynosVideoConverterSetupIppData );
	suite_add_tcase ( s, tc_exynosVideoConverterHasKeys );
	suite_add_tcase ( s, tc_exynosVideoConverterSetProperty );
	suite_add_tcase ( s, tc_exynosVideoConverterSetBuf );
	suite_add_tcase ( s, tc_exynosVideoConverterClose );
	suite_add_tcase ( s, tc_exynosVideoConverterAdaptSize );
	suite_add_tcase ( s, tc_exynosVideoConverterOpen );


	SRunner *sr = srunner_create( s );
	srunner_run_all( sr, CK_VERBOSE );

	stat->num_tests = srunner_ntests_run( sr );
	stat->num_pass = srunner_ntests_run( sr ) - srunner_ntests_failed( sr );
	stat->num_failure = srunner_ntests_failed( sr );

	return 0;
}
