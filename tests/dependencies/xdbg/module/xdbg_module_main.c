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

#include <fcntl.h>
#include <unistd.h>
#include <windowstr.h>
#include <xacestr.h>
#include <xdbg.h>

#include "xdbg_dbus_server.h"
#include "xdbg_module_types.h"
#include "xdbg_module_command.h"
#include "xdbg_module_evlog.h"

#define __USE_GNU
#include <sys/socket.h>

DevPrivateKeyRec debug_client_key;

static XDbgDbusServerMethod method;

static void
_debugClientInfo (ClientPtr client)
{
    int fd = XaceGetConnectionNumber (client);
    ModuleClientInfo *info = GetClientInfo (client);

    /* For local clients, try and determine the executable name */
    if (XaceIsLocal (client))
    {
        struct ucred creds;
        socklen_t len = sizeof (creds);
        char path[PATH_MAX + 1] = {0,};
        int bytes;

        memset (&creds, 0, sizeof (creds));
        if (getsockopt (fd, SOL_SOCKET, SO_PEERCRED, &creds, &len) < 0)
            goto finish;

        snprintf (path, PATH_MAX + 1, "/proc/%d/cmdline", creds.pid);
        fd = open (path, O_RDONLY);
        if (fd < 0)
            goto finish;

        bytes = read (fd, path, PATH_MAX + 1);
        close (fd);
        if (bytes <= 0)
            goto finish;

        strncpy (info->command, path, PATH_MAX);

        info->pid = creds.pid;
        info->uid = creds.uid;
        info->gid = creds.gid;
        info->conn_fd = fd;
        info->index = client->index;
    }
    else
    {
        info->pid = -1;
        info->index = client->index;
        strncpy (info->command, "REMOTE", PATH_MAX);
    }

    return;

finish:
    XDBG_ERROR (MXDBG, "Failed to make client info(index:%d)\n",
                 client->index);
    return;
}

static void
_traceClientState (CallbackListPtr *list, pointer closure, pointer calldata)
{
    NewClientInfoRec *clientinfo = (NewClientInfoRec*)calldata;
    ClientPtr client = clientinfo->client;
    ModuleClientInfo *info = GetClientInfo (client);
    static char* clientState[]=
    {
        "ClientStateInitial",
        "ClientStateAuthenticating",
        "ClientStateRunning",
        "ClientStateRetained",
        "ClientStateGone",
        "ClientStateCheckingDebug",
        "ClientStateCheckedDebug"
    };

    if (!info)
        return;

    if ((client->clientState == ClientStateInitial) || (client->clientState == ClientStateGone))
    {
        if (client->clientState == ClientStateInitial)
              _debugClientInfo (client);

        XDBG_INFO (MXDBG, "id:%d, conn_fd:%d, pid:%d, uid:%d, name:%s (%s)\n",
              info->index, info->conn_fd, info->pid, info->uid, info->command,
              clientState[client->clientState]);
        return;
    }

    XDBG_DEBUG (MXDBG, "id:%d, conn_fd:%d, pid:%d, uid:%d, name:%s (%s)\n",
                 info->index, info->conn_fd, info->pid, info->uid, info->command,
                 clientState[client->clientState]);
}

Bool
xDbgModuleMain (XDbgModule *pMod)
{
    Bool ret = TRUE;

    if (!dixRegisterPrivateKey (DebugClientKey, PRIVATE_CLIENT, sizeof (ModuleClientInfo)))
    {
        XDBG_ERROR (MXDBG, "failed: dixRegisterPrivateKey\n");
        return FALSE;
    }

    ret &= AddCallback (&ClientStateCallback, _traceClientState, pMod);

    if (!ret)
    {
        XDBG_ERROR (MXDBG, "failed: AddCallback\n");
        return FALSE;
    }

    xDbgModuleEvlogInstallHooks (pMod);

    if (!xDbgDBusServerConnect ())
    {
        XDBG_ERROR (MXDBG, "failed: xDbgDBusServerConnect\n");
        return FALSE;
    }

    if (!xDbgModuleCommandInitLogPath (pMod))
    {
        XDBG_ERROR (MXDBG, "failed: xDbgModuleInitLogPath\n");
        return FALSE;
    }

    snprintf (method.name, sizeof (method.name), "%s", XDBG_DBUS_METHOD);
    method.func = xDbgModuleCommand;
    method.data = pMod;

    xDbgDBusServerAddMethod (&method);

    return TRUE;
}

void
xDbgModuleMainExit (XDbgModule *pMod)
{
    DeleteCallback (&ClientStateCallback, _traceClientState, pMod);

    xDbgModuleEvlogUninstallHooks (pMod);

    xDbgDBusServerRemoveMethod (&method);

    xDbgDBusServerDisconnect ();
}
