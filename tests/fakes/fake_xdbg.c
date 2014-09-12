
#include "../../src/sec.h"

/* Note:  XDBG_SECURE defined as macro in xdbg_log.h */
void XDBG_SECURE(unsigned int module, char * b, ... )
{

}

//#define XDBG_SECURE(mod, fmt, ARG...)     do { } while(0)


void* xDbgLog( unsigned int module, int logoption, const char *file, int line, const char *f, ... ) 
{
	return (void*)NULL;
}

void xDbgLogPListDrawAddRefPixmap    (DrawablePtr pDraw, PixmapPtr pRefPixmap)
{

}

void xDbgLogPListDrawRemoveRefPixmap (DrawablePtr pDraw, PixmapPtr pRefPixmap)
{

}

void xDbgLogFpsDebugDestroy (FpsDebugPtr pFpsDebug)
{

}

FpsDebugPtr xDbgLogFpsDebugCreate  (void)
{
	return NULL;
}

void xDbgLogFpsDebugCount (FpsDebugPtr pFpsDebug, int connector_type)
{

}

void * xDbgLogDrmEventAddVblank ( int crtc_pipe, unsigned int client_idx,
								  unsigned int draw_id, int flag)
{
	return (void*)NULL;
}

void xDbgLogDrmEventRemoveVblank (void *vblank_data)
{

}

void *xDbgLogDrmEventAddPageflip (int crtc_pipe, unsigned int client_idx, unsigned int draw_id)
{
	return (void *)NULL;
}

void xDbgLogDrmEventRemovePageflip (void *pageflip_data)
{

}

void xDbgLogDrmEventInit (void)
{

}





















