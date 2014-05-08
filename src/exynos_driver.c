/**************************************************************************

xserver-xorg-video-exynos

Copyright 2011 Samsung Electronics co., Ltd. All Rights Reserved.

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/stat.h>
#include <fcntl.h>

/* all driver need this */
#include "xf86.h"
#include "xf86_OSproc.h"

#include "fb.h"
#include "mipointer.h"
#include "mibstore.h"
#include "micmap.h"
#include "colormapst.h"
#include "xf86cmap.h"
#include "xf86xv.h"
#include "xf86Crtc.h"
#include "exynos_driver.h"
#include "exynos_display.h"
#include "exynos_accel.h"
#include "exynos_util.h"
#include <tbm_bufmgr.h>
#include "fimg2d.h"
#include "exynos_mem_pool.h"
#include "libudev.h"


/* prototypes */
static const OptionInfoRec* EXYNOSAvailableOptions (int chipid, int busid);
static void EXYNOSIdentify              (int flags);
static Bool EXYNOSProbe                 (DriverPtr pDrv, int flags);
static Bool EXYNOSPreInit               (ScrnInfoPtr pScrn, int flags);
static Bool EXYNOSScreenInit            (ScreenPtr pScreen, int argc, char **argv);
static Bool EXYNOSSwitchMode            (ScrnInfoPtr pScrn, DisplayModePtr pMode);
static void EXYNOSAdjustFrame           (ScrnInfoPtr pScrn, int x, int y);
static Bool EXYNOSEnterVT               (ScrnInfoPtr pScrn);
static void EXYNOSLeaveVT               (ScrnInfoPtr pScrn);
static ModeStatus EXYNOSValidMode       (ScrnInfoPtr pScrn, DisplayModePtr pMode, Bool verbose, int flags);
static Bool EXYNOSCloseScreen           (ScreenPtr pScreen);
static Bool EXYNOSCreateScreenResources (ScreenPtr pScreen);

#ifdef HAVE_UDEV
static void EXYNOSUdevEventsHandler (int fd, void *closure);
#endif

/* os signal */
static OsSigWrapperPtr old_sig_wrapper;

/* This DriverRec must be defined in the driver for Xserver to load this driver */
_X_EXPORT DriverRec EXYNOS =
{
    EXYNOS_VERSION,
    EXYNOS_DRIVER_NAME,
    EXYNOSIdentify,
    EXYNOSProbe,
    EXYNOSAvailableOptions,
    NULL,
    0,
    NULL,
};

/* Supported "chipsets" */
static SymTabRec EXYNOSChipsets[] =
{
    { 0, "exynos" },
    {-1, NULL }
};

/* Supported options */
typedef enum
{
    OPTION_EXA,
    OPTION_SWEXA,
    OPTION_DRI2,
    OPTION_CACHABLE,
    OPTION_SCANOUT,
    OPTION_ROTATE,
    OPTION_SNAPSHOT,
    OPTION_CLONE,
    OPTION_VIRTUAL,
} EXYNOSOpts;

static const OptionInfoRec EXYNOSOptions[] =
{
    { OPTION_EXA,      "exa",        OPTV_BOOLEAN, {0}, TRUE },
    { OPTION_SWEXA,    "sw_exa",     OPTV_BOOLEAN, {0}, TRUE },
    { OPTION_DRI2,     "dri2",       OPTV_BOOLEAN, {0}, TRUE },
    { OPTION_CACHABLE, "cachable",   OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_SCANOUT,  "scanout",    OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_ROTATE,   "rotate",     OPTV_STRING,  {0}, FALSE },
    { OPTION_SNAPSHOT, "snapshot",   OPTV_STRING,  {0}, FALSE },
    { OPTION_CLONE,    "clone",      OPTV_BOOLEAN, {0}, FALSE },
    { OPTION_VIRTUAL,  "virtual",    OPTV_BOOLEAN, {0}, FALSE },
    { -1,              NULL,         OPTV_NONE,    {0}, FALSE }
};

/* -------------------------------------------------------------------- */
#ifdef XFree86LOADER

MODULESETUPPROTO (EXYNOSSetup);

static XF86ModuleVersionInfo EXYNOSVersRec =
{
    "exynos",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    PACKAGE_VERSION_MAJOR,
    PACKAGE_VERSION_MINOR,
    PACKAGE_VERSION_PATCHLEVEL,
    ABI_CLASS_VIDEODRV,
    ABI_VIDEODRV_VERSION,
    NULL,
    {0,0,0,0}
};

_X_EXPORT XF86ModuleData exynosModuleData = { &EXYNOSVersRec, EXYNOSSetup, NULL };

pointer
EXYNOSSetup (pointer module, pointer opts, int *errmaj, int *errmin)
{
    static Bool setupDone = FALSE;

    if (!setupDone)
    {
        setupDone = TRUE;
        xf86AddDriver (&EXYNOS, module, HaveDriverFuncs);
        return (pointer) 1;
    }
    else
    {
        if (errmaj) *errmaj = LDR_ONCEONLY;
        return NULL;
    }
}

#endif /* XFree86LOADER */
/* -------------------------------------------------------------------- */

/* TODO:::check if the drmmode_setting is available. */
static Bool
_has_drm_mode_setting()
{
    /* TODO:: check the sysfs dri2 device name */
    int drm_fd = -1;
    drm_fd = drmOpen ("exynos", NULL);
    if (drm_fd >= 0)
    {
        drmClose (drm_fd);
        return TRUE;
    }
    return FALSE;
}


/* open drm */
static int
_openDrmMaster (ScrnInfoPtr pScrn)
{
    int drm_fd = -1;
    int ret;

    /* open drm */
    drm_fd = drmOpen ("exynos", NULL);
    if (drm_fd < 0)
    {
        struct udev *udev;
        struct udev_enumerate *e;
        struct udev_list_entry *entry;
        struct udev_device *device, *drm_device;
        const char *path, *device_seat;
        const char *filename;

        xf86DrvMsg (pScrn->scrnIndex, X_WARNING, "[DRM] Cannot open drm device.. search by udev\n");

        /* STEP 1: Find drm device */
        udev = udev_new();
        if (udev == NULL)
        {
            xf86DrvMsg (pScrn->scrnIndex, X_ERROR,"[DRM] fail to initialize udev context\n");
            goto fail_to_open_drm_master;
        }

        e = udev_enumerate_new(udev);
        udev_enumerate_add_match_subsystem(e, "drm");
        udev_enumerate_add_match_sysname(e, "card[0-9]*");
        udev_enumerate_scan_devices(e);

        drm_device = NULL;
        udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(e))
        {
            path = udev_list_entry_get_name(entry);
            device = udev_device_new_from_syspath(udev, path);
            device_seat = udev_device_get_property_value(device, "ID_SEAT");
            xf86DrvMsg (pScrn->scrnIndex, X_CONFIG, "[DRM] drm info: device:%p, patch:%s, seat:%s\n", device, path, device_seat);

            if(!device_seat)
                device_seat = "seat0";

            if(strcmp(device_seat, "seat0") == 0)
            {
                drm_device = device;
                break;
            }
            udev_device_unref(device);
        }

        if(drm_device == NULL)
        {
            xf86DrvMsg (pScrn->scrnIndex, X_ERROR,"[DRM] fail to find drm device\n");
            goto fail_to_open_drm_master;
        }

        filename = udev_device_get_devnode(drm_device);

        drm_fd  = open(filename, O_RDWR|O_CLOEXEC);
        if (drm_fd < 0)
        {
            xf86DrvMsg (pScrn->scrnIndex, X_ERROR, "[DRM] Cannot open drm device(%s)\n", filename);

            udev_device_unref(drm_device);
            udev_enumerate_unref(e);
            udev_unref(udev);

            goto fail_to_open_drm_master;
        }
        else
        {
            xf86DrvMsg (pScrn->scrnIndex, X_CONFIG, "[DRM] Succeed to open drm device(%s)\n", filename);
        }

        udev_device_unref(drm_device);
        udev_enumerate_unref(e);
        udev_unref(udev);
    }
    else
    {
        xf86DrvMsg (pScrn->scrnIndex, X_CONFIG, "[DRM] Succeed to open drm device\n");
    }

    /* enable drm vblank */
    ret = drmCtlInstHandler (drm_fd, 217);
    if (ret)
    {
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
                    "[DRM] Fail to enable drm VBlank(%d)\n", ret);
        goto fail_to_open_drm_master;
    }

    xf86DrvMsg (pScrn->scrnIndex, X_CONFIG,
                "[DRM] Enable drm VBlank(%d)\n", ret);

    return drm_fd;

fail_to_open_drm_master:
    if (drm_fd >= 0)
        drmClose (drm_fd);

    return -1;
}

/* close drm */
static void
_closeDrmMaster (int drm_fd)
{
    if (drm_fd < 0)
        return;

    drmClose (drm_fd);
}


static Bool
_exynosAllocScrnPriv (ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate != NULL)
        return TRUE;

    pScrn->driverPrivate = ctrl_calloc (sizeof (EXYNOSRec), 1);
    if (!pScrn->driverPrivate)
        return FALSE;

    return TRUE;
}

static void
_exynosFreeScrnPriv (ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate == NULL)
        return;
    ctrl_free (pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}

/*
 * Check the driver option.
 * Set the option flags to the driver private
 */
static void
_exynosCheckOptions (ScrnInfoPtr pScrn)
{
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    const char *s;

    /* exa */
    if (xf86ReturnOptValBool (pExynos->Options, OPTION_EXA, FALSE))
        pExynos->is_exa = TRUE;

    /* sw exa */
    if (pExynos->is_exa)
    {
        if (xf86ReturnOptValBool (pExynos->Options, OPTION_SWEXA, TRUE))
        {
            pExynos->is_sw_exa = TRUE;
            ErrorF("SW EXA enabled\n");
        }
    }

    /* dri2 */
    if (xf86ReturnOptValBool (pExynos->Options, OPTION_DRI2, FALSE))
    {
        pExynos->is_dri2 = TRUE;

    }

    /* cachable */
       if (xf86ReturnOptValBool (pExynos->Options, OPTION_CACHABLE, FALSE))
       {
           if (xf86ReturnOptValBool (pExynos->Options, OPTION_CACHABLE, TRUE))
           {
               pExynos->cachable = TRUE;
               ErrorF ("Use cachable buffer.\n");
           }
       }

    /* rotate */
       pExynos->rotate = RR_Rotate_0;
       if (( s= xf86GetOptValString (pExynos->Options, OPTION_ROTATE)))
       {
           if (!xf86NameCmp (s, "CW"))
           {
               pExynos->rotate = RR_Rotate_90;
               xf86DrvMsg (pScrn->scrnIndex, X_CONFIG, "rotating screen clockwise\n");
           }
           else if (!xf86NameCmp (s, "CCW"))
           {
               pExynos->rotate = RR_Rotate_270;
               xf86DrvMsg (pScrn->scrnIndex, X_CONFIG, "rotating screen counter-clockwise\n");
           }
           else if (!xf86NameCmp (s, "UD"))
           {
               pExynos->rotate = RR_Rotate_180;
               xf86DrvMsg (pScrn->scrnIndex, X_CONFIG, "rotating screen upside-down\n");
           }
           else
           {
               xf86DrvMsg (pScrn->scrnIndex, X_CONFIG, "\"%s\" is not valid option", s);
           }
       }
    /* clone */
       if (xf86ReturnOptValBool (pExynos->Options, OPTION_CLONE, FALSE))
       {
           if (xf86ReturnOptValBool (pExynos->Options, OPTION_CLONE, TRUE))
               pExynos->is_clone = TRUE;
       }


    /* Virtual display */
//       pScrn->virtualX=1500;
//       pScrn->virtualY=2600;
//       pScrn->display->virtualX=1500;
//       pScrn->display->virtualY=2600;

}

/* SigHook */
static int
_exynosOsSigWrapper (int sig)
{
    XDBG_KLOG(MDRV,"Catch SIG: %d\n", sig);

    return old_sig_wrapper(sig); /*Contiue*/
}

#ifdef HAVE_UDEV
static void
_exynosInitUdev (ScrnInfoPtr pScrn)
{
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    struct udev *u;
    struct udev_monitor *mon;

    xf86DrvMsg (pScrn->scrnIndex, X_CONFIG, "hotplug detection\n");

    u = udev_new();
    if(!u)
        return;

    mon = udev_monitor_new_from_netlink(u, "udev");
    if(!mon)
    {
        udev_unref(u);
        return;
    }

    if (udev_monitor_filter_add_match_subsystem_devtype(mon, "drm", "drm_minor") > 0 ||
            udev_monitor_enable_receiving(mon) < 0)
    {
        udev_monitor_unref(mon);
        udev_unref(u);
        return;
    }

    pExynos->uevent_handler = xf86AddGeneralHandler(udev_monitor_get_fd(mon), EXYNOSUdevEventsHandler, pScrn);
    if (!pExynos->uevent_handler)
    {
        udev_monitor_unref(mon);
        udev_unref(u);
        return;
    }

    pExynos->uevent_monitor = mon;
}

static void
_exynosDeinitUdev (ScrnInfoPtr pScrn)
{
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);

    if (pExynos->uevent_handler)
    {
        struct udev *u = udev_monitor_get_udev(pExynos->uevent_monitor);

        udev_monitor_unref(pExynos->uevent_monitor);
        udev_unref(u);
        pExynos->uevent_handler = NULL;
        pExynos->uevent_monitor = NULL;
    }

}

static void
EXYNOSUdevEventsHandler (int fd, void *closure)
{
    ScrnInfoPtr pScrn = closure;
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    struct udev_device *dev;
    const char *hotplug;
    struct stat s;
    dev_t udev_devnum;
    int ret;

    dev = udev_monitor_receive_device (pExynos->uevent_monitor);
    if (!dev)
        return;

    udev_devnum = udev_device_get_devnum(dev);

    ret = fstat (pExynos->drm_fd, &s);
    if (ret == -1)
        return;

    /*
     * Check to make sure this event is directed at our
     * device (by comparing dev_t values), then make
     * sure it's a hotplug event (HOTPLUG=1)
     */
    hotplug = udev_device_get_property_value (dev, "HOTPLUG");

    if (memcmp(&s.st_rdev, &udev_devnum, sizeof (dev_t)) == 0 &&
            hotplug && atoi(hotplug) == 1)
    {
        XDBG_INFO(MDRV, "EXYNOS-UDEV: HotPlug\n");
        RRGetInfo (screenInfo.screens[pScrn->scrnIndex], TRUE);
    }

    udev_device_unref(dev);
}
#endif /* UDEV_HAVE */

/*
 * Probing the device with the device node, this probing depend on the specific hw.
 * This function just verify whether the display hw is avaliable or not.
 */
static Bool
_exynosProbeHw (struct pci_device * pPci, char *device,char **namep)
{
    return _has_drm_mode_setting();
}

static tbm_bufmgr
_exynosInitBufmgr (int drm_fd, void * arg)
{
    tbm_bufmgr bufmgr = NULL;

    /* get buffer manager */
    setenv("BUFMGR_LOCK_TYPE", "once", 1);
    setenv("BUFMGR_MAP_CACHE", "true", 1);
    bufmgr = tbm_bufmgr_init (drm_fd);

    if (bufmgr == NULL)
        return NULL;

    return bufmgr;
}

static void
_exynosDeInitBufmgr (tbm_bufmgr bufmgr)
{
    if (bufmgr)
        tbm_bufmgr_deinit (bufmgr);
}


/*
 * Initialize the device Probing the device with the device node,
 * this probing depend on the specific hw.
 * This function just verify whether the display hw is avaliable or not.
 */
static Bool
_exynosInitHw (ScrnInfoPtr pScrn, struct pci_device *pPci, char *device)
{
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);

    /* initialize drm master */
    pExynos->drm_fd = _openDrmMaster (pScrn);
    if (pExynos->drm_fd < 0)
    {
        xf86DrvMsg (pScrn->scrnIndex, X_CONFIG, "[DRM] drm is disabled\n");
        return FALSE;
    }
    else
    {
        xf86DrvMsg (pScrn->scrnIndex, X_CONFIG, "[DRM] drm is enabled.\n");

        pExynos->drm_device_name = drmGetDeviceNameFromFd (pExynos->drm_fd);
        xf86DrvMsg (pScrn->scrnIndex, X_CONFIG, "[DRM] drm device name: %s\n",
                pExynos->drm_device_name);
    }

    /* initialize tbm bufmgr */
    xf86DrvMsg (pScrn->scrnIndex, X_CONFIG,"[TBM] %i\n",pExynos->drm_fd);
    pExynos->bufmgr = _exynosInitBufmgr (pExynos->drm_fd, NULL);
    if (pExynos->bufmgr == NULL)
    {
        xf86DrvMsg (pScrn->scrnIndex, X_CONFIG,
                "[TBM] fail to initialize bufmgr.\n");
        _closeDrmMaster (pExynos->drm_fd);
        pExynos->drm_fd = -1;
        return FALSE;
    }
    else
    {
        xf86DrvMsg (pScrn->scrnIndex, X_CONFIG ,
                "[TBM] buffer manager is enabled.\n");
    }

    /* TODO: intialize the hw if needed. (ex. hw 2d, hw scaler and so on */
    if (g2d_init(pExynos->drm_fd))
        xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "G2D is enabled\n");
    else
        xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "G2D is disabled\n");

    return TRUE;
}

/*
 * DeInitialize the hw
 */
static void
_exynosDeinitHw (ScrnInfoPtr pScrn)
{
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);

    /* TODO: deintialize the hw if needed. (ex. hw 2d, hw scaler and so on */
    /*
     *
     */


    /* deinit tbm_bufmgr */
    if (pExynos->bufmgr)
    {
        _exynosDeInitBufmgr (pExynos->bufmgr);
        pExynos->bufmgr = NULL;
    }

    if (pExynos->drm_device_name)
    {
        ctrl_free (pExynos->drm_device_name);
        pExynos->drm_device_name = NULL;
    }

    /* deinit drm master */
    _closeDrmMaster (pExynos->drm_fd);
    pExynos->drm_fd = -1;
}


static Bool
EXYNOSSaveScreen (ScreenPtr pScreen, int mode)
{
 //   ErrorF("hs(%d):mode=%d\n", __func__, __LINE__, mode);
    /* dummpy save screen */
    return TRUE;
}

static const OptionInfoRec *
EXYNOSAvailableOptions (int chipid, int busid)
{
    return EXYNOSOptions;
}

static void
EXYNOSIdentify (int flags)
{
    xf86PrintChipsets (EXYNOS_NAME, "driver for Exynos Chipsets", EXYNOSChipsets);
}

static void
EXYNOSDevPointerMoved(ScrnInfoPtr pScrn, int x, int y) //(int index, int x, int y)
{

    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);

    int newX, newY;

    switch (pExynos->rotate)
    {
    case RR_Rotate_90:
    /* 90 degrees CW rotation. */
    newX = pScrn->pScreen->height - y - 1;
    newY = x;
    break;

    case RR_Rotate_270:
    /* 90 degrees CCW rotation. */
    newX = y;
    newY = pScrn->pScreen->width - x - 1;
    break;

    case RR_Rotate_180:
    /* 180 degrees UD rotation. */
    newX = pScrn->pScreen->width - x - 1;
    newY = pScrn->pScreen->height - y - 1;
    break;

    default:
    /* No rotation. */
    newX = x;
    newY = y;
    break;
    }

    /* Pass adjusted pointer coordinates to wrapped PointerMoved function. */
    (*pExynos->PointerMoved)(pScrn, newX, newY);
}


/* The purpose of this function is to identify all instances of hardware supported
 * by the driver. The probe must find the active device that match the driver
 * by calling xf86MatchDevice().
 */
static Bool
EXYNOSProbe (DriverPtr pDrv, int flags)
{
    int i;
    ScrnInfoPtr pScrn;
    GDevPtr *ppDevSections;
    int numDevSections;
    Bool foundScreen = FALSE;

    /* check the drm mode setting */
    if (!_exynosProbeHw (NULL, NULL, NULL))
        return FALSE;

    if ((numDevSections = xf86MatchDevice (EXYNOS_DRIVER_NAME, &ppDevSections)) <= 0)
    {
        xf86Msg(X_ERROR, "Didn't find exynos settings in configuration file\n");
        if (flags & PROBE_DETECT)
        {

            /* just add the device.. we aren't a PCI device, so
            *  call xf86AddBusDeviceToConfigure() directly
            */
            xf86AddBusDeviceToConfigure(EXYNOS_DRIVER_NAME,
                                        BUS_NONE, NULL, 1);
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    for (i = 0; i < numDevSections; i++)
    {
        pScrn = xf86AllocateScreen (pDrv, flags);
        if (!pScrn)
        {
            xf86Msg(X_ERROR, "Cannot allocate a ScrnInfoPtr\n");
            return FALSE;
        }
        if (ppDevSections)
        {
            int entity = xf86ClaimNoSlot(pDrv, 0, ppDevSections[i], TRUE);
            xf86AddEntityToScreen(pScrn, entity);
        }

        foundScreen = TRUE;
        pScrn->driverVersion  = EXYNOS_VERSION;
        pScrn->driverName     = EXYNOS_DRIVER_NAME;
        pScrn->name           = EXYNOS_NAME;
        pScrn->Probe          = EXYNOSProbe;
        pScrn->PreInit        = EXYNOSPreInit;
        pScrn->ScreenInit     = EXYNOSScreenInit;
        pScrn->SwitchMode     = EXYNOSSwitchMode;
        pScrn->AdjustFrame    = EXYNOSAdjustFrame;
        pScrn->EnterVT        = EXYNOSEnterVT;
        pScrn->LeaveVT        = EXYNOSLeaveVT;
        pScrn->ValidMode      = EXYNOSValidMode;

        xf86DrvMsg (pScrn->scrnIndex, X_INFO,
                   "using drm mode setting device\n");
    }
    ctrl_free (ppDevSections);

    return foundScreen;
}

/*
 * This is called before ScreenInit to probe the screen configuration.
 * The main tasks to do in this funtion are probing, module loading, option handling,
 * card mapping, and mode setting setup.
 */
static Bool
EXYNOSPreInit (ScrnInfoPtr pScrn, int flags)
{
    EXYNOSPtr pExynos;
    Gamma default_gamma = {0.0, 0.0, 0.0};
    rgb default_weight = {0, 0, 0};
    int flag24;

    if (flags & PROBE_DETECT)
        return FALSE;

    /* allocate scrninfo private */
    if (!_exynosAllocScrnPriv (pScrn))
        return FALSE;

    /* get scrninfo private */
    pExynos = EXYNOSPTR (pScrn);

    /* Check the number of entities, and fail if it isn't one. */
    if (pScrn->numEntities != 1)
        return FALSE;

    pExynos->pEnt = xf86GetEntityInfo (pScrn->entityList[0]);

    /* initialize the hardware specifics */
    if (!_exynosInitHw (pScrn, NULL, NULL))
    {
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
                    "fail to initialize hardware\n");
        goto bail1;
    }

    pScrn->displayWidth = 640; /*default width */
    pScrn->monitor = pScrn->confScreen->monitor;
    pScrn->progClock = TRUE;
    pScrn->rgbBits   = 8;

    /* set the depth and the bpp to pScrn */
    flag24 = Support24bppFb | Support32bppFb;
    if (!xf86SetDepthBpp (pScrn, 0, 0, 0, flag24))
    {
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
                    "fail to find the depth\n");
        goto bail1;
    }
    xf86PrintDepthBpp (pScrn); /* just print out the depth and the bpp */

    /* color weight */
    if (!xf86SetWeight (pScrn, default_weight, default_weight))
    {
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR ,
                    "fail to set the color weight of RGB\n");
        goto bail1;
    }

    /* visual init, make a TrueColor, -1 */
    if (!xf86SetDefaultVisual (pScrn, -1))
    {
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR ,
                    "fail to initialize the default visual\n");
        goto bail1;
    }

    /* Collect all the option flags (fill in pScrn->options) */
    xf86CollectOptions (pScrn, NULL);

    /*
     * Process the options based on the information EXYNOSOptions.
     * The results are written to pExynos->Options. If all the options
     * processing is done within this fuction a local variable "options"
     * can be used instead of pExynos->Options
     */
    if (!(pExynos->Options = malloc (sizeof (EXYNOSOptions))))
        goto bail1;
    memcpy (pExynos->Options, EXYNOSOptions, sizeof (EXYNOSOptions));
    xf86ProcessOptions (pScrn->scrnIndex, pExynos->pEnt->device->options,
                        pExynos->Options);

    /* Check with the driver options */
    _exynosCheckOptions (pScrn);

    /* drm mode init:: Set the Crtc,  the default Output, and the current Mode */
    if (!exynosDisplayPreInit (pScrn, pExynos->drm_fd))
    {
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR ,
                    "fail to initialize drm mode setting\n");
        goto bail1;
    }

    /* set gamma */
    if (!xf86SetGamma (pScrn,default_gamma))
    {
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR ,
                    "fail to set the gamma\n");
        goto bail1;
    }

    pScrn->currentMode = pScrn->modes;
    pScrn->displayWidth = pScrn->virtualX;
    xf86PrintModes (pScrn); /* just print the current mode */

    /* set dpi */
    xf86SetDpi (pScrn, 0, 0);

    /* Load modules */
    if (!xf86LoadSubModule (pScrn, "fb"))
    {
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR ,
                    "fail to load fb module\n");
        goto bail1;
    }

    if (!xf86LoadSubModule (pScrn, "exa"))
    {
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR ,
                    "fail to load exa module\n");
        goto bail1;
    }

    if (!xf86LoadSubModule (pScrn, "dri2"))
    {
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR ,
                    "fail to load dri2 module\n");
        goto bail1;
    }

    old_sig_wrapper = OsRegisterSigWrapper(_exynosOsSigWrapper);
    return TRUE;

bail1:
    _exynosFreeScrnPriv (pScrn);
    _exynosDeinitHw (pScrn);
    return FALSE;
}

static Bool
EXYNOSScreenInit (ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    VisualPtr visual;
    int init_picture = 0,flag;

    xf86DrvMsg (pScrn->scrnIndex,X_INFO,
                "Infomation of Visual is \n\tbitsPerPixel=%d, depth=%d, defaultVisual=%s\n"
                "\tmask: %x,%x,%x, offset: %d,%d,%d\n",
                pScrn->bitsPerPixel,
                pScrn->depth,
                xf86GetVisualName (pScrn->defaultVisual),
                (unsigned int) pScrn->mask.red,
                (unsigned int) pScrn->mask.green,
                (unsigned int) pScrn->mask.blue,
                (int) pScrn->offset.red,
                (int) pScrn->offset.green,
                (int) pScrn->offset.blue);
/* TODO:: allocate framebuffer here???? */
#if 0
    /* initialize the framebuffer */
    pFb = exynosFbAllocate (pScrn, pScrn->virtualX, pScrn->virtualY);
    if  (!pFb)
    {
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR, "cannot allocate framebuffer\n");
        return FALSE;
    }
    pExynos->pFb = pFb;
#else
    pExynos->default_bo = exynosDisplayRenderBoCreate(pScrn, -1, -1, pScrn->virtualX, pScrn->virtualY);
    if (pExynos->default_bo == NULL)
    {
        XDBG_ERROR (MEXA, "fail to allocate a default bo\n");
        return FALSE;
    }
#endif

    /* mi layer */
    miClearVisualTypes();
    if (!miSetVisualTypes (pScrn->depth, TrueColorMask, pScrn->rgbBits, TrueColor))
    {
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
                    "visual type setup failed for %d bits per pixel [1]\n",
                    pScrn->bitsPerPixel);
        return FALSE;
    }

    if (!miSetPixmapDepths())
    {
        xf86DrvMsg (pScrn->scrnIndex,X_ERROR ,
                    "pixmap depth setup failed\n");
        return FALSE;
    }

    switch (pScrn->bitsPerPixel)
    {
    case 16:
    case 24:
    case 32:
        if (! fbScreenInit (pScreen, (void*)ROOT_FB_ADDR,
                            pScrn->virtualX, pScrn->virtualY,
                            pScrn->xDpi, pScrn->yDpi,
                            pScrn->virtualX, /*Pixel width for framebuffer*/
                            pScrn->bitsPerPixel))
            return FALSE;

        init_picture = 1;

        break;
    default:
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
                    "internal error: invalid number of bits per pixel (%d) encountered\n",
                    pScrn->bitsPerPixel);
        break;
    }

    if (pScrn->bitsPerPixel > 8)
    {
        /* Fixup RGB ordering */
        visual = pScreen->visuals + pScreen->numVisuals;
        while (--visual >= pScreen->visuals)
        {
            if ((visual->class | DynamicClass) == DirectColor)
            {
                visual->offsetRed   = pScrn->offset.red;
                visual->offsetGreen = pScrn->offset.green;
                visual->offsetBlue  = pScrn->offset.blue;
                visual->redMask     = pScrn->mask.red;
                visual->greenMask   = pScrn->mask.green;
                visual->blueMask    = pScrn->mask.blue;
            }
        }
    }

    /* must be after RGB ordering fixed */
    if (init_picture && !fbPictureInit (pScreen, NULL, 0))
    {
        xf86DrvMsg (pScrn->scrnIndex, X_WARNING,
                    "Render extension initialisation failed\n");
    }

    /* init the exa */
    if (pExynos->is_exa)
    {
        if (!exynosAccelInit (pScreen))
        {
            xf86DrvMsg (pScrn->scrnIndex, X_WARNING,
                        "EXA initialization failed\n");
        }
    }

    /* XVideo Initiailization here */
    if (!exynosVideoInit (pScreen))
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR,
                    "XVideo extention initialization failed\n");

    xf86SetBlackWhitePixels (pScreen);
    miInitializeBackingStore (pScreen);
    xf86SetBackingStore (pScreen);

    /* use dummy hw_cursro instead of sw_cursor */
    miDCInitialize (pScreen, xf86GetPointerScreenFuncs());
    xf86DrvMsg (pScrn->scrnIndex, X_INFO
                , "Initializing HW Cursor\n");

    if (!xf86_cursors_init (pScreen, EXYNOS_CURSOR_W, EXYNOS_CURSOR_H,
                            (HARDWARE_CURSOR_TRUECOLOR_AT_8BPP |
                             HARDWARE_CURSOR_BIT_ORDER_MSBFIRST |
                             HARDWARE_CURSOR_INVERT_MASK |
                             HARDWARE_CURSOR_SWAP_SOURCE_AND_MASK |
                             HARDWARE_CURSOR_AND_SOURCE_WITH_MASK |
                             HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_64 |
                             HARDWARE_CURSOR_ARGB)))
    {
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR
                    , "Hardware cursor initialization failed\n");
    }

    /* crtc init */
    if (!xf86CrtcScreenInit (pScreen))
        return FALSE;

    /* set the desire mode : set the mode to xf86crtc here */
    xf86SetDesiredModes (pScrn);

    /* colormap */
    if (!miCreateDefColormap (pScreen))
    {
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR
                    , "internal error: miCreateDefColormap failed \n");
        return FALSE;
    }

    if (!xf86HandleColormaps (pScreen, 256, 8, exynosDisplayLoadPalette, NULL,
                              CMAP_PALETTED_TRUECOLOR))
        return FALSE;

    /* dpms */
    xf86DPMSInit (pScreen, xf86DPMSSet, 0);

    /* screen saver */
    pScreen->SaveScreen = EXYNOSSaveScreen;

    exynosDisplayInit(pScrn);

    /* Wrap the current CloseScreen function */
    pExynos->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = EXYNOSCloseScreen;

    /* Wrap the current CloseScreen function */
    pExynos->CreateScreenResources = pScreen->CreateScreenResources;
    pScreen->CreateScreenResources = EXYNOSCreateScreenResources;

    if(pExynos->rotate && !pExynos->PointerMoved)
    {
    /* driver fall if uncommented*/
      pExynos->PointerMoved = pScrn->PointerMoved;
      pScrn->PointerMoved = EXYNOSDevPointerMoved;
    }
    pExynos->lcdstatus=0;
#ifdef HAVE_UDEV
    _exynosInitUdev(pScrn);
#endif


#ifdef USE_XDBG_EXTERNAL
    xDbgLogPListInit (pScreen);
#endif
#if 0
    xDbgLogSetLevel (MDRI2, 0);
    xDbgLogSetLevel (MTST, 0);
    xDbgLogSetLevel (MTVO, 0);
    xDbgLogSetLevel (MEXA, 0);
    xDbgLogSetLevel (MDISP, 0);
    xDbgLogSetLevel (MCRTC, 0);
#endif
    /* \todo TEMP */
    EXYNOSDispInfoPtr pDispInfo = EXYNOSDISPINFOPTR(pScrn);
    pDispInfo->pipe0_on = TRUE;
    XDBG_KLOG(MDRV, "Init Screen\n");
    return TRUE;
}

static Bool
EXYNOSSwitchMode (ScrnInfoPtr pScrn, DisplayModePtr pMode)
{
    XDBG_KLOG(MDRV, "EXYNOSSwitchMode\n");
    if (!pScrn->vtSema)
    {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "We do not own the active VT, exiting.\n");
        return TRUE;
    }

    /**
     * When setting a mode through XFree86-VidModeExtension or XFree86-DGA,
     * take the specified mode and apply it to the crtc connected to the compat
     * output. Then, find similar modes for the other outputs, as with the
     * InitialConfiguration code above. The goal is to clone the desired
     * mode across all outputs that are currently active.
     */
    return xf86SetSingleMode (pScrn, pMode, RR_Rotate_0);

}

static void
EXYNOSAdjustFrame (ScrnInfoPtr pScrn, int x, int y)
{

}

static Bool
EXYNOSEnterVT (ScrnInfoPtr pScrn)
{
    xf86DrvMsg (pScrn->scrnIndex, X_INFO
                , "EnterVT::Hardware state at EnterVT:\n");

    return TRUE;
}

static void
EXYNOSLeaveVT (ScrnInfoPtr pScrn)
{
    xf86DrvMsg (pScrn->scrnIndex, X_INFO
                , "LeaveVT::Hardware state at LeaveVT:\n");
}

static ModeStatus
EXYNOSValidMode (ScrnInfoPtr pScrn, DisplayModePtr pMode, Bool verbose, int flags)
{
    return MODE_OK;
}


/**
 * Adjust the screen pixmap for the current location of the front buffer.
 * This is done at EnterVT when buffers are bound as long as the resources
 * have already been created, but the first EnterVT happens before
 * CreateScreenResources.
 */
static Bool
EXYNOSCreateScreenResources (ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    PixmapPtr pPixmap;

    pScreen->CreateScreenResources = pExynos->CreateScreenResources;
    if (!(*pScreen->CreateScreenResources) (pScreen))
        return FALSE;


    return TRUE;
}

static Bool
EXYNOSCloseScreen (ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);

#ifdef HAVE_UDEV
    _exynosDeinitUdev(pScrn);
#endif

#if 0
    exynosVideoFini (pScreen);
#endif
    exynosAccelDeinit (pScreen);
    exynosDisplayDeinit (pScrn);

    if (pExynos->default_bo)
        tbm_bo_unref (pExynos->default_bo);

    _exynosDeinitHw (pScrn);

    pScrn->vtSema = FALSE;

    pScreen->CreateScreenResources = pExynos->CreateScreenResources;
    pScreen->CloseScreen = pExynos->CloseScreen;

    XDBG_KLOG(MDRV, "Close Screen\n");
    return (*pScreen->CloseScreen) (pScreen);
}


