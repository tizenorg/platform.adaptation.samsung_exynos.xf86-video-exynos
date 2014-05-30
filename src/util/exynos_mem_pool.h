/*
 * exynos_mem_pool.h
 *
 *  Created on: Sep 9, 2013
 *  Author: Andrey Sokolenko
 */
#ifndef __EXYNOS_MEM_POOL_H__
#define __EXYNOS_MEM_POOL_H__

#include <sys/types.h>
#include <fbdevhw.h>
#include <X11/Xdefs.h>
#include <exynos/exynos_drm.h>
#include <tbm_bufmgr.h>

#ifdef USE_EXYNOS_DRM
#define PLANE_CNT EXYNOS_DRM_PLANAR_MAX
#else
#define PLANE_CNT 3
#endif
#define BUF_TYPE_DMABUF  0
#define BUF_TYPE_LEGACY  1
#define BUF_TYPE_VPIX    2
#define OPTION_READ     1
#define OPTION_WRITE    2


typedef struct _EXYNOSBufInfo
{
    int ref_cnt;

    ClientPtr   pClient;
    ScrnInfoPtr pScrn;

    int fourcc;
    int width;
    int height;
    xRectangle crop;

    int           num_planes;
    tbm_bo        tbos[PLANE_CNT];
    unsigned int  names[PLANE_CNT];
    tbm_bo_handle handles[PLANE_CNT];
    unsigned int  type;

    int     pitches[PLANE_CNT];
    int     offsets[PLANE_CNT];
    int     lengths[PLANE_CNT];
    int     size;

    void (*free_func)(pointer);
    Bool converting;        /* now converting. */
    Bool showing;           /* now showing or now waiting until vblank. */

    Bool dirty;
    Bool need_reset;

    unsigned int fb_id;     /* fb_id of vpix */
    CARD32 stamp;

} EXYNOSBufInfo, *EXYNOSBufInfoPtr;


EXYNOSBufInfoPtr exynosMemPoolCreateBO (ScrnInfoPtr pScrn, int id, int width, int height);
EXYNOSBufInfoPtr exynosMemPoolAllocateBO (ScrnInfoPtr pScrn, int id, int width,
                                          int height, tbm_bo *tbos,
                                          unsigned int *names, unsigned int buf_type);
void exynosMemPoolDestroyBO (EXYNOSBufInfoPtr bufinfo);
Bool exynosMemPoolPrepareAccess (EXYNOSBufInfoPtr bufinfo, tbm_bo_handle *handles, int option);
void exynosMemPoolFinishAccess (EXYNOSBufInfoPtr bufinfo);

EXYNOSBufInfoPtr exynosMemPoolGetBufInfo (pointer b_object);
Bool exynosMemPoolGetGemHandle (ScrnInfoPtr scrn, uint64_t phy_addr, uint64_t size,
                                unsigned int *handle);
Bool exynosMemPoolGetGemBo (ScrnInfoPtr pScrn, unsigned int handle, tbm_bo *);
void exynosMemPoolFreeHandle (ScrnInfoPtr pScrn, unsigned int handle);
void exynosMemPoolCacheFlush (ScrnInfoPtr scrn);
void exynosUtilClearNormalVideoBuffer (EXYNOSBufInfoPtr vbuf);


void* ctrl_calloc( size_t, size_t );
void* ctrl_malloc( size_t );
void  ctrl_free( void* );
char* ctrl_strdup( const char *__s );

#ifndef TEST_COMPILE
 #define ctrl_calloc calloc
 #define ctrl_malloc malloc
 #define ctrl_free free
 #define ctrl_strdup strdup
 #define ctrl_xorg_list_for_each_entry_safe xorg_list_for_each_entry_safe
 #define f_xorg_list_for_each_entry_safe xorg_list_for_each_entry_safe
 #define f_xorg_list_add xorg_list_add
 #define f_xorg_list_del xorg_list_del
#else
	// fake-defines
	#define ctrl_xorg_list_for_each_entry_safe( pCur, pNext, list, func_ptr ) \
				pCur = ctrl_calloc( 1, sizeof( EXYNOSOutputPrivRec ) ); \
				pCur->mode_output = ctrl_calloc( 1, sizeof( drmModeConnector ) ); \
				pCur->mode_output->connector_type = 11; // any digital
	/*
	#define nt_list_for_each_entry( entry, list, member ) entry = calloc( 1, sizeof( exynosCloneNotifyFuncInfo ) ); \
				info->noti = CLONE_NOTI_START; \
				info->func = 1;
	 */
#endif //TEST_COMPILE



#endif /* __EXYNOS_MEM_POOL_H__ */
