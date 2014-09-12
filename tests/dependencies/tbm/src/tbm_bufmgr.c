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
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include "tbm_bufmgr.h"
#include "tbm_bufmgr_glock.h"
#include "tbm_bufmgr_backend.h"
#include "tbm_bufmgr_int.h"
#include "list.h"

#define DEBUG
#ifdef DEBUG
static int bDebug = 0;
#define DBG(...) if(bDebug&0x1) fprintf(stderr, __VA_ARGS__)
#define DBG_LOCK(...) if(bDebug&0x2) fprintf(stderr, __VA_ARGS__)
#else
#define DBG(...)
#define DBG_LOCK(...)
#endif

/* check condition */
#define TBM_RETURN_IF_FAIL(cond) {\
    if (!(cond)) {\
        fprintf (stderr, "[%s] : '%s' failed.\n", __FUNCTION__, #cond);\
        return;\
    }\
}
#define TBM_RETURN_VAL_IF_FAIL(cond, val) {\
    if (!(cond)) {\
        fprintf (stderr, "[%s] : '%s' failed.\n", __FUNCTION__, #cond);\
        return val;\
    }\
}

/* check flags */
#define RETURN_CHECK_FLAG(cond) {\
    if ((cond)) {\
        return;\
    }\
}
#define RETURN_VAL_CHECK_FLAG(cond, val) {\
    if ((cond)) {\
        return val;\
    }\
}


/* check validation */
#define TBM_BUFMGR_IS_VALID(mgr) (mgr && \
                           mgr->link.next &&\
                           mgr->link.next->prev == &mgr->link)
#define TBM_BO_IS_VALID(bo) (bo && \
                         TBM_BUFMGR_IS_VALID(bo->bufmgr) && \
                         bo->item_link.next && \
                         bo->item_link.next->prev == &bo->item_link)

#define CTRL_BACKEND_VALID(flags) ((flags&TBM_CACHE_CTRL_BACKEND) && \
                          (flags&TBM_LOCK_CTRL_BACKEND))

#define PREFIX_LIB    "libtbm_"
#define SUFFIX_LIB    ".so"
#define DEFAULT_LIB   PREFIX_LIB"default"SUFFIX_LIB

#define BO_IS_CACHEABLE(bo) ((bo->flags & TBM_BO_NONCACHABLE)?0:1)
#define DEVICE_IS_CACHE_AWARE(device) ((device == TBM_DEVICE_CPU)?(1):(0))

/* tgl key values */
#define GLOBAL_KEY   ((unsigned int)(-1))
#define INITIAL_KEY  ((unsigned int)(-2))

#define CACHE_OP_CREATE     (-1)
#define CACHE_OP_ATTACH     (-2)
#define CACHE_OP_IMPORT     (-3)

/* values to indicate unspecified fields in XF86ModReqInfo. */
#define MAJOR_UNSPEC        0xFF
#define MINOR_UNSPEC        0xFF
#define PATCH_UNSPEC        0xFFFF
#define ABI_VERS_UNSPEC   0xFFFFFFFF

#define MODULE_VERSION_NUMERIC(maj, min, patch) \
               ((((maj) & 0xFF) << 24) | (((min) & 0xFF) << 16) | (patch & 0xFFFF))
#define GET_MODULE_MAJOR_VERSION(vers)	(((vers) >> 24) & 0xFF)
#define GET_MODULE_MINOR_VERSION(vers)	(((vers) >> 16) & 0xFF)
#define GET_MODULE_PATCHLEVEL(vers)	((vers) & 0xFFFF)

enum {
    LOCK_TRY_ONCE,
    LOCK_TRY_ALWAYS,
    LOCK_TRY_NEVER
};

enum {
    DEVICE_NONE = 0,
    DEVICE_CA,       /* cache aware device */
    DEVICE_CO        /* cache oblivious device */
};

typedef struct
{
    unsigned long key;
    void *data;
    tbm_data_free free_func ;

    /* link of user_data */
    struct list_head item_link;
} tbm_user_data;

/* list of bufmgr */
static struct list_head *gBufMgrs = NULL;


static inline int
_tgl_init (int fd, unsigned int key)
{
    struct tgl_attribute attr;
    int err;

    attr.key = key;
    attr.timeout_ms = 1000;

    err = ioctl (fd, TGL_IOC_INIT_LOCK, &attr);
    if (err)
    {
        fprintf (stderr, "[libtbm:%d] "
                "error(%s) %s:%d key:%d\n",
                getpid(), strerror(errno), __FUNCTION__, __LINE__, key);
        return 0;
    }

    return 1;
}

static inline int
_tgl_destroy (int fd, unsigned int key)
{
    int err;
    err = ioctl (fd, TGL_IOC_DESTROY_LOCK, key);
    if (err)
    {
        fprintf (stderr, "[libtbm:%d] "
                "error(%s) %s:%d key:%d\n",
                getpid(), strerror(errno), __FUNCTION__, __LINE__, key);
        return 0;
    }

    return 1;
}

static inline int
_tgl_lock (int fd, unsigned int key)
{
    int err;
    err = ioctl (fd, TGL_IOC_LOCK_LOCK, key);
    if (err)
    {
        fprintf (stderr, "[libtbm:%d] "
                "error(%s) %s:%d key:%d\n",
                getpid(), strerror(errno), __FUNCTION__, __LINE__, key);
        return 0;
    }

    return 1;
}

static inline int
_tgl_unlock (int fd, unsigned int key)
{
    int err;
    err = ioctl (fd, TGL_IOC_UNLOCK_LOCK, key);
    if (err)
    {
        fprintf (stderr, "[libtbm:%d] "
                "error(%s) %s:%d key:%d\n",
                getpid(), strerror(errno), __FUNCTION__, __LINE__, key);
        return 0;
    }

    return 1;
}

static inline int
_tgl_set_data (int fd, unsigned int key, unsigned int val)
{
    int err;
    struct tgl_user_data arg;

    arg.key = key;
    arg.data1 = val;
    err = ioctl (fd, TGL_IOC_SET_DATA, &arg);
    if (err)
    {
        fprintf (stderr, "[libtbm:%d] "
                "error(%s) %s:%d key:%d\n",
                getpid(), strerror(errno), __FUNCTION__, __LINE__, key);
        return 0;
    }

    return 1;
}

static inline unsigned int
_tgl_get_data (int fd, unsigned int key, unsigned int *locked)
{
    int err;
    struct tgl_user_data arg = {0,};

    arg.key = key;
    err = ioctl (fd, TGL_IOC_GET_DATA, &arg);
    if (err)
    {
        fprintf (stderr, "[libtbm:%d] "
                "error(%s) %s:%d key:%d\n",
                getpid(), strerror(errno), __FUNCTION__, __LINE__, key);
        return 0;
    }

    if (locked)
        *locked = arg.locked;

    return arg.data1;
}

static tbm_user_data *
_user_data_lookup (struct list_head *user_data_list, unsigned long key)
{
    tbm_user_data *user_data = NULL;
    tbm_user_data *old_data = NULL, *tmp = NULL;

    if (!LIST_IS_EMPTY (user_data_list))
    {
        LIST_FOR_EACH_ENTRY_SAFE (old_data, tmp, user_data_list, item_link)
        {
            if (old_data->key == key)
            {
                user_data = old_data;
                return user_data;
            }
        }
    }

    return user_data;
}

static tbm_user_data *
_user_data_create (unsigned long key, tbm_data_free data_free_func)
{
    tbm_user_data * user_data = NULL;

    user_data = calloc (1, sizeof (tbm_user_data));
    if (!user_data)
        return NULL;

    user_data->key = key;
    user_data->free_func = data_free_func;
    user_data->data = (void *)0;

    return user_data;
}

static void
_user_data_delete (tbm_user_data *user_data)
{
    if (user_data->data && user_data->free_func)
        user_data->free_func(user_data->data);

    LIST_DEL (&user_data->item_link);

    free(user_data);
}

static int
_bo_lock (tbm_bo bo)
{
    tbm_bufmgr bufmgr = bo->bufmgr;
    int ret = 0;

    if (bufmgr->backend->flags&TBM_LOCK_CTRL_BACKEND &&
        bufmgr->backend->bo_lock)
    {
        /* use backend lock */
        ret = bufmgr->backend->bo_lock (bo);
    }
    else
    {
        /* use tizen global lock */
        ret = _tgl_lock (bufmgr->lock_fd, bo->tgl_key);
    }

    return ret;
}

static void
_bo_unlock (tbm_bo bo)
{
    tbm_bufmgr bufmgr = bo->bufmgr;

    if (bufmgr->backend->flags&TBM_LOCK_CTRL_BACKEND &&
        bufmgr->backend->bo_unlock)
    {
        /* use backend unlock */
        bufmgr->backend->bo_unlock (bo);
    }
    else
    {
        /* use tizen global unlock */
        _tgl_unlock (bufmgr->lock_fd, bo->tgl_key);
    }
}

static int
_tbm_bo_lock (tbm_bo bo)
{
    tbm_bufmgr bufmgr = NULL;
    int old;
    int ret = 0;

    if (!bo)
        return 0;

    bufmgr = bo->bufmgr;

    RETURN_VAL_CHECK_FLAG (CTRL_BACKEND_VALID(bufmgr->backend->flags), 1);

    if (bo->lock_cnt < 0)
    {
        fprintf (stderr, "[libtbm:%d] "
                "error %s:%d bo:%p(%d) LOCK_CNT=%d\n",
                getpid(), __FUNCTION__, __LINE__, bo, bo->tgl_key, bo->lock_cnt);
    }

    old = bo->lock_cnt;
    switch (bufmgr->lock_type)
    {
        case LOCK_TRY_ALWAYS:    /* LOCK_TRY_ALWAYS */
            pthread_mutex_unlock (&bufmgr->lock);
            ret = _bo_lock (bo);
            pthread_mutex_lock (&bufmgr->lock);
            if(ret)
                bo->lock_cnt++;
            break;
        case LOCK_TRY_NEVER:    /* LOCK_TRY_NEVER */
            return 1;
            break;
        default:
            if (bo->lock_cnt == 0)
            {
                pthread_mutex_unlock (&bufmgr->lock);
                ret = _bo_lock (bo);
                pthread_mutex_lock (&bufmgr->lock);
                if (ret)
                    bo->lock_cnt++;
            }

            break;
    }

    DBG_LOCK ("[libtbm:%d] >> LOCK bo:%p(%d, %d->%d)\n", getpid(),
            bo, bo->tgl_key, old, bo->lock_cnt);

    return 1;
}

static void
_tbm_bo_unlock (tbm_bo bo)
{
    tbm_bufmgr bufmgr = NULL;

    int old;

    if (!bo)
        return;

    bufmgr = bo->bufmgr;

    RETURN_CHECK_FLAG (CTRL_BACKEND_VALID(bufmgr->backend->flags));

    old = bo->lock_cnt;
    if (bo->lock_cnt > 0)
    {
        bo->lock_cnt--;
        if (bo->lock_cnt == 0)
           _bo_unlock (bo);
    }
    else if (bo->lock_cnt < 0)
    {
        bo->lock_cnt = 0;
    }

    DBG_LOCK ("[libtbm:%d] << unlock bo:%p(%d, %d->%d)\n", getpid(),
             bo, bo->tgl_key, old, bo->lock_cnt);
}


static int
_tbm_bo_set_state (tbm_bo bo, int device, int opt)
{
    tbm_bufmgr bufmgr = bo->bufmgr;
    char need_flush = 0;
    unsigned short cntFlush = 0;
    unsigned int is_locked;

    RETURN_VAL_CHECK_FLAG (CTRL_BACKEND_VALID(bufmgr->backend->flags), 1);

    /* get cache state of a bo */
    bo->cache_state.val = _tgl_get_data (bufmgr->lock_fd, bo->tgl_key, &is_locked);

    if (!bo->cache_state.data.isCacheable)
        return 1;

    /* get global cache flush count */
    cntFlush = (unsigned short)_tgl_get_data (bufmgr->lock_fd, GLOBAL_KEY, NULL);

    if (DEVICE_IS_CACHE_AWARE (device))
    {
        if (bo->cache_state.data.isDirtied == DEVICE_CO &&
            bo->cache_state.data.isCached)
        {
            need_flush = TBM_CACHE_INV;
        }

        bo->cache_state.data.isCached = 1;
        if (opt & TBM_OPTION_WRITE)
            bo->cache_state.data.isDirtied = DEVICE_CA;
        else
            bo->cache_state.data.isDirtied = DEVICE_NONE;
    }
    else
    {
        if (bo->cache_state.data.isDirtied == DEVICE_CA &&
            bo->cache_state.data.isCached &&
            bo->cache_state.data.cntFlush == cntFlush)
        {
            need_flush = TBM_CACHE_CLN | TBM_CACHE_ALL;
        }

        if (opt & TBM_OPTION_WRITE)
            bo->cache_state.data.isDirtied = DEVICE_CO;
        else
            bo->cache_state.data.isDirtied = DEVICE_NONE;
    }

    if (need_flush)
    {
        /* call backend cache flush */
        bufmgr->backend->bo_cache_flush (bo, need_flush);

        /* set global cache flush count */
        if (need_flush & TBM_CACHE_ALL)
            _tgl_set_data (bufmgr->lock_fd, GLOBAL_KEY, (unsigned int)(++cntFlush));

        DBG ("[libtbm:%d] \tcache(%d,%d,%d)....flush:0x%x, cntFlush(%d)\n", getpid(),
             bo->cache_state.data.isCacheable,
             bo->cache_state.data.isCached,
             bo->cache_state.data.isDirtied,
             need_flush, cntFlush);
    }

    return 1;
}

static void
_tbm_bo_save_state (tbm_bo bo)
{
    tbm_bufmgr bufmgr = bo->bufmgr;
    unsigned short cntFlush = 0;

    RETURN_CHECK_FLAG (CTRL_BACKEND_VALID(bufmgr->backend->flags));

    /* get global cache flush count */
    cntFlush = (unsigned short)_tgl_get_data (bufmgr->lock_fd, GLOBAL_KEY, NULL);

    /* save global cache flush count */
    bo->cache_state.data.cntFlush = cntFlush;
    _tgl_set_data(bufmgr->lock_fd, bo->tgl_key, bo->cache_state.val);
}

static int
_tbm_bo_init_state (tbm_bo bo, int opt)
{
    tbm_bufmgr bufmgr = bo->bufmgr;
    tbm_bo_cache_state cache_state;

    RETURN_VAL_CHECK_FLAG (CTRL_BACKEND_VALID(bufmgr->backend->flags), 1);

    cache_state.val = 0;
    switch (opt)
    {
    case CACHE_OP_CREATE:    /*Create*/
        if (bo->tgl_key == INITIAL_KEY)
           bo->tgl_key = bufmgr->backend->bo_get_global_key (bo);

        _tgl_init (bufmgr->lock_fd, bo->tgl_key);

        cache_state.data.isCacheable = BO_IS_CACHEABLE(bo);
        cache_state.data.isDirtied = DEVICE_NONE;
        cache_state.data.isCached = 0;
        cache_state.data.cntFlush = 0;

        _tgl_set_data (bufmgr->lock_fd, bo->tgl_key, cache_state.val);
        break;
    case CACHE_OP_IMPORT:    /*Import*/
        if (bo->tgl_key == INITIAL_KEY)
           bo->tgl_key = bufmgr->backend->bo_get_global_key (bo);

        _tgl_init (bufmgr->lock_fd, bo->tgl_key);
        break;
    default:
        break;
    }

    return 1;
}

static void
_tbm_bo_destroy_state (tbm_bo bo)
{
    tbm_bufmgr bufmgr = bo->bufmgr;

    RETURN_CHECK_FLAG (CTRL_BACKEND_VALID(bufmgr->backend->flags));

    _tgl_destroy (bufmgr->lock_fd, bo->tgl_key);
}

static void
_tbm_bo_ref (tbm_bo bo)
{
    bo->ref_cnt++;
}

static void
_tbm_bo_unref (tbm_bo bo)
{
    tbm_bufmgr bufmgr = bo->bufmgr;
    tbm_user_data *old_data = NULL, *tmp = NULL;

    if (bo->ref_cnt <= 0)
        return;

    bo->ref_cnt--;
    if (bo->ref_cnt == 0)
    {
        /* destory the user_data_list */
        if (!LIST_IS_EMPTY (&bo->user_data_list))
        {
            LIST_FOR_EACH_ENTRY_SAFE (old_data, tmp, &bo->user_data_list, item_link)
            {
                DBG ("[libtbm:%d] free user_data \n", getpid());
                _user_data_delete (old_data);
            }
        }

        if (bo->lock_cnt > 0)
        {
            fprintf (stderr, "[libtbm:%d] "
                    "error %s:%d lock_cnt:%d\n",
                    getpid(), __FUNCTION__, __LINE__, bo->lock_cnt);
            _bo_unlock (bo);
        }

        /* Destroy Global Lock */
        _tbm_bo_destroy_state (bo);

        /* call the bo_free */
        bufmgr->backend->bo_free (bo);
        bo->priv = NULL;

        LIST_DEL (&bo->item_link);
        free(bo);
        bo = NULL;
    }

}

static int
_tbm_bufmgr_init_state (tbm_bufmgr bufmgr)
{
    RETURN_VAL_CHECK_FLAG (CTRL_BACKEND_VALID(bufmgr->backend->flags), 1);

    bufmgr->lock_fd = open(tgl_devfile, O_RDWR);

    if(bufmgr->lock_fd < 0)
    {
        fprintf (stderr, "[libtbm:%d] "
                "error: Fail to open global_lock:%s\n",
                getpid(), tgl_devfile);
        return 0;
    }

   if (!_tgl_init(bufmgr->lock_fd, GLOBAL_KEY))
   {
        fprintf (stderr, "[libtbm:%d] "
                "error: Fail to initialize the tgl\n",
                getpid());
        return 0;
   }

   return 1;
}

static void
_tbm_bufmgr_destroy_state (tbm_bufmgr bufmgr)
{
    RETURN_CHECK_FLAG (CTRL_BACKEND_VALID(bufmgr->backend->flags));

    close (bufmgr->lock_fd);
}

static int
_check_version (TBMModuleVersionInfo *data)
{
    int abimaj, abimin;
    int vermaj, vermin;

    abimaj = GET_ABI_MAJOR (data->abiversion);
    abimin = GET_ABI_MINOR (data->abiversion);

    fprintf (stderr, "[libtbm:%d] "
            "TBM module %s: vendor=\"%s\" ABI=%d,%d\n",
            getpid(), data->modname ? data->modname : "UNKNOWN!",
            data->vendor ? data->vendor : "UNKNOWN!", abimaj, abimin);

    vermaj = GET_ABI_MAJOR (TBM_ABI_VERSION);
    vermin = GET_ABI_MINOR (TBM_ABI_VERSION);

    DBG ("[libtbm:%d] " "TBM ABI version %d.%d\n", getpid(), vermaj, vermin);

    if (abimaj != vermaj)
    {
        fprintf (stderr, "[libtbm:%d] "
                "TBM module ABI major ver(%d) doesn't match the TBM's ver(%d)\n",
                getpid(), abimaj, vermaj);
        return 0;
    }
    else if (abimin > vermin)
    {
        fprintf (stderr, "[libtbm:%d] "
                "TBM module ABI minor ver(%d) is newer than the TBM's ver(%d)\n",
                getpid(), abimin, vermin);
        return 0;
    }
    return 1;
}

static int
_tbm_bufmgr_load_module (tbm_bufmgr bufmgr, int fd, const char *file)
{
    char path[PATH_MAX] = {0,};
    TBMModuleData *initdata = NULL;
    void * module_data;

    snprintf(path, sizeof(path), BUFMGR_MODULE_DIR "/%s", file);

    module_data = dlopen (path, RTLD_LAZY);
    if (!module_data)
    {
        fprintf (stderr, "[libtbm:%d] "
                "failed to load module: %s(%s)\n",
                getpid(), dlerror(), file);
        return 0;
    }

    initdata = dlsym (module_data, "tbmModuleData");
    if (initdata)
    {
        ModuleInitProc init;
        TBMModuleVersionInfo *vers;

        vers = initdata->vers;
        init = initdata->init;

        if (vers)
        {
            if (!_check_version (vers))
            {
                dlclose (module_data);
                return 0;
            }
        }
        else
        {
            fprintf (stderr, "[libtbm:%d] "
                    "Error: module does not supply version information.\n",
                    getpid());

            dlclose (module_data);
            return 0;
        }

        if (init)
        {
            if(!init (bufmgr, fd))
            {
                fprintf (stderr, "[libtbm:%d] "
                        "Fail to init module(%s)\n",
                        getpid(), file);
                dlclose (module_data);
                return 0;
            }
        }
        else
        {
            fprintf (stderr, "[libtbm:%d] "
                    "Error: module does not supply init symbol.\n", getpid());
            dlclose (module_data);
            return 0;
        }
    }
    else
    {
        fprintf (stderr, "[libtbm:%d] "
                "Error: module does not have data object.\n", getpid());
        dlclose (module_data);
        return 0;
    }

    bufmgr->module_data = module_data;

    fprintf (stderr,"[libtbm:%d] "
            "Success to load module(%s)\n", getpid(), file);

    return 1;
}

static int _tbm_load_module (tbm_bufmgr bufmgr, int fd)
{
    struct dirent **namelist;
    const char *p = NULL;
    int n;
    int ret = 0;

    /* load bufmgr priv from default lib */
    ret = _tbm_bufmgr_load_module (bufmgr, fd, DEFAULT_LIB);

    /* load bufmgr priv from configured path */
    if (!ret)
    {
        n = scandir (BUFMGR_MODULE_DIR, &namelist, 0, alphasort);
        if (n < 0)
            fprintf (stderr,"[libtbm:%d] "
                    "no files : %s\n", getpid(), BUFMGR_MODULE_DIR);
        else
        {
            while(n--)
            {
                if (!ret && strstr (namelist[n]->d_name, PREFIX_LIB))
                {
                    p = strstr (namelist[n]->d_name, SUFFIX_LIB);
                    if (!strcmp (p, SUFFIX_LIB))
                    {
                        ret = _tbm_bufmgr_load_module (bufmgr, fd, namelist[n]->d_name);
                    }
                }
                free(namelist[n]);
            }
            free(namelist);
        }
    }

    return ret;
}

tbm_bufmgr
tbm_bufmgr_init (int fd)
{
    char *env;
    tbm_bufmgr bufmgr;

#ifdef DEBUG
    env = getenv("GEM_DEBUG");
    if(env)
    {
        bDebug = atoi(env);
        fprintf(stderr, "GEM_DEBUG=%s\n", env);
    }
    else
        bDebug = 0;
#endif

    /* initialize buffer manager */
    if (fd < 0)
        return NULL;

    if(gBufMgrs == NULL)
    {
        gBufMgrs = malloc(sizeof(struct list_head));
        LIST_INITHEAD(gBufMgrs);
    }
    else
    {
        LIST_FOR_EACH_ENTRY(bufmgr, gBufMgrs, link)
        {
            if(bufmgr->fd == fd)
            {
                bufmgr->ref_count++;
                fprintf (stderr, "[libtbm:%d] bufmgr ref: fd=%d, ref_count:%d\n",
                        getpid(), fd, bufmgr->ref_count);
                return bufmgr;
            }
        }
        bufmgr = NULL;
    }
    fprintf(stderr, "[libtbm:%d] bufmgr init: fd=%d\n", getpid(), fd);

    /* allocate bufmgr */
    bufmgr = calloc (1, sizeof(struct _tbm_bufmgr));
    if (!bufmgr)
        return NULL;

    /* load bufmgr priv from env */
    if (!_tbm_load_module(bufmgr, fd))
    {
        fprintf (stderr,"[libtbm:%d] "
                "error : Fail to load bufmgr backend\n",
                getpid());
        free (bufmgr);
        bufmgr = NULL;
        return NULL;
    }

    bufmgr->ref_count = 1;
    bufmgr->fd = fd;

    fprintf(stderr, "[libtbm:%d] create tizen bufmgr: ref_count:%d\n",
         getpid(), bufmgr->ref_count);

    if (pthread_mutex_init (&bufmgr->lock, NULL) != 0)
    {
        bufmgr->backend->bufmgr_deinit (bufmgr);
        free (bufmgr);
        bufmgr = NULL;
        return NULL;
    }

    /* intialize the tizen global status */
    if (!_tbm_bufmgr_init_state (bufmgr))
    {
        fprintf (stderr, "[libtbm:%d] "
                "error: Fail to init state\n",
                getpid());
        bufmgr->backend->bufmgr_deinit (bufmgr);
        free (bufmgr);
        bufmgr = NULL;
        return NULL;
    }

    /* setup the lock_type */
    env = getenv ("BUFMGR_LOCK_TYPE");
    if (env && !strcmp (env, "always"))
        bufmgr->lock_type = LOCK_TRY_ALWAYS;
    else if(env && !strcmp(env, "none"))
        bufmgr->lock_type = LOCK_TRY_NEVER;
    else
        bufmgr->lock_type = LOCK_TRY_ONCE;
    DBG ("[libtbm:%d] BUFMGR_LOCK_TYPE=%s\n", getpid(), env?env:"default:once");

    /* setup the map_cache */
    env = getenv ("BUFMGR_MAP_CACHE");
    if (env && !strcmp (env, "false"))
        bufmgr->use_map_cache = 0;
    else
        bufmgr->use_map_cache = 1;
    DBG ("[libtbm:%d] BUFMGR_MAP_CACHE=%s\n", getpid(), env?env:"default:true");

    /* intialize bo_list */
    LIST_INITHEAD (&bufmgr->bo_list);

    /* add bufmgr to the gBufMgrs */
    LIST_ADD(&bufmgr->link, gBufMgrs);

    return bufmgr;
}

void
tbm_bufmgr_deinit (tbm_bufmgr bufmgr)
{
    TBM_RETURN_IF_FAIL (TBM_BUFMGR_IS_VALID(bufmgr));

    tbm_bo bo = NULL;
    tbm_bo tmp = NULL;

    bufmgr->ref_count--;
    if (bufmgr->ref_count > 0)
    {
        fprintf (stderr, "[libtbm:%d] "
                "tizen bufmgr destroy: bufmgr:%p, ref_cnt:%d\n",
                getpid(), bufmgr, bufmgr->ref_count);
        return;
    }

    /* destroy bo_list */
    if(!LIST_IS_EMPTY (&bufmgr->bo_list))
    {
        LIST_FOR_EACH_ENTRY_SAFE (bo, tmp, &bufmgr->bo_list, item_link)
        {
            fprintf (stderr, "[libtbm:%d] "
                    "Un-freed bo(%p, ref:%d) \n",
                    getpid(), bo, bo->ref_cnt);
            bo->ref_cnt = 1;
            tbm_bo_unref(bo);
        }
    }

    /* destroy the tizen global status */
    _tbm_bufmgr_destroy_state (bufmgr);

    /* destroy bufmgr priv */
    bufmgr->backend->bufmgr_deinit (bufmgr->backend->priv);
    bufmgr->backend->priv = NULL;
    tbm_backend_free (bufmgr->backend);
    bufmgr->backend = NULL;

    pthread_mutex_destroy (&bufmgr->lock);

    fprintf (stderr, "[libtbm:%d] "
            "tizen bufmgr destroy: bufmgr:%p, ref_cnt:%d\n",
            getpid(), bufmgr, bufmgr->ref_count);

    dlclose (bufmgr->module_data);

    LIST_DEL (&bufmgr->link);

    free (bufmgr);
    bufmgr = NULL;
}


int
tbm_bo_size (tbm_bo bo)
{
    TBM_RETURN_VAL_IF_FAIL (TBM_BO_IS_VALID(bo), 0);

    tbm_bufmgr bufmgr = bo->bufmgr;
    int size;

    pthread_mutex_lock(&bufmgr->lock);
    size = bufmgr->backend->bo_size(bo);
    pthread_mutex_unlock(&bufmgr->lock);

    return size;
}

tbm_bo
tbm_bo_ref (tbm_bo bo)
{
    TBM_RETURN_VAL_IF_FAIL(TBM_BO_IS_VALID(bo), NULL);

    tbm_bufmgr bufmgr = bo->bufmgr;

    pthread_mutex_lock(&bufmgr->lock);

    _tbm_bo_ref (bo);

    pthread_mutex_unlock(&bufmgr->lock);

    return bo;
}

void
tbm_bo_unref (tbm_bo bo)
{
    TBM_RETURN_IF_FAIL(TBM_BO_IS_VALID(bo));

    tbm_bufmgr bufmgr = bo->bufmgr;

    pthread_mutex_lock (&bufmgr->lock);

    _tbm_bo_unref (bo);

    pthread_mutex_unlock(&bufmgr->lock);
}

tbm_bo
tbm_bo_alloc (tbm_bufmgr bufmgr, int size, int flags)
{
    TBM_RETURN_VAL_IF_FAIL (TBM_BUFMGR_IS_VALID(bufmgr) && (size > 0), NULL);

    tbm_bo bo = NULL;
    void * bo_priv = NULL;

    bo = calloc (1, sizeof(struct _tbm_bo));
    if(!bo)
        return NULL;

    bo->bufmgr = bufmgr;

    pthread_mutex_lock (&bufmgr->lock);

    bo_priv = bufmgr->backend->bo_alloc (bo, size, flags);
    if (!bo_priv)
    {
        free (bo);
        pthread_mutex_unlock (&bufmgr->lock);
        return NULL;
    }

    bo->ref_cnt = 1;
    bo->flags = flags;
    bo->tgl_key = INITIAL_KEY;
    bo->priv = bo_priv;

    /* init bo state */
    if (!_tbm_bo_init_state (bo, CACHE_OP_CREATE))
    {
        _tbm_bo_unref (bo);
        pthread_mutex_unlock (&bufmgr->lock);
        return NULL;
    }

    LIST_INITHEAD (&bo->user_data_list);

    LIST_ADD (&bo->item_link, &bufmgr->bo_list);

    pthread_mutex_unlock(&bufmgr->lock);

    return bo;
}

tbm_bo
tbm_bo_import (tbm_bufmgr bufmgr, unsigned int key)
{
    TBM_RETURN_VAL_IF_FAIL(TBM_BUFMGR_IS_VALID(bufmgr), NULL);

    tbm_bo bo = NULL;
    void * bo_priv = NULL;

    bo = calloc (1, sizeof(struct _tbm_bo));
    if(!bo)
        return NULL;

    bo->bufmgr = bufmgr;

    pthread_mutex_lock (&bufmgr->lock);

    bo_priv = bufmgr->backend->bo_import (bo, key);
    if (!bo_priv)
    {
        free (bo);
        pthread_mutex_unlock (&bufmgr->lock);
        return NULL;
    }

    bo->ref_cnt = 1;
    bo->tgl_key = INITIAL_KEY;
    bo->priv = bo_priv;

    /* init bo state */
    if (!_tbm_bo_init_state (bo, CACHE_OP_IMPORT))
    {
        _tbm_bo_unref (bo);
        pthread_mutex_unlock (&bufmgr->lock);
        return NULL;
    }

    LIST_INITHEAD (&bo->user_data_list);

    LIST_ADD (&bo->item_link, &bufmgr->bo_list);

    pthread_mutex_unlock (&bufmgr->lock);

    return bo;
}

unsigned int
tbm_bo_export (tbm_bo bo)
{
    TBM_RETURN_VAL_IF_FAIL (TBM_BO_IS_VALID(bo), 0);

    tbm_bufmgr bufmgr;
    int ret;

    bufmgr = bo->bufmgr;

    pthread_mutex_lock (&bufmgr->lock);
    ret = bufmgr->backend->bo_export (bo);
    pthread_mutex_unlock (&bufmgr->lock);

    return ret;
}

tbm_bo_handle
tbm_bo_get_handle (tbm_bo bo, int device)
{
    TBM_RETURN_VAL_IF_FAIL (TBM_BO_IS_VALID(bo), (tbm_bo_handle)0);

    tbm_bufmgr bufmgr;
    tbm_bo_handle bo_handle;

    bufmgr = bo->bufmgr;

    pthread_mutex_lock (&bufmgr->lock);
    bo_handle = bufmgr->backend->bo_get_handle (bo, device);
    pthread_mutex_unlock (&bufmgr->lock);

    return bo_handle;
}

tbm_bo_handle
tbm_bo_map (tbm_bo bo, int device, int opt)
{
    TBM_RETURN_VAL_IF_FAIL (TBM_BO_IS_VALID(bo), (tbm_bo_handle)0);

    tbm_bufmgr bufmgr;
    tbm_bo_handle bo_handle;

    bufmgr = bo->bufmgr;

    pthread_mutex_lock (&bufmgr->lock);

    _tbm_bo_lock (bo);
    bo_handle = bufmgr->backend->bo_map (bo, device, opt);

    /* increase the ref_count */
//    _tbm_bo_ref (bo);

    if (bufmgr->use_map_cache == 1 && bo->map_cnt == 0)
        _tbm_bo_set_state (bo, device, opt);

    /* increase the map_count */
    bo->map_cnt++;

    pthread_mutex_unlock (&bufmgr->lock);

    return bo_handle;
}

int
tbm_bo_unmap (tbm_bo bo)
{
    TBM_RETURN_VAL_IF_FAIL (TBM_BO_IS_VALID(bo), 0);

    tbm_bufmgr bufmgr;
    int ret;

    bufmgr = bo->bufmgr;

    pthread_mutex_lock (&bufmgr->lock);

    ret = bufmgr->backend->bo_unmap (bo);

    /* decrease the map_count */
    bo->map_cnt--;

    if (bo->map_cnt == 0)
        _tbm_bo_save_state (bo);

     _tbm_bo_unlock (bo);

    /* decrease the ref_count */
//    _tbm_bo_unref (bo);

    pthread_mutex_unlock (&bufmgr->lock);

    return ret;
}


int        tbm_bo_get_flag        (tbm_bo bo){
    return bo->flags;
}


int        tbm_bo_get_cache_stat        (tbm_bo bo){
    return bo->cache_state.val;
}

int
tbm_bo_swap (tbm_bo bo1, tbm_bo bo2)
{
    TBM_RETURN_VAL_IF_FAIL (TBM_BO_IS_VALID(bo1), 0);
    TBM_RETURN_VAL_IF_FAIL (TBM_BO_IS_VALID(bo2), 0);

    void* temp;
    unsigned int tmp_key;

    if (bo1->bufmgr->backend->bo_size (bo1) != bo2->bufmgr->backend->bo_size (bo2))
        return 0;

    pthread_mutex_lock (&bo1->bufmgr->lock);

    tmp_key = bo1->tgl_key;
    bo1->tgl_key = bo2->tgl_key;
    bo2->tgl_key = tmp_key;

    temp = bo1->priv;
    bo1->priv = bo2->priv;
    bo2->priv = temp;

    pthread_mutex_unlock (&bo1->bufmgr->lock);

    return 1;
}

int
tbm_bo_locked (tbm_bo bo)
{
    TBM_RETURN_VAL_IF_FAIL (TBM_BO_IS_VALID(bo), 0);

    tbm_bufmgr bufmgr;

    bufmgr = bo->bufmgr;

    if (bufmgr->lock_type == LOCK_TRY_NEVER)
        return 0;

    pthread_mutex_lock (&bufmgr->lock);

    if (bo->lock_cnt > 0)
    {
        pthread_mutex_unlock (&bufmgr->lock);
        return 1;
    }

    pthread_mutex_unlock (&bufmgr->lock);

    return 0;
}


int
tbm_bo_add_user_data (tbm_bo bo, unsigned long key, tbm_data_free data_free_func)
{
    TBM_RETURN_VAL_IF_FAIL(TBM_BO_IS_VALID(bo), 0);

    tbm_user_data *data;

    /* check if the data according to the key exist if so, return false.*/
    data = _user_data_lookup (&bo->user_data_list, key);
    if (data)
    {
        fprintf (stderr, "[libtbm:%d] "
                "waring: %s:%d user data already exist. key:%ld\n",
                getpid(), __FUNCTION__, __LINE__, key);
        return 0;
    }

    data = _user_data_create (key, data_free_func);
    if (!data)
        return 0;

    LIST_ADD (&data->item_link, &bo->user_data_list);

    return 1;
}

int
tbm_bo_set_user_data (tbm_bo bo, unsigned long key, void* data)
{
    TBM_RETURN_VAL_IF_FAIL(TBM_BO_IS_VALID(bo), 0);

    tbm_user_data *old_data;

    if (LIST_IS_EMPTY (&bo->user_data_list))
        return 0;

    old_data = _user_data_lookup (&bo->user_data_list, key);
    if (!old_data)
        return 0;

    if (old_data->data && old_data->free_func)
        old_data->free_func(old_data->data);

    old_data->data = data;

    return 1;
}

int
tbm_bo_get_user_data (tbm_bo bo, unsigned long key, void** data)
{
    TBM_RETURN_VAL_IF_FAIL(TBM_BO_IS_VALID(bo), 0);

    tbm_user_data* old_data;

    if (!data || LIST_IS_EMPTY (&bo->user_data_list))
        return 0;

    old_data = _user_data_lookup (&bo->user_data_list, key);
    if (!old_data)
    {
        *data = NULL;
        return 0;
    }

    *data = old_data->data;

    return 1;
}

int
tbm_bo_delete_user_data (tbm_bo bo, unsigned long key)
{
    TBM_RETURN_VAL_IF_FAIL(TBM_BO_IS_VALID(bo), 0);

    tbm_user_data *old_data = (void *)0;

    if (LIST_IS_EMPTY (&bo->user_data_list))
        return 0;

    old_data = _user_data_lookup (&bo->user_data_list, key);
    if (!old_data)
        return 0;

    _user_data_delete (old_data);

    return 1;
}


