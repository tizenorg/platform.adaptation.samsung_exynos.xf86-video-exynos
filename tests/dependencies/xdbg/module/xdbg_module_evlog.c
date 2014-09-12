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
#include <time.h>

#define __USE_GNU
#include <sys/socket.h>
#include <linux/socket.h>

#ifdef HAS_GETPEERUCRED
# include <ucred.h>
#endif

#include <xace.h>
#include <xacestr.h>
#include <X11/Xatom.h>
#include <X11/extensions/XI2proto.h>
#include <windowstr.h>

#include <xdbg.h>

#include "xdbg_module_types.h"
#include "xdbg_module_evlog.h"
#include "xdbg_module_evlog_request.h"
#include "xdbg_module_evlog_event.h"
#include "bool_exp_rule_checker.h"

#define XREGISTRY
#include "registry.h"

#define FP1616toDBL(x) ((x) * 1.0 / (1 << 16))

static Bool	xev_trace_on = FALSE;
static int  xev_trace_fd = -1;
static int  xev_trace_record_fd = -1;
static Atom atom_rotate = None;
static Atom atom_client_pid = None;

RULE_CHECKER rc = NULL;

#define UNKNOWN_EVENT "<unknown>"

typedef enum
{
    EVENT,
    REQUEST,
    REPLY,
    FLUSH
} evtType;

#define EVTDATA_MASK_CLIENT_INFO    0x1
#define EVTDATA_MASK_CLIENT_REQ     0x2
#define EVTDATA_MASK_EVENT          0x4

static char *evt_type[] = { "Event", "Request", "Reply", "Flush" };
static char *evt_dir[]  = { "<====", "---->",   "<----", "*****" };

static void _evtPrintF (int fd, const char * format, ...)
{
    va_list args;

    va_start (args, format);
    if (fd < 0)
        VErrorF (format, args);
    else
        vdprintf (fd, format, args);
    va_end (args);
}

static void evtGetReqInfo (evtType type, EvlogClientInfo *evinfo, xReq *req, xEvent *ev,
                           int *req_id, const char **req_string,
                           char **c_cmd, int *c_index, int *c_pid)
{
    if (type == REQUEST && req)
    {
        *req_id = req->reqType;
        if (req->reqType < EXTENSION_BASE)
            *req_string = LookupRequestName (req->reqType, 0);
        else
            *req_string = LookupRequestName (req->reqType, req->data);
    }
    else if (type == EVENT && ev)
        *req_string = LookupEventName ((int)(ev->u.u.type));

    if(evinfo)
    {
        *c_index = evinfo->index;
        *c_pid = evinfo->pid;

        *c_cmd = rindex (evinfo->command, '/');
        if (*c_cmd == NULL)
            *c_cmd = evinfo->command;
        else
            *c_cmd = *c_cmd + 1;
    }
}

static void evtRecord (CARD32 msec, evtType type, EvlogClientInfo *evinfo, xEvent *ev)
{
    int mask = 0;
    int write_len = sizeof (int) +
                    sizeof (CARD32) +
                    sizeof (evtType) +
                    sizeof (int);

    if (xev_trace_record_fd < 0)
        return;

    if (evinfo)
    {
        mask |= EVTDATA_MASK_CLIENT_INFO;
        write_len += sizeof (EvlogClientInfo);

        if (evinfo->requestBuffer)
        {
            mask |= EVTDATA_MASK_CLIENT_REQ;
            write_len += evinfo->req_len;
        }
    }

    if (ev)
    {
        mask |= EVTDATA_MASK_EVENT;
        write_len += sizeof (xEvent);
    }

    if (write (xev_trace_record_fd, &write_len, sizeof(int)) == -1)
    {
        XDBG_ERROR (MXDBG, "failed: write write_len\n");
        return;
    }
    if (write (xev_trace_record_fd, &msec, sizeof(CARD32)) == -1)
    {
        XDBG_ERROR (MXDBG, "failed: write msec\n");
        return;
    }
    if (write (xev_trace_record_fd, &type, sizeof(evtType)) == -1)
    {
        XDBG_ERROR (MXDBG, "failed: write type\n");
        return;
    }
    if (write (xev_trace_record_fd, &mask, sizeof(int)) == -1)
    {
        XDBG_ERROR (MXDBG, "failed: write mask\n");
        return;
    }

    if (evinfo)
    {
        if (write (xev_trace_record_fd, evinfo, sizeof (EvlogClientInfo)) == -1)
        {
            XDBG_ERROR (MXDBG, "failed: write client\n");
            return;
        }
        if (evinfo->requestBuffer)
            if (write (xev_trace_record_fd, evinfo->requestBuffer, evinfo->req_len) == -1)
            {
                XDBG_ERROR (MXDBG, "failed: write requestBuffer\n");
                return;
            }
    }

    if (ev)
        if (write (xev_trace_record_fd, ev, sizeof (xEvent)) == -1)
        {
            XDBG_ERROR (MXDBG, "failed: write ev\n");
            return;
        }
}

static void evtPrintF (int fd, CARD32 msec, evtType type, EvlogClientInfo *evinfo, xEvent *ev)
{
    int   req_id = 0;
    const char *req_string = "";
    const char *req_detail = "";
    char  tempbuf[256] = {0,};
    char *c_cmd = "";
    int   c_index = 0;
    int   c_pid = 0;
    static CARD32 prev;
    xReq *req = NULL;

    if(evinfo)
    {
        req = (xReq *)evinfo->requestBuffer;
    }

    evtGetReqInfo (type, evinfo, req, ev, &req_id, &req_string, &c_cmd, &c_index, &c_pid);

    if (type == REQUEST)
    {
        XDBG_RETURN_IF_FAIL (evinfo != NULL);
        XDBG_RETURN_IF_FAIL (req != NULL);
        xDbgModuleEvlogReqeust (evinfo, req, tempbuf, sizeof (tempbuf));
        req_detail = tempbuf;
    }
    else if (type == EVENT)
    {
        XDBG_RETURN_IF_FAIL (ev != NULL);
        xDbgModuleEvlogEvent (ev, tempbuf, sizeof (tempbuf));
        req_detail = tempbuf;
    }
    else
        req_detail = req_string;

    _evtPrintF (fd, "[%10.3f][%4ld] %15s(%2d:%4d) %s %7s (%s)\n",
                msec / 1000.0,
                msec - prev,
                c_cmd, c_index, c_pid,
                evt_dir[type],
                evt_type[type], req_detail);

    prev = msec;
}

static void evtPrint (evtType type, ClientPtr client, xEvent *ev)
{
    int   req_id = 0;
    const char *req_string = "";
    char *c_cmd = "";
    int   c_index = 0;
    int   c_pid = 0;
    CARD32 curr;
    ModuleClientInfo *info = NULL;
    EvlogClientInfo evinfo;
    xReq *req = NULL;

    if (xev_trace_on == FALSE)
        return;

    if (client)
    {
        info = GetClientInfo (client);
        XDBG_RETURN_IF_FAIL (info != NULL);

        memset (&evinfo, 0, sizeof (EvlogClientInfo));

        evinfo.index = info->index;
        evinfo.pid = info->pid;
        evinfo.gid = info->gid;
        evinfo.uid = info->uid;
        strncpy (evinfo.command, info->command, strlen (info->command));
        evinfo.requestBuffer = client->requestBuffer;
        evinfo.req_len = client->req_len;

        req = (xReq *)evinfo.requestBuffer;
    }

    evtGetReqInfo (type, (client)?&evinfo:NULL, req, ev, &req_id, &req_string, &c_cmd, &c_index, &c_pid);

    if (rc == NULL)
        rc = rulechecker_init();

    if (!rulechecker_validate_rule (rc, type, req_id, req_string, c_pid, c_cmd))
        return;

    curr = GetTimeInMillis ();

    if (xev_trace_record_fd >= 0)
        evtRecord (curr, type, (client)?&evinfo:NULL, ev);
    else
        evtPrintF (xev_trace_fd, curr, type, (client)?&evinfo:NULL, ev);
}

static void _mergeArgs (char * target, int argc, const char ** argv)
{
    int i;
    int len;

    for (i=0; i<argc; i++)
    {
        len = sprintf (target, "%s", argv[i]);
        target += len;

        if (i != argc - 1)
            *(target++) = ' ';
    }
}

static const char*
_traceGetWindowName (ClientPtr client, Window window)
{
    int rc;
    WindowPtr pWin;
    Mask win_mode = DixGetPropAccess, prop_mode = DixReadAccess;
    Atom property;
    PropertyPtr pProp;
    static char winname[128];
    int datalen;

    rc = dixLookupWindow (&pWin, window, client, win_mode);
    if (rc != Success)
        return NULL;

    property = MakeAtom ("WM_NAME", strlen ("WM_NAME"), TRUE);
    while (pWin)
    {
        rc = dixLookupProperty (&pProp, pWin, property, client, prop_mode);
        if (rc == Success && pProp->data)
        {
            datalen = (pProp->size>127) ?127:pProp->size;
            strncpy (winname, pProp->data, datalen);
            winname[datalen] = 0;

            return winname;
        }

        pWin = pWin->parent;
    }

    return NULL;
}

static void
_traceFlush (CallbackListPtr *pcbl, pointer nulldata, pointer calldata)
{
    evtPrint (FLUSH, NULL, NULL);
}

static void
_traceAReply (CallbackListPtr *pcbl, pointer nulldata, pointer calldata)
{
    ReplyInfoRec *pri = (ReplyInfoRec*)calldata;

    evtPrint (REPLY, pri->client, NULL);
}

static void
_traceEvent (CallbackListPtr *pcbl, pointer nulldata, pointer calldata)
{
    EventInfoRec *pei = (EventInfoRec*)calldata;
    ClientPtr pClient;
    ModuleClientInfo *info;
    int ev; /* event index */
    static int xi2_opcode = -1;
    xEvent *pev;
    static char* ename[]=
    {
        "KeyPress",
        "KeyRelease",
        "ButtonPress",
        "ButtonRelease",
    };

    XDBG_RETURN_IF_FAIL (pei != NULL);

    pClient = pei->client;
    XDBG_RETURN_IF_FAIL (pClient != NULL);

    pev = pei->events;
    XDBG_RETURN_IF_FAIL (pev != NULL);

    info = GetClientInfo (pClient);
    XDBG_RETURN_IF_FAIL (info != NULL);

    for (ev=0; ev < pei->count; ev++, pev++)
    {
        int type = pev->u.u.type & 0177;

        if (type < LASTEvent)
        {
            switch (type)
            {
            case KeyPress:
            case KeyRelease:
                XDBG_INFO (MXDBG, "%s(%d)_%d(%s.%d : %s.0x%lx) root(%d,%d) win(%d,%d)\n"
                        , ename[type-KeyPress], pev->u.u.detail, pev->u.u.type
                        , info->command, info->pid
                        , _traceGetWindowName (pClient, pev->u.keyButtonPointer.event), pev->u.keyButtonPointer.event
                        , pev->u.keyButtonPointer.rootX, pev->u.keyButtonPointer.rootY
                        , pev->u.keyButtonPointer.eventX, pev->u.keyButtonPointer.eventY);
                break;

            case ButtonPress:
            case ButtonRelease:
                XDBG_INFO (MXDBG, "%s(%d)_%d(%s.%d : %s.0x%lx) root(%d,%d) win(%d,%d)\n"
                        , ename[type-KeyPress], pev->u.u.detail, pev->u.u.type
                        , info->command, info->pid
                        , _traceGetWindowName (pClient, pev->u.keyButtonPointer.event), pev->u.keyButtonPointer.event
                        , pev->u.keyButtonPointer.rootX, pev->u.keyButtonPointer.rootY
                        , pev->u.keyButtonPointer.eventX, pev->u.keyButtonPointer.eventY);
                break;
            case GenericEvent:
                if(!xi2_opcode) break;
                if(xi2_opcode < 0)
                {
                    ExtensionEntry *pExt = CheckExtension("XInputExtension");
                    if(!pExt) xi2_opcode = 0;
                    else xi2_opcode = pExt->base;
                }

                if(((xGenericEvent*)pev)->extension != xi2_opcode) break;

                xXIDeviceEvent *xidev = (xXIDeviceEvent *)pev;
                if(xidev->deviceid==2) break;
                if(xidev->evtype==XI_ButtonPress)
                    XDBG_TRACE (MXDBG, "XI_ButtonPress(%d) device(%d), event win(0x%x), child(0x%x), root(%.f,%.f), win(%.f, %.f)\n", XI_ButtonPress, xidev->deviceid, xidev->event, xidev->child, FP1616toDBL(xidev->root_x), FP1616toDBL(xidev->root_y), FP1616toDBL(xidev->event_x), FP1616toDBL(xidev->event_y));
                else if(xidev->evtype==XI_ButtonRelease)
                    XDBG_TRACE (MXDBG, "XI_ButtonRelease(%d) device(%d), event win(0x%x), child(0x%x), root(%.f,%.f), win(%.f, %.f)\n", XI_ButtonRelease,  xidev->deviceid, xidev->event, xidev->child, FP1616toDBL(xidev->root_x), FP1616toDBL(xidev->root_y), FP1616toDBL(xidev->event_x), FP1616toDBL(xidev->event_y));
                else if(xidev->evtype==XI_Motion)
                    XDBG_TRACE (MXDBG, "XI_Motion(%d) device(%d), event win(0x%x), child(0x%x), root(%.f,%.f), win(%.f, %.f)\n", XI_Motion, xidev->deviceid, xidev->event, xidev->child, FP1616toDBL(xidev->root_x), FP1616toDBL(xidev->root_y), FP1616toDBL(xidev->event_x), FP1616toDBL(xidev->event_y));
                break;
            default:
                break;
            }
        }

        evtPrint (EVENT, pClient, pev);
    }
}

static void
_traceACoreEvents (CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceCoreDispatchRec *rec = calldata;

    XDBG_RETURN_IF_FAIL (rec != NULL);

    evtPrint (REQUEST, rec->client, NULL);
}

static void
_traceAExtEvents (CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceExtAccessRec *rec = calldata;

    XDBG_RETURN_IF_FAIL (rec != NULL);

    evtPrint (REQUEST, rec->client, NULL);
}

static void
_traceProperty (CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XacePropertyAccessRec *rec = calldata;
    ModuleClientInfo *info = GetClientInfo (rec->client);
    PropertyPtr pProp = *rec->ppProp;
    Atom name = pProp->propertyName;

    /* Don't care about the new content check */
    if (rec->client == serverClient || rec->access_mode & DixPostAccess)
        return;

    if (name == atom_client_pid && (rec->access_mode & DixWriteAccess))
    {
        XDBG_WARNING (MXDBG, "Invalid access X_CLINET_PID pid:%d, uid:%d\n", info->pid, info->uid);
        rec->status = BadAccess;
        return;
    }

    rec->status = Success;
    return;
}

static void
_traceResource (CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceResourceAccessRec *rec = calldata;
    Mask access_mode = rec->access_mode;
    ModuleClientInfo *info = GetClientInfo (rec->client);

    /* Perform the background none check on windows */
    if (access_mode & DixCreateAccess && rec->rtype == RT_WINDOW)
    {
        WindowPtr pWin = (WindowPtr) rec->res;
        int rc;
        int pid = info->pid;

        rc = dixChangeWindowProperty (serverClient,
                                      pWin, atom_client_pid, XA_CARDINAL, 32,
                                      PropModeReplace, 1, &pid, FALSE);
        if (rc != Success)
            XDBG_ERROR (MXDBG, "failed : set X_CLIENT_PID to %d.\n", pid);
    }
}

static void
_traceReceive (CallbackListPtr *pcbl, pointer unused, pointer calldata)
{
    XaceReceiveAccessRec *rec = calldata;

    if (rec->events->u.u.type != VisibilityNotify)
        return;

    rec->status = BadAccess;
}

Bool
xDbgModuleEvlogInstallHooks (XDbgModule *pMod)
{
    int ret = TRUE;

    ret &= AddCallback (&EventCallback, _traceEvent, NULL);
    ret &= AddCallback (&FlushCallback, _traceFlush, NULL);
    ret &= AddCallback (&ReplyCallback, _traceAReply, NULL);
    ret &= XaceRegisterCallback (XACE_PROPERTY_ACCESS, _traceProperty, NULL);
    ret &= XaceRegisterCallback (XACE_RESOURCE_ACCESS, _traceResource, NULL);

    /*Disable Visibility Event*/
    ret &= XaceRegisterCallback (XACE_RECEIVE_ACCESS, _traceReceive, NULL);

    if (atom_client_pid == None)
        atom_client_pid = MakeAtom ("X_CLIENT_PID", 12, TRUE);
    if (atom_rotate == None)
        atom_rotate = MakeAtom ("_E_ILLUME_ROTATE_ROOT_ANGLE", 12, TRUE);

    if (!ret)
    {
        XDBG_ERROR (MXDBG, "failed: register one or more callbacks\n");
        return FALSE;
    }

    if (pMod && pMod->evlog_path)
        xDbgModuleEvlogSetEvlogPath (pMod, -1, pMod->evlog_path, NULL, NULL);

    return TRUE;
}

void
xDbgModuleEvlogUninstallHooks (XDbgModule *pMod)
{
    DeleteCallback (&EventCallback, _traceEvent, NULL);
    DeleteCallback (&FlushCallback, _traceFlush, NULL);
    DeleteCallback (&ReplyCallback, _traceAReply, NULL);
    XaceDeleteCallback (XACE_PROPERTY_ACCESS, _traceProperty, NULL);
    XaceDeleteCallback (XACE_RESOURCE_ACCESS, _traceResource, NULL);
}

void
xDbgModuleEvlogPrintEvents (XDbgModule *pMod, Bool on, const char * client_name, char *reply, int *len)
{
    int ret = TRUE;

    xev_trace_on = on;

    if (xev_trace_on)
    {
        int i;

        //Find client's pid
        for (i=1 ; i< currentMaxClients ; i++)
        {
            ClientPtr pClient;
            ModuleClientInfo *info;

            pClient = clients[i];
            if (!pClient)
                continue;

            info = GetClientInfo (pClient);
            if (!info)
                continue;

            if (strlen (info->command) > 0 && strstr (client_name, info->command))
            {
                char fd_name[256];

                if (xev_trace_fd >= 0)
                    close (xev_trace_fd);

                snprintf (fd_name, 256, "/proc/%d/fd/1", info->pid);
                xev_trace_fd = open (fd_name, O_RDWR);
                if (xev_trace_fd < 0)
                    XDBG_REPLY ("failed: open consol '%s'. (%s)\n", fd_name, strerror(errno));
            }
        }


        ret &= XaceRegisterCallback (XACE_CORE_DISPATCH, _traceACoreEvents, NULL);
        ret &= XaceRegisterCallback (XACE_EXT_DISPATCH, _traceAExtEvents, NULL);

        if (!ret)
        {
            XDBG_REPLY ("failed: register one or more callbacks.\n");
            return;
        }
    }
    else
    {
        XaceDeleteCallback (XACE_CORE_DISPATCH, _traceACoreEvents, NULL);
        XaceDeleteCallback (XACE_EXT_DISPATCH, _traceAExtEvents, NULL);
    }

    return;
}

int
xDbgModuleEvlogInfoSetRule (XDbgModule *pMod, const int argc, const char ** argv, char *reply, int *len)
{
    const char * command;

    if (rc == NULL)
        rc = rulechecker_init();

    if (argc == 0)
    {
        rulechecker_print_rule (rc, reply);
        return 0;
    }

    command = argv[0];

    if (!strcasecmp (command, "add"))
    {
        POLICY_TYPE policy_type;
        RC_RESULT_TYPE result;
        const char * policy = argv[1];
        char rule[8192];

        if (argc < 3)
        {
            XDBG_REPLY ("Error : Too few arguments.\n");
            return -1;
        }

        if (!strcasecmp (policy, "ALLOW"))
            policy_type = ALLOW;
        else if (!strcasecmp (policy, "DENY"))
            policy_type = DENY;
        else
        {
            XDBG_REPLY ("Error : Unknown policy : [%s].\n          Policy should be ALLOW or DENY.\n", policy);
            return -1;
        }

        _mergeArgs (rule, argc - 2, &(argv[2]));

        result = rulechecker_add_rule (rc, policy_type, rule);
        if (result == RC_ERR_TOO_MANY_RULES)
        {
            XDBG_REPLY ("Error : Too many rules were added.\n");
            return -1;
        }
        else if (result == RC_ERR_PARSE_ERROR)
        {
            XDBG_REPLY ("Error : An error occured during parsing the rule [%s]\n", rule);
            return -1;
        }

        XDBG_REPLY ("The rule was successfully added.\n\n");
        rulechecker_print_rule (rc, reply);
        return 0;
    }
    else if (!strcasecmp (command, "remove"))
    {
        const char * remove_idx;
        int i;

        if (argc < 2)
        {
            XDBG_REPLY ("Error : Too few arguments.\n");
            return -1;
        }

        for (i=0; i<argc - 1; i++)
        {
            remove_idx = argv[i+1];

            if (!strcasecmp (remove_idx, "all"))
            {
                rulechecker_destroy (rc);
                rc = rulechecker_init();
                XDBG_REPLY ("Every rules were successfully removed.\n");
            }
            else
            {
                int index = atoi (remove_idx);
                if (isdigit (*remove_idx) && rulechecker_remove_rule (rc, index) == 0)
                    XDBG_REPLY ("The rule [%d] was successfully removed.\n", index);
                else
                    XDBG_REPLY ("Rule remove fail : No such rule [%s].\n", remove_idx);
            }
        }
        rulechecker_print_rule (rc, reply);
        return 0;
    }
    else if (!strcasecmp (command, "print"))
    {
        rulechecker_print_rule (rc, reply);
        return 0;
    }
    else if (!strcasecmp (command, "help"))
    {
        XDBG_REPLY ("%s", rulechecker_print_usage());
        return 0;
    }

    XDBG_REPLY ("%s\nUnknown command : [%s].\n\n", rulechecker_print_usage(), command);

    return 0;
}

Bool
xDbgModuleEvlogSetEvlogPath (XDbgModule *pMod, int pid, char *path, char *reply, int *len)
{
    char fd_name[XDBG_PATH_MAX];
    char *temp[3] = {"/", "./", "../"};
    Bool valid = FALSE;
    int i;

    if (!path || strlen (path) <= 0)
    {
        XDBG_REPLY ("failed: invalid path\n");
        return FALSE;
    }

    if (xev_trace_record_fd >= 0)
    {
        close (xev_trace_record_fd);
        xev_trace_record_fd = -1;
    }

    if (!strcmp (path, "console"))
    {
        if (pid > 0)
        {
            char fd_name[256];

            if (xev_trace_fd >= 0)
                close (xev_trace_fd);

            snprintf (fd_name, sizeof (fd_name), "/proc/%d/fd/1", pid);

            xev_trace_fd = open (fd_name, O_RDWR);
            if (xev_trace_fd < 0)
                XDBG_REPLY ("failed: open consol '%s'. (%s)\n", fd_name, strerror(errno));
        }

        return TRUE;
    }

    for (i = 0; i < sizeof (temp) / sizeof (char*); i++)
        if (path == strstr (path, temp[i]))
        {
            valid = TRUE;
            break;
        }

    if (!valid)
    {
        XDBG_REPLY ("failed: invalid path(%s)\n", path);
        return FALSE;
    }

    if (path[0] == '/')
        snprintf (fd_name, XDBG_PATH_MAX, "%s", path);
    else
    {
        char cwd[128];
        if (getcwd (cwd, sizeof (cwd)))
            snprintf (fd_name, XDBG_PATH_MAX, "%s/%s", cwd, path);
        else
            snprintf (fd_name, XDBG_PATH_MAX, "%s", path);
    }

    xev_trace_record_fd = open (fd_name, O_CREAT|O_RDWR|O_APPEND, 0755);
    if (xev_trace_record_fd < 0)
    {
        XDBG_REPLY ("failed: open file '%s'. (%s)\n", fd_name, strerror(errno));
        return FALSE;
    }

    return TRUE;
}

void
xDbgModuleEvlogPrintEvlog (XDbgModule *pMod, int pid, char *evlog_path, char *reply, int *len)
{
    int fd = -1, cfd = -1;
    int total, read_len;
    int evlog_len;
    pointer requestBuffer = NULL;
    char fd_name[256];

    if (!evlog_path)
    {
        XDBG_REPLY ("failed: no evlog path\n");
        return;
    }

    fd = open (evlog_path, O_RDONLY);
    if (fd < 0)
    {
        XDBG_REPLY ("failed: open '%s'. (%s)\n", evlog_path, strerror(errno));
        return;
    }

    snprintf (fd_name, sizeof (fd_name), "/proc/%d/fd/1", pid);
    cfd = open (fd_name, O_RDWR);
    if (cfd < 0)
    {
        XDBG_REPLY ("failed: open consol '%s'. (%s)\n", fd_name, strerror(errno));
        goto print_done;
    }

    while ((read_len = read (fd, &evlog_len, sizeof (int))) == sizeof (int))
    {
        CARD32 msec;
        evtType type;
        int mask;
        EvlogClientInfo evinfo;
        xEvent ev;

        total = read_len;

        read_len = read (fd, &msec, sizeof (CARD32));
        XDBG_GOTO_IF_FAIL (read_len == sizeof (CARD32), print_done);
        total += read_len;

        read_len = read (fd, &type, sizeof (evtType));
        XDBG_GOTO_IF_FAIL (read_len == sizeof (evtType), print_done);
        total += read_len;

        read_len = read (fd, &mask, sizeof (int));
        XDBG_GOTO_IF_FAIL (read_len == sizeof (int), print_done);
        total += read_len;

        if (mask & EVTDATA_MASK_CLIENT_INFO)
        {
            read_len = read (fd, &evinfo, sizeof (EvlogClientInfo));
            XDBG_GOTO_IF_FAIL (read_len == sizeof (EvlogClientInfo), print_done);
            total += read_len;

            if (mask & EVTDATA_MASK_CLIENT_REQ && evinfo.req_len > 0)
            {
                requestBuffer = malloc (evinfo.req_len);
                XDBG_GOTO_IF_FAIL (requestBuffer != NULL, print_done);

                read_len = read (fd, requestBuffer, evinfo.req_len);
                XDBG_GOTO_IF_FAIL (read_len == evinfo.req_len, print_done);
                total += read_len;

                evinfo.requestBuffer = requestBuffer;
            }
        }

        if (mask & EVTDATA_MASK_EVENT)
        {
            read_len = read (fd, &ev, sizeof (xEvent));
            XDBG_GOTO_IF_FAIL (read_len == sizeof (xEvent), print_done);
            total += read_len;
        }

        XDBG_GOTO_IF_FAIL (evlog_len == total, print_done);

        evtPrintF (cfd, msec, type, &evinfo, &ev);

        if (requestBuffer)
        {
            free (requestBuffer);
            requestBuffer = NULL;
        }
    }

print_done:
    if (requestBuffer)
        free (requestBuffer);

    if (cfd >= 0)
        close (cfd);

    if (fd >= 0)
        close (fd);
}
