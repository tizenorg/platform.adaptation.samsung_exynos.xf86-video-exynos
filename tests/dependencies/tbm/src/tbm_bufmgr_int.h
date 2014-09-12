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

#ifndef _TBM_BUFMGR_INT_H_
#define _TBM_BUFMGR_INT_H_

#include <tbm_bufmgr.h>
#include <pthread.h>
#include "tbm_bufmgr_backend.h"

typedef union _tbm_bo_cache_state tbm_bo_cache_state;

struct list_head
{
    struct list_head *prev;
    struct list_head *next;
};

union _tbm_bo_cache_state
{
    unsigned int val;
    struct {
        unsigned int cntFlush:16;    /*Flush all index for sync*/
        unsigned int isCacheable:1;
        unsigned int isCached:1;
        unsigned int isDirtied:2;
    } data;
};

/**
 * @brief tbm buffer object
 *  buffer object of Tizen Buffer Manager
 */
struct _tbm_bo
{
    tbm_bufmgr bufmgr; /**< tbm buffer manager */

    int ref_cnt;       /**< ref count of bo */

    int flags;         /**< TBM_BO_FLAGS :bo memory type */

    unsigned int tgl_key; /**< global key for tizen global lock */

    /* for cache control */
    unsigned int map_cnt; /**< device map count */
    tbm_bo_cache_state cache_state; /**< cache state */

    int lock_cnt; /**< lock count of bo */

    struct list_head user_data_list; /**< list of the user_date in bo */

    void *priv; /**< bo private */

    struct list_head item_link; /**< link of bo */
};

/**
 * @brief tbm_bufmgr : structure for tizen buffer manager
 *
 */
struct _tbm_bufmgr
{
    int ref_count; /**< ref count of bufmgr */

    pthread_mutex_t lock; /**< mutex lock */

    int fd;  /**< bufmgr fd */

    int lock_fd; /**< fd of tizen global lock */

    int lock_type; /**< lock_type of bufmgr */

    int use_map_cache; /**< flag to use the map_cahce */

    struct list_head bo_list; /**< list of bos belonging to bufmgr */

    void *module_data;

    tbm_bufmgr_backend backend; /**< bufmgr backend */

    struct list_head link; /**< link of bufmgr */
};

#endif  /* _TBM_BUFMGR_INT_H_ */
