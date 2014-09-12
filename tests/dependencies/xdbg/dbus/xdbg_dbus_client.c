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

#include <sys/types.h>
#include <unistd.h>

#include "xdbg.h"
#include "xdbg_dbus_client.h"

#define REPLY_TIME 1000
#define STR_LEN  128

#define WARNING_IF_FAIL(cond)         {if (!(cond)) { fprintf (stderr, "[%s] '%s' failed.\n", __FUNCTION__, #cond);}}
#define RETURN_IF_FAIL(cond)          {if (!(cond)) { fprintf (stderr, "[%s] '%s' failed.\n", __FUNCTION__, #cond); return; }}
#define RETURN_VAL_IF_FAIL(cond, val) {if (!(cond)) { fprintf (stderr, "[%s] '%s' failed.\n", __FUNCTION__, #cond); return val; }}
#define RETURN_VAL_IF_ERRNO(cond, val, errno)       {if (!(cond)) { fprintf (stderr, "[%s] '%s' failed. (err=%s(%d))\n", __FUNCTION__, #cond, strerror(errno), errno); return val; }}
#define GOTO_IF_FAIL(cond, dst)       {if (!(cond)) { fprintf (stderr, "[%s] '%s' failed.\n", __FUNCTION__, #cond); goto dst; }}
#define GOTO_IF_ERRNO(cond, dst, errno)       {if (!(cond)) { fprintf (stderr, "[%s] '%s' failed. (err=%s(%d))\n", __FUNCTION__, #cond, strerror(errno), errno); goto dst; }}

struct _XDbgDBusClientInfo
{
    DBusConnection *conn;
    char reqname[STR_LEN];
    char client[STR_LEN];
    int  pid;
};

static DBusHandlerResult
_xDbgDBusClinetMsgFilter (DBusConnection *conn, DBusMessage *msg, void *data)
{
    XDbgDBusClientInfo *info = (XDbgDBusClientInfo*)data;

    /* If we get disconnected, then take everything down, and attempt to
     * reconnect immediately (assuming it's just a restart).  The
     * connection isn't valid at this point, so throw it out immediately. */
    if (dbus_message_is_signal (msg, DBUS_INTERFACE_LOCAL, "Disconnected"))
    {
        fprintf (stderr, "[CLIENT:%s] disconnected by signal\n", info->client);
        info->conn = NULL;

        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static Bool
_xDbgDBusClinetInit (XDbgDBusClientInfo *info)
{
    DBusError err;
    int ret;

    dbus_error_init (&err);
    RETURN_VAL_IF_FAIL (info->conn == NULL, FALSE);

    info->conn = dbus_bus_get (DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set (&err))
    {
        fprintf (stderr, "[CLIENT:%s] failed: connection (%s)\n", info->client, err.message);
        goto err_get;
    }
    if (!info->conn)
    {
        fprintf (stderr, "[CLIENT:%s] failed: connection NULL\n", info->client);
        goto err_get;
    }

    dbus_connection_set_exit_on_disconnect (info->conn, FALSE);

    if (!dbus_connection_add_filter (info->conn, _xDbgDBusClinetMsgFilter, info, NULL))
    {
        fprintf (stderr, "[CLIENT:%s] failed: add filter (%s)\n", info->client, err.message);
        goto err_get;
    }

    ret = dbus_bus_request_name (info->conn, info->reqname,
                                 DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
    if (dbus_error_is_set (&err))
    {
        fprintf (stderr, "[CLIENT:%s] failed: request name (%s)\n", info->client, err.message);
        goto err_request;
    }
    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
        fprintf (stderr, "[CLIENT:%s] failed: Not Primary Owner (%d)\n", info->client, ret);
        goto err_request;
    }

    dbus_error_free (&err);

//    fprintf (stderr, "[CLIENT:%s] connected\n", info->client);

    return TRUE;

err_request:
    dbus_connection_remove_filter (info->conn, _xDbgDBusClinetMsgFilter, info);

err_get:
    if (info->conn)
    {
        dbus_connection_unref (info->conn);
        info->conn = NULL;
    }

    dbus_error_free (&err);

    return FALSE;
}


static void
_xDbgDBusClinetDeinit (XDbgDBusClientInfo *info)
{
    DBusError err;

    if (!info->conn)
        return;

    dbus_error_init (&err);
    dbus_bus_release_name (info->conn, info->reqname, &err);
    if (dbus_error_is_set (&err))
        fprintf (stderr, "[CLIENT:%s] failed: release name (%s)\n", info->client, err.message);
    dbus_error_free (&err);

    dbus_connection_remove_filter (info->conn, _xDbgDBusClinetMsgFilter, info);
    dbus_connection_unref (info->conn);
    info->conn = NULL;

//    fprintf (stderr, "[CLIENT:%s] disconnected\n", info->client);
}

XDbgDBusClientInfo*
xDbgDBusClientConnect (void)
{
    XDbgDBusClientInfo *info = NULL;

    info = calloc (1, sizeof (XDbgDBusClientInfo));
    GOTO_IF_FAIL (info != NULL, err_conn);

    snprintf (info->client, STR_LEN, "%d", getpid());
    snprintf (info->reqname, STR_LEN, "%s%d", XDBG_DBUS_CLIENT, getpid());

    if (!_xDbgDBusClinetInit (info))
        goto err_conn;

    return info;

err_conn:
    if (info)
        free (info);

    return NULL;
}

void
xDbgDBusClientDisconnect (XDbgDBusClientInfo* info)
{
    if (!info)
        return;

    _xDbgDBusClinetDeinit (info);

    free (info);
}

void
xDbugDBusClientSendMessage (XDbgDBusClientInfo *info, int argc, char **argv)
{
    DBusMessage *msg = NULL;
    DBusMessage *reply_msg = NULL;
    DBusMessageIter iter;
    DBusError err;
    char *arg = NULL;
    int i;

    RETURN_IF_FAIL (info != NULL);
    RETURN_IF_FAIL (info->conn != NULL);
    RETURN_IF_FAIL (argc > 0);
    RETURN_IF_FAIL (argv[0] != '\0');

    dbus_error_init (&err);

    msg = dbus_message_new_method_call (XDBG_DBUS_SERVER, XDBG_DBUS_PATH,
                                        XDBG_DBUS_INTERFACE, XDBG_DBUS_METHOD);
    GOTO_IF_FAIL (msg != NULL, err_send);

    dbus_message_iter_init_append (msg, &iter);
    for (i = 0; i < argc; i++)
        if (!dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &argv[i]))
        {
            fprintf (stderr, "[CLIENT:%s] failed: append\n", info->client);
            goto err_send;
        }

    reply_msg = dbus_connection_send_with_reply_and_block (info->conn, msg,
                                                           REPLY_TIME, &err);
    if (dbus_error_is_set (&err))
    {
        fprintf (stderr, "[CLIENT:%s] failed: send (%s)\n", info->client, err.message);
        goto err_send;
    }
    GOTO_IF_FAIL (reply_msg != NULL, err_send);

    if (!dbus_message_iter_init (reply_msg, &iter))
    {
        fprintf (stderr, "[CLIENT:%s] Message has no arguments\n", info->client);
        goto err_send;
    }

    do
    {
        arg = NULL;

        if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING)
        {
            fprintf (stderr, "[CLIENT:%s] Argument is not string!\n", info->client);
            goto err_send;
        }

        dbus_message_iter_get_basic (&iter, &arg);
        if (!arg)
        {
            fprintf (stderr, "[CLIENT:%s] arg is NULL\n", info->client);
            goto err_send;
        }
        else
            fprintf (stderr, "%s\n", arg);
    } while (dbus_message_iter_has_next (&iter) &&
           dbus_message_iter_next (&iter));

err_send:
    if (msg)
        dbus_message_unref(msg);
    if (reply_msg)
        dbus_message_unref(reply_msg);
}
