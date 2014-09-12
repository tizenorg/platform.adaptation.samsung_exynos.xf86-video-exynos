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

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>

#include <dix.h>
#define XREGISTRY
#include <registry.h>
#include <xace.h>
#include <xacestr.h>
#include <X11/Xatom.h>
#include <X11/extensions/damageproto.h>
#include <X11/extensions/damagewire.h>
#include <X11/extensions/XI2proto.h>
#include <windowstr.h>

#include <xdbg.h>

#include "xdbg_module_types.h"
#include "xdbg_module_evlog_request.h"

#define UNKNOWN_EVENT "<unknown>"

static ExtensionEntry *damage;

static Bool
_EvlogEventGetExtentionEntry (void)
{
    static int init = 0;
    static Bool success = FALSE;

    if (init)
        return success;

    init = 1;

    damage = CheckExtension (DAMAGE_NAME); 
    XDBG_RETURN_VAL_IF_FAIL (damage != NULL, FALSE);

    success = TRUE;

    return success;
}

Bool
xDbgModuleEvlogEvent (xEvent *ev, char *buf, int remain)
{
    const char *evt_name;
    int len;

    if (!_EvlogEventGetExtentionEntry ())
        return FALSE;

    evt_name = LookupEventName ((int)(ev->u.u.type));
    len = snprintf (buf, remain, "%s", evt_name);
    buf += len;
    remain -= len;

    if (ev->u.u.type == damage->eventBase + XDamageNotify)
    {
        xDamageNotifyEvent *damage_e = (xDamageNotifyEvent*)ev;
        snprintf (buf, remain, ": XID(%lx) area(%d,%d %dx%d) geo(%d,%d %dx%d)",
            damage_e->drawable,
            damage_e->area.x,
            damage_e->area.y,
            damage_e->area.width,
            damage_e->area.height,
            damage_e->geometry.x,
            damage_e->geometry.y,
            damage_e->geometry.width,
            damage_e->geometry.height);
        return TRUE;
    }

    return FALSE;
}

