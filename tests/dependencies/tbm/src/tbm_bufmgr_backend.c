/**************************************************************************

libtbm

Copyright 2012 Samsung Electronics co., Ltd. All Rights Reserved.

Contact: SooChan Lim <sc1.lim@samsung.com>, Sangjin Lee <lsj119@samsung.com>

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

#include "config.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "tbm_bufmgr_int.h"

tbm_bufmgr_backend
tbm_backend_alloc (void)
{
    tbm_bufmgr_backend bufmgr_backend;

    bufmgr_backend = calloc (1, sizeof(struct _tbm_bufmgr_backend));
    if (!bufmgr_backend)
        return NULL;

    return bufmgr_backend;
}

void
tbm_backend_free (tbm_bufmgr_backend backend)
{
    if (!backend)
        return;

    free (backend);
    backend = NULL;
}

int
tbm_backend_init (tbm_bufmgr bufmgr, tbm_bufmgr_backend backend)
{
    int flags = 0;

    if (!bufmgr)
    {
        fprintf (stderr, "[libtbm:%d] "
            "error (%s): fail to init tbm backend... bufmgr is null\n",
            getpid(), __FUNCTION__);
        return 0;
    }

    if (!backend)
    {
        fprintf (stderr, "[libtbm:%d] "
            "error (%s): fail to init tbm backend... backend is null\n",
            getpid(), __FUNCTION__);
        return 0;
    }

    flags = backend->flags;
    /* check the backend flags */
    if (!(flags&TBM_CACHE_CTRL_BACKEND))
    {
        if (!backend->bo_cache_flush)
        {
            fprintf (stderr, "[libtbm:%d] "
                "error (%s): TBM_FLAG_CACHE_CTRL_TBM needs backend->bo_cache_flush\n",
                getpid(), __FUNCTION__);
            return 0;
        }
    }

    /* log for tbm flags */
    fprintf (stderr, "[libtbm:%d] ", getpid());
    fprintf (stderr, "cache_crtl:");
    if (flags&TBM_CACHE_CTRL_BACKEND)
        fprintf (stderr, "BACKEND ");
    else
        fprintf (stderr, "TBM ");
    fprintf (stderr, "lock_crtl:");
    if (flags&TBM_LOCK_CTRL_BACKEND)
        fprintf (stderr, "BACKEND ");
    else
        fprintf (stderr, "TBM ");
    fprintf (stderr, "\n");

    bufmgr->backend = backend;

    return 1;
}

void *
tbm_backend_get_bufmgr_priv (tbm_bo bo)
{
    tbm_bufmgr_backend backend = bo->bufmgr->backend;

    return backend->priv;
}

void
tbm_backend_set_bo_priv (tbm_bo bo, void *bo_priv)
{
    bo->priv = bo_priv;
}

void *
tbm_backend_get_bo_priv (tbm_bo bo)
{
    return bo->priv;
}



