//---------------------------------------------------------------------------------------------

#include "../../src/sec.h"
#include "dri2.h"
#include "exa.h"
#include "property.h"
#include "damage.h"
#include "regionstr.h"
#include "dix.h"

#include "../../src/accel/sec_accel.h"

#include "fake_x_and_os.h"

OsTimerPtr get_timer_ptr( void )
{
	return 0;
}

CARD32 GetTimeInMillis( void ) 
{
    return 0;
}

_X_EXPORT OsTimerPtr TimerSet( OsTimerPtr t/* timer */ ,
                               int f/* flags */ ,
                               CARD32 m/* millis */ ,
                               OsTimerCallback fc/* func */ ,
                               pointer ptr/* arg */ )
{
	return NULL;
}

void TimerFree( OsTimerPtr pTimer ) 
{

}

void AddGeneralSocket(int fd/*fd */ ) 
{

}


void LogVWrite(int verb, const char *f, va_list args) 
{

}

void ErrorF ( const char * format, ... ) 
{

}

void NoopDDA(void) {

}

Bool RegisterBlockAndWakeupHandlers(BlockHandlerProcPtr a
                                                     /*blockHandler */ ,
                                                     WakeupHandlerProcPtr b
                                                     /*wakeupHandler */ ,
                                                     pointer c/*blockData */ )
{
	return FALSE;
}

Atom MakeAtom(const char * a/*string */ ,
                               unsigned b/*len */ ,
                               Bool c/*makeit */ )
{
    if (a && b)
        return 1;

	return 0;
}

int dixLookupProperty(PropertyPtr * a/*result */ ,
                                       WindowPtr b/*pWin */ ,
                                       Atom c/*proprty */ ,
                                       ClientPtr d/*pClient */ ,
                                       Mask e/*access_mode */ )
{


	return 0;
}



pointer
dixLookupPrivate_test(PrivatePtr *privates, const DevPrivateKey key)
{
    if (key->size)
        return 0;
    else
        return 0;
}

void xf86DrvMsg(int scrnIndex, MessageType type, const char *format, ...) 
{

}

void xf86SetModeCrtc(DisplayModePtr p, int adjustFlags) 
{

}

double xf86ModeVRefresh(const DisplayModeRec * mode) 
{
	return 0.0;
}

void  xf86CrtcConfigInit(ScrnInfoPtr scrn, const xf86CrtcConfigFuncsRec * funcs) 
{

}

void xf86CrtcSetSizeRange(ScrnInfoPtr scrn,int minWidth, int minHeight, int maxWidth, int maxHeight) 
{

}

Bool  xf86InitialConfiguration(ScrnInfoPtr pScrn, Bool canGrow) 
{
	return FALSE;
}

void  xf86CrtcDestroy(xf86CrtcPtr crtc) 
{

}

void xf86OutputDestroy(xf86OutputPtr output) 
{

}

int xf86ModeWidth(const DisplayModeRec * mode, Rotation rotati) 
{
	return 0;
}

int xf86ModeHeight(const DisplayModeRec * mode, Rotation rotation) 
{
	return 0;
}

pixman_bool_t pixman_region_copy(pixman_region16_t *dest, pixman_region16_t *source) 
{
	return 0;
}

int dixChangeWindowProperty(ClientPtr pClient, WindowPtr pWin, Atom property,
                        Atom type, int format, int mode, unsigned long len,
                        pointer value, Bool sendevent)
{
	return 0;
}

void RemoveBlockAndWakeupHandlers(BlockHandlerProcPtr blockHandler,
                             WakeupHandlerProcPtr wakeupHandler,
                             pointer blockData)
{

}

int dixLookupResourceByType(pointer *result, XID id, RESTYPE rtype,
                        ClientPtr client, Mask mode)
{
	return 0;
}

Bool AddResource(XID id, RESTYPE type, pointer value)
{
	return FALSE;
}

RESTYPE CreateNewResourceType(DeleteType deleteFunc, const char *name) 
{
	return 0;
}

RegionPtr RegionCreate(BoxPtr rect, int size)
{
	return NULL;
}

void RegionDestroy(RegionPtr pReg) 
{

}

DevPrivateKey XvGetScreenKey(void)
{
	return NULL;
}

Bool dixRegisterPrivateKey(DevPrivateKey key, DevPrivateType type, unsigned size)
{
	return TRUE;
}

void DamageDamageRegion(DrawablePtr pDrawable, const RegionPtr pRegion)
{

}

Bool xf86XVScreenInit(ScreenPtr pScreen, XF86VideoAdaptorPtr * adaptors, int num)
{
	return TRUE;
}

Bool xf86CrtcRotate(xf86CrtcPtr crtc)
{
    return TRUE;
}

void  xf86_reload_cursors(ScreenPtr screen)
{

}

xf86CrtcPtr xf86CrtcCreate(ScrnInfoPtr scrn, const xf86CrtcFuncsRec * funcs)
{
	return NULL;
}

xf86MonPtr xf86InterpretEDID(int screenIndex, Uchar * block)
{
	return (xf86MonPtr)0;
}

void xf86OutputSetEDID(xf86OutputPtr output, xf86MonPtr edid_mon)
{

}

DisplayModePtr xf86ModesAdd(DisplayModePtr modes, DisplayModePtr new)
{
	return NULL;
}

int RRConfigureOutputProperty(RROutputPtr output, Atom property,
                          Bool pending, Bool range, Bool immutable,
                          int num_values, INT32 *values)
{
	return 0;
}

int ProcRRChangeOutputProperty(ClientPtr client)
{
	return 0;
}

int RRChangeOutputProperty(RROutputPtr output, Atom property, Atom type,
                       int format, int mode, unsigned long len,
                       pointer value, Bool sendevent, Bool pending)
{
	return 0;
}

Bool  RRGetInfo(ScreenPtr pScreen, Bool force_query)
{
	return TRUE;
}

const char *NameForAtom( Atom atom )
{
	return NULL;
}

xf86OutputPtr xf86OutputCreate(ScrnInfoPtr scrn,
                 const xf86OutputFuncsRec * funcs, const char *name)
{
	 return NULL;
}

void* exaGetPixmapDriverPrivate( PixmapPtr p )
{
  SECPixmapPriv* pExaPixPriv = NULL;

	p->refcnt = 11;

	if( p->devKind != 9 ) return NULL;

	pExaPixPriv = ctrl_calloc( 1, sizeof( SECPixmapPriv ) );

	if( p->usage_hint != 25 )
	  pExaPixPriv->bo = (void*)1;

	if( p->devPrivates == (void*)1 )
	  pExaPixPriv->isFrameBuffer = 1;

	return pExaPixPriv;
}

void fbFill(DrawablePtr pDrawable, GCPtr pGC, int x, int y, int width, int height)
{

}

DevPrivateKey fbGetScreenPrivateKey(void)
{
	return NULL;
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
                     int                 height)
{

	return 0;
}

void fbBlt(FbBits * src,
      FbStride srcStride,
      int srcX,
      FbBits * dst,
      FbStride dstStride,
      int dstX,
      int width,
      int height, int alu, FbBits pm, int bpp, Bool reverse, Bool upsidedown)
{

}

pixman_image_t *image_from_pict(PicturePtr pict,Bool has_clip,int *xoff, int *yoff)
{
	return 0;
}

pixman_image_t *pixman_image_create_bits(pixman_format_code_t format,
	                              int                           width,
    	                          int                           height,
    	                          uint32_t                     *bits,
    	                          int                           rowstride_bytes)
{
	return 0;
}

void pixman_image_composite(pixman_op_t       op,
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
                           uint16_t           height)
{

}

void free_pixman_pict(PicturePtr pict, pixman_image_t *pImage)
{

}

pixman_bool_t pixman_image_unref (pixman_image_t *image)
{

	return 0;
}

GCPtr GetScratchGC(unsigned d/*depth */ , ScreenPtr p/*pScreen */ )
{
	return NULL;
}

int ChangeGC(ClientPtr c/*client */ ,
                              GCPtr gc/*pGC */ ,
                              BITS32 b/*mask */ ,
                              ChangeGCValPtr v/*pCGCV */ )
{
	return 0;
}

void ValidateGC(DrawablePtr pd/*pDraw */ ,GCPtr pGC/*pGC */ )
{

}

void FreeScratchGC(GCPtr pGC/*pGC */ )
{

}

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

}

void DRI2WaitMSCComplete(ClientPtr client, DrawablePtr pDraw,
                                          int frame, unsigned int tv_sec,
                                          unsigned int tv_usec)
{

}

void DRI2BlockClient(ClientPtr client, DrawablePtr pDraw)
{

}

int dixLookupDrawable( DrawablePtr *result, XID id, ClientPtr client, Mask type_mask, Mask access_mode )
{
	return 0;
}

Bool DRI2ScreenInit( ScreenPtr pScreen, DRI2InfoPtr info )
{
  return TRUE;
}

void DRI2CloseScreen( ScreenPtr pScreen )
{

}
 
Bool miModifyPixmapHeader( PixmapPtr pPixmap, int width, int height, int depth,
                                            int bitsPerPixel, int devKind, pointer pPixData  )
{
	return TRUE;
}

ExaDriverPtr exaDriverAlloc( void )
{
	return NULL;
}

Bool exaDriverInit( ScreenPtr pScreen, ExaDriverPtr pScreenInfo )
{
	if( 1 == pScreen->numDepths )
		return FALSE;
	return TRUE;
}

void DestroyPixmap_fake( PixmapPtr ptr )
{

}

void exynosCrtcAddPendingFlip(xf86CrtcPtr pCrtc, DRI2FrameEventPtr pEvent)
{

}

pixman_bool_t pixman_region_equal(pixman_region16_t *region1, pixman_region16_t *region2)
{
	return 0;
}

int tbm_bo_delete_user_data (tbm_bo bo, unsigned long key)
{
	return 0;
}


PixmapPtr GetScratchPixmapHeader(ScreenPtr s/*pScreen */ ,
                                                  int w/*width */ ,
                                                  int h/*height */ ,
                                                  int d/*depth */ ,
                                                  int bpp/*bitsPerPixel */ ,
                                                  int dk/*devKind */ ,
                                                  pointer data/*pPixData */ )
{
    return NULL;
}


void FreeScratchPixmapHeader(PixmapPtr s/*pPixmap */ )
{

}

void xf86AddDriver(DriverPtr driver, pointer module, int flags)
{

}

Bool xf86ReturnOptValBool(const OptionInfoRec *table, int token, Bool def)
{
	return FALSE;
}

int xf86NameCmp(const char *s1, const char *s2)
{
	return 0;
}

char *xf86GetOptValString(const OptionInfoRec *table, int token)
{
	return NULL;
}

pointer xf86AddGeneralHandler(int fd, InputHandlerProc proc, pointer data)
{
	return NULL;
}

ScreenPtr xf86ScrnToScreen(ScrnInfoPtr pScrn)
{
	return NULL;
}

void xf86PrintChipsets(const char *drvname, const char *drvmsg, SymTabPtr chips)
{

}

int xf86MatchDevice(const char *drivername, GDevPtr ** driversectlist)
{
	return 0;
}

int xf86ClaimNoSlot(DriverPtr drvp, int chipset, GDevPtr dev, Bool active)
{
	return 0;
}

ScrnInfoPtr xf86AllocateScreen(DriverPtr drv, int flags)
{
	return NULL;
}

void xf86AddEntityToScreen(ScrnInfoPtr pScrn, int entityIndex)
{

}

EntityInfoPtr xf86GetEntityInfo(int entityIndex)
{
	return 0;
}

Bool xf86SetDepthBpp(ScrnInfoPtr scrp, int depth, int bpp,
                                      int fbbpp, int depth24flags)
{
	return 0;
}

void xf86PrintDepthBpp(ScrnInfoPtr scrp)
{

}

Bool xf86SetDefaultVisual(ScrnInfoPtr scrp, int visual)
{
	return FALSE;
}

void xf86CollectOptions(ScrnInfoPtr pScrn, XF86OptionPtr extraOpts)
{

}

void xf86ProcessOptions(int scrnIndex, XF86OptionPtr options, OptionInfoPtr optinfo)
{

}

Bool xf86SetWeight(ScrnInfoPtr scrp, rgb weight, rgb mask)
{
	return FALSE;
}

Bool xf86SetGamma(ScrnInfoPtr scrp, Gamma newGamma)
{
	return FALSE;
}

void xf86SetDpi(ScrnInfoPtr pScrn, int x, int y)
{

}

void xf86PrintModes(ScrnInfoPtr scrp)
{

}

pointer xf86LoadSubModule(ScrnInfoPtr pScrn, const char *name)
{
	return NULL;
}

OsSigWrapperPtr OsRegisterSigWrapper(OsSigWrapperPtr newWrap)
{
	return NULL;
}

const char *xf86GetVisualName(int visual)
{
	return NULL;
}

void miClearVisualTypes(void)
{

}

Bool fbScreenInit(ScreenPtr pScreen, pointer pbits,
             int xsize, int ysize, int dpix, int dpiy, int width, int bpp)
{
	return FALSE;
}

Bool fbPictureInit(ScreenPtr pScreen, PictFormatPtr formats, int nformats)
{
	return FALSE;
}

void xf86SetBlackWhitePixels(ScreenPtr pScreen)
{

}

void xf86SetBackingStore(ScreenPtr pScreen)
{

}

void *xf86GetPointerScreenFuncs(void)
{
	return NULL;
}

Bool xf86_cursors_init(ScreenPtr screen, int max_width, int max_height, int flags)
{
	return FALSE;
}

ScreenInitRetType xf86CrtcScreenInit(ScreenPtr pScreen)
{
	return 0;
}

Bool xf86SetDesiredModes(ScrnInfoPtr pScrn)
{
	return FALSE;
}

Bool xf86HandleColormaps(ScreenPtr pScreen,
                                          int maxCol,
                                          int sigRGBbits,
                                          xf86LoadPaletteProc * loadPalette,
                                          xf86SetOverscanProc * setOverscan,
                                          unsigned int flags)
{
	return FALSE;
}

void xf86DPMSSet(ScrnInfoPtr pScrn, int PowerManagementMode, int flags)
{

}

Bool xf86DPMSInit(ScreenPtr pScreen, DPMSSetProcPtr set, int flags)
{
	return FALSE;
}

void xDbgLogPListInit (ScreenPtr pScreen)
{

}

Bool xf86SetSingleMode(ScrnInfoPtr pScrn, DisplayModePtr desired, Rotation rotation)
{
	return FALSE;
}

Bool miSetVisualTypes(int a, int b, int c, int d )
{
	return FALSE;
}

Bool miSetPixmapDepths(void)
{
	return FALSE;
}

Bool miDCInitialize(ScreenPtr pScreen ,miPointerScreenFuncPtr screenFuncs)
{
	return FALSE;
}


Bool miCreateDefColormap(ScreenPtr pScreen)
{
	return FALSE;
}

DamagePtr DamageCreate(DamageReportFunc damageReport,
             DamageDestroyFunc damageDestroy,
             DamageReportLevel damageLevel,
             Bool isInternal, ScreenPtr pScreen, void *closure)
{
	return NULL;
}

void DamageRegister(DrawablePtr pDrawable, DamagePtr pDamage)
{

}

void DamageDestroy(DamagePtr pDamage)
{

}

void FatalError(const char *f, ...)
{

}

int XIChangeDeviceProperty(DeviceIntPtr a/* dev */ ,
                                            Atom b /* property */ ,
                                            Atom c /* type */ ,
                                            int d /* format */ ,
                                            int f /* mode */ ,
                                            unsigned long h /* len */ ,
                                            const void * i /* value */ ,
                                            Bool        j /* sendevent */ )
{
	return 0;
}

InputInfoPtr xf86FirstLocalDevice(void)
{
	return NULL;
}

Bool AddCallback(CallbackListPtr * pcbl ,
                 CallbackProcPtr callback ,
                 pointer data )
{
	return FALSE;
}

Bool DeleteCallback(CallbackListPtr * pcbl ,
                    CallbackProcPtr callback ,
                    pointer data )
{
	return FALSE;
}

void memcpy_backward_32_neon (unsigned long *dst, unsigned long *src, int size)
{

}

void memcpy_neon (void *dst, const void *src, size_t count)
{

}

void memcpy_forward_32_neon (unsigned long *dst, unsigned long *src, int size)
{

}

void memcpy_backward_16_neon (unsigned short *dst, unsigned short *src, int size)
{

}

void memcpy_forward_16_neon (unsigned short *dst, unsigned short *src, int size)
{

}

RegionPtr RegionFromRects(int nrects ,
                          xRectanglePtr prect ,
                          int ctype )
{
	return NULL;
}

int XvdiSendPortNotify(XvPortPtr pPort, Atom attribute, INT32 value)
{
	return 0;
}


















