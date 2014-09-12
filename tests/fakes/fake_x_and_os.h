#ifndef FAKE_X_AND_OS_H_
#define FAKE_X_AND_OS_H_

#define XACE_NUM_HOOKS	17

extern void tbm_bo_forTests( tbm_bo bo, unsigned int val );

CallbackListPtr XaceHooks[XACE_NUM_HOOKS];

unsigned int sp;

ClientPtr clients[MAXCLIENTS];

ScreenInfo screenInfo;

ScrnInfoPtr *xf86Screens;

RESTYPE RRModeType;

ClientPtr serverClient;

BoxRec RegionEmptyBox;

RegDataRec RegionEmptyData;

CallbackListPtr DPMSCallback;

int xf86CrtcConfigPrivateIndex;

CallbackListPtr ClientStateCallback;

typedef struct TimerRec
{
  int field;
} _OsTimerRec;

_OsTimerRec some_value;

typedef struct {
    CARD16	control B16;	/* control type		*/
    CARD16	length B16;	/* control length		*/
} xDeviceCtl;

typedef struct _InputDriverRec {
    int driverVersion;
    const char *driverName;
    void (*Identify) (int flags);
    int (*PreInit) (struct _InputDriverRec * drv,
                    struct _InputInfoRec * pInfo, int flags);
    void (*UnInit) (struct _InputDriverRec * drv,
                    struct _InputInfoRec * pInfo, int flags);
    pointer module;
    const char **default_options;
} InputDriverRec, *InputDriverPtr;

typedef struct _InputInfoRec {
    struct _InputInfoRec *next;
    char *name;
    char *driver;

    int flags;

    Bool (*device_control) (DeviceIntPtr device, int what);
    void (*read_input) (struct _InputInfoRec * local);
    int (*control_proc) (struct _InputInfoRec * local, xDeviceCtl * control);
    int (*switch_mode) (ClientPtr client, DeviceIntPtr dev, int mode);
    int (*set_device_valuators)
     (struct _InputInfoRec * local,
      int *valuators, int first_valuator, int num_valuators);

    int fd;
    DeviceIntPtr dev;
    pointer private;
    const char *type_name;
    InputDriverPtr drv;
    pointer module;
    XF86OptionPtr options;
    InputAttributes *attrs;
} *InputInfoPtr;

#endif /* FAKE_X_AND_OS_H_ */
