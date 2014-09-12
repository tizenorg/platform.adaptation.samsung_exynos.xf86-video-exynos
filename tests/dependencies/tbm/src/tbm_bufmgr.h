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

#ifndef _TBM_BUFMGR_H_
#define _TBM_BUFMGR_H_

#include <stdint.h>

/**
 * \file tbm_bufmgr.h
 * \brief Tizen Buffer Manager
 */

typedef struct _tbm_bufmgr *tbm_bufmgr;
typedef struct _tbm_bo *tbm_bo;

/* TBM_DEVICE_TYPE */
#define TBM_DEVICE_DEFAULT   0  /**< device type to get the default handle    */
#define TBM_DEVICE_CPU       1  /**< device type to get the virtual pointer   */
#define TBM_DEVICE_2D        2  /**< device type to get the 2D memory handle  */
#define TBM_DEVICE_3D        3  /**< device type to get the 3D memory handle  */
#define TBM_DEVICE_MM        4  /**< device type to get the multimedia handle */

/* TBM_OPTION */
#define TBM_OPTION_READ      (1 << 0) /**< access option to read  */
#define TBM_OPTION_WRITE     (1 << 1) /**< access option to write */

/**
 * @brief tbm_bo_handle
 *  abstraction of the memory handle by TBM_DEVICE_TYPE
 */
typedef union _tbm_bo_handle
{
   void     *ptr;
   int32_t  s32;
   uint32_t u32;
   int64_t  s64;
   uint64_t u64;
} tbm_bo_handle;

/**
 * @brief Enumeration of bo memory type
 */
enum TBM_BO_FLAGS
{
    TBM_BO_DEFAULT = 0,          /**< physically non-contiguous memory */
    TBM_BO_SCANOUT = (1<<0),     /**< physically contiguous memory     */
    TBM_BO_NONCACHABLE = (1<<1), /**< non-cachable memory              */
    TBM_BO_WC = (1<<2),          /**< write-combine memory             */
};

/* Functions for buffer manager */

/**
 * @brief initialize the buffer manager.
 * @param[in] fd : file descripter of the system buffer manager
 * @return a buffer manager
 */
tbm_bufmgr tbm_bufmgr_init   (int fd);

/**
 * @brief deinitialize the buffer manager.
 * @param[in] bufmgr : the buffer manager
 */
void       tbm_bufmgr_deinit (tbm_bufmgr bufmgr);

/* Functions for bo */

/**
 * @brief allocate the buffer object
 * @param[in] bufmgr : the buffer manager
 * @param[in] size : the size of buffer object
 * @param[in] flags : the flags of memory type
 * @return a buffer object
 */
tbm_bo        tbm_bo_alloc      (tbm_bufmgr bufmgr, int size, int flags);

/**
 * @brief increase the reference count of bo.
 * @param[in] bo : the buffer object
 * @return a buffer object
 */
tbm_bo        tbm_bo_ref        (tbm_bo bo);


/**
 * @brief debug purposes
 */
int        tbm_bo_get_flag        (tbm_bo bo);

/**
 * @brief debug purposes
 */
int        tbm_bo_get_cache_stat        (tbm_bo bo);

/**
 * @brief increase the reference count of bo.
 * @param[in] bo : the buffer object
 */
void          tbm_bo_unref      (tbm_bo bo);

/**
 * @brief map the buffer object according to the device type and the option.
 * @param[in] bo : the buffer object
 * @param[in] device : the device type to get a handle
 * @param[in] option : the option to access the buffer object
 * @return the handle of the buffer object
 */
tbm_bo_handle tbm_bo_map        (tbm_bo bo, int device, int opt);

/**
 * @brief unmap the buffer object.
 * @param[in] bo : the buffer object
 * @return 1 if this function succeeds, otherwise 0.
 */
int           tbm_bo_unmap      (tbm_bo bo);

/**
 * @brief get the tbm_bo_handle according to the device type.
 * @param[in] bo : the buffer object
 * @param[in] device : the device type to get a handle
 * @return the handle of the buffer object
 */
tbm_bo_handle tbm_bo_get_handle (tbm_bo bo, int device);

/**
 * @brief export the buffer object
 * @param[in] bo : the buffer object
 * @return key associated with the buffer object
 */
unsigned int  tbm_bo_export     (tbm_bo bo);

/**
 * @brief import the buffer object associated with the key.
 * @param[in] bufmgr : the buffer manager
 * @param[in] key : the key associated with the buffer object
 * @return a buffer object
 */
tbm_bo        tbm_bo_import     (tbm_bufmgr bufmgr, unsigned int key);

/**
 * @brief get the size of a bo.
 * @param[in] bo : the buffer object
 * @return 1 if this function succeeds, otherwise 0.
 */
int           tbm_bo_size       (tbm_bo bo);

/**
 * @brief get the state where the buffer object is locked.
 * @param[in] bo : the buffer object
 * @return 1 if this function succeeds, otherwise 0.
 */
int           tbm_bo_locked     (tbm_bo bo);

/**
 * @brief swap the buffer object.
 * @param[in] bo1 : the buffer object
 * @param[in] bo2 : the buffer object
 * @return 1 if this function succeeds, otherwise 0.
 */
int           tbm_bo_swap       (tbm_bo bo1, tbm_bo bo2);


/* Functions for userdata of bo */
typedef void (*tbm_data_free)(void *);

/**
 * @brief add a user_data to the buffer object
 * @param[in] bo : the buffer object
 * @param[in] key : the key associated with the user_date
 * @param[in] data_free_func : the function pointer to free the user_data
 * @return 1 if this function succeeds, otherwise 0.
 */
int tbm_bo_add_user_data    (tbm_bo bo, unsigned long key, tbm_data_free data_free_func);

/**
 * @brief delete the user_data in the buffer object.
 * @param[in] bo : the buffer object
 * @param[in] key : the key associated with the user_date
 * @return 1 if this function succeeds, otherwise 0.
 */
int tbm_bo_delete_user_data (tbm_bo bo, unsigned long key);

/**
 * @brief set a user_date to the buffer object.
 * @param[in] bo : the buffer object
 * @param[in] key : the key associated with the user_date
 * @param[in] data : a pointer of the user_data
 * @return 1 if this function succeeds, otherwise 0.
 */
int tbm_bo_set_user_data    (tbm_bo bo, unsigned long key, void* data);

/**
 * @brief get a user_date from the buffer object with the key.
 * @param[in] bo : the buffer object
 * @param[in] key : the key associated with the user_date
 * @param[out] data : to get the user data
 * @return 1 if this function succeeds, otherwise 0.
 */
int tbm_bo_get_user_data    (tbm_bo bo, unsigned long key, void** data);

#endif /* _TBM_BUFMGR_H_ */

