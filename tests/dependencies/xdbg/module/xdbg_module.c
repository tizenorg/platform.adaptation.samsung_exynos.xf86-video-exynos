/**************************************************************************

xdbg

Copyright (c) 2013 Samsung Electronics Co., Ltd All Rights Reserved

Contact: Boram Park <boram1288.park@samsung.com>
         Sangjin LEE <lsj119@samsung.com>

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

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <xf86.h>

#include "xdbg_module_main.h"
#include "xdbg_module_options.h"

MODULESETUPPROTO (xDbgModuleSetup);
MODULETEARDOWNPROTO (xDbgModuleTearDown);

static XF86ModuleVersionInfo ModuleVersRec =
{
    MODULE_NAME,
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    PACKAGE_VERSION_MAJOR,
    PACKAGE_VERSION_MINOR,
    PACKAGE_VERSION_PATCHLEVEL,
    ABI_CLASS_NONE,
    SET_ABI_VERSION (0,1),
    NULL,
    {0,0,0,0}
};

_X_EXPORT XF86ModuleData xdbgModuleData =
{
    &ModuleVersRec,
    xDbgModuleSetup,
    xDbgModuleTearDown
};

static XDbgModule module_xdbg;

static void
_xDbgModuleBlockHandler (pointer data, OSTimePtr pTimeout, pointer pRead)
{
    /* _xDbgModuleBlockHandler called only at the first. */
    RemoveBlockAndWakeupHandlers(_xDbgModuleBlockHandler,
                                 (WakeupHandlerProcPtr)NoopDDA,
                                 NULL);

    /* main */
    xDbgModuleMain (&module_xdbg);
}

pointer
xDbgModuleSetup (pointer module, pointer opts, int *errmaj, int *errmin)
{
    XF86OptionPtr pOpt = (XF86OptionPtr)opts;
    static Bool setupDone = FALSE;

    if (!setupDone)
    {
        setupDone = TRUE;

        /* Parse Options */
        xDbgModuleParseOptions (&module_xdbg, pOpt);

        /* Register block handler */
        RegisterBlockAndWakeupHandlers (_xDbgModuleBlockHandler,
                                       (WakeupHandlerProcPtr)NoopDDA,
                                       NULL);
        return (pointer) 1;
    }
    else
    {
        if (errmaj)
            *errmaj = LDR_ONCEONLY;

        return NULL;
    }
}

void
xDbgModuleTearDown (pointer module)
{
    xDbgModuleMainExit (module);
}
