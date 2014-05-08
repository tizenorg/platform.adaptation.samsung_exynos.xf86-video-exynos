/*
 * drm.c
 *
 *  Created on: Sep 24, 2013
 *      Author: svoloshynov
 */

#include "exynos_driver.h"
#include <exynos_drm.h>
#include "mem.h"
//---------------------------------------------------------------------------------------------
// lib_drm mock
//---------------------------------------------------------------------------------------------


//-----------------------------------------------------------------

typedef enum{
    TYPE_DRM_FB,
    TYPE_DRM_CONNECTOR,
    TYPE_DRM_ENCODER,
    TYPE_DRM_PLANE,
    TYPE_DRM_PLANE_RES,
    TYPE_DRM_RES,
    TYPE_DRM_CRTC_MODE,
    TYPE_DRM_PROPS,
    TYPE_DRM_PROP,
    TYPE_DRM_PROP_VAL}type_drm_obj;

typedef struct _drm_obj{
  int ref_cnt;
  int buf_id;
  type_drm_obj type;
  void *data;
}drm_obj;

static int buf_cnt=0;


int _add_fb(int bo) {
    drm_obj* dobj=calloc(1,sizeof(drm_obj)) ;
    dobj->ref_cnt=1;
    dobj->buf_id=++buf_cnt;
    dobj->type=TYPE_DRM_FB;
    dobj->data=(void*)bo;
    _add_calloc(dobj, DRM_OBJ);
    return buf_cnt;
}

int drmModeAddFB(int fd, uint32_t width, uint32_t height, uint8_t depth,
        uint8_t bpp, uint32_t pitch, uint32_t bo_handle, uint32_t *buf_id)
{
	if( 11 == fd || 12 == fd )
		return 123;
	*buf_id = _add_fb(bo_handle);
    return 0;
}



int drmModeAddFB2(int fd, uint32_t width, uint32_t height,
        uint32_t pixel_format, uint32_t bo_handles[4], uint32_t pitches[4],
        uint32_t offsets[4], uint32_t *buf_id, uint32_t flags) {
    *buf_id = _add_fb(bo_handles[0]);
    return 0;
}


static int cmp_drm_fb_func (void*ptr,int key){
#ifdef DEBUG_VIEW
    printf("buf_id %i type %i bo %i\n",((drm_obj*)ptr)->buf_id,((drm_obj*)ptr)->type,((drm_obj*)ptr)->data);
#endif
    drm_obj* dro=(drm_obj*)ptr;
    return (dro->buf_id==key)&&(dro->type==TYPE_DRM_FB);
}


int drmModeRmFB(int fd, uint32_t bufferId) {
    drm_obj* co= find_mem_obj_by_func(cmp_drm_fb_func,bufferId);
    if(co)
        ctrl_free(co);
}


struct drm_exynos_gem_create stored_arg;
struct drm_exynos_gem_userptr stored_userptr;
struct drm_exynos_gem_mmap stored_arg_map;
struct drm_exynos_gem_create *parg;
struct drm_exynos_gem_userptr *puserptr;
struct drm_exynos_gem_mmap *parg_map;
int drmCommandWriteRead(int fd, unsigned long drmCommandIndex, void *data,
        unsigned long size) {
	switch( drmCommandIndex ) {
	case DRM_EXYNOS_GEM_CREATE:
		parg = (struct drm_exynos_gem_create *)data;
        parg->handle = 12;
        parg->size = 34;
        memcpy(&stored_arg,data,sizeof(struct drm_exynos_gem_create));
		if( fd < 0 )
			return -1;
		break;
	case DRM_EXYNOS_GEM_MMAP:
		parg_map = (struct drm_exynos_gem_mmap *)data;
		parg_map->mapped = 2;
		memcpy(&stored_arg_map,data,sizeof(struct drm_exynos_gem_mmap));
		if( fd < 0 )
			return -1;
		break;
	case DRM_EXYNOS_GEM_USERPTR:
		puserptr = (struct drm_exynos_gem_userptr *)data;
		puserptr->handle = 1;
		memcpy(&stored_userptr,data,sizeof(struct drm_exynos_gem_userptr));
		if( fd < 0 )
			return -1;
		break;

	}
	return 0;
}

struct drm_exynos_ipp_property stored_property;
struct drm_exynos_g2d_set_cmdlist store_cmdlist;
struct drm_gem_close stored_arg_close;
struct drm_exynos_ipp_property *pprop;
struct drm_exynos_g2d_get_ver *ver;
struct drm_exynos_g2d_exec *exec;
struct drm_exynos_gem_phy_imp *phy_imp;
int drmIoctl(int fd, unsigned long request, void *arg) {
    switch (request) {
    case DRM_IOCTL_EXYNOS_IPP_SET_PROPERTY:
    	pprop = (struct drm_exynos_ipp_property *)arg;
    	pprop->prop_id = 1;
        memcpy(&stored_property,arg,sizeof(struct drm_exynos_ipp_property));
        stored_property.prop_id = 1;
        break;
    case DRM_IOCTL_EXYNOS_G2D_GET_VER:
    	ver = (struct drm_exynos_g2d_get_ver*) arg;
    	ver->major = 12;
    	ver->minor = 34;
        if( fd < 0 )
        	return -1;
        break;
    case DRM_IOCTL_EXYNOS_G2D_EXEC:
    	exec = (struct drm_exynos_g2d_exec*) arg;
        if( fd < 0 )
        	return -1;
        break;
    case DRM_IOCTL_EXYNOS_G2D_SET_CMDLIST:
    	memcpy(&store_cmdlist,arg,sizeof(struct drm_exynos_g2d_set_cmdlist));
        if( fd < 0 )
        	return -1;
        break;
    case DRM_IOCTL_GEM_CLOSE:
    	memcpy(&stored_arg_close,arg,sizeof(struct drm_gem_close));
        if( fd < 0 )
        	return -1;
        break;
    case DRM_IOCTL_EXYNOS_GEM_PHY_IMP:
        phy_imp =  (struct drm_exynos_gem_phy_imp*) arg;
        phy_imp->gem_handle = 1;
        break;
    }
    return 0;
}



int drmWaitVBlank(int fd, drmVBlankPtr vbl) {

}



int drmModePageFlip(int fd, uint32_t crtc_id, uint32_t fb_id, uint32_t flags,
        void *user_data) {

}

int drmModeSetCrtc(int fd, uint32_t crtcId, uint32_t bufferId, uint32_t x,
        uint32_t y, uint32_t *connectors, int count, drmModeModeInfoPtr mode) {

}

static int cmp_drm_planeres_func (void*ptr,int key){
#ifdef DEBUG_VIEW
    printf("buf_id %i type %i bo %i\n",((drm_obj*)ptr)->buf_id,((drm_obj*)ptr)->type,((drm_obj*)ptr)->data);
#endif
    drm_obj* dro=(drm_obj*)ptr;
    return (dro->data==key)&&(dro->type==TYPE_DRM_PLANE_RES);
}

//----------------PlaneResources----------------------
drmModePlaneResPtr drmModeGetPlaneResources(int fd) {
    drmModePlaneResPtr ptr=calloc(1,sizeof(drmModePlaneRes));
    drm_obj* dobj=calloc(1,sizeof(drm_obj));
    dobj->type=TYPE_DRM_PLANE_RES;
    dobj->data=ptr;
    _add_calloc(dobj, DRM_OBJ);
    return ptr;
}



void drmModeFreePlaneResources(drmModePlaneResPtr ptr) {
    drm_obj* co= find_mem_obj_by_func(cmp_drm_planeres_func,ptr);
    if(co)
        ctrl_free(co);
}
//----------------PlaneResources----------------------


//----------------Resources----------------------
static int cmp_drm_res_func (void*ptr,int key){
#ifdef DEBUG_VIEW
    printf("buf_id %i type %i bo %i\n",((drm_obj*)ptr)->buf_id,((drm_obj*)ptr)->type,((drm_obj*)ptr)->data);
#endif
    drm_obj* dro=(drm_obj*)ptr;
    return (dro->data==key)&&(dro->type==TYPE_DRM_RES);
}

drmModeResPtr drmModeGetResources(int fd) {
    drmModeResPtr ptr=calloc(1,sizeof(drmModeRes));
    drm_obj* dobj=calloc(1,sizeof(drm_obj));
    dobj->type=TYPE_DRM_RES;
    dobj->data=ptr;
    _add_calloc(dobj, DRM_OBJ);
    return ptr;
}
void drmModeFreeResources(drmModeResPtr ptr) {
    drm_obj* co= find_mem_obj_by_func(cmp_drm_res_func,ptr);
    if(co)
        ctrl_free(co);
}
//----------------Resources----------------------




// -------------ModeGetCrtc--------------------

static int cmp_drm_crtcmode_func(void*ptr, int key) {
#ifdef DEBUG_VIEW
    printf("buf_id %i type %i bo %i\n",((drm_obj*)ptr)->buf_id,((drm_obj*)ptr)->type,((drm_obj*)ptr)->data);
#endif
    drm_obj* dro = (drm_obj*) ptr;
    return (dro->data == key) && (dro->type == TYPE_DRM_CRTC_MODE);
}

#define NUM_CRTC 10

drmModeCrtcPtr drmModeGetCrtc(int fd, uint32_t crtcId) {
	if ( fd == 101 )
		return NULL;
    drmModeResPtr ptr = calloc(1, NUM_CRTC);
    drm_obj* dobj = calloc(1, sizeof(drm_obj));
    dobj->type = TYPE_DRM_CRTC_MODE;
    dobj->data = ptr;
    _add_calloc(dobj, DRM_OBJ);
    return ptr;
}

void drmModeFreeCrtc( drmModeCrtcPtr ptr )
{
    drm_obj* co = find_mem_obj_by_func(cmp_drm_crtcmode_func, ptr);
    if (co)
        ctrl_free(co);
}
// -------------ModeGetCrtc--------------------


static int cmp_drm_props_func(void*ptr, int key) {
#ifdef DEBUG_VIEW
    printf("buf_id %i type %i bo %i\n",((drm_obj*)ptr)->buf_id,((drm_obj*)ptr)->type,((drm_obj*)ptr)->data);
#endif
    drm_obj* dro = (drm_obj*) ptr;
    return (dro->data == key) && (dro->type == TYPE_DRM_PROPS);
}


#define NUM_PROPS 10

drmModeObjectPropertiesPtr drmModeObjectGetProperties(int fd,
        uint32_t object_id, uint32_t object_type) {
    drmModeObjectPropertiesPtr prop = calloc(1,sizeof(drmModeObjectProperties));
    prop->props = ctrl_calloc( 1, sizeof( uint32_t ) );
    //create drmModeObjectProperties obj
    drm_obj* dobj_props = calloc(1, sizeof(drm_obj));

    dobj_props->type = TYPE_DRM_PROPS;
    dobj_props->data = prop;
    _add_calloc(dobj_props, DRM_OBJ);//add drmModeObjectProperties obj

    prop->count_props = NUM_PROPS;

    return prop;
}



int drmModeObjectSetProperty(int fd, uint32_t object_id, uint32_t object_type,
        uint32_t property_id, uint64_t value) {
    return 0;
}

void drmModeFreeObjectProperties(drmModeObjectPropertiesPtr ptr) {
    drm_obj* co = find_mem_obj_by_func(cmp_drm_props_func, ptr);
    if (co)
        ctrl_free(co);
}

///-----------------drmModeGetProperty--------------------------
drmModePropertyPtr drmModeGetProperty(int fd, uint32_t propertyId) {

	if ( fd == 101 )
		return NULL;

    drmModePropertyPtr prop=ctrl_calloc(1,sizeof(drmModePropertyRes));
    prop->prop_id=123;
    strcpy(prop->name,"zpos");
    return prop;
}

void drmModeFreeProperty(drmModePropertyPtr ptr) {

}

///-----------------drmModeGetProperty--------------------------




//randr
Bool RRCrtcGammaSet(RRCrtcPtr crtc, CARD16 *red, CARD16 *green, CARD16 *blue) {

}

///-----------------GetPlane--------------------------

static int cmp_drm_plane_func(void*ptr, int key) {
#ifdef DEBUG_VIEW
    printf("buf_id %i type %i bo %i\n",((drm_obj*)ptr)->buf_id,((drm_obj*)ptr)->type,((drm_obj*)ptr)->data);
#endif
    drm_obj* dro = (drm_obj*) ptr;
    return (dro->data == key) && (dro->type == TYPE_DRM_PLANE);
}

drmModePlanePtr drmModeGetPlane(int fd, uint32_t plane_id) {

	if( fd == 101 )
		return NULL;

    drmModePlanePtr ptr = calloc(1, sizeof(drmModePlane));
    if( 121 == fd )
    	ptr->crtc_id = 1;
    drm_obj* dobj = calloc(1, sizeof(drm_obj));
    dobj->type = TYPE_DRM_PLANE;
    dobj->data = ptr;
    _add_calloc(dobj, DRM_OBJ);
    return ptr;
}

void drmModeFreePlane(drmModePlanePtr ptr) {
    drm_obj* co = find_mem_obj_by_func(cmp_drm_plane_func, ptr);
    if (co)
        ctrl_free(co);
}

///-----------------GetPlane--------------------------

///-----------------GetEncoder--------------------------

static int cmp_drm_enc_func(void*ptr, int key) {
#ifdef DEBUG_VIEW
    printf("buf_id %i type %i bo %i\n",((drm_obj*)ptr)->buf_id,((drm_obj*)ptr)->type,((drm_obj*)ptr)->data);
#endif
    drm_obj* dro = (drm_obj*) ptr;
    return (dro->data == key) && (dro->type == TYPE_DRM_ENCODER);
}

drmModeEncoderPtr drmModeGetEncoder(int fd, uint32_t encoder_id) {
    drmModeEncoderPtr ptr = calloc(1, sizeof(drmModeEncoder));
    drm_obj* dobj = calloc(1, sizeof(drm_obj));
    dobj->type = TYPE_DRM_ENCODER;
    dobj->data = ptr;
    _add_calloc(dobj, DRM_OBJ);
    return ptr;
}

void drmModeFreeEncoder(drmModeEncoderPtr ptr) {
    drm_obj* co = find_mem_obj_by_func(cmp_drm_enc_func, ptr);
    if (co)
        ctrl_free(co);
}
///-----------------GetEncoder--------------------------

int drmModeCrtcSetGamma(int fd, uint32_t crtc_id, uint32_t size,
                   uint16_t *red, uint16_t *green, uint16_t *blue){

}

///-----------------GetConnector--------------------------


static int cmp_drm_conn_func(void*ptr, int key) {
#ifdef DEBUG_VIEW
    printf("buf_id %i type %i bo %i\n",((drm_obj*)ptr)->buf_id,((drm_obj*)ptr)->type,((drm_obj*)ptr)->data);
#endif
    drm_obj* dro = (drm_obj*) ptr;
    return (dro->data == key) && (dro->type == TYPE_DRM_CONNECTOR);
}
drmModeConnectorPtr drmModeGetConnector(int fd, uint32_t connectorId){
    drmModeConnectorPtr ptr = calloc(1, sizeof(drmModeConnector));
        drm_obj* dobj = calloc(1, sizeof(drm_obj));
        dobj->type = TYPE_DRM_CONNECTOR;
        dobj->data = ptr;
        _add_calloc(dobj, DRM_OBJ);
        return ptr;

}
void drmModeFreeConnector( drmModeConnectorPtr ptr ){
    drm_obj* co = find_mem_obj_by_func(cmp_drm_conn_func, ptr);
      if (co)
          ctrl_free(co);
}
///-----------------GetConnector--------------------------


drmModePropertyBlobPtr drmModeGetPropertyBlob(int fd, uint32_t blob_id){

}

drmModeFBPtr drmModeGetFB(int fd, uint32_t bufferId){

}

int drmModeConnectorSetProperty(int fd, uint32_t connector_id, uint32_t property_id,
                    uint64_t value){

}



drmModeFreePropertyBlob(drmModePropertyBlobPtr ptr){

}


int cnt_drmModeSetPlane=0;

Fixed_test fixed_t;
Crtc_test crtc_t;

int drmModeSetPlane(int fd, uint32_t plane_id, uint32_t crtc_id, uint32_t fb_id,
        uint32_t flags, uint32_t crtc_x, uint32_t crtc_y, uint32_t crtc_w,
        uint32_t crtc_h, uint32_t src_x, uint32_t src_y, uint32_t src_w,
        uint32_t src_h) {
    cnt_drmModeSetPlane++;

    fixed_t.x = src_x;
    fixed_t.y = src_y;
    fixed_t.w = src_w;
    fixed_t.h = src_h;

    crtc_t.x = crtc_x;
    crtc_t.y = crtc_y;
    crtc_t.w = crtc_w;
    crtc_t.h = crtc_h;

    if( (101 << 16) == src_x || 141 == fd ) //Using for make artificial fail for exynosPlaneDraw function in exynos_plane.c
    	return 1;
    return 0;
}



unsigned cnt_fb_check_refs (){
    return ctrl_get_cnt_mem_obj(DRM_OBJ);
}


void create_crtc(unsigned num){

}


unsigned get_crtc_cnt(void ){
  return 0;
}

void print_drm(drm_obj*bo) {
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

void drm_test(){


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
    a = drmModeObjectGetProperties(0, 0, 9);
    a = drmModeObjectGetProperties(0, 0, 9);
    ctrl_print_mem();
    printf("test drmModeFreeObjectProperties\n");
    drmModeFreeObjectProperties(a);
    ctrl_print_mem();

    printf("test drmModeGetConnector\n");
    a = drmModeGetConnector(0,  9);
    a = drmModeGetConnector(0,  9);
    ctrl_print_mem();
    printf("test drmModeGetConnector\n");
    drmModeFreeConnector(a);
    ctrl_print_mem();

    printf("test drmModeGetPlane\n");
    a = drmModeGetPlane(0, 9);
    a = drmModeGetPlane(0, 9);
    ctrl_print_mem();
    printf("test drmModeGetPlane\n");
    drmModeFreePlane(a);
    ctrl_print_mem();

    printf("test drmModeGetEncoder\n");
    a = drmModeGetEncoder(0,  9);
    a = drmModeGetEncoder(0,  9);
    ctrl_print_mem();
    printf("test drmModeGetEncoder\n");
    drmModeFreeEncoder(a);
    ctrl_print_mem();

}
