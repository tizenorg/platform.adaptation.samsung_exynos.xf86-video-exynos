/*
 * mem.c
 *
 *  Created on: Oct 9, 2013
 *      Author: svoloshynov
 */

#include "exynos_driver.h"
#include "mem.h"
//---------------------------------------------------------------------------------------------
// mem mock
//---------------------------------------------------------------------------------------------


static struct xorg_list mem_list;

typedef struct _memory_ctrl
{
	int count_to_crash;	//set step number until mem crash ( calloc return "NULL")
	int is_free_error;

} memory_ctrl;

memory_ctrl mem_ctrl;

void set_step_to_crash(int i)
{
	mem_ctrl.count_to_crash = i;
}

int is_free_error() {return mem_ctrl.is_free_error;}

void clear_mem_ctrl()
{
	mem_ctrl.count_to_crash = 0;
	mem_ctrl.is_free_error = 0;
}


void calloc_init() {
	if(mem_list.next != 0)
	{
	    calloc_obj_item *cur = NULL, *next = NULL;
	    xorg_list_for_each_entry_safe (cur, next, &mem_list, link)
	    {
	    	if(cur->calloc_object != NULL)
	    		free(cur->calloc_object);
	    }
	}

    xorg_list_init(&mem_list);
    clear_mem_ctrl();
}

void _add_calloc(void*data, type_mem_object t) {
    calloc_obj_item* co = calloc(1, sizeof(calloc_obj_item));
    co->calloc_object = data;
    co->type = t;
    xorg_list_add(&co->link, &mem_list);
}

void *ctrl_calloc(size_t num, size_t size) {
	if(mem_ctrl.count_to_crash)
		if(--mem_ctrl.count_to_crash == 0)
			return (void *)0;

    void *ret_ptr = calloc(num, size);
    _add_calloc(ret_ptr, CALLOC_OBJ);
    return ret_ptr;
}

void* ctrl_malloc( size_t size )
{
    void* ret_ptr = malloc( size );
    _add_calloc( ret_ptr, MALLOC_OBJ );
    return ret_ptr;
}

calloc_obj_item *find_mem_obj(void *ptr) {
    calloc_obj_item *cur = NULL, *next = NULL;
    xorg_list_for_each_entry_safe (cur, next, &mem_list, link)
    {
        if(cur->calloc_object == ptr)return cur;
    }
    return NULL;
}


calloc_obj_item *find_mem_obj_by_func(cmp_func cmp,int key) {
    calloc_obj_item *cur = NULL, *next = NULL;
    xorg_list_for_each_entry_safe (cur, next, &mem_list, link)
    {
        if(cmp(cur->calloc_object,key))return cur->calloc_object;
    }
    return NULL;
}

void *find_any_tbo() {
    calloc_obj_item *cur = NULL, *next = NULL;
    xorg_list_for_each_entry_safe (cur, next, &mem_list, link)
    {
        if(TBM_OBJ==cur->type)return cur->calloc_object;
    }
    return NULL;
}

void ctrl_free(void* ptr) {//TODO use find_mem_obj
    Bool our_ptr = FALSE;
    calloc_obj_item *cur = NULL, *next = NULL;
    xorg_list_for_each_entry_safe (cur, next, &mem_list, link)
    {
        if (cur->calloc_object == ptr) {
            xorg_list_del(&cur->link);
            our_ptr = TRUE;
        }
    }
    if (our_ptr)
        free(ptr);
    else {
        printf("ERROR!!! Attempt to free strange ptr!\n");
        mem_ctrl.is_free_error++;
    }
}

/* *
 * for debug purpose
 * print list of ptrs
 */

void ctrl_print_mem_2(char * msg) {
	if(msg)
		printf("(%s) List of objects:\n", msg);
	else
		printf("List of objects:\n");
    calloc_obj_item *cur = NULL, *next = NULL;
    char *name;
    xorg_list_for_each_entry_safe (cur, next, &mem_list, link)
    {
        printf("ptr %x ", cur->calloc_object);
        switch (cur->type) {
        case CALLOC_OBJ:
            name = "CALLOC_OBJ";
            break;
        case TBM_OBJ:
            name = "TBM_OBJ";
            print_tbm(cur->calloc_object);
            break;
        case DRM_OBJ:
            name = "DRM_OBJ";
            print_drm(cur->calloc_object);
            break;
        case STR_OBJ:
            name = "STR_OBJ";
            break;
        case MALLOC_OBJ:
            name = "MALLOC_OBJ";
            break;
        }
        printf("TYPE:%s\n", name);
    }
}

void ctrl_print_mem() {
	ctrl_print_mem_2(NULL);
}


/* *
 * return 0 if list is not free
 */

int ctrl_is_all_freed() {

    return !xorg_list_is_empty(&mem_list);
}


/* *
 * return num of elements in list
 */

unsigned ctrl_get_cnt_mem_obj(type_mem_object type) {

    calloc_obj_item *cur = NULL, *next = NULL;
    unsigned cnt = 0;
    xorg_list_for_each_entry_safe (cur, next, &mem_list, link)
    {
        if((cur->type==type)||(ALL_OBJ==type))cnt++;
    }
    return cnt;
}

unsigned cnt_all_mem_obj(){
    //return count of all object
    return ctrl_get_cnt_mem_obj(ALL_OBJ);
}

unsigned ctrl_get_cnt_calloc() {
    //return count of ctrl_calloc-objects
    return ctrl_get_cnt_mem_obj(CALLOC_OBJ);
}

char* ctrl_strdup(const char *__s) {
    char* ptr = strdup(__s);

    // increment only if memory allocated is passed
    if (ptr != NULL) {
        _add_calloc(ptr, STR_OBJ);
    }

    return ptr;
}
