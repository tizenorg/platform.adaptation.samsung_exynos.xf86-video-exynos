//---------------------------------------------------------------------------------------------

#include "exynos_driver.h"
#include "dri2.h"
#include "exa.h"
#include "property.h"
#include "damage.h"
#include "regionstr.h"
#include "dix.h"

#include "exynos_accel_int.h"

#define PRINTF() ( printf( "\n\nDEBUG\n\n" ) )
#define PRINTF_ADR( adr ) ( printf( "\n%s = %p\n\n", #adr, adr ) )
#define PRINTF_VAL( val ) ( printf( "\n%s = %d\n\n", #val, val ) )

int xf86CrtcConfigPrivateIndex = 1;

EXYNOSExaPixPrivPtr pExaPixPrivForTest;

// for RegionInit() in regionstr.h
BoxRec RegionEmptyBox;
RegDataRec RegionEmptyData;

//os.h

// because of absence of real _OsTimerRec typedef
typedef struct TimerRec
{
  int field;
} _OsTimerRec;

_OsTimerRec some_value;

OsTimerPtr get_timer_ptr( void )
{
	return (OsTimerPtr)( &some_value );
}

CARD32 GetTimeInMillis( void )
{
    return some_value.field++;
}

_X_EXPORT OsTimerPtr TimerSet( OsTimerPtr t/* timer */ ,
                               int f/* flags */ ,
                               CARD32 m/* millis */ ,
                               OsTimerCallback fc/* func */ ,
                               pointer ptr/* arg */ )
{
	some_value.field = f; // any digital
	return (OsTimerPtr)( &some_value );
}

//X/include/os.h
void TimerFree( OsTimerPtr pTimer )
{
	some_value.field--;
}

void AddGeneralSocket(int fd/*fd */ ){

}


void LogVWrite(int verb, const char *f, va_list args){

}

void ErrorF ( const char * format, ... )
{
  va_list args;
  va_start (args, format);
  vprintf (format, args);
  va_end (args);
}


// dix.h
void NoopDDA(void){

}

Bool RegisterBlockAndWakeupHandlers(BlockHandlerProcPtr a
                                                     /*blockHandler */ ,
                                                     WakeupHandlerProcPtr b
                                                     /*wakeupHandler */ ,
                                                     pointer c/*blockData */ ){

}

Atom MakeAtom(const char * a/*string */ ,
                               unsigned b/*len */ ,
                               Bool c/*makeit */ )
{
	return 11;
}

ClientPtr serverClient;

//property.h
int dixLookupProperty(PropertyPtr * a/*result */ ,
                                       WindowPtr b/*pWin */ ,
                                       Atom c/*proprty */ ,
                                       ClientPtr d/*pClient */ ,
                                       Mask e/*access_mode */ ){

}

//xf86.h

void xf86DrvMsg(int scrnIndex, MessageType type, const char *format, ...){

}

void xf86SetModeCrtc(DisplayModePtr p, int adjustFlags){

}

double xf86ModeVRefresh(const DisplayModeRec * mode)
{
	return 11.0;
}

void  xf86CrtcConfigInit(ScrnInfoPtr scrn, const xf86CrtcConfigFuncsRec * funcs){

}

void xf86CrtcSetSizeRange(ScrnInfoPtr scrn,int minWidth, int minHeight, int maxWidth, int maxHeight){

}

Bool  xf86InitialConfiguration(ScrnInfoPtr pScrn, Bool canGrow){

}

int forTest_xf86CrtcDestroy = 0;
void  xf86CrtcDestroy(xf86CrtcPtr crtc){
	forTest_xf86CrtcDestroy = 1;
}

void xf86OutputDestroy(xf86OutputPtr output){

}

int xf86ModeWidth(const DisplayModeRec * mode, Rotation rotati){

}

int xf86ModeHeight(const DisplayModeRec * mode, Rotation rotation){

}

//---------------------------------------------------------------------------------------
/* exynos_video_image.c */

// scratch.i586.0/usr/include/pixman-1/pixman.h
pixman_bool_t pixman_region_copy(pixman_region16_t *dest, pixman_region16_t *source)
{
	pixman_bool_t tmp;
	return tmp;
}

// X/dix/property.c
int dixChangeWindowProperty(ClientPtr pClient, WindowPtr pWin, Atom property,
                        Atom type, int format, int mode, unsigned long len,
                        pointer value, Bool sendevent)
{
	return 0;
}

// X/dix/dixutils.c
void RemoveBlockAndWakeupHandlers(BlockHandlerProcPtr blockHandler,
                             WakeupHandlerProcPtr wakeupHandler,
                             pointer blockData)
{

}

// X/dix/resource.c
int dixLookupResourceByType(pointer *result, XID id, RESTYPE rtype,
                        ClientPtr client, Mask mode)
{
	if( id == 11 )
	{
		return 9;	 // !0
	}

	return 0;
}

// X/dix/resource.c
Bool AddResource(XID id, RESTYPE type, pointer value)
{
	if( id == 11 )
	{
		return FALSE;	 // !TRUE
	}

	return TRUE;
}

// X/dix/resource.c
RESTYPE CreateNewResourceType(DeleteType deleteFunc, const char *name)
{
	RESTYPE tmp;
	return tmp;
}

// X/dix/region.c
RegionPtr RegionCreate(BoxPtr rect, int size)
{
	RegionPtr pReg = ctrl_calloc(1, sizeof(RegionRec));
	return pReg;
}

// X/dix/region.c
void RegionDestroy(RegionPtr pReg)
{
	ctrl_free(pReg);
}

// scratch.i586.0/usr/include/xorg/xf86.h
ScrnInfoPtr *xf86Screens;

// scratch.i586.0/usr/include/xorg/xvdix.h
DevPrivateKey XvGetScreenKey(void)
{
	DevPrivateKey tmp;
	return tmp;
}

// X/dix/privates.c
Bool dixRegisterPrivateKey(DevPrivateKey key, DevPrivateType type, unsigned size)
{
	return TRUE;
}

// X/miext/damage/damage.h
DamageDamageRegion(DrawablePtr pDrawable, const RegionPtr pRegion)
{
	if(pDrawable != 0 && pRegion != 0 ) {
		pDrawable->serialNumber++;	//only to know what the function was called
	}
}

//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
/* exynos_video.c */

// X/hw/xfree86/common/xf86xv.h
Bool xf86XVScreenInit(ScreenPtr pScreen, XF86VideoAdaptorPtr * adaptors, int num)
{
	return TRUE;
}
//---------------------------------------------------------------------------------------


// scratch.i586.0/usr/include/xorg/xf86Crtc.h
Bool xf86CrtcRotate(xf86CrtcPtr crtc)
{
    return TRUE;
}

// scratch.i586.0/usr/include/xorg/xf86Crtc.h
void  xf86_reload_cursors(ScreenPtr screen)
{

}

extern void prepare_crtc(xf86CrtcPtr pCrtc);
// scratch.i586.0/usr/include/xorg/xf86Crtc.h
xf86CrtcPtr xf86CrtcCreate(ScrnInfoPtr scrn, const xf86CrtcFuncsRec * funcs)
{
	if( 101 == scrn->numClocks )
		return NULL;
	xf86CrtcPtr pCrtc = ctrl_calloc( 1, sizeof( xf86CrtcRec ) );
	prepare_crtc( pCrtc );
	return pCrtc;
}
// scratch.i586.0/usr/include/xorg/xf86DDC.h
xf86MonPtr xf86InterpretEDID(int screenIndex, Uchar * block)
{

}

// scratch.i586.0/usr/include/xorg/xf86Crtc.h
void xf86OutputSetEDID(xf86OutputPtr output, xf86MonPtr edid_mon)
{

}

// scratch.i586.0/usr/include/xorg/xf86.h
DisplayModePtr xf86ModesAdd(DisplayModePtr modes, DisplayModePtr new)
{
	return NULL;
}

// scratch.i586.0/usr/include/xorg/randstr.h
RESTYPE RRModeType;

// scratch.i586.0/usr/include/xorg/randstr.h
int RRConfigureOutputProperty(RROutputPtr output, Atom property,
                          Bool pending, Bool range, Bool immutable,
                          int num_values, INT32 *values)
{
	return 0;
}

// scratch.i586.0/usr/include/xorg/randstr.h
int ProcRRChangeOutputProperty(ClientPtr client)
{
	return 0;
}


// scratch.i586.0/usr/include/xorg/randstr.h
int RRChangeOutputProperty(RROutputPtr output, Atom property, Atom type,
                       int format, int mode, unsigned long len,
                       pointer value, Bool sendevent, Bool pending)
{
	return 0;
}

// scratch.i586.0/usr/include/xorg/randstr.h
Bool  RRGetInfo(ScreenPtr pScreen, Bool force_query)
{
	return TRUE;
}

// scratch.i586.0/usr/include/xorg/dix.h
const char *NameForAtom( Atom atom )
{
	return NULL;
}

// scratch.i586.0/usr/include/xorg/xf86Crtc.h
xf86OutputPtr xf86OutputCreate(ScrnInfoPtr scrn,
                 const xf86OutputFuncsRec * funcs, const char *name)
{
	 return NULL;
}
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
/* exa_sw_s */

void* exaGetPixmapDriverPrivate( PixmapPtr p )
{
	pExaPixPrivForTest = ctrl_calloc( 1, sizeof( EXYNOSExaPixPrivRec ) );
	extern PrivateRec;
	int test_var = 5;
/*
	101 102 103 104 105 test_exa.c use these
	106 test_video_buffer use this
 */

	switch( p->refcnt )
	{
	case 101:
		return NULL;
		break;
	case 102:
		pExaPixPrivForTest->tbo = tbm_bo_alloc( 0, 1, 0 );
		return pExaPixPrivForTest;
		break;
	case 103:
		pExaPixPrivForTest->pPixData = ( void* )test_var;
		return pExaPixPrivForTest;
		break;
	case 104:
		pExaPixPrivForTest->tbo = tbm_bo_alloc( 0, 1, 0 );
		p->devPrivate.ptr = (void *)test_var;
		return pExaPixPrivForTest;
		break;
	case 105:
		pExaPixPrivForTest->owner = 1;
		pExaPixPrivForTest->sbc = 2;
		return pExaPixPrivForTest;
		break;
	case 106:
		pExaPixPrivForTest->tbo = tbm_bo_alloc( 0, 1, 0 );
		tbm_bo_forTests( pExaPixPrivForTest->tbo, 1 );
		return pExaPixPrivForTest;
		break;
	}
	return pExaPixPrivForTest;
}

void fbFill(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int width, int height){

}

DevPrivateKey fbGetScreenPrivateKey(void)
{
	DevPrivateKey tmp;
	return tmp;
}

pixman_bool_t pixman_blt                (uint32_t           *src_bits,
                     uint32_t           *dst_bits,
                     int                 src_stride,
                     int                 dst_stride,
                     int                 src_bpp,
                     int                 dst_bpp,
                     int                 src_x,
                     int                 src_y,
                     int                 dest_x,
                     int                 dest_y,
                     int                 width,
                     int                 height){

	pixman_bool_t tmp;
	return tmp;
}

void fbBlt(FbBits * src,
      FbStride srcStride,
      int srcX,
      FbBits * dst,
      FbStride dstStride,
      int dstX,
      int width,
      int height, int alu, FbBits pm, int bpp, Bool reverse, Bool upsidedown){

}

pixman_image_t *image_from_pict(PicturePtr pict,Bool has_clip,int *xoff, int *yoff)
{
	return 0;
}

pixman_image_t *pixman_image_create_bits             (pixman_format_code_t          format,
                              int                           width,
                              int                           height,
                              uint32_t                     *bits,
                              int                           rowstride_bytes){
	return 0;
}

void          pixman_image_composite          (pixman_op_t        op,
                           pixman_image_t    *src,
                           pixman_image_t    *mask,
                           pixman_image_t    *dest,
                           int16_t            src_x,
                           int16_t            src_y,
                           int16_t            mask_x,
                           int16_t            mask_y,
                           int16_t            dest_x,
                           int16_t            dest_y,
                           uint16_t           width,
                           uint16_t           height){

}

void free_pixman_pict(PicturePtr pict, pixman_image_t *pImage){

}

pixman_bool_t   pixman_image_unref                   (pixman_image_t               *image)
{
	pixman_bool_t tmp;
	return tmp;
}

//---------------------------------------------------------------------------------------
/* exynos_dri2.c */

int drmAuthMagic( int fd, drm_magic_t magic )
{
  return 0;
}

int DRI2GetSBC( DrawablePtr pDraw, CARD64 * sbc, unsigned int* swaps_pending )
{
  return 0;
}

void DRI2SwapComplete(ClientPtr client, DrawablePtr pDraw,
                                       int frame, unsigned int tv_sec,
                                       unsigned int tv_usec, int type,
                                       DRI2SwapEventPtr swap_complete,
                                       void *swap_data)
{
	*((int *)swap_data) = 1;

}

void DRI2WaitMSCComplete(ClientPtr client, DrawablePtr pDraw,
                                          int frame, unsigned int tv_sec,
                                          unsigned int tv_usec)
{

}

/* Note: use *only* for MSC related waits */
void DRI2BlockClient(ClientPtr client, DrawablePtr pDraw)
{

}

// dix.h
int dixLookupDrawable( DrawablePtr *result, XID id, ClientPtr client, Mask type_mask, Mask access_mode )
{
	return 0;
}

// dri2.h

DRI2InfoRec dri2_info;
Bool DRI2ScreenInit( ScreenPtr pScreen, DRI2InfoPtr info )
{
	static char * driverNames[1];

	memcpy(&dri2_info, info, sizeof(DRI2InfoRec));
	dri2_info.driverNames = driverNames;

	driverNames[0] = info->driverNames[0];
	return TRUE;
}

int ck_DRI2CloseScreen = 0;
void DRI2CloseScreen( ScreenPtr pScreen )
{
	ck_DRI2CloseScreen = 1;
}
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
/* accel.c */

// mi.h
Bool miModifyPixmapHeader( PixmapPtr pPixmap, int width, int height, int depth,
                                            int bitsPerPixel, int devKind, pointer pPixData  )
{
	return TRUE;
}

// exa.h
ExaDriverPtr exaDriverAlloc( void )
{
	ExaDriverPtr pExaDriver = ctrl_calloc( 1, sizeof( ExaDriverRec ) );
	return pExaDriver;
}
Bool exaDriverInit( ScreenPtr pScreen, ExaDriverPtr pScreenInfo )
{
	if( 1 == pScreen->numDepths )
		return FALSE;
	return TRUE;
}
//---------------------------------------------------------------------------------------

void DestroyPixmap_fake( PixmapPtr ptr )
{

}

//---------------------------------------------------------------------------------------
//scratch.i586.0/usr/include/pixman-1/pixman.h
pixman_bool_t pixman_region_equal(pixman_region16_t *region1, pixman_region16_t *region2)
{
	return 1;
}


//exynos_clone.h

EXYNOSBufInfo* vbuf_gl;

//-------------------------------------------------------
void CloneQueue( EXYNOS_CloneObject *clone, EXYNOSBufInfo  *vbuf )
{
	// see to _exynosCaptureVideoRemoveReturnBuf()
	vbuf_gl = vbuf;
	vbuf_gl->dirty = TRUE;  // any value
}

//-------------------------------------------------------
EXYNOSBufInfo* get_vbuf_gl( void )
{
	return vbuf_gl;
}

//tbm/src/tbm_bufmgr.h
int tbm_bo_delete_user_data (tbm_bo bo, unsigned long key)
{
	return 1;
}
//---------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------
////exynos_clone.h
//void exynosCloneRemoveNotifyFunc(EXYNOS_CloneObject* clone,CloneNotifyFunc func)
//{
//
//}
//
////exynos_clone.h
//Bool exynosCloneIsOpened(void)
//{
//	return TRUE;
//}
//
////exynos_clone.h
//Bool exynosCloneIsRunning(void)
//{
//	return TRUE;
//}
//
////exynos_clone.h
//Bool exynosCloneCanDequeueBuffer(EXYNOS_CloneObject* clone)
//{
//	return TRUE;
//}
//
////exynos_clone.h
//Bool exynosCloneSetBuffer(EXYNOS_CloneObject *wb, EXYNOSBufInfo **vbufs,int bufnum)
//{
//	return TRUE;
//}

PixmapPtr GetScratchPixmapHeader(ScreenPtr s/*pScreen */ ,
                                                  int w/*width */ ,
                                                  int h/*height */ ,
                                                  int d/*depth */ ,
                                                  int bpp/*bitsPerPixel */ ,
                                                  int dk/*devKind */ ,
                                                  pointer data/*pPixData */ ){
    return 1;
}

int test_FreeScratchPixmapHeader = 0;
void FreeScratchPixmapHeader(PixmapPtr s/*pPixmap */ ){
	test_FreeScratchPixmapHeader = 1;
}
