/**************************************************************************

xserver-xorg-video-exynos

Copyright 2011 Samsung Electronics co., Ltd. All Rights Reserved.

Contact: SooChan Lim <sc1.lim@samsung.com>

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

#include "exynos_driver.h"
#include "exynos_display.h"
#include "exynos_display_int.h"
#include "exynos_util.h"

#include <xf86Crtc.h>
#include <xf86DDC.h>
#include <X11/Xatom.h>
//#include <X11/randr/randrstr.h>
#include <X11/extensions/dpmsconst.h>
#include <list.h>
#include "exynos_mem_pool.h"
//*********************************
#include <sys/ioctl.h> /*ioctl() prototype*/
#include <unistd.h> //
#include <stdio.h>  //ex

#define STR_XRR_DISPLAY_MODE_PROPERTY "XRR_PROPERTY_DISPLAY_MODE"
#define STR_OUR_PROPERTY "ENUM_PROPERTY"
#define STR_IMMUT_PROPERTY "IMMUTABLE"
/************************************************************************/
#define ATOM_EDID             "EDID"
#define ATOM_EDID2            "EDID_DATA"
#define ATOM_SIGNAL_FORMAT    "SignalFormat"
#define ATOM_CONNECTOR_TYPE   "ConnectorType"
#define ATOM_CONNECTOR_NUMBER "ConnectorNumber"
#define ATOM_OUTPUT_NUMBER    "_OutputNumber"
#define ATOM_PANNING_AREA     "_PanningArea"
#define ATOM_BACKLIGHT        "Backlight"
#define ATOM_COHERENT         "_Coherent"
#define ATOM_HDMI             "_HDMI"
#define ATOM_AUDIO_WORKAROUND "_AudioStreamSilence"


static Atom atom_SignalFormat, atom_ConnectorType, atom_ConnectorNumber,
    atom_OutputNumber, atom_PanningArea, atom_Backlight, atom_Coherent,
    atom_HdmiProperty, atom_AudioWorkaround;
static Atom atom_unknown, atom_TMDS, atom_LVDS, atom_DisplayPort, atom_TV;
static Atom atom_HDMI, atom_Panel;
static Atom atom_EDID, atom_EDID2;

static Bool g_display_mode_prop_init = FALSE;
static Bool g_lvds_func_prop_init = FALSE;
static Bool g_fb_visible_prop_init = FALSE;
static Bool g_video_offset_prop_init = FALSE;

static Atom xrr_property_display_mode_atom;
static Atom xrr_property_lvds_func_atom;
static Atom xrr_property_fb_visible_atom;
static Atom xrr_property_video_offset_atom;

#define STR_XRR_DISPLAY_MODE_PROPERTY "XRR_PROPERTY_DISPLAY_MODE"
#define STR_XRR_LVDS_FUNCTION "XRR_PROPERTY_LVDS_FUNCTION"
#define STR_XRR_FB_VISIBLE_PROPERTY "XRR_PROPERTY_FB_VISIBLE"
#define STR_XRR_VIDEO_OFFSET_PROPERTY "XRR_PROPERTY_VIDEO_OFFSET"

Bool exynosPropSetLvdsFunc (xf86OutputPtr pOutput, Atom property, RRPropertyValuePtr value);
/************************************************************************/

static const int subpixel_conv_table[7] =
{
    0,
    SubPixelUnknown,
    SubPixelHorizontalRGB,
    SubPixelHorizontalBGR,
    SubPixelVerticalRGB,
    SubPixelVerticalBGR,
    SubPixelNone
};
typedef enum
{
    XRR_OUTPUT_DISPLAY_MODE_NULL,
    XRR_OUTPUT_DISPLAY_MODE_WB_CLONE,
    XRR_OUTPUT_DISPLAY_MODE_VIDEO_ONLY,
} XRROutputPropDisplayMode;

typedef enum
{
    XRR_OUTPUT_LVDS_FUNC_NULL,
    XRR_OUTPUT_LVDS_FUNC_INIT_VIRTUAL,
    XRR_OUTPUT_LVDS_FUNC_HIBERNATION,
    XRR_OUTPUT_LVDS_FUNC_ACCESSIBILITY,
} XRROutputPropLvdsFunc;

static const char *output_names[] =
{
    "None",
    "VGA",
    "DVI",
    "DVI",
    "DVI",
    "Composite",
    "TV",
    "LVDS",
    "CTV",
    "DIN",
    "DP",
    "HDMI",
    "HDMI",
    "TV",
    "eDP",
    "Virtual",
};



static void
_exynosOutputAttachEdid(xf86OutputPtr pOutput)
{
    EXYNOSOutputPrivPtr pOutputPriv = pOutput->driver_private;
    EXYNOSDispInfoPtr pDispInfo = pOutputPriv->pDispInfo;
    drmModeConnectorPtr koutput = pOutputPriv->mode_output;
    drmModePropertyBlobPtr edid_blob = NULL;
    xf86MonPtr mon = NULL;
    int i;

    /* look for an EDID property */
    for (i = 0; i < koutput->count_props; i++)
    {
        drmModePropertyPtr props;

        props = drmModeGetProperty (pDispInfo->drm_fd, koutput->props[i]);
        if (!props)
            continue;

        if (!(props->flags & DRM_MODE_PROP_BLOB))
        {
            drmModeFreeProperty (props);
            continue;
        }

        if (!strcmp (props->name, "EDID"))
        {
            drmModeFreePropertyBlob (edid_blob);
            edid_blob = drmModeGetPropertyBlob (pDispInfo->drm_fd,
                                        koutput->prop_values[i]);
        }
        drmModeFreeProperty (props);
    }

    if (edid_blob)
    {
        mon = xf86InterpretEDID (pOutput->scrn->scrnIndex,
                                 edid_blob->data);

        if (mon && edid_blob->length > 128)
            mon->flags |= MONITOR_EDID_COMPLETE_RAWDATA;
    }

    xf86OutputSetEDID (pOutput, mon);

    if (edid_blob)
        drmModeFreePropertyBlob (edid_blob);
}

static Bool
_exynosOutputPropertyIgnore (drmModePropertyPtr prop)
{
    if (!prop)
        return TRUE;

    /* ignore blob prop */
    if (prop->flags & DRM_MODE_PROP_BLOB)
        return TRUE;

    /* ignore standard property */
    if (!strcmp (prop->name, "EDID") ||
        !strcmp (prop->name, "DPMS"))
        return TRUE;

    return FALSE;
}

static void
propUnSetWbClone (xf86OutputPtr pOutput)
{
    displaySetDispSetMode (pOutput, DISPLAY_SET_MODE_OFF);
    displaySetDispConnMode (pOutput->scrn, DISPLAY_CONN_MODE_NONE);
}


static void
propUnSetDisplayMode (xf86OutputPtr pOutput)
{
    XRROutputPropDisplayMode disp_mode;
    EXYNOSOutputPrivPtr pOutputPriv;

    pOutputPriv = pOutput->driver_private;
    disp_mode = pOutputPriv->disp_mode;

    if (disp_mode == XRR_OUTPUT_DISPLAY_MODE_NULL){
        XDBG_INFO (MOUTP, "XRR_OUTPUT_DISPLAY_MODE_NULL\n");
        return;
    }

    /* check the private and unset the diplaymode */
    switch (disp_mode)
    {
        case XRR_OUTPUT_DISPLAY_MODE_WB_CLONE:
            XDBG_INFO (MOUTP, "[DISPLAY_MODE]: UnSet Clone\n");
            propUnSetWbClone (pOutput);
            break;
        default:
            break;
    }

    pOutputPriv->disp_mode = XRR_OUTPUT_DISPLAY_MODE_NULL;
}

static xf86OutputStatus
EXYNOSOutputDetect (xf86OutputPtr output)
{
    /* go to the hw and retrieve a new output struct */
    EXYNOSOutputPrivPtr pOutputPriv = output->driver_private;
    EXYNOSDispInfoPtr pDispInfo = pOutputPriv->pDispInfo;
    xf86OutputStatus status = XF86OutputStatusUnknown;

    /* update output */
    drmModeFreeConnector (pOutputPriv->mode_output);
    pOutputPriv->mode_output =
        drmModeGetConnector (pDispInfo->drm_fd, pOutputPriv->output_id);

    /* update encoder */
    drmModeFreeEncoder (pOutputPriv->mode_encoder);
    pOutputPriv->mode_encoder =
        drmModeGetEncoder (pDispInfo->drm_fd, pOutputPriv->mode_output->encoders[0]);
    /****** test **********************/
   // if ((strcmp(output->name,"Virtual1")==0))
   //     pOutputPriv->mode_output->connection = DRM_MODE_CONNECTED;
    /**********************************/
    switch (pOutputPriv->mode_output->connection)
    {
    case DRM_MODE_CONNECTED:
        status = XF86OutputStatusConnected;
        break;
    case DRM_MODE_DISCONNECTED:
        status = XF86OutputStatusDisconnected;
        XDBG_INFO (MOUTP, "DRM_MODE_DISCONNECTED\n");
        propUnSetDisplayMode (output);
        break;
    case DRM_MODE_UNKNOWNCONNECTION:
        status = XF86OutputStatusUnknown;
        break;
    default:
        break;
    }

#if 1
        char *conn_str[] = {"connected", "disconnected", "unknown"};
        XDBG_INFO (MOUTP, "detect : connect(%d, type:%d, status:%s) encoder(%d) crtc(%d).\n",
                   pOutputPriv->output_id, pOutputPriv->mode_output->connector_type,
                   conn_str[pOutputPriv->mode_output->connection-1],
                   pOutputPriv->mode_encoder->encoder_id, pOutputPriv->mode_encoder->crtc_id);
#endif

    return status;
}

static Bool
EXYNOSOutputModeValid (xf86OutputPtr pOutput, DisplayModePtr pModes)
{
    EXYNOSOutputPrivPtr pOutputPriv = pOutput->driver_private;
    drmModeConnectorPtr koutput = pOutputPriv->mode_output;
    int i;

    /* driver want to remain available modes which is same as mode
       supported from drmmode */
    if (pOutputPriv->mode_output->connector_type == DRM_MODE_CONNECTOR_LVDS)
    {
        for (i = 0; i < koutput->count_modes; i++)
        {
            if (pModes->HDisplay == koutput->modes[i].hdisplay &&
                pModes->VDisplay == koutput->modes[i].vdisplay)
                return MODE_OK;
        }
        return MODE_ERROR;
    }

    return MODE_OK;
}

static DisplayModePtr
EXYNOSOutputGetModes (xf86OutputPtr pOutput)
{
    EXYNOSOutputPrivPtr pOutputPriv = pOutput->driver_private;
    drmModeConnectorPtr koutput = pOutputPriv->mode_output;
    DisplayModePtr pModes = NULL;
    DisplayModePtr pMode = NULL;
    int i;

    /* LVDS1 (main LCD) does not provide edid data */
    if (pOutputPriv->mode_output->connector_type != DRM_MODE_CONNECTOR_LVDS)
        _exynosOutputAttachEdid(pOutput);

    /* modes should already be available */
    for (i = 0; i < koutput->count_modes; i++)
    {
        pMode = ctrl_calloc (1, sizeof (DisplayModeRec));
        if (pMode)
        {
            /* generate the fake modes when screen rotation is set */
            exynosDisplayModeFromKmode(pOutput->scrn, &koutput->modes[i], pMode);
            pModes = xf86ModesAdd(pModes, pMode);
        }
    }

    return pModes;
}

static void
EXYNOSOutputDestroy (xf86OutputPtr pOutput)
{
    EXYNOSOutputPrivPtr pOutputPriv = pOutput->driver_private;
    int i;

    for (i = 0; i < pOutputPriv->num_props; i++)
    {
        drmModeFreeProperty (pOutputPriv->props[i].mode_prop);
        ctrl_free (pOutputPriv->props[i].atoms);
    }
    ctrl_free (pOutputPriv->props);

    drmModeFreeEncoder (pOutputPriv->mode_encoder);
    drmModeFreeConnector (pOutputPriv->mode_output);

    xorg_list_del (&pOutputPriv->link);

    ctrl_free (pOutputPriv);
    pOutput->driver_private = NULL;
}

static void
EXYNOSOutputDpms (xf86OutputPtr pOutput, int dpms)
{
    EXYNOSOutputPrivPtr pOutputPriv = pOutput->driver_private;
    EXYNOSDispInfoPtr pDispInfo = pOutputPriv->pDispInfo;

    drmModeConnectorPtr koutput = pOutputPriv->mode_output;
    ScrnInfoPtr pScrn = pOutput->scrn;
    EXYNOSPtr pExynos = EXYNOSPTR (pScrn);
    int old_dpms = pOutputPriv->dpms_mode;
    int i;

    if (!strcmp(pOutput->name, "HDMI1") ||
        !strcmp(pOutput->name, "Virtual1"))
            return;


    if (dpms == DPMSModeSuspend || dpms == DPMSModeStandby)
        return;

    for (i = 0; i < koutput->count_props; i++)
    {
        drmModePropertyPtr props;

        props = drmModeGetProperty (pDispInfo->drm_fd, koutput->props[i]);
        if (!props)
            continue;

        if ((old_dpms == DPMSModeStandby && dpms == DPMSModeOn) ||
            (old_dpms == DPMSModeOn && dpms == DPMSModeStandby))
        {
            if (!strcmp (props->name, "panel"))
            {
                int value = (dpms == DPMSModeStandby)? 1 : 0;
                drmModeConnectorSetProperty(pDispInfo->drm_fd,
                                            pOutputPriv->output_id,
                                            props->prop_id,
                                            value);
                pOutputPriv->dpms_mode = dpms;
                drmModeFreeProperty (props);
                XDBG_INFO (MDPMS, "panel '%s'\n", (value)?"OFF":"ON");
                return;
            }
        }
        else if (!strcmp (props->name, "DPMS"))
        {
            switch (dpms)
            {
            case DPMSModeStandby:
            case DPMSModeOn:
                if (pOutputPriv->dpms_mode == DPMSModeOn)
                {
                    drmModeFreeProperty (props);
                    return;
                }

                XDBG_INFO (MDPMS, "\t DPMS status of [%s] is DPMSModeOn.\n", pOutput->name);

                /* TODO: temp */
                pDispInfo->pipe0_on = TRUE;
                pExynos->lcdstatus = dpms;
#if 0
                /* set current fb to crtc */
                if(!exynosCrtcApply(pOutput->crtc))
                {
                    XDBG_ERROR(MDPMS, "Fail crtc apply(output name, %s)\n",
                               pOutput->name);
                }
#endif
                break;
            case DPMSModeOff:
                if (pOutputPriv->dpms_mode == DPMSModeOff)
                {
                    drmModeFreeProperty (props);
                    return;
                }
                pExynos->lcdstatus = dpms;
                XDBG_INFO (MDPMS, "\t DPMS status of [%s] is DPMSModeOff.\n", pOutput->name);

                /* TODO: temp */
                pDispInfo->pipe0_on = FALSE;

                break;
            default:
                drmModeFreeProperty (props);
                return;
            }

            if (drmModeConnectorSetProperty (pDispInfo->drm_fd, pOutputPriv->output_id, props->prop_id, dpms))
            {
                XDBG_ERRNO (MDPMS, "drmModeConnectorSetProperty failed. dpms(%d)\n", dpms);
                drmModeFreeProperty (props);
                return;
            }

            /* sucess to set dpms */
            pOutputPriv->dpms_mode = dpms;
            drmModeFreeProperty (props);
            return;
        }

        drmModeFreeProperty (props);
    }
}

/**
 * Callback for setting up a video mode after fixups have been made.
 *
 * This is only called while the output is disabled.  The dpms callback
 * must be all that's necessary for the output, to turn the output on
 * after this function is called.
 */
/*void
 (*mode_set) (xf86OutputPtr output,
              DisplayModePtr mode, DisplayModePtr adjusted_mode);*/
void EXYNOSOutputModeSet (xf86OutputPtr output,
                    DisplayModePtr mode, DisplayModePtr adjusted_mode)
{

   XDBG_INFO (MOUTP, "EXYNOSOutputModeSet\n");

}


static drmModePropertyPtr createPropertyRange(const char *name,uint32_t id,uint64_t low,uint64_t high){
    drmModePropertyPtr prop=ctrl_calloc(1,sizeof(drmModePropertyRes));
    prop->prop_id=id;
    memset(prop->name,0,DRM_PROP_NAME_LEN);
    strncpy(prop->name,name,DRM_PROP_NAME_LEN-1);
    prop->flags = DRM_MODE_PROP_RANGE;
    prop->values = ctrl_calloc(2,sizeof(uint64_t));
    prop->values[0]=low;
    prop->values[1]=high;
    return prop;
}

static drmModePropertyPtr createPropertyEnum(const char *name,uint32_t id){
    drmModePropertyPtr prop;
    prop=ctrl_calloc(1,sizeof(drmModePropertyRes));
    prop->prop_id=id;
    memset(prop->name,0,DRM_PROP_NAME_LEN);
    strncpy(prop->name,name,DRM_PROP_NAME_LEN-1);
    prop->flags = DRM_MODE_PROP_ENUM;
    prop->count_values=0;
    prop->count_enums=0;
    prop->enums=0;
    prop->values = ctrl_calloc(1,sizeof(uint64_t));
    return prop;
}


static drmModePropertyPtr createPropertyImmutable(const char *name,uint32_t id){
    drmModePropertyPtr prop;
    prop=ctrl_calloc(1,sizeof(drmModePropertyRes));
    prop->prop_id=id;
    memset(prop->name,0,DRM_PROP_NAME_LEN);
    strncpy(prop->name,name,DRM_PROP_NAME_LEN-1);
    prop->flags = DRM_MODE_PROP_IMMUTABLE;
    prop->count_values=0;
    prop->count_enums=0;
    prop->enums=0;
    prop->values=ctrl_calloc(1,sizeof(uint64_t));
    return prop;
}

typedef struct drm_mode_property_enum property_enum;


static void addEnumProperty(drmModePropertyPtr prop,const char *name){
    int num = 1 + prop->count_enums;
    //we create new array of enums
    property_enum* p=(property_enum*)ctrl_calloc(num , sizeof(property_enum));
    //fill new prop name
    memset(p->name,0,DRM_PROP_NAME_LEN);
    strncpy(p->name,name,DRM_PROP_NAME_LEN-1);
    p->value=prop->values;
    //copy old enums
    property_enum* old_enums=prop->enums;
    memcpy(&p[1],old_enums,prop->count_enums*sizeof(property_enum));
    //free old lists
    if(old_enums){
        ctrl_free(old_enums);
    }
    prop->enums=p;
    prop->count_enums=num;
}


void
EXYNOSOutputCreateResources (xf86OutputPtr pOutput)
{
    EXYNOSOutputPrivPtr pOutputPriv = pOutput->driver_private;
    drmModeConnectorPtr mode_output = pOutputPriv->mode_output;
    drmModePropertyPtr prop[5];
    EXYNOSDispInfoPtr pDispInfo = pOutputPriv->pDispInfo;
    int i, j, errno;

    pOutputPriv->props = ctrl_calloc (mode_output->count_props, sizeof (EXYNOSPropertyRec));
    if (!pOutputPriv->props)
        return;

    /********************************************************************/
    /*  Create atoms for RandR 1.3 properties ***************************/
     if(!atom_EDID)
     {
       atom_EDID            = MakeAtom(ATOM_EDID,
                       sizeof(ATOM_EDID)-1, TRUE);
       atom_EDID2           = MakeAtom(ATOM_EDID2,
                       sizeof(ATOM_EDID2)-1, TRUE);
       atom_SignalFormat    = MakeAtom(ATOM_SIGNAL_FORMAT,
                       sizeof(ATOM_SIGNAL_FORMAT)-1, TRUE);
       atom_ConnectorType   = MakeAtom(ATOM_CONNECTOR_TYPE,
                       sizeof(ATOM_CONNECTOR_TYPE)-1, TRUE);
       atom_ConnectorNumber = MakeAtom(ATOM_CONNECTOR_NUMBER,
                       sizeof(ATOM_CONNECTOR_NUMBER)-1, TRUE);
       atom_OutputNumber    = MakeAtom(ATOM_OUTPUT_NUMBER,
                       sizeof(ATOM_OUTPUT_NUMBER)-1, TRUE);
       atom_PanningArea     = MakeAtom(ATOM_PANNING_AREA,
                       sizeof(ATOM_PANNING_AREA)-1, TRUE);

       /* Create atoms for RandR 1.3 property values */
       atom_unknown         = MakeAtom("unknown", 7, TRUE);
       atom_TMDS            = MakeAtom("TMDS", 4, TRUE);
       atom_LVDS            = MakeAtom("LVDS", 4, TRUE);
       atom_DisplayPort     = MakeAtom("DisplayPort", 11, TRUE);
       atom_TV              = MakeAtom("TV", 2, TRUE);
       atom_HDMI            = MakeAtom("HDMI", 4, TRUE);
       atom_Panel           = MakeAtom("Panel", 5, TRUE);
     }
    /********************************************************************/

    pOutputPriv->num_props = 0;

    for (i = j = 0; i < mode_output->count_props; i++)
    {
        drmModePropertyPtr drmmode_prop;

        drmmode_prop = drmModeGetProperty(pDispInfo->drm_fd,
                                          mode_output->props[i]);
        if (_exynosOutputPropertyIgnore(drmmode_prop))
        {
            drmModeFreeProperty (drmmode_prop);
            continue;
        }

        pOutputPriv->props[j].mode_prop = drmmode_prop;
        pOutputPriv->props[j].value = mode_output->prop_values[i];
        j++;
    }
    int k=0;
    if(strcmp(pOutput->name,"LVDS1")==0)
    {
        prop[0]=createPropertyRange("RANGE_PROPERTY",0x123,0,100);//we create prop just as example
        prop[1]=createPropertyImmutable("ConnectorType",0x124);
        prop[2]=createPropertyImmutable("_OutputNumber",0x125);
        prop[0]=createPropertyRange("DEBUG_COMMAND",0x126,0,20);//we create prop just as example
        k = 4;
    }
    if(strcmp(pOutput->name,"HDMI1")==0)
    {
        prop[0]=createPropertyEnum(STR_OUR_PROPERTY,0x130);//and another one
        addEnumProperty(prop[0],"ON");
        addEnumProperty(prop[0],"OFF");
        k=1;
    }

    if(strcmp(pOutput->name,"Virtual1")==0)
    {

        prop[0]=createPropertyRange("DEBUG_COMMAND",0x135,0,20);//we create prop just as example
        k=1;
     }

     for (i=0; i<k; i++)
     {
        pOutputPriv->props[j].mode_prop=prop[i];
        pOutputPriv->props[j++].value = prop[i]->values;
     }
        pOutputPriv->num_props = j;
        XDBG_DEBUG (MDPMS, "Number of xrandr properties = %d\n", pOutputPriv->num_props);

     for (i = 0; i < pOutputPriv->num_props; i++)
     {
        EXYNOSPropertyPtr p = &pOutputPriv->props[i];
        drmModePropertyPtr drmmode_prop = p->mode_prop;

        if (drmmode_prop->flags & DRM_MODE_PROP_RANGE)
        {
            INT32 range[2];

            p->num_atoms = 1;
            p->atoms = ctrl_calloc (p->num_atoms, sizeof (Atom));
            if (!p->atoms)
                continue;

            p->atoms[0] = MakeAtom (drmmode_prop->name, strlen (drmmode_prop->name), TRUE);
            range[0] = drmmode_prop->values[0];
            range[1] = drmmode_prop->values[1];
            errno = RRConfigureOutputProperty (pOutput->randr_output, p->atoms[0],
                                             FALSE, TRUE,
                                             drmmode_prop->flags & DRM_MODE_PROP_IMMUTABLE ? TRUE : FALSE,
                                             2, range);
            if (errno != 0)
            {
                XDBG_ERRNO (MDPMS, "RRConfigureOutputProperty error\n");
            }
            errno = RRChangeOutputProperty (pOutput->randr_output, p->atoms[0],
                                          XA_INTEGER, 32, PropModeReplace, 1, &p->value, FALSE, TRUE);
            if (errno != 0)
            {
                XDBG_ERRNO(MDPMS, "RRChangeOutputProperty error\n");
            }
        }
        else if (drmmode_prop->flags & DRM_MODE_PROP_ENUM)
        {
            p->num_atoms = drmmode_prop->count_enums + 1;
            p->atoms = ctrl_calloc (p->num_atoms, sizeof (Atom));
            if (!p->atoms)
                continue;
            //creating of atom with property's name
            p->atoms[0] = MakeAtom (drmmode_prop->name, strlen (drmmode_prop->name), TRUE);
            for (j = 1; j <= drmmode_prop->count_enums; j++)
            {
                struct drm_mode_property_enum *e = &drmmode_prop->enums[j-1];
                //creating of atoms with enums
                p->atoms[j] = MakeAtom (e->name, strlen (e->name), TRUE);
            }

            errno = RRConfigureOutputProperty (pOutput->randr_output, p->atoms[0],
                                             FALSE, FALSE,
                                             drmmode_prop->flags & DRM_MODE_PROP_IMMUTABLE ? TRUE : FALSE,
                                             p->num_atoms - 1, (INT32*)&p->atoms[1]);
            if (errno != 0)
            {
                XDBG_ERRNO (MDPMS, "RRConfigureOutputProperty error\n");
            }

            for (j = 0; j < drmmode_prop->count_enums; j++)
                if (drmmode_prop->enums[j].value == p->value)
                    break;
            /* there's always a matching value */
            errno = RRChangeOutputProperty (pOutput->randr_output, p->atoms[0],
                                          XA_ATOM, 32, PropModeReplace, 1, &p->atoms[j+1], FALSE, TRUE);
            if (errno != 0)
            {
                XDBG_ERRNO (MDPMS, "RRChangeOutputProperty error\n");
            }
        }else if (drmmode_prop->flags & DRM_MODE_PROP_IMMUTABLE)

        {
            if(strcmp(pOutput->name,"LVDS1")==0)
            {
                  /* Example
                   *
                   RRConfigureOutputProperty(out->randr_output, atom_OutputNumber,
                                 FALSE, FALSE, FALSE, 0, NULL);
                   RRChangeOutputProperty(out->randr_output, atom_OutputNumber,
                              XA_INTEGER, 32, PropModeReplace,
                              1, &num, FALSE, FALSE);

                   RRConfigureOutputProperty(out->randr_output, atom_PanningArea,
                                 FALSE, FALSE, FALSE, 0, NULL);
                   RRChangeOutputProperty(out->randr_output, atom_PanningArea,
                              XA_STRING, 8, PropModeReplace,
                              0, NULL, FALSE, FALSE);*/
                switch(i)
                {
                case 1:
                {
                    /*ConnectorType property*/
                    p->num_atoms = 1;
                    p->atoms = ctrl_calloc (p->num_atoms, sizeof (Atom));
                    if (!p->atoms)
                        continue;

                    p->atoms[0] = atom_ConnectorType;

                    errno = RRConfigureOutputProperty (pOutput->randr_output,p->atoms[0],
                            FALSE, FALSE,TRUE,1, (INT32*)&atom_LVDS );
                    if (errno != 0)
                    {
                         XDBG_ERRNO (MDPMS, "RRConfigureOutputProperty error\n");
                    }

                    /* there's always a matching value */
                    errno = RRChangeOutputProperty (pOutput->randr_output, p->atoms[0],
                            XA_ATOM, 32, PropModeReplace, 1, (INT32*)&atom_LVDS, FALSE, FALSE);
                    if (errno != 0)
                    {
                        XDBG_ERRNO (MDPMS, "RRChangeOutputProperty error\n");
                    }

                    break;
                }
                case 2:
                {
                    p->num_atoms = 1;
                    p->atoms = ctrl_calloc (p->num_atoms, sizeof (Atom));
                    if (!p->atoms)
                        continue;

                    p->atoms[0] = atom_OutputNumber;

                    errno = RRConfigureOutputProperty (pOutput->randr_output,p->atoms[0],
                            FALSE, FALSE,TRUE,0, NULL );
                    if (errno != 0)
                    {
                        XDBG_ERRNO (MDPMS, "RRConfigureOutputProperty error\n");

                    }

                    /* there's always a matching value */
                    errno = RRChangeOutputProperty (pOutput->randr_output, p->atoms[0],
                            XA_INTEGER, 32, PropModeReplace, 1, (INT32*)&pOutputPriv->mode_output->connector_id, FALSE, FALSE);
                    if (errno != 0)
                    {
                        XDBG_ERRNO (MDPMS, "RRChangeOutputProperty error\n");
                    }
                    break;
                }
                default:
                    break;
                }

            }


         }
    }
}

#define PROP_VERIFY_RR_MODE(id, ptr, a)\
    {\
    int rc = dixLookupResourceByType((pointer *)&(ptr), id,\
                                     RRModeType, serverClient, a);\
    if (rc != Success) {\
        serverClient->errorValue = id;\
        return 0;\
    }\
    }

#define FLAG_BITS (RR_HSyncPositive | \
           RR_HSyncNegative | \
           RR_VSyncPositive | \
           RR_VSyncNegative | \
           RR_Interlace | \
           RR_DoubleScan | \
           RR_CSync | \
           RR_CSyncPositive | \
           RR_CSyncNegative | \
           RR_HSkewPresent | \
           RR_BCast | \
           RR_PixelMultiplex | \
           RR_DoubleClock | \
           RR_ClockDivideBy2)


/*
 * Convert a RandR mode to a DisplayMode
 */
static void
_RRModeConvertToDisplayMode (ScrnInfoPtr    scrn,
              RRModePtr     randr_mode,
              DisplayModePtr    mode)
{
    memset(mode, 0, sizeof(DisplayModeRec));
    mode->status = MODE_OK;

    mode->Clock = randr_mode->mode.dotClock / 1000;

    mode->HDisplay = randr_mode->mode.width;
    mode->HSyncStart = randr_mode->mode.hSyncStart;
    mode->HSyncEnd = randr_mode->mode.hSyncEnd;
    mode->HTotal = randr_mode->mode.hTotal;
    mode->HSkew = randr_mode->mode.hSkew;

    mode->VDisplay = randr_mode->mode.height;
    mode->VSyncStart = randr_mode->mode.vSyncStart;
    mode->VSyncEnd = randr_mode->mode.vSyncEnd;
    mode->VTotal = randr_mode->mode.vTotal;
    mode->VScan = 0;

    mode->Flags = randr_mode->mode.modeFlags & FLAG_BITS;

    xf86SetModeCrtc (mode, scrn->adjustFlags);
}

static int
_PropUnsetCrtc (xf86OutputPtr pOutput)
{
    if (!pOutput->crtc)
        return 1;

    ScrnInfoPtr pScrn = pOutput->scrn;
    //SECOutputPrivPtr pOutputPriv = pOutput->driver_private;
    //SECModePtr pSecMode =  pOutputPriv->pSecMode;
    EXYNOSOutputPrivPtr pOutputPriv = pOutput->driver_private;

    pOutputPriv->unset_connector_type = pOutputPriv->mode_output->connector_type;
    RRGetInfo (pScrn->pScreen, TRUE);
    //pSecMode->unset_connector_type = 0;
    RRGetInfo (pScrn->pScreen, TRUE);

    return 1;
}

static int
_exynosPropSetWbClone (xf86OutputPtr pOutput, int mode_xid)
{
    EXYNOSOutputPrivPtr pOutputPriv = pOutput->driver_private;
    EXYNOSDispInfoPtr pDispInfo = pOutputPriv->pDispInfo;
    RRModePtr pRRMode;
    DisplayModeRec mode;

    XDBG_TRACE (MPROP, "pOutputPriv->mode_output->connector_type - (%i) mode_xid - (%i)\n",pOutputPriv->mode_output->connector_type,mode_xid);
    /* find kmode and set the external default mode */
    PROP_VERIFY_RR_MODE (mode_xid, pRRMode, DixSetAttrAccess);
    _RRModeConvertToDisplayMode (pOutput->scrn, pRRMode, &mode);
    exynosDisplayModeToKmode (pOutput->scrn, &pDispInfo->ext_connector_mode, &mode);

    if (pOutputPriv->mode_output->connector_type == DRM_MODE_CONNECTOR_HDMIA ||
        pOutputPriv->mode_output->connector_type == DRM_MODE_CONNECTOR_HDMIB)
    {   XDBG_TRACE (MPROP, "DISPLAY_CONN_MODE_HDMI\n");
        displaySetDispConnMode (pOutput->scrn, DISPLAY_CONN_MODE_HDMI);
        _PropUnsetCrtc (pOutput);
    }
    else if (pOutputPriv->mode_output->connector_type == DRM_MODE_CONNECTOR_VIRTUAL)
    {   XDBG_TRACE (MPROP, "DISPLAY_CONN_MODE_VIRTUAL\n");
        displaySetDispConnMode (pOutput->scrn, DISPLAY_CONN_MODE_VIRTUAL);
        _PropUnsetCrtc (pOutput);
    }
    else
    {
        ErrorF("Warning : (WB_CLONE) Not support for this connector type\n");
        return 0;
    }

    if(!displaySetDispSetMode (pOutput, DISPLAY_SET_MODE_CLONE))
    {
        return 0;
    }

    return 1;
}

static const char fake_edid_info[] = {
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x4c, 0x2d, 0x05, 0x05,
    0x00, 0x00, 0x00, 0x00, 0x30, 0x12, 0x01, 0x03, 0x80, 0x10, 0x09, 0x78,
    0x0a, 0xee, 0x91, 0xa3, 0x54, 0x4c, 0x99, 0x26, 0x0f, 0x50, 0x54, 0xbd,
    0xee, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x66, 0x21, 0x50, 0xb0, 0x51, 0x00,
    0x1b, 0x30, 0x40, 0x70, 0x36, 0x00, 0xa0, 0x5a, 0x00, 0x00, 0x00, 0x1e,
    0x01, 0x1d, 0x00, 0x72, 0x51, 0xd0, 0x1e, 0x20, 0x6e, 0x28, 0x55, 0x00,
    0xa0, 0x5a, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x18,
    0x4b, 0x1a, 0x44, 0x17, 0x00, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x00, 0x00, 0x00, 0xfc, 0x00, 0x53, 0x41, 0x4d, 0x53, 0x55, 0x4e, 0x47,
    0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0xbc, 0x02, 0x03, 0x1e, 0xf1,
    0x46, 0x84, 0x05, 0x03, 0x10, 0x20, 0x22, 0x23, 0x09, 0x07, 0x07, 0x83,
    0x01, 0x00, 0x00, 0xe2, 0x00, 0x0f, 0x67, 0x03, 0x0c, 0x00, 0x10, 0x00,
    0xb8, 0x2d, 0x01, 0x1d, 0x80, 0x18, 0x71, 0x1c, 0x16, 0x20, 0x58, 0x2c,
    0x25, 0x00, 0xa0, 0x5a, 0x00, 0x00, 0x00, 0x9e, 0x8c, 0x0a, 0xd0, 0x8a,
    0x20, 0xe0, 0x2d, 0x10, 0x10, 0x3e, 0x96, 0x00, 0xa0, 0x5a, 0x00, 0x00,
    0x00, 0x18, 0x02, 0x3a, 0x80, 0x18, 0x71, 0x38, 0x2d, 0x40, 0x58, 0x2c,
    0x45, 0x00, 0xa0, 0x5a, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x06
};

static void
_exynosPropSetVirtual (xf86OutputPtr pOutput, int sc_conn)
{
    EXYNOSPtr  pExynos = EXYNOSPTR (pOutput->scrn);
    int fd = pExynos->drm_fd;

    struct drm_exynos_vidi_connection vidi;

    if (sc_conn == 1)
    {
        vidi.connection = 1;
        vidi.extensions = 1;
        vidi.edid = (uint64_t *)fake_edid_info;
    }
    else if (sc_conn == 2)
    {
        vidi.connection = 0;
    }
    else
    {
        XDBG_WARNING (MDISP, "Warning : wrong virtual connection command\n");
        return;
    }

    ioctl (fd, DRM_IOCTL_EXYNOS_VIDI_CONNECTION, &vidi);
}

Bool
exynosPropSetDisplayMode (xf86OutputPtr pOutput, Atom property, RRPropertyValuePtr value)
{
    XDBG_RETURN_VAL_IF_FAIL(value, FALSE);
    XDBG_TRACE (MPROP, "%s\n", __FUNCTION__);

    XRROutputPropDisplayMode disp_mode = XRR_OUTPUT_DISPLAY_MODE_NULL;
    EXYNOSOutputPrivPtr pOutputPriv;


    XDBG_DEBUG (MCRTC, "output_name=%s, data=%d size=%ld\n", pOutput->name, *(int *)value->data, value->size);

    if (g_display_mode_prop_init == FALSE)
    {
        xrr_property_display_mode_atom = MakeAtom (STR_XRR_DISPLAY_MODE_PROPERTY, strlen (STR_XRR_DISPLAY_MODE_PROPERTY), TRUE);
        g_display_mode_prop_init = TRUE;
    }

    if (xrr_property_display_mode_atom != property)
    {
        //ErrorF ("[Display_mode]: Unrecognized property name.\n");
        return FALSE;
    }

    if (!value->data || value->size == 0)
    {
        //ErrorF ("[Display_mode]: Unrecognized property value.\n");
        return TRUE;
    }

    ErrorF("%s(%d): output_name=%s, data=%d size=%ld\n", __func__, __LINE__, pOutput->name, *(int *)value->data, value->size);

    disp_mode = *(int *)value->data;

    if (disp_mode == XRR_OUTPUT_DISPLAY_MODE_NULL)
        return TRUE;

    ErrorF("%s(%d): output_name=%s, disp_mode=%d\n", __func__, __LINE__, pOutput->name, disp_mode);

    pOutputPriv = pOutput->driver_private;

    int mode_xid;
    switch (disp_mode)
    {
        case XRR_OUTPUT_DISPLAY_MODE_WB_CLONE:
            mode_xid = *((int *)value->data+1);
            ErrorF ("[DISPLAY_MODE]: Set WriteBack Clone\n");
            _exynosPropSetWbClone (pOutput, mode_xid);
            pOutputPriv->disp_mode = disp_mode;
            break;
        default:
            break;
    }

    return TRUE;
}

extern void debug_out();

static Bool
EXYNOSOutputSetProperty (xf86OutputPtr output, Atom property,
                     RRPropertyValuePtr value)
{
    char *prop_name= NameForAtom(property);
    XDBG_INFO (MOUTP, "EXYNOSOutputSetProperty out %s prop %s val %i\n",output->name,prop_name,*(uint32_t*)value->data);
    EXYNOSOutputPrivPtr pOutputPriv = output->driver_private;
    EXYNOSDispInfoPtr pDispInfo = pOutputPriv->pDispInfo;
    int i;

    for (i = 0; i < pOutputPriv->num_props; i++)
    {
        EXYNOSPropertyPtr p = &pOutputPriv->props[i];
        /***** Protection from immutable properties *****/
        if (p->mode_prop->flags & DRM_MODE_PROP_IMMUTABLE)
           continue;
        /************************************************/
        if (p->atoms[0] != property)
            continue;
        if(strcmp(prop_name,STR_OUR_PROPERTY)==0){
            //we can do some USEFUL work
            debug_out();
            XDBG_INFO (MPROP, "set property %s to %s. atom value-%i\n",prop_name,NameForAtom(*(uint32_t*)value->data),*(uint32_t*)value->data);
            };


        if (p->mode_prop->flags & DRM_MODE_PROP_RANGE)
        {
            uint32_t val;

            if (value->type != XA_INTEGER || value->format != 32 ||
                    value->size != 1)
                return FALSE;
            val = *(uint32_t*)value->data;
            /******* Control from xrandr property "DEBUG_COMMAND" **************************/
            if ((strcmp(prop_name,"DEBUG_COMMAND")==0) && ((strcmp(output->name,"LVDS1")==0)|(strcmp(output->name,"Virtual1")==0)))
            {
                switch(val)
                {
                case 2:
                {
                    /* int pid;
                     if ((pid=fork()) == 0) {*/
                    /* This is the child process */
                    /*execl("/usr/bin/xclock", "xclock","", (char *)NULL);
                     }*/
                    system("./usr/bin/xclock &");
                    break;
                }
                case 3:
                {
                    // displaySetDispSetMode (output, DISPLAY_SET_MODE_VIRTUAL);
                    XDBG_INFO (MDISP, "[LVDS_FUNC]: Set virtual output \n");
                    _exynosPropSetVirtual (output, 1);
                    break;
                }
                case 4:
                {
                    // displaySetDispSetMode (output, DISPLAY_SET_MODE_VIRTUAL);
                    XDBG_INFO (MDISP, "[LVDS_FUNC]: Unset virtual output \n");
                    _exynosPropSetVirtual (output, 2);
                    break;
                }
                default:
                    break;
                }

            }
            /******************************************************************************/

            drmModeConnectorSetProperty (pDispInfo->drm_fd, pOutputPriv->output_id,
                                         p->mode_prop->prop_id, (uint64_t) val);

            return TRUE;
        }
        else if (p->mode_prop->flags & DRM_MODE_PROP_ENUM)
        {
            Atom atom;
            const char *name;
            int j;

            if (value->type != XA_ATOM || value->format != 32 || value->size != 1)
                return FALSE;
            memcpy (&atom, value->data, 4);
            name = NameForAtom (atom);

            /* search for matching name string, then set its value down */
            for (j = 0; j < p->mode_prop->count_enums; j++)
            {
                if (!strcmp (p->mode_prop->enums[j].name, name))
                {

                    drmModeConnectorSetProperty (pDispInfo->drm_fd, pOutputPriv->output_id,
                                                 p->mode_prop->prop_id, p->mode_prop->enums[j].value);

                    return TRUE;
                }
            }
            return FALSE;
        }
    }


#if 1
    /* We didn't recognise this property, just report success in order
     * to allow the set to continue, otherwise we break setting of
     * common properties like EDID.
     */
    /* set the hidden properties : features for sec debugging*/
    /* TODO : xberc can works on only LVDS????? */
    if (pOutputPriv->mode_output->connector_type == DRM_MODE_CONNECTOR_LVDS)
    {
        if (exynosPropSetLvdsFunc (output, property, value))
            return TRUE;

      /*  if (secPropSetFbVisible (output, property, value))
            return TRUE;

        if (secPropSetVideoOffset (output, property, value))
            return TRUE;

        if (secXbercSetProperty (output, property, value))
            return TRUE;*/
    }
    /* set the hidden properties : features for driver specific funtions */
    if (pOutputPriv->mode_output->connector_type == DRM_MODE_CONNECTOR_HDMIA ||
        pOutputPriv->mode_output->connector_type == DRM_MODE_CONNECTOR_HDMIB ||
        pOutputPriv->mode_output->connector_type == DRM_MODE_CONNECTOR_VIRTUAL)
    {
        /* set the property for the display mode */
        if (exynosPropSetDisplayMode(output, property, value))
            return TRUE;
    }
#endif

    return TRUE;
}

static Bool
EXYNOSOutputGetProperty (xf86OutputPtr pOutput, Atom property)
{
    char *prop_name= NameForAtom(property);
    EXYNOSOutputPrivPtr pOutputPriv = pOutput->driver_private;

    int err = BadValue;
    union exynosPropertyData val;

    XDBG_INFO (MOUTP, "EXYNOSOutputGetProperty out %s prop %s \n",pOutput->name,prop_name);

    if (property == atom_Backlight) {
      /*  if (rout->Output->Property == NULL)
                return FALSE;

            if (!rout->Output->Property(rout->Output, rhdPropertyGet,
                            RHD_OUTPUT_BACKLIGHT, &val))
                return FALSE;
            err = RRChangeOutputProperty(out->randr_output, atom_Backlight,
                             XA_INTEGER, 32, PropModeReplace,
                             1, &val.integer, FALSE, FALSE);
        */
    } else if (property == atom_ConnectorType)
       {

         /*Function to get property with val*/

        err = RRChangeOutputProperty (pOutput->randr_output, atom_ConnectorType,
                                      XA_ATOM, 32, PropModeReplace, 1, (INT32*)&atom_LVDS, FALSE, FALSE);

       }

       if (err != 0)
            return FALSE;
       return TRUE;
}

static const xf86OutputFuncsRec exynos_output_funcs =
{
    .create_resources = EXYNOSOutputCreateResources,
    .set_property = EXYNOSOutputSetProperty,
    .get_property = EXYNOSOutputGetProperty,
    .dpms = EXYNOSOutputDpms,
    .detect = EXYNOSOutputDetect,
    .mode_valid = EXYNOSOutputModeValid,
    .get_modes = EXYNOSOutputGetModes,
    .destroy = EXYNOSOutputDestroy,
    .mode_set = EXYNOSOutputModeSet,
#if 0
    .save = NULL,
    .restore = NULL,
    .mode_fixup = NULL,
    .prepare = NULL,
    .mode_set = NULL,
    .commit = NULL,
    .get_crtc = NULL,
#endif
};

void
exynosOutputInit (ScrnInfoPtr pScrn, EXYNOSDispInfoPtr pDispInfo, int num)
{
    xf86OutputPtr pOutput;
    EXYNOSOutputPrivPtr pOutputPriv;
    drmModeConnectorPtr koutput;
    drmModeEncoderPtr kencoder;
    const char *output_name;
    char name[32];

    koutput = drmModeGetConnector (pDispInfo->drm_fd,
                                   pDispInfo->mode_res->connectors[num]);
    if (!koutput)
        return;

    kencoder = drmModeGetEncoder (pDispInfo->drm_fd, koutput->encoders[0]);
    if (!kencoder)
    {
        drmModeFreeConnector (koutput);
        return;
    }

    if (koutput->connector_type < ARRAY_SIZE (output_names))
        output_name = output_names[koutput->connector_type];
    else
        output_name = "UNKNOWN";
    snprintf (name, 32, "%s%d", output_name, koutput->connector_type_id);

    pOutput = xf86OutputCreate (pScrn, &exynos_output_funcs, name);
    if (!pOutput)
    {
        drmModeFreeEncoder (kencoder);
        drmModeFreeConnector (koutput);
        return;
    }

    pOutputPriv = ctrl_calloc (1, sizeof (EXYNOSOutputPrivRec));
    if (!pOutputPriv)
    {
        xf86OutputDestroy (pOutput);
        drmModeFreeConnector (koutput);
        drmModeFreeEncoder (kencoder);
        return;
    }
    pOutput->driver_private = pOutputPriv;

    pOutputPriv->pOutput = pOutput;
    pOutputPriv->pDispInfo = pDispInfo;
    pOutputPriv->output_id = pDispInfo->mode_res->connectors[num];
    pOutputPriv->mode_output = koutput;
    pOutputPriv->mode_encoder = kencoder;

    pOutput->mm_width = koutput->mmWidth;
    pOutput->mm_height = koutput->mmHeight;
    pOutput->subpixel_order = subpixel_conv_table[koutput->subpixel];
    pOutput->possible_crtcs = kencoder->possible_crtcs;
    pOutput->possible_clones = kencoder->possible_clones;
    pOutput->interlaceAllowed = TRUE;

    xorg_list_add(&pOutputPriv->link, &pDispInfo->outputs);
    XDBG_INFO (MOUTP, "count_prop:%i\n",koutput->count_props);
    XDBG_INFO (MOUTP, "create output name:%s\n",name);
}

Bool
outputDrmUpdate (ScrnInfoPtr pScrn)
{
    EXYNOSDispInfoPtr pExynosMode = (EXYNOSDispInfoPtr) EXYNOSPTR (pScrn)->pDispInfo;
    Bool ret = TRUE;
    int i;

    for (i = 0; i < pExynosMode->mode_res->count_connectors; i++)
    {
        EXYNOSOutputPrivPtr pOutputPriv = NULL,pCur, pNext;
        drmModeConnectorPtr koutput;
        drmModeEncoderPtr kencoder;
        char *conn_str[] = {"connected", "disconnected", "unknow"};

        xorg_list_for_each_entry_safe (pCur, pNext, &pExynosMode->outputs, link)
        {
            if (pCur->output_id == pExynosMode->mode_res->connectors[i])
            {
                pOutputPriv = pCur;
                break;
            }
        }

        if (!pOutputPriv)
        {
            ret = FALSE;
            break;
        }

        koutput = drmModeGetConnector (pExynosMode->drm_fd,
                pExynosMode->mode_res->connectors[i]);
        if (!koutput)
        {
            ret = FALSE;
            break;
        }

        kencoder = drmModeGetEncoder (pExynosMode->drm_fd, koutput->encoders[0]);
        if (!kencoder)
        {
            drmModeFreeConnector (koutput);
            ret = FALSE;
            break;
        }

        if (pOutputPriv->mode_output)
        {
            drmModeFreeConnector (pOutputPriv->mode_output);
            pOutputPriv->mode_output = NULL;
        }
        pOutputPriv->mode_output = koutput;

        if (pOutputPriv->mode_encoder)
        {
            drmModeFreeEncoder (pOutputPriv->mode_encoder);
            pOutputPriv->mode_encoder = NULL;
        }
        pOutputPriv->mode_encoder = kencoder;

        XDBG_INFO (MOUTP, "drm update : connect(%d, type:%d, status:%s) encoder(%d) crtc(%d).\n",
                   pExynosMode->mode_res->connectors[i], koutput->connector_type,
                   conn_str[pOutputPriv->mode_output->connection-1],
                   kencoder->encoder_id, kencoder->crtc_id);
#if 0
        /* Does these need to update? */
        pOutput->mm_width = koutput->mmWidth;
        pOutput->mm_height = koutput->mmHeight;

        pOutput->possible_crtcs = kencoder->possible_crtcs;
        pOutput->possible_clones = kencoder->possible_clones;
#endif
    }

    if (!ret)
        xf86DrvMsg (pScrn->scrnIndex, X_ERROR, "drm(output) update error. (%s)\n", strerror (errno));

    return ret;
}




Bool
exynosPropSetLvdsFunc (xf86OutputPtr pOutput, Atom property, RRPropertyValuePtr value)
{
    XDBG_TRACE (MPROP, "%s\n", __FUNCTION__);
    XDBG_RETURN_VAL_IF_FAIL(value, FALSE);

    XRROutputPropLvdsFunc lvds_func = XRR_OUTPUT_LVDS_FUNC_NULL;

    if (g_lvds_func_prop_init == FALSE)
    {
        xrr_property_lvds_func_atom = MakeAtom (STR_XRR_LVDS_FUNCTION, strlen (STR_XRR_LVDS_FUNCTION), TRUE);
        g_lvds_func_prop_init = TRUE;
    }

    if (xrr_property_lvds_func_atom != property)
    {
        return FALSE;
    }

    if (!value->data || value->size == 0)
    {
        //ErrorF ("[Display_mode]: Unrecognized property value.\n");
        return TRUE;
    }

    XDBG_DEBUG (MDISP, "output_name=%s, data=%d size=%ld\n", pOutput->name, *(int *)value->data, value->size);

    lvds_func = *(int *)value->data;

    if (lvds_func == XRR_OUTPUT_LVDS_FUNC_NULL)
        return TRUE;

    XDBG_DEBUG (MDISP, "output_name=%s, lvds_func=%d\n", pOutput->name, lvds_func);

    int sc_conn;
    switch (lvds_func)
    {
        case XRR_OUTPUT_LVDS_FUNC_INIT_VIRTUAL:
            sc_conn = *((int *)value->data+1);
            XDBG_INFO (MDISP, "[LVDS_FUNC]: set virtual output (%d)\n", sc_conn);
            _exynosPropSetVirtual (pOutput, sc_conn);
            break;
        case XRR_OUTPUT_LVDS_FUNC_HIBERNATION:
            XDBG_INFO (MDISP, "[LVDS_FUNC]: set hibernationn\n");
            break;
        case XRR_OUTPUT_LVDS_FUNC_ACCESSIBILITY:
            XDBG_INFO (MDISP, "[LVDS_FUNC]: set accessibility\n");
            break;
        default:
            break;
    }

    return TRUE;
}


