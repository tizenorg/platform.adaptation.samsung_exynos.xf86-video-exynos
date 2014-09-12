#ifndef FAKE_DRM_H_
#define FAKE_DRM_H_

#include "fake_mem.h"

#define G2D_MAX_CMD_NR		64
#define G2D_MAX_GEM_CMD_NR	64
#define G2D_MAX_CMD_LIST_NR	64
#define G2D_PLANE_MAX_NR	2

typedef enum
{
    TYPE_DRM_FB,
    TYPE_DRM_CONNECTOR,
    TYPE_DRM_ENCODER,
    TYPE_DRM_PLANE,
    TYPE_DRM_PLANE_RES,
    TYPE_DRM_RES,
    TYPE_DRM_CRTC_MODE,
    TYPE_DRM_PROPS,
    TYPE_DRM_PROP,
    TYPE_DRM_PROP_VAL
}type_drm_obj;

typedef struct _drm_obj
{
  int ref_cnt;
  int buf_id;
  type_drm_obj type;
  void *data;
}drm_obj;

void print_drm(drm_obj*bo);
void drm_test();


#endif /* FAKE_DRM_H_ */
