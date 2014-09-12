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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dbus/dbus.h>
#include <os.h>
#include <dix.h>

#include "xdbg.h"
#include "xdbg_dbus_server.h"

#define ARGV_NUM        128
#define REP_MSG_SIZE    8192
#define STR_LEN         128

#define RECONNECT_TIME  1000
#define DISPATCH_TIME   50

typedef struct _XDbgDBusServerInfo
{
    OsTimerPtr timer;
    DBusConnection *conn;
    XDbgDbusServerMethod *methods;
    char rule[STR_LEN];
    int fd;
} XDbgDBusServerInfo;

static XDbgDBusServerInfo server_info;

static CARD32 _xDbgDBusServerTimeout (OsTimerPtr timer, CARD32 time, pointer arg);
static Bool _xDbgDBusServerInit (XDbgDBusServerInfo *info);
static void _xDbgDBusServerDeinit (XDbgDBusServerInfo *info);

static Bool
_xDbgDBusServerReplyMessage (XDbgDBusServerInfo *info, DBusMessage *msg, char *reply)
{
    DBusMessage *reply_msg = NULL;
    DBusMessageIter iter;

    XDBG_RETURN_VAL_IF_FAIL (info->conn != NULL, FALSE);

    reply_msg = dbus_message_new_method_return (msg);
    XDBG_RETURN_VAL_IF_FAIL (reply_msg != NULL, FALSE);

    dbus_message_iter_init_append (reply_msg, &iter);
    if (!dbus_message_iter_append_basic (&iter, DBUS_TYPE_STRING, &reply))
    {
        XDBG_ERROR (MDBUS, "[SERVER] out of memory\n");
        dbus_message_unref (reply_msg);
        return FALSE;
    }

    if (!dbus_connection_send (info->conn, reply_msg, NULL))
    {
        XDBG_ERROR (MDBUS, "[SERVER] failed: send reply\n");
        dbus_message_unref (reply_msg);
        return FALSE;
    }

    XDBG_INFO (MDBUS, "[SERVER] send reply\n");

    dbus_connection_flush (info->conn);
    dbus_message_unref (reply_msg);

    return TRUE;
}

static void
_xDbgDBusServerProcessMessage (XDbgDBusServerInfo *info, DBusMessage *msg)
{
    XDbgDbusServerMethod **prev;
    DBusError err;
    char err_buf[REP_MSG_SIZE] = {0,};
    char *argv[ARGV_NUM] = {0,};
    int argc = 0;
    int i;

    snprintf (err_buf, REP_MSG_SIZE, "error message!\n");

    dbus_error_init (&err);

    XDBG_INFO (MDBUS, "[SERVER] Process a message (%s.%s)\n",
               dbus_message_get_interface (msg), dbus_message_get_member (msg));

    XDBG_RETURN_IF_FAIL (info->conn != NULL);

    for (prev = &info->methods; *prev; prev = &(*prev)->next)
    {
        XDbgDbusServerMethod *method = *prev;

        if (!strcmp (dbus_message_get_member (msg), method->name))
        {
            DBusMessageIter iter;
            char reply[REP_MSG_SIZE] = {0,};
            char *p;
            int   len;

            if (!dbus_message_iter_init (msg, &iter))
            {
                XDBG_ERROR (MDBUS, "[SERVER] Message has no arguments!\n");
                snprintf (err_buf, REP_MSG_SIZE, "Message has no arguments!\n");
                goto send_fail;
            }

            do
            {
                p = NULL;

                if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_STRING)
                {
                    XDBG_ERROR (MDBUS, "[SERVER] Argument is not string!\n");
                    snprintf (err_buf, REP_MSG_SIZE, "Argument is not string!\n");
                    goto send_fail;
                }

                dbus_message_iter_get_basic (&iter, &p);

                if (!p)
                {
                    XDBG_ERROR (MDBUS, "[SERVER] Can't get string!\n");
                    snprintf (err_buf, REP_MSG_SIZE, "Can't get string!\n");
                    goto send_fail;
                }

                argv[argc] = strdup (p);
                argc++;
            } while (dbus_message_iter_has_next (&iter) &&
                   dbus_message_iter_next (&iter) &&
                   argc < ARGV_NUM);

            len = REP_MSG_SIZE - 1;

            if (method->func)
                method->func (method->data, argc, argv, reply, &len);

            _xDbgDBusServerReplyMessage (info, msg, reply);

            for (i = 0; i < ARGV_NUM; i++)
                if (argv[i])
                    free (argv[i]);
            dbus_error_free (&err);

            return;
        }
    }

    return;

send_fail:
   _xDbgDBusServerReplyMessage (info, msg, err_buf);

    for (i = 0; i < ARGV_NUM; i++)
        if (argv[i])
            free (argv[i]);
    dbus_error_free (&err);
}

static void
_xDbgDBusServerWakeupHandler (pointer data, int error, pointer pRead)
{
    XDbgDBusServerInfo *info = (XDbgDBusServerInfo*)data;

    if (!info || !info->conn || info->fd < 0)
        return;

    if (FD_ISSET(info->fd, (fd_set*)pRead))
    {
        do {
            dbus_connection_read_write_dispatch (info->conn, 0);
        } while (info->conn &&
                 dbus_connection_get_is_connected (info->conn) &&
                 dbus_connection_get_dispatch_status (info->conn) ==
                 DBUS_DISPATCH_DATA_REMAINS);

        /* DON'T use info->conn from here. info->conn becomes NULL by
         * dbus_connection_read_write_dispatch(_xDbgDBusServerMsgFilter)
         */
    }
}

static DBusHandlerResult
_xDbgDBusServerMsgHandler (DBusConnection *connection, DBusMessage *msg, void *data)
{
    XDbgDBusServerInfo *info = (XDbgDBusServerInfo*)data;

    if (!info || !info->conn || !msg)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    XDBG_INFO (MDBUS, "[SERVER] Got a message (%s.%s)\n",
               dbus_message_get_interface (msg), dbus_message_get_member (msg));

    if (!dbus_message_is_method_call (msg, XDBG_DBUS_INTERFACE, XDBG_DBUS_METHOD))
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    _xDbgDBusServerProcessMessage (info, msg);

    return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
_xDbgDBusServerMsgFilter (DBusConnection *conn, DBusMessage *msg, void *data)
{
    XDbgDBusServerInfo *info = (XDbgDBusServerInfo*)data;

    if (!info)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    if (dbus_message_is_signal (msg, DBUS_INTERFACE_LOCAL, "Disconnected"))
    {
        XDBG_INFO (MDBUS, "[SERVER] disconnected by signal\n");
        _xDbgDBusServerDeinit (info);

        if (info->timer)
            TimerFree(info->timer);
        info->timer = TimerSet(NULL, 0, 1, _xDbgDBusServerTimeout, info);

        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static CARD32
_xDbgDBusServerTimeout (OsTimerPtr timer, CARD32 time, pointer arg)
{
    XDbgDBusServerInfo *info = (XDbgDBusServerInfo*)arg;

    if (!info)
        return 0;

    XDBG_DEBUG (MDBUS, "[SERVER] timeout\n");

    if (_xDbgDBusServerInit (info))
    {
        TimerFree (info->timer);
        info->timer = NULL;
        return 0;
    }

    return RECONNECT_TIME;
}

static Bool
_xDbgDBusServerInit (XDbgDBusServerInfo *info)
{
    DBusObjectPathVTable vtable = {.message_function = _xDbgDBusServerMsgHandler, };
    DBusError err;
    int ret;

    dbus_error_init (&err);

    XDBG_RETURN_VAL_IF_FAIL (info->conn == NULL, FALSE);

    info->conn = dbus_bus_get (DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set (&err))
    {
        XDBG_ERROR (MDBUS, "[SERVER] failed: connection (%s)\n", err.message);
        goto free_err;
    }
    if (!info->conn)
    {
        XDBG_ERROR (MDBUS, "[SERVER] failed: connection NULL\n");
        goto free_err;
    }

    ret = dbus_bus_request_name (info->conn, XDBG_DBUS_SERVER,
                                 DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
    if (dbus_error_is_set (&err))
    {
        XDBG_ERROR (MDBUS, "[SERVER] failed: request name (%s)\n", err.message);
        goto free_conn;
    }
    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
    {
        XDBG_ERROR (MDBUS, "[SERVER] failed: Not Primary Owner (%d)\n", ret);
        goto free_conn;
    }

	snprintf (info->rule, sizeof (info->rule), "type='method_call',interface='%s'",
              XDBG_DBUS_INTERFACE);

    /* blocks until we get a reply. */
    dbus_bus_add_match (info->conn, info->rule, &err);
    if (dbus_error_is_set (&err))
    {
        XDBG_ERROR (MDBUS, "[SERVER] failed: add match (%s)\n", err.message);
        goto free_name;
    }

    if (!dbus_connection_register_object_path (info->conn,
                                               XDBG_DBUS_PATH, &vtable,
                                               info))
    {
        XDBG_ERROR (MDBUS, "[SERVER] failed: register object path\n");
        goto free_match;
    }

    dbus_connection_set_exit_on_disconnect (info->conn, FALSE);

    if (!dbus_connection_add_filter (info->conn, _xDbgDBusServerMsgFilter, info, NULL))
    {
        XDBG_ERROR (MDBUS, "[SERVER] failed: add filter (%s)\n", err.message);
        goto free_register;
    }

	if (!dbus_connection_get_unix_fd (info->conn, &info->fd) || info->fd < 0)
    {
        XDBG_ERROR (MDBUS, "[SERVER] failed: get fd\n");
        goto free_filter;
    }

    AddGeneralSocket (info->fd);
    RegisterBlockAndWakeupHandlers ((BlockHandlerProcPtr)NoopDDA,
                                    _xDbgDBusServerWakeupHandler, info);

    XDBG_INFO (MDBUS, "[SERVER] connected\n");

    dbus_error_free (&err);

    return TRUE;

free_filter:
    dbus_connection_remove_filter (info->conn, _xDbgDBusServerMsgFilter, info);
free_register:
    dbus_connection_unregister_object_path (info->conn, XDBG_DBUS_PATH);
free_match:
    dbus_bus_remove_match (info->conn, info->rule, &err);
    dbus_error_free (&err);
free_name:
    dbus_bus_release_name (info->conn, XDBG_DBUS_SERVER, &err);
    dbus_error_free (&err);
free_conn:
    dbus_connection_close (info->conn);
free_err:
    dbus_error_free (&err);
    info->conn = NULL;
    info->fd = -1;

    return FALSE;
}

static void
_xDbgDBusServerDeinit (XDbgDBusServerInfo *info)
{
    if (info->timer)
    {
        TimerFree (info->timer);
        info->timer = NULL;
    }

    if (info->conn)
    {
        DBusError err;
        dbus_error_init (&err);
        dbus_connection_remove_filter (info->conn, _xDbgDBusServerMsgFilter, info);
        dbus_connection_unregister_object_path (info->conn, XDBG_DBUS_PATH);
        dbus_bus_remove_match (info->conn, info->rule, &err);
        dbus_error_free (&err);
        dbus_bus_release_name (info->conn, XDBG_DBUS_SERVER, &err);
        dbus_error_free (&err);
        dbus_connection_unref (info->conn);
        info->conn = NULL;
    }

    RemoveBlockAndWakeupHandlers ((BlockHandlerProcPtr)NoopDDA,
                                   _xDbgDBusServerWakeupHandler, info);
    if (info->fd >= 0)
    {
        RemoveGeneralSocket (info->fd);
        info->fd = -1;
    }

    XDBG_INFO (MDBUS, "[SERVER] disconnected\n");
}

Bool
xDbgDBusServerConnect (void)
{
    XDBG_DEBUG (MDBUS, "[SERVER] connecting\n");

    memset (&server_info, 0, sizeof(server_info));

    server_info.fd = -1;
    server_info.timer = TimerSet (NULL, 0, 1, _xDbgDBusServerTimeout, &server_info);

    return TRUE;
}

void
xDbgDBusServerDisconnect (void)
{
    XDBG_DEBUG (MDBUS, "[SERVER] disconnecting\n");

    _xDbgDBusServerDeinit (&server_info);
}

Bool
xDbgDBusServerAddMethod (XDbgDbusServerMethod *method)
{
    XDbgDbusServerMethod **prev;

    for (prev = &server_info.methods; *prev; prev = &(*prev)->next);

    method->next = NULL;
    *prev = method;

    return TRUE;
}

void
xDbgDBusServerRemoveMethod (XDbgDbusServerMethod *method)
{
    XDbgDbusServerMethod **prev;

    for (prev = &server_info.methods; *prev; prev = &(*prev)->next)
        if (*prev == method)
        {
            *prev = method->next;
            break;
        }
}
