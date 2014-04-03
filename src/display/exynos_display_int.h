/**************************************************************************

xserver-xorg-video-exynos

Copyright 2013 Samsung Electronics co., Ltd. All Rights Reserved.

Contact: SooChan Lim <sc1.lim@samsung.com>

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#ifndef __EXYNOS_DISPLAY_INT_H__
#define __EXYNOS_DISPLAY_INT_H__

#include <xf86Crtc.h>
#include "exynos_driver.h"
#include "exynos_display.h"

typedef enum
{
    LAYER_NONE      = 0,
    LAYER_LOWER1    = 1,
    LAYER_LOWER2    = 2,
    LAYER_EFL       = 3, /* Enlightenment */
    LAYER_UPPER     = 4,
    LAYER_MAX       = 5,
} EXYNOSPlanePos;
/* exynos drm event context */
typedef struct _exynosDrmEventContext EXYNOSDrmEventCtxRec, *EXYNOSDrmEventCtxPtr;

/* exynos crtc privates */
typedef struct _exynosCrtcPriv EXYNOSCrtcPrivRec, *EXYNOSCrtcPrivPtr;
typedef struct _exynosVBlankInfo EXYNOSVBlankInfoRec, *EXYNOSVBlankInfoPtr;
typedef struct _exynosPageFlip EXYNOSPageFlipRec, *EXYNOSPageFlipPtr;

/* exynos output privates */
typedef struct _exynosOutputPriv EXYNOSOutputPrivRec, *EXYNOSOutputPrivPtr;
typedef struct _exynosProperty EXYNOSPropertyRec, *EXYNOSPropertyPtr;

/* exynos plane privates */
typedef struct _exynosPlanePriv EXYNOSPlanePrivRec, *EXYNOSPlanePrivPtr;

struct _exynosDrmEventContext
{
    void (*vblank_handler) (int fd,
                            unsigned int sequence,
                            unsigned int tv_sec,
                            unsigned int tv_usec,
                            void *user_data);

    void (*page_flip_handler) (int fd,
                               unsigned int sequence,
                               unsigned int tv_sec,
                               unsigned int tv_usec,
                               void *user_data);


    void (*ipp_handler) (int fd,
                         unsigned int  prop_id,
                         unsigned int *buf_idx,
                         unsigned int  tv_sec,
                         unsigned int  tv_usec,
                         void *user_data);

    void (*g2d_handler) (int fd,
                         unsigned int cmdlist_no,
                         unsigned int tv_exynos,
                         unsigned int tv_uexynos,
                         void *user_data);


}exynosDrmEventContext, *exynosDrmEventContextPtr;



struct _exynosCrtcPriv
{
    xf86CrtcPtr pCrtc;
    EXYNOSDispInfoPtr pDispInfo;

    int crtc_id;
    drmModeCrtcPtr mode_crtc;
    drmModeModeInfo kmode;

    int pipe;
    PixmapPtr pFrontPix;
    uint32_t fb_id;
    uint32_t old_fb_id;
    uint32_t rotate_fb_id;

    tbm_bo rotate_bo;

    /* crtc rotate by display conf */
    Rotation rotate;

    int flip_count;
    int rotate_pitch;
    unsigned int fe_frame;
    unsigned int fe_tv_sec;
    unsigned int fe_tv_usec;
    void *event_data;

    struct {
        int num;        /* number of flip back pixmaps */
        int lub;		/* Last used backbuffer */
        Bool *pix_free;   /* flags for a flip pixmap to be free */
        DrawablePtr *flip_draws;
        PixmapPtr *flip_pixmaps; /* back flip pixmaps in a crtc */
    } flip_backpixs;

    void *xdbg_fpsdebug;

    struct xorg_list pending_flips;
    struct xorg_list link;
};

struct _exynosVBlankInfo
{
    EXYNOSVBlankType type;
    void *data; /* request data pointer */

    void *xdbg_log_vblank;
};

struct _exynosPageFlip
{
    xf86CrtcPtr pCrtc;
    Bool dispatch_me;
    Bool clone;

    tbm_bo back_bo;

    void *data; /* request data pointer */

    void *xdbg_log_pageflip;
};

struct _exynosOutputPriv
{
    xf86OutputPtr pOutput;
    EXYNOSDispInfoPtr pDispInfo;

    int output_id;
    drmModeConnectorPtr mode_output;
    drmModeEncoderPtr mode_encoder;
    int num_props;
    EXYNOSPropertyPtr props;

    int dpms_mode;
    int disp_mode;

    struct xorg_list link;
    int unset_connector_type;
};

struct _exynosProperty
{
    drmModePropertyPtr mode_prop;
    uint64_t value;
    int num_atoms; /* if range prop, num_atoms == 1; if enum prop, num_atoms == num_enums + 1 */
    Atom *atoms;
};

struct _exynosPlanePriv
{
    EXYNOSDispInfoPtr pDispInfo;

    int plane_id;
    drmModePlanePtr mode_plane;

    struct xorg_list link;
};

/* framebuffer infomation */
typedef struct
{
    ScrnInfoPtr pScrn;

    int width;
    int height;
    int num_bo;
    struct xorg_list list_bo;
    void* tbl_bo;       /* bo hash table: key=(x,y)position */

    tbm_bo default_bo;
} EXYNOS_FbRec, *EXYNOS_FbPtr;

/* framebuffer bo data */
typedef struct
{
    EXYNOS_FbPtr pFb;
    BoxRec pos;
    uint32_t gem_handle;
    int fb_id;
    int pitch;
    int size;
    PixmapPtr   pPixmap;
    ScrnInfoPtr pScrn;
} EXYNOS_FbBoDataRec, *EXYNOS_FbBoDataPtr;


void    exynosCrtcInit       (ScrnInfoPtr pScrn, EXYNOSDispInfoPtr pDispInfo, int num);
void    exynosOutputInit   (ScrnInfoPtr pScrn, EXYNOSDispInfoPtr pDispInfo, int num);
void    exynosPlaneInit     (ScrnInfoPtr pScrn, EXYNOSDispInfoPtr pDispInfo, int num);
void    exynosPlaneDeinit  (EXYNOSPlanePrivPtr pPlanePriv);


void    exynosDisplayModeFromKmode(ScrnInfoPtr pScrn, drmModeModeInfoPtr kmode, DisplayModePtr pMode);
void    exynosDisplayModeToKmode(ScrnInfoPtr pScrn, drmModeModeInfoPtr kmode, DisplayModePtr pMode);

Bool    exynosCrtcApply (xf86CrtcPtr pCrtc);
int     EXYNOSCrtcGetConnectType (xf86CrtcPtr pCrtc);
Bool    outputDrmUpdate (ScrnInfoPtr pScrn);
EXYNOS_FbPtr fbAllocate (ScrnInfoPtr pScrn, int width, int height);
xf86CrtcPtr EXYNOSCrtcGetAtGeometry (ScrnInfoPtr pScrn, int x, int y, int width, int height);
xf86CrtcPtr exynosFindCrtc (ScrnInfoPtr pScrn, int pipe);

#if 0
void   exynosDisplaySwapModeFromKmode(ScrnInfoPtr pScrn, drmModeModeInfoPtr kmode, DisplayModePtr pMode);
void   exynosDisplaySwapModeToKmode(ScrnInfoPtr pScrn, drmModeModeInfoPtr kmode, DisplayModePtr pMode);
#endif

#if 1
Bool    exynosCrtcOn    (xf86CrtcPtr pCrtc);

xf86CrtcPtr exynosCrtcGetAtGeometry     (ScrnInfoPtr pScrn, int x, int y, int width, int height);
int         exynosCrtcGetConnectType    (xf86CrtcPtr pCrtc);

void    exynosCrtcCountFps(xf86CrtcPtr pCrtc);


int     exynosOutputDpmsStatus (xf86OutputPtr pOutput);
void    exynosOutputDpmsSet    (xf86OutputPtr pOutput, int mode);

Bool    exynosOutputDrmUpdate  (ScrnInfoPtr pScrn);

EXYNOSOutputPrivPtr exynosOutputGetPrivateForConnType (ScrnInfoPtr pScrn, int connect_type);
#endif

#endif /* __EXYNOS_DISPLAY_INT_H__ */

