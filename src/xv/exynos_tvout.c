/*
 * xf86-video-exynos
 *
 * Copyright 2010 - 2014 Samsung Electronics co., Ltd. All Rights Reserved.
 *
 * Contact: Andrii Sokolenko <a.sokolenko@samsung.com>
 * 			Roman Marchenko <r.marchenko@samsung.com>
 *
 * Permission to use, copy, modify, distribute and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of the authors and/or copyright holders
 * not be used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  The authors and
 * copyright holders make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without any express
 * or implied warranty.
 *
 * THE AUTHORS AND COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "exynos_tvout.h"
#include "exynos_driver.h"
#include "exynos_util.h"
#include "exynos_display.h"
#include <xf86Crtc.h>
#define XF86_CRTC_PTR(p)	((xf86CrtcConfigPtr) ((p)->privates[xf86CrtcConfigPrivateIndex].ptr))


int
exynosTvoutDisableClone(ScrnInfoPtr pScrn)
{

    XDBG_RETURN_VAL_IF_FAIL(pScrn != NULL, -1);
    xf86CrtcConfigPtr   xf86_config = XF86_CRTC_PTR (pScrn);
    int i;
    Bool ret = FALSE;

    for (i = 0; i < xf86_config->num_output; i++)
    {
        xf86OutputPtr pOutput = xf86_config->output[i];
        EXYNOSOutputPrivPtr pOutputPriv;

        pOutputPriv = pOutput->driver_private;

        displaySetDispSetMode (pOutput, DISPLAY_SET_MODE_EXT);
        ret = TRUE;
    }

return ret;

}

int
exynosTvoutEnableClone(ScrnInfoPtr pScrn)
{

    XDBG_RETURN_VAL_IF_FAIL(pScrn != NULL, -1);
    xf86CrtcConfigPtr   xf86_config = XF86_CRTC_PTR (pScrn);
    int i;
    Bool ret = FALSE;

    for (i = 0; i < xf86_config->num_output; i++)
    {
        xf86OutputPtr pOutput = xf86_config->output[i];
        EXYNOSOutputPrivPtr pOutputPriv;

        pOutputPriv = pOutput->driver_private;

        displaySetDispSetMode (pOutput, DISPLAY_SET_MODE_CLONE);
        ret = TRUE;
    }

return ret;

}
