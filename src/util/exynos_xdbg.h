#ifndef EXYNOS_XDBG_H
#define EXYNOS_XDBG_H

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

#define MDRV    XDBG_M('D','R','V',0)
#define MDISP   XDBG_M('D','I','S','P')
#define MCRTC   XDBG_M('C','R','T','C')
#define MOUTP   XDBG_M('O','U','T','P')
#define MPLN    XDBG_M('P','L','N',0)
#define MEXA    XDBG_M('E','X','A',0)
#define MEXAS   XDBG_M('E','X','A','S')
#define MEVT    XDBG_M('E','V','T',0)
#define MDRI2   XDBG_M('D','R','I','2')
#define MCRS    XDBG_M('C','R','S',0)
#define MDPMS   XDBG_M('D','P','M','S')
#define MIA     XDBG_M('I','A',0,0)
#define MCA     XDBG_M('C','A',0,0)
#define MVBUF   XDBG_M('V','B','U','F')
#define MCVT    XDBG_M('C','V','T','0')
#define MMPL    XDBG_M('M','P','L',0)
#define MG2D    XDBG_M('G','2','D',0)
#define MTRC    XDBG_M('T','R','C',0)
#define MDUMP   XDBG_M('D','U','M','P')
#define MFB     XDBG_M('F','B',0,0)
#define MVA     XDBG_M('V','A',0,0)
#define MOVA    XDBG_M('O','V','A',0)
#define MPROP   XDBG_M('P','R','O','P')
#define MTVO    XDBG_M('T','V','O',0)
/* Maybe need to delete below modules*/
#define MCLO    XDBG_M('C','L','O',0)
#define MDRM    XDBG_M('D','R','M',0)
#define MEVN    XDBG_M('E','V','N',0)
#define MPXM    XDBG_M('P','X','M',0)

#ifndef USE_XDBG_EXTERNAL

/** Temporary internal logging mechanism
 * @todo Implement separate on modules
*/

#define XLOG_LEVEL_DEBUG    0
#define XLOG_LEVEL_TRACE    1
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




#endif /* ifndef USE_XDBG_EXTERNAL */


#endif // EXYNOS_XDBG_H
