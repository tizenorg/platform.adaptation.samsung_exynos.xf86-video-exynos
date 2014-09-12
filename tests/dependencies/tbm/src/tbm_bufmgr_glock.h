/*
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __TBM_BUFMGR_GLOCK_H__
#define __TBM_BUFMGR_GLOCK_H__

#include <linux/ioctl.h>

static char tgl_devfile[] = "/dev/slp_global_lock";

#define TGL_IOC_BASE				0x32

struct tgl_attribute {
	unsigned int key;
	unsigned int timeout_ms;
};

struct tgl_user_data {
	unsigned int key;
	unsigned int data1;
	unsigned int data2;
	unsigned int locked;
};

typedef enum {
	_TGL_INIT_LOCK = 1,
	_TGL_DESTROY_LOCK,
	_TGL_LOCK_LOCK,
	_TGL_UNLOCK_LOCK,
	_TGL_SET_DATA,
	_TGL_GET_DATA,
} _tgl_ioctls;

#define TGL_IOC_INIT_LOCK			_IOW(TGL_IOC_BASE, _TGL_INIT_LOCK, struct tgl_attribute *)
#define TGL_IOC_DESTROY_LOCK		_IOW(TGL_IOC_BASE, _TGL_DESTROY_LOCK, unsigned int)
#define TGL_IOC_LOCK_LOCK			_IOW(TGL_IOC_BASE, _TGL_LOCK_LOCK, unsigned int)
#define TGL_IOC_UNLOCK_LOCK			_IOW(TGL_IOC_BASE, _TGL_UNLOCK_LOCK, unsigned int)
#define TGL_IOC_SET_DATA			_IOW(TGL_IOC_BASE, _TGL_SET_DATA, struct tgl_user_data *)
#define TGL_IOC_GET_DATA			_IOW(TGL_IOC_BASE, _TGL_GET_DATA, struct tgl_user_data *)

#endif /* __TBM_BUFMGR_GLOCK_H__ */
