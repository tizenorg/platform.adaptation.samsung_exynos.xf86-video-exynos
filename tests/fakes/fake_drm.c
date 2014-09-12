#include "../../src/sec.h"

#include "fake_drm.h"
#include "fake_mem.h"

extern unsigned ctrl_get_cnt_mem_obj(type_mem_object type);
extern void ctrl_print_mem();

extern void* find_mem_obj_by_func( cmp_func cmp, int key );

extern void ctrl_free(void* ptr);

static int buf_cnt=0;

int drmOpen(const char *name, const char *busid)
{
	return 0;
}

int drmClose(int fd)
{
	return 0;
}

void drmCloseOnce(int fd)
{

}

void drmModeFreeFB( drmModeFBPtr ptr )
{

}

char *drmGetDeviceNameFromFd(int fd)
{
	return NULL;
}

int drmModeDirtyFB(int fd, uint32_t bufferId,
			  drmModeClipPtr clips, uint32_t num_clips)
{
	return 0;
}

int drmCtlInstHandler(int fd, int irq)
{
	return 0;
}

int _add_fb(int bo) 
{
    drm_obj* dobj=calloc(1,sizeof(drm_obj)) ;
    dobj->ref_cnt=1;
    dobj->buf_id=++buf_cnt;
    dobj->type=TYPE_DRM_FB;
    dobj->data=(void*)bo;
    _add_calloc( dobj, DRM_OBJ, 1, sizeof( drm_obj ) );
    return (int)buf_cnt;
}

int drmModeAddFB(int fd, uint32_t width, uint32_t height, uint8_t depth,
        uint8_t bpp, uint32_t pitch, uint32_t bo_handle, uint32_t *buf_id)
{
	if( 100 == fd )
		return 1;
	if( 101 == fd || 102 == fd )
	{
		*buf_id = 1;
		return 0;
	}
	return 0;
}

int drmModeAddFB2( int fd, uint32_t width, uint32_t height,
        uint32_t pixel_format, uint32_t bo_handles[4], uint32_t pitches[4],
        uint32_t offsets[4], uint32_t *buf_id, uint32_t flags )
{
    return _add_fb( 0 );
}

static int cmp_drm_fb_func (void*ptr,int key)
{
#ifdef DEBUG_VIEW
    printf("buf_id %i type %i bo %i\n",((drm_obj*)ptr)->buf_id,((drm_obj*)ptr)->type,((drm_obj*)ptr)->data);
#endif
    drm_obj* dro=(drm_obj*)ptr;
    return (dro->buf_id==key)&&(dro->type==TYPE_DRM_FB);
}

int drmModeRmFB(int fd, uint32_t bufferId) 
{
    drm_obj* co= (drm_obj*)find_mem_obj_by_func(cmp_drm_fb_func,bufferId);
    if(co)
        ctrl_free(co);
    return (int)1;
}

int drmCommandWriteRead(int fd, unsigned long drmCommandIndex, void *data,
        unsigned long size) 
{
	if( 100 == fd  )
		return 1;
	return 0;
}


int drmIoctl(int fd, unsigned long request, void *arg) 
{
    
    return 0;
}

int drmWaitVBlank(int fd, drmVBlankPtr vbl) 
{
	return 0;
}

int drmModePageFlip(int fd, uint32_t crtc_id, uint32_t fb_id, uint32_t flags,
        void *user_data) 
{
	return (int)0;
}

int drmModeSetCrtc(int fd, uint32_t crtcId, uint32_t bufferId, uint32_t x,
        uint32_t y, uint32_t *connectors, int count, drmModeModeInfoPtr mode) 
{
	return 0;
}

static int cmp_drm_planeres_func (void*ptr,int key)
{
#ifdef DEBUG_VIEW
    printf("buf_id %i type %i bo %i\n",((drm_obj*)ptr)->buf_id,((drm_obj*)ptr)->type,((drm_obj*)ptr)->data);
#endif
    drm_obj* dro=(drm_obj*)ptr;
    return ((dro->data==(void*)key)&&(dro->type==TYPE_DRM_PLANE_RES));
}

//----------------PlaneResources----------------------
drmModePlaneResPtr drmModeGetPlaneResources(int fd) 
{
    drmModePlaneResPtr ptr=calloc(1,sizeof(drmModePlaneRes));
    drm_obj* dobj=calloc(1,sizeof(drm_obj));
    dobj->type=TYPE_DRM_PLANE_RES;
    dobj->data=ptr;
    _add_calloc( dobj, DRM_OBJ, 1, sizeof( drm_obj ) );
    return ptr;
}



void drmModeFreePlaneResources(drmModePlaneResPtr ptr) 
{
    drm_obj* co= (drm_obj*)find_mem_obj_by_func(cmp_drm_planeres_func,(int)ptr);
    if(co)
        ctrl_free(co);
}

static int cmp_drm_res_func (void*ptr,int key)
{
#ifdef DEBUG_VIEW
    printf("buf_id %i type %i bo %i\n",((drm_obj*)ptr)->buf_id,((drm_obj*)ptr)->type,((drm_obj*)ptr)->data);
#endif
    drm_obj* dro=(drm_obj*)ptr;
    return (dro->data==(void*)key)&&(dro->type==TYPE_DRM_RES);
}

drmModeResPtr drmModeGetResources(int fd) 
{
    drmModeResPtr ptr=calloc(1, sizeof(drmModeRes ));
    drm_obj* dobj=calloc(1,sizeof(drm_obj));
    dobj->type=TYPE_DRM_RES;
    dobj->data=ptr;
    _add_calloc( dobj, DRM_OBJ, 1, sizeof( drm_obj ) );
    return ptr;
}
void drmModeFreeResources(drmModeResPtr ptr) 
{
    drm_obj* co= (drm_obj*)find_mem_obj_by_func(cmp_drm_res_func,(int)ptr);
    if(co)
        ctrl_free(co);
}

static int cmp_drm_crtcmode_func(void*ptr, int key) 
{
#ifdef DEBUG_VIEW
    printf("buf_id %i type %i bo %i\n",((drm_obj*)ptr)->buf_id,((drm_obj*)ptr)->type,((drm_obj*)ptr)->data);
#endif
    drm_obj* dro = (drm_obj*) ptr;
    return (dro->data == (void*)key) && (dro->type == TYPE_DRM_CRTC_MODE);
}

#define NUM_CRTC 10

drmModeCrtcPtr drmModeGetCrtc(int fd, uint32_t crtcId) 
{
    drmModeResPtr ptr = calloc(1, NUM_CRTC);
    drm_obj* dobj = calloc(1, sizeof(drm_obj));
    dobj->type = TYPE_DRM_CRTC_MODE;
    dobj->data = ptr;
    _add_calloc( dobj, DRM_OBJ, 1, sizeof( drm_obj ) );
    return (drmModeCrtcPtr)ptr;
}

void drmModeFreeCrtc( drmModeCrtcPtr ptr )
{
    drm_obj* co = (drm_obj*)find_mem_obj_by_func(cmp_drm_crtcmode_func, (int)ptr);
    if (co)
        ctrl_free(co);
}

static int cmp_drm_props_func(void*ptr, int key) 
{
#ifdef DEBUG_VIEW
    printf("buf_id %i type %i bo %i\n",((drm_obj*)ptr)->buf_id,((drm_obj*)ptr)->type,((drm_obj*)ptr)->data);
#endif
    drm_obj* dro = (drm_obj*) ptr;
    return (dro->data == (void*)key) && (dro->type == TYPE_DRM_PROPS);
}


#define NUM_PROPS 10

drmModeObjectPropertiesPtr drmModeObjectGetProperties(int fd,
        uint32_t object_id, uint32_t object_type) 
{
    drmModeObjectPropertiesPtr prop = calloc(1,sizeof(drmModeObjectProperties));
    prop->props = ctrl_calloc( 1, sizeof( uint32_t ) );
    //create drmModeObjectProperties obj
    drm_obj* dobj_props = calloc(1, sizeof(drm_obj));

    dobj_props->type = TYPE_DRM_PROPS;
    dobj_props->data = prop;
    _add_calloc( dobj_props, DRM_OBJ, 1, sizeof( drm_obj ) );//add drmModeObjectProperties obj

    prop->count_props = NUM_PROPS;

    return prop;
}



int drmModeObjectSetProperty(int fd, uint32_t object_id, uint32_t object_type,
        uint32_t property_id, uint64_t value) 
{
    if( 100 == fd )
      return -1;
    return 0;
}

void drmModeFreeObjectProperties(drmModeObjectPropertiesPtr ptr) 
{
    drm_obj* co = (drm_obj*)find_mem_obj_by_func(cmp_drm_props_func, (int)ptr);
    if (co)
        ctrl_free(co);
}

drmModePropertyPtr drmModeGetProperty(int fd, uint32_t propertyId) 
{
    drmModePropertyPtr prop=ctrl_calloc(1,sizeof(drmModePropertyRes));
    prop->prop_id=123;
    strcpy(prop->name,"zpos");
    return prop;
}

void drmModeFreeProperty(drmModePropertyPtr ptr) 
{

}

Bool RRCrtcGammaSet(RRCrtcPtr crtc, CARD16 *red, CARD16 *green, CARD16 *blue) 
{
	return FALSE;
}

static int cmp_drm_plane_func(void*ptr, int key) 
{
#ifdef DEBUG_VIEW
    printf("buf_id %i type %i bo %i\n",((drm_obj*)ptr)->buf_id,((drm_obj*)ptr)->type,((drm_obj*)ptr)->data);
#endif
    drm_obj* dro = (drm_obj*) ptr;
    return (dro->data == (void*)key) && (dro->type == TYPE_DRM_PLANE);
}

drmModePlanePtr drmModeGetPlane(int fd, uint32_t plane_id) 
{

	if( fd == 100 )
		return NULL;

    drmModePlanePtr ptr = calloc(1, sizeof(drmModePlane));
    drm_obj* dobj = calloc(1, sizeof(drm_obj));
    dobj->type = TYPE_DRM_PLANE;
    dobj->data = ptr;
    _add_calloc( dobj, DRM_OBJ, 1, sizeof( drm_obj ) );
    return ptr;
}

void drmModeFreePlane(drmModePlanePtr ptr)
{
    drm_obj* co = (drm_obj*)find_mem_obj_by_func(cmp_drm_plane_func, (int)ptr);
    if (co)
        ctrl_free(co);
}

static int cmp_drm_enc_func(void*ptr, int key) 
{
#ifdef DEBUG_VIEW
    printf("buf_id %i type %i bo %i\n",((drm_obj*)ptr)->buf_id,((drm_obj*)ptr)->type,((drm_obj*)ptr)->data);
#endif
    drm_obj* dro = (drm_obj*) ptr;
    return ((int)dro->data == key) && (dro->type == TYPE_DRM_ENCODER);
}

drmModeEncoderPtr drmModeGetEncoder(int fd, uint32_t encoder_id) 
{
    drmModeEncoderPtr ptr = calloc(1, sizeof(drmModeEncoder));
    drm_obj* dobj = calloc(1, sizeof(drm_obj));
    dobj->type = TYPE_DRM_ENCODER;
    dobj->data = ptr;
    _add_calloc( dobj, DRM_OBJ, 1, sizeof( drm_obj ) );
    return ptr;
}

void drmModeFreeEncoder(drmModeEncoderPtr ptr) 
{
    drm_obj* co = (drm_obj*)find_mem_obj_by_func(cmp_drm_enc_func, (int)ptr);
    if (co)
        ctrl_free(co);
}

int drmModeCrtcSetGamma(int fd, uint32_t crtc_id, uint32_t size,
                   uint16_t *red, uint16_t *green, uint16_t *blue)
{
	return 0;
}

static int cmp_drm_conn_func(void*ptr, int key) 
{
#ifdef DEBUG_VIEW
    printf("buf_id %i type %i bo %i\n",((drm_obj*)ptr)->buf_id,((drm_obj*)ptr)->type,((drm_obj*)ptr)->data);
#endif
    drm_obj* dro = (drm_obj*) ptr;
    return ((int)dro->data == key) && (dro->type == TYPE_DRM_CONNECTOR);
}
drmModeConnectorPtr drmModeGetConnector(int fd, uint32_t connectorId)
{
    drmModeConnectorPtr ptr = calloc(1, sizeof(drmModeConnector));
        drm_obj* dobj = calloc(1, sizeof(drm_obj));
        dobj->type = TYPE_DRM_CONNECTOR;
        dobj->data = ptr;
        _add_calloc( dobj, DRM_OBJ, 1, sizeof( drm_obj ) );
        return ptr;

}
void drmModeFreeConnector( drmModeConnectorPtr ptr )
{
    drm_obj* co = ( drm_obj*)find_mem_obj_by_func(cmp_drm_conn_func, (int)ptr);
      if (co)
          ctrl_free(co);
}

drmModePropertyBlobPtr drmModeGetPropertyBlob(int fd, uint32_t blob_id)
{
	return NULL;
}

drmModeFBPtr drmModeGetFB(int fd, uint32_t bufferId)
{
	return NULL;
}

int drmModeConnectorSetProperty(int fd, uint32_t connector_id, uint32_t property_id,
                    uint64_t value)
{
	return 0;
}

void drmModeFreePropertyBlob(drmModePropertyBlobPtr ptr)
{

}

Fixed_test fixed_t;
Crtc_test crtc_t;

int drmModeSetPlane(int fd, uint32_t plane_id, uint32_t crtc_id,
		    uint32_t fb_id, uint32_t flags,
		    int32_t crtc_x, int32_t crtc_y,
		    uint32_t crtc_w, uint32_t crtc_h,
		    uint32_t src_x, uint32_t src_y,
		    uint32_t src_w, uint32_t src_h)
{
	if( 100 == fd || 101 == fd )
		return 1;
	return 0;
}

unsigned cnt_fb_check_refs ()
{
    return ctrl_get_cnt_mem_obj(DRM_OBJ);
}


void create_crtc(unsigned num)
{

}


unsigned get_crtc_cnt(void )
{
  return 0;
}

void print_drm(drm_obj*bo) 
{
    char *s;
    switch (bo->type) {
    case TYPE_DRM_FB:
        s = "TYPE_DRM_FB";
        break;
    case TYPE_DRM_CONNECTOR:
        s = "TYPE_DRM_CONNECTOR";
        break;
    case TYPE_DRM_ENCODER:
        s = "TYPE_DRM_ENCODER";
        break;
    case TYPE_DRM_PLANE:
        s = "TYPE_DRM_PLANE";
        break;
    case TYPE_DRM_PLANE_RES:
        s = "TYPE_DRM_PLANE_RES";
        break;
    case TYPE_DRM_RES:
        s = "TYPE_DRM_RES";
        break;
    case TYPE_DRM_CRTC_MODE:
        s = "TYPE_DRM_CRTC_MODE";
        break;
    case TYPE_DRM_PROPS:
        s = "TYPE_DRM_PROPS";
        break;
    case TYPE_DRM_PROP:
        s = "TYPE_DRM_PROP";
        break;
    case TYPE_DRM_PROP_VAL:
        s = "TYPE_DRM_PROP_VAL";
        break;
    default:
        s = "UNKOWN";
        break;
    };
    printf(" buf_id (%x) type(%s) ", bo->buf_id, s);
}

void drm_test()
{


    int a = _add_fb(12345);
#if 0
    printf("test add fb\n");
    a = _add_fb(56789);
    ctrl_print_mem();
    printf("test rm fb\n");
    drmModeRmFB(0,a);
    ctrl_print_mem();

    printf("test GetPlaneResources\n");
    a = drmModeGetPlaneResources(12345);
    a = drmModeGetPlaneResources(56789);
    ctrl_print_mem();
    printf("test FreePlaneResources\n");
    drmModeFreePlaneResources(a);
    ctrl_print_mem();

    printf("test GetResources\n");
    a = drmModeGetResources(12345);
    a = drmModeGetResources(56789);
    ctrl_print_mem();
    printf("test FreePlaneResources\n");
    drmModeFreeResources(a);
    ctrl_print_mem();

    printf("test drmModeGetCrtc\n");
    a = drmModeGetCrtc(12345,1);
    a = drmModeGetCrtc(56789,0);
    ctrl_print_mem();
    printf("test drmModeFreeCrtc\n");
    drmModeFreeCrtc(a);
    ctrl_print_mem();
#endif
    printf("test drmModeObjectPropertiesPtr\n");
    a = (int)drmModeObjectGetProperties(0, 0, 9);
    a = (int)drmModeObjectGetProperties(0, 0, 9);
    ctrl_print_mem();
    printf("test drmModeFreeObjectProperties\n");
    drmModeFreeObjectProperties((drmModeObjectPropertiesPtr)a);
    ctrl_print_mem();

    printf("test drmModeGetConnector\n");
    a = (int)drmModeGetConnector(0,  9);
    a = (int)drmModeGetConnector(0,  9);
    ctrl_print_mem();
    printf("test drmModeGetConnector\n");
    drmModeFreeConnector((drmModeConnectorPtr)a);
    ctrl_print_mem();

    printf("test drmModeGetPlane\n");
    a = (int)drmModeGetPlane(0, 9);
    a = (int)drmModeGetPlane(0, 9);
    ctrl_print_mem();
    printf("test drmModeGetPlane\n");
    drmModeFreePlane((drmModePlanePtr)a);
    ctrl_print_mem();

    printf("test drmModeGetEncoder\n");
    a = (int)drmModeGetEncoder(0,  9);
    a = (int)drmModeGetEncoder(0,  9);
    ctrl_print_mem();
    printf("test drmModeGetEncoder\n");
    drmModeFreeEncoder((drmModeEncoderPtr)a);
    ctrl_print_mem();


}
