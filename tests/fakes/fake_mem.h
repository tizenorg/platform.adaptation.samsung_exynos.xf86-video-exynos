#ifndef MEM_H_
#define MEM_H_

#include "../../src/sec.h"

typedef enum {CALLOC_OBJ,TBM_OBJ,DRM_OBJ,STR_OBJ,TBM_OBJ_MAP,ALL_OBJ,MALLOC_OBJ}type_mem_object;

typedef struct _calloc_obj {
    void *calloc_object;
    type_mem_object type;
    struct xorg_list link;
    size_t calloc_nmemb;
    size_t calloc_size;   // size of one elements in bytes
} calloc_obj_item;

typedef struct for_test_drmModeSetPlane
{
    unsigned int x;
    unsigned int y;
    unsigned int w;
    unsigned int h;
} Fixed_test, Crtc_test;

void _add_calloc( void* data, type_mem_object t, size_t nmemb, size_t size );
calloc_obj_item* find_mem_obj(void *ptr);


typedef int (*cmp_func)(void*,int);

void ctrl_free(void* ptr);


#endif /* MEM_H_ */
