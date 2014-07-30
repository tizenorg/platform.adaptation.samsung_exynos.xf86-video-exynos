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

#include "exynos_driver.h"
#include "exynos_accel.h"
#include "exynos_accel_int.h"
#include "exynos_util.h"
#include "exynos_mem_pool.h"

Bool
exynosAccelInit (ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    EXYNOSAccelInfoPtr pAccelInfo = NULL;

    /* allocate the pAccelInfo private */
    pAccelInfo = ctrl_calloc (1, sizeof (EXYNOSAccelInfoRec));
    XDBG_RETURN_VAL_IF_FAIL (pAccelInfo != NULL, FALSE);

    pExynos->pAccelInfo = pAccelInfo;

    if (pExynos->is_exa)
    {
        if (!exynosExaInit (pScreen))
        {
            ctrl_free (pAccelInfo);
            return FALSE;
        }
        if (pExynos->is_dri2)
        {
            if (!exynosDri2Init (pScreen))
            {
                exynosExaDeinit (pScreen);
                ctrl_free (pAccelInfo);
                return FALSE;
            }
        }
        
//		if( pExynos->is_present )
        {
			if(!exynosPresentScreenInit(pScreen))
			{
				exynosExaDeinit (pScreen);
				ctrl_free (pAccelInfo);
				return FALSE;
			}
        }
    }

    return TRUE;
}

void
exynosAccelDeinit (ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);

    exynosDri2Deinit (pScreen);
    exynosExaDeinit (pScreen);
    ctrl_free (pExynos->pAccelInfo);
    pExynos->pAccelInfo = NULL;
}

