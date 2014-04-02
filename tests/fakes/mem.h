/*
 * mem.h
 *
 *  Created on: Nov 12, 2013
 *      Author: svoloshynov
 */

#ifndef MEM_H_
#define MEM_H_

#include "exynos_driver.h"

typedef enum {CALLOC_OBJ,TBM_OBJ,DRM_OBJ,STR_OBJ,TBM_OBJ_MAP,ALL_OBJ,MALLOC_OBJ}type_mem_object;

typedef struct _calloc_obj {
    void *calloc_object;
    type_mem_object type;
    struct xorg_list link;
} calloc_obj_item;

typedef struct for_test_drmModeSetPlane
{
    unsigned int x;
    unsigned int y;
    unsigned int w;
    unsigned int h;
} Fixed_test, Crtc_test;

void _add_calloc(void*data,type_mem_object t);
calloc_obj_item* find_mem_obj(void *ptr);
void set_step_to_crash(int i);
void clear_mem_ctrl();
int is_free_error();
unsigned ctrl_get_cnt_mem_obj(type_mem_object type);
void ctrl_print_mem();
void ctrl_print_mem_2(char * msg);


typedef int (*cmp_func)(void*,int);

#endif /* MEM_H_ */
