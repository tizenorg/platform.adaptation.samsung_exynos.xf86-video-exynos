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

#ifndef __EXYNOS_DISPLAY_H__
#define __EXYNOS_DISPLAY_H__

#include <xf86drmMode.h>
#include <xf86Crtc.h>
#include <list.h>
#include "exynos_driver.h"
#include <xf86Crtc.h>

typedef struct _exynosDisplayInfo  EXYNOSDispInfoRec, *EXYNOSDispInfoPtr;

//#include "exynos_display_int.h"

/* get a display info */
#define EXYNOSDISPINFOPTR(pScrn) ((EXYNOSDispInfoPtr)(EXYNOSPTR(pScrn)->pDispInfo))

/* drm bo data type */
typedef enum
{
    TBM_BO_DATA_FB = 1,
} TBM_BO_DATA;


typedef enum
{
    DISPLAY_SET_MODE_OFF,
    DISPLAY_SET_MODE_CLONE,
    DISPLAY_SET_MODE_EXT,
    DISPLAY_SET_MODE_VIRTUAL
} EXYNOS_DisplaySetMode;

typedef enum
{
    DISPLAY_CONN_MODE_NONE,
    DISPLAY_CONN_MODE_HDMI,
    DISPLAY_CONN_MODE_VIRTUAL,
    DISPLAY_CONN_MODE_MAX,
} EXYNOS_DisplayConnMode;

typedef enum
{
    VBLANK_INFO_NONE,
    VBLANK_INFO_SWAP,
    VBLANK_INFO_PLANE,
    VBLANK_INFO_MAX
} EXYNOSVBlankType;




struct _exynosDisplayInfo
{
    int drm_fd;
    drmModeResPtr mode_res;
    drmModePlaneResPtr plane_res;

    void *pEventCtx;

    /* temp var */
    Bool pipe0_on;
    drmModeModeInfo main_lcd_mode;
    drmModeModeInfo ext_connector_mode;
    
    /* using planes*/
    pointer using_planes;

    struct xorg_list crtcs;
    struct xorg_list outputs;
    struct xorg_list planes;

    EXYNOS_DisplaySetMode  set_mode;
    EXYNOS_DisplayConnMode conn_mode;

    //use in exynosDisplayInitDispMode
    uint32_t hdmi_crtc_id;
    tbm_bo hdmi_bo;

    Bool crtc_already_connected;
};

union exynosPropertyData
{
    CARD32 integer;
    char *string;
    Bool Bool;
};

/**
 * @brief pre-initialize the display infomation.
 * @param[in] pScrn : screen information
 * @param[in] drm_fd : drm file descripter
 * @return true or false
 */
Bool        exynosDisplayPreInit (ScrnInfoPtr pScrn, int drm_fd);

/**
 * @brief initalize the display with drm_Fd
 * @param[in] pScrn : screen information
 */
void        exynosDisplayInit    (ScrnInfoPtr pScrn);

/**
 * @brief de-initialize the display infomation.
 * @param[in] pScrn : screen information
 */
void        exynosDisplayDeinit  (ScrnInfoPtr pScrn);


void        exynosDisplayLoadPalette (ScrnInfoPtr pScrn, int numColors, int* indices, LOCO* colors, VisualPtr pVisual);

int         exynosDisplayGetPipe (ScrnInfoPtr pScrn, DrawablePtr pDraw);

xf86CrtcPtr exynosDisplayGetCrtc (DrawablePtr pDraw);

Bool        exynosDisplayPipeIsOn (ScrnInfoPtr pScrn, int pipe);

Bool        exynosDisplayGetCurMSC (ScrnInfoPtr pScrn, int pipe, CARD64 *ust, CARD64 *msc);

Bool        exynosDisplayVBlank (ScrnInfoPtr pScrn, int pipe, CARD64 *target_msc, int flip, EXYNOSVBlankType type, void *data);

Bool        exynosDisplayPageFlip (ScrnInfoPtr pScrn, int pipe, PixmapPtr pPix, void* data);

//int         exynosDisplaySetPlane (ScrnInfoPtr pScrn, int pipe, PixmapPtr pPixmap, xRectangle in_crop, xRentangle out_crop, int zpos);

//Bool        exynosDisplaySetPlaneZpos (ScrnInfoPtr pScrn, int plane_idx, int zpos);

EXYNOS_DisplaySetMode  exynosDisplayGetDispSetMode  (ScrnInfoPtr pScrn);
Bool displaySetDispSetMode  (xf86OutputPtr pOutput, EXYNOS_DisplaySetMode set_mode);
Bool displaySetDispConnMode (ScrnInfoPtr pScrn, EXYNOS_DisplayConnMode conn_mode);
xf86CrtcPtr exynosModeCoveringCrtc (ScrnInfoPtr pScrn, BoxPtr pBox, xf86CrtcPtr pDesiredCrtc, BoxPtr pBoxCrtc);

unsigned int exynosPlaneGetAvailable (ScrnInfoPtr pScrn);
Bool exynosPlaneDraw (ScrnInfoPtr pScrn, xRectangle box_in, xRectangle box_out,
                      unsigned int fb_id, unsigned int plane_id, 
                      unsigned int crtc_id, unsigned int zpos);

unsigned int exynosPlaneGetCrtcId (ScrnInfoPtr pScrn, unsigned int plane_id);
Bool exynosPlaneHide (ScrnInfoPtr pScrn, unsigned int plane_id);

void exynosCrtcRemoveFlipPixmap (xf86CrtcPtr pCrtc);

tbm_bo exynosDisplayRenderBoCreate (ScrnInfoPtr pScrn, int x, int y, int width, int height);

#if 0
xf86CrtcPtr exynosDisplayCoveringCrtc (ScrnInfoPtr pScrn, BoxPtr pBox, xf86CrtcPtr pDesiredCrtc, BoxPtr pBoxCrtc);
int         exynosDisplayGetCrtcPipe (xf86CrtcPtr pCrtc);
int exynosDisplayCrtcPipe (ScrnInfoPtr pScrn, int crtc_id);


#endif
#endif /* __EXYNOS_DISPLAY_H__ */

