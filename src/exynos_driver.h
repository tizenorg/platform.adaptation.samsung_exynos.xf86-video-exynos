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

#ifndef _EXYNOS_DRIVER_H_
#define _EXYNOS_DRIVER_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <xorg/xorg-server.h>
#include <xorg/xf86.h>
#include <xorg/xf86_OSproc.h>
#include <xf86drm.h>
#include <tbm_bufmgr.h>
#include "exynos_display.h"
#include "exynos_display_int.h"
#include "exynos_accel.h"
#include "exynos_util.h"
#include "exynos_video.h"
#include "exynos_clone.h"


#ifdef HAVE_UDEV
#include <libudev.h>
#endif

/* exynos scrninfo private infomation */
typedef struct _exynosScrnPriv EXYNOSRec, *EXYNOSPtr;

/* get a private screen of ScrnInfo */
#define EXYNOSPTR(pScrn) ((EXYNOSPtr)((pScrn)->driverPrivate))

/* the version of the driver */
#define EXYNOS_VERSION 1000

/* the name used to prefix messages */
#define EXYNOS_NAME "exynos"

/* the driver name used in display.conf file. exynos_drv.so */
#define EXYNOS_DRIVER_NAME "exynos"

/* dummy framebuffer address associated with root window */
#define ROOT_FB_ADDR (~0UL)

#define EXYNOS_CURSOR_W 64
#define EXYNOS_CURSOR_H 64

/* ScrnInfo private information */
struct _exynosScrnPriv
{
    EntityInfoPtr pEnt;

    /* driver options */
    OptionInfoPtr Options;
    Bool is_exa;
    Bool is_sw_exa;
    Bool is_dri2;
    Bool is_clone;
    Bool is_vdisplay;
    Rotation  rotate;

    /* drm */
    int drm_fd;
    char *drm_device_name;

    /* main fb information */
    EXYNOS_FbPtr pFb;

    /* tbm bufmgr */
    tbm_bufmgr bufmgr;

    /* default_bo */
    tbm_bo default_bo;
    //****** test ***********
    /* bo for virtual display*/
    tbm_bo vdisplay_bo;
    int vdisplay_fb_id;
    //***********************

    /* display info */
    EXYNOSDispInfoPtr pDispInfo;

    /* accel info */
    EXYNOSAccelInfoPtr pAccelInfo;

    /* xv info */
    EXYNOSXvInfoPtr pXvInfo;
    int lcdstatus;

    /* screen wrapper functions */
    CloseScreenProcPtr CloseScreen;
    CreateScreenResourcesProcPtr CreateScreenResources;
    void   (*PointerMoved) (ScrnInfoPtr pScr, int x, int y);//(int index, int x, int y);

    EXYNOS_CloneObject* clone;
    Bool cachable;

#ifdef HAVE_UDEV
    struct udev_monitor *uevent_monitor;
    InputHandlerProc uevent_handler;
#endif
};


#endif /* _EXYNOS_DRIVER_H_ */

