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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <X11/Xlib.h>
#include <xdbg.h>
#include "xdbg_dbus_client.h"

int main(int argc, char ** argv)
{
    XDbgDBusClientInfo* dbus_info;
    Display *dpy;
    char **new_argv;
    int new_argc, i;
    char temp[128];

    dpy = XOpenDisplay (NULL);
    if (!dpy)
    {
        fprintf (stderr, "failed: open display\n");
        exit (-1);
    }

    dbus_info = xDbgDBusClientConnect ();
    if (!dbus_info)
    {
        fprintf (stderr, "failed: connect dbus\n");
        exit (-1);
    }

    new_argc = argc + 1;
    new_argv = (char**)malloc (new_argc * sizeof (char*));
    if (!new_argv)
    {
        fprintf (stderr, "failed: malloc new argv\n");
        exit (-1);
    }

    snprintf (temp, sizeof(temp), "%d", getpid ());
    new_argv[0] = temp;

    for (i = 0; i < argc; i++)
        new_argv[i+1] = argv[i];

    xDbugDBusClientSendMessage (dbus_info, new_argc, new_argv);

    xDbgDBusClientDisconnect (dbus_info);

    free (new_argv);

    XCloseDisplay (dpy);

    return 0;
}
