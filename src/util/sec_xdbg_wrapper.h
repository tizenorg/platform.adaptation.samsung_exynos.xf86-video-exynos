#ifndef SEC_XDBG_WRAPPER_H
#define SEC_XDBG_WRAPPER_H
#include "config.h"
#undef USE_XDBG_EXTERNAL
#ifdef USE_XDBG_EXTERNAL
#include <xdbg.h>
#else
#include <xf86.h>
#include <errno.h>
#endif

#ifndef USE_XDBG_EXTERNAL
#define XDBG_M(a,b,c,d)
#define xDbgLogSetLevel(mod,level)
#endif

#define MFB     XDBG_M('F','B',0,0)
#define MDISP   XDBG_M('D','I','S','P')
#define MLYR    XDBG_M('L','Y','R',0)
#define MPLN    XDBG_M('P','L','N',0)
#define MSEC    XDBG_M('S','E','C',0)
#define MEXA    XDBG_M('E','X','A',0)
#define MEXAS   XDBG_M('E','X','A','S')
#define MEVT    XDBG_M('E','V','T',0)
#define MDRI2   XDBG_M('D','R','I','2')
#define MCRS    XDBG_M('C','R','S',0)
#define MFLIP   XDBG_M('F','L','I','P')
#define MDPMS   XDBG_M('D','P','M','S')
#define MVDO    XDBG_M('V','D','O',0)
#define MDA     XDBG_M('D','A',0,0)
#define MTVO    XDBG_M('T','V','O',0)
#define MWB     XDBG_M('W','B',0,0)
#define MVA     XDBG_M('V','A',0,0)
#define MPROP   XDBG_M('P','R','O','P')
#define MXBRC   XDBG_M('X','B','R','C')
#define MVBUF   XDBG_M('V','B','U','F')
#define MDRM    XDBG_M('D','R','M',0)
#define MACCE   XDBG_M('A','C','C','E')
#define MCVT    XDBG_M('C','V','T',0)
#define MEXAH   XDBG_M('E','X','A','H')
#define MG2D    XDBG_M('G','2','D',0)
#define MDOUT   XDBG_M('D','O','T',0)

#ifndef USE_XDBG_EXTERNAL

/** Temporary internal logging mechanism
 * @todo Implement separate on modules
*/

#define XLOG_LEVEL_DEBUG    0
#define XLOG_LEVEL_TRACE    1
#define XLOG_LEVEL_SECURE   1
#define XLOG_LEVEL_INFO     2
#define XLOG_LEVEL_WARNING  3
#define XLOG_LEVEL_ERROR    4
#define XLOG_LEVEL_NONE     5

#ifndef LOG_LEVEL
#define LOG_LEVEL XLOG_LEVEL_DEBUG
#endif

#if LOG_LEVEL == XLOG_LEVEL_DEBUG
#define XDBG_DEBUG(mod, fmt, ARG...)      xf86Msg(X_DEBUG, "[%s] "fmt, __FUNCTION__, ##ARG)
#else
#define XDBG_DEBUG(mod, fmt, ARG...)
#endif

#if LOG_LEVEL <= XLOG_LEVEL_TRACE
#define XDBG_TRACE(mod, fmt, ARG...)      xf86Msg(X_DEBUG, "[%s] "fmt, __FUNCTION__, ##ARG)
#else
#define XDBG_TRACE(mod, fmt, ARG...)
#endif

#if LOG_LEVEL <= XLOG_LEVEL_INFO
#define XDBG_INFO(mod, fmt, ARG...)       xf86Msg(X_INFO, "[%s] "fmt, __FUNCTION__, ##ARG)
#else
#define XDBG_INFO(mod, fmt, ARG...)
#endif

#if LOG_LEVEL <= XLOG_LEVEL_WARNING
#define XDBG_WARNING(mod, fmt, ARG...)    xf86Msg(X_WARNING, "[%s] "fmt, __FUNCTION__, ##ARG)
#else
#define XDBG_WARNING(mod, fmt, ARG...)
#endif

#if LOG_LEVEL <= XLOG_LEVEL_WARNING
#define XDBG_SECURE(mod, fmt, ARG...)    xf86Msg(X_INFO, "[%s] "fmt, __FUNCTION__, ##ARG)
#else
#define XDBG_SECURE(mod, fmt, ARG...)
#endif

#if LOG_LEVEL <= XLOG_LEVEL_WARNING
#define XDBG_REPLY(...) xf86Msg(X_INFO, __VA_ARGS__)
#else
#define XDBG_REPLY(...)
#endif


#if LOG_LEVEL <= XLOG_LEVEL_ERROR
#define XDBG_ERROR(mod, fmt, ARG...)      xf86Msg(X_ERROR, "[%s] "fmt, __FUNCTION__, ##ARG)
#define XDBG_ERRNO(mod, fmt, ARG...)      xf86Msg(X_ERROR, "[%s](err=%s(%d)) "fmt, __FUNCTION__, strerror(errno), errno, ##ARG)
#define XDBG_NEVER_GET_HERE(mod)          xf86Msg(X_ERROR, "[%s] ** NEVER GET HERE **\n", __FUNCTION__)
#define XDBG_WARNING_IF_FAIL(cond)         {if (!(cond)) { xf86Msg(X_ERROR,"[%s] '%s' failed.\n", __FUNCTION__, #cond);}}
#define XDBG_RETURN_IF_FAIL(cond)          {if (!(cond)) { xf86Msg (X_ERROR,"[%s] '%s' failed.\n", __FUNCTION__, #cond); return; }}
#define XDBG_RETURN_VAL_IF_FAIL(cond, val) {if (!(cond)) { xf86Msg (X_ERROR,"[%s] '%s' failed.\n", __FUNCTION__, #cond); return val; }}
#define XDBG_GOTO_IF_FAIL(cond, dst)       {if (!(cond)) { xf86Msg (X_ERROR,"[%s] '%s' failed.\n", __FUNCTION__, #cond); goto dst; }}
#define XDBG_GOTO_IF_ERRNO(cond, dst, errno)       {if (!(cond)) { XDBG_ERRNO (XDBG, "'%s' failed.\n", #cond); goto dst; }}
#define XDBG_RETURN_VAL_IF_ERRNO(cond, val, errno)       {if (!(cond)) { XDBG_ERRNO (XDBG, "'%s' failed.\n", #cond); return val; }}
#define XDBG_KLOG(mod, fmt, ARG...)       xf86Msg(X_INFO, "[%s] "fmt, __FUNCTION__, ##ARG)
#define XDBG_SLOG(mod, fmt, ARG...)       xf86Msg(X_INFO, "[%s] "fmt, __FUNCTION__, ##ARG)
#else
#define XDBG_ERRNO(mod, fmt, ARG...)
#define XDBG_ERROR(mod, fmt, ARG...)
#define XDBG_NEVER_GET_HERE(mod)
#define XDBG_WARNING_IF_FAIL(cond)         {if (!(cond)) {;}}
#define XDBG_RETURN_IF_FAIL(cond)          {if (!(cond)) { ; return; }}
#define XDBG_RETURN_VAL_IF_FAIL(cond, val) {if (!(cond)) {; return val; }}
#define XDBG_GOTO_IF_FAIL(cond, dst)       {if (!(cond)) {; goto dst; }}
#define XDBG_GOTO_IF_ERRNO(cond, dst, errno)       {if (!(cond)) {; goto dst; }}
#define XDBG_RETURN_VAL_IF_ERRNO(cond, val, errno)       {if (!(cond)) {; return val; }}
#define XDBG_KLOG(mod, fmt, ARG...)
#define XDBG_SLOG(mod, fmt, ARG...)
#endif

struct _fpsDebug
{
    int nPanCount;
    CARD32 tStart, tCur, tLast;
    OsTimerPtr fpsTimer;
    int  connector_type;
    int  cid;
};

typedef struct _fpsDebug *FpsDebugPtr;

typedef enum
{
    MODE_NAME_ONLY,
    MODE_WITH_STATUS
} LOG_ENUM_MODE;


FpsDebugPtr
xDbgLogFpsDebugCreate ();

void
xDbgLogFpsDebugDestroy (FpsDebugPtr pFpsDebug);

void
xDbgLogFpsDebugCount (FpsDebugPtr pFpsDebug, int connector_type);

void
xDbgLogFpsDebug (int on);

void
xDbgLogDrmEventInit ();

void
xDbgLogDrmEventDeInit ();

void *
xDbgLogDrmEventAddVblank ( int crtc_pipe, unsigned int client_idx, unsigned int draw_id, int flag);

void
xDbgLogDrmEventRemoveVblank (void *vblank_data);


void *
xDbgLogDrmEventAddPageflip (int crtc_pipe, unsigned int client_idx, unsigned int draw_id);

void
xDbgLogDrmEventRemovePageflip (void *pageflip_data);


void
xDbgLogDrmEventPendingLists ( char *reply, int *remain);

void
xDbgLogPListInit (ScreenPtr pScreen);

#endif /* ifndef USE_XDBG_EXTERNAL */
#endif // SEC_XDBG_WRAPPER_H
