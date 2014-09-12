#include "../../src/sec.h"

#include "fake_mem.h"
#include "fake_drm.h"
#include "fake_tbm.h"

static struct xorg_list mem_list;

void calloc_init() 
{
    xorg_list_init(&mem_list);
}

void _add_calloc( void* data, type_mem_object t, size_t nmemb, size_t size )
{
    calloc_obj_item* co = calloc(1, sizeof(calloc_obj_item));
    co->calloc_object = data;
    co->calloc_nmemb = nmemb;   // in some case we must know how much memory has been allocated
    co->calloc_size = size;
    co->type = t;
    xorg_list_add(&co->link, &mem_list);
}

void *ctrl_calloc(size_t num, size_t size) 
{
    void *ret_ptr = calloc(num, size);
    _add_calloc( ret_ptr, CALLOC_OBJ, num, size );
    return ret_ptr;
}

void* ctrl_malloc( size_t size )
{
    void* ret_ptr = malloc( size );
    _add_calloc( ret_ptr, MALLOC_OBJ, 1, size );
    return ret_ptr;
}

// find wrap, around pointer, structure
//==============================================================
calloc_obj_item *find_mem_obj(void *ptr) 
{
    calloc_obj_item *cur = NULL, *next = NULL;
    xorg_list_for_each_entry_safe( cur, next, &mem_list, link )
    {
        if( cur->calloc_object == ptr ) return cur;
    }
    return NULL;
}

// this function find memory area on which some element of mem_list list points to
// pick is occurring by using 'picking' function cmp()
// this function must return pointer to memory area, not to calloc_obj_item !
// used, for example, by drmModeRmFB() function
//==============================================================
void* find_mem_obj_by_func( cmp_func cmp, int key )
{
    calloc_obj_item *cur = NULL, *next = NULL;

    xorg_list_for_each_entry_safe( cur, next, &mem_list, link )
    {
        if( cmp( cur->calloc_object,key ) ) return cur->calloc_object;
    }
    return NULL;
}

void *find_any_tbo() 
{
    calloc_obj_item *cur = NULL, *next = NULL;
    xorg_list_for_each_entry_safe (cur, next, &mem_list, link)
    {
        if(TBM_OBJ==cur->type)return cur->calloc_object;
    }
    return NULL;
}

// this function is wrap for free and warning us, if we are attempted to free memory by using invalid address
// valid addresses are those, which are in mem_list list(i.e. memory areas which were allocated with ctrl_* functions)
//==============================================================
void ctrl_free( void* ptr )
{
    calloc_obj_item* found = NULL;

    found = find_mem_obj( ptr );
    if( !found )
    {
      printf( "ERROR!!! Attempt to free strange ptr!\n" );
      return;
    }

    // delete element (calloc_obj_item) from list
    xorg_list_del( &found->link );

    // TODO: ptr may points to structure, that also have pointer to dynamically allocated memory
    free( ptr );
    free( found );
}

/* *
 * for debug purpose
 * print list of ptrs
 */

void ctrl_print_mem() {
    printf("List of objects:\n");
    calloc_obj_item *cur = NULL, *next = NULL;
    char *name;
    xorg_list_for_each_entry_safe (cur, next, &mem_list, link)
    {
        printf("ptr %p ", cur->calloc_object);
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
        case TBM_OBJ_MAP:
        case ALL_OBJ:
        	break;
        }
        printf("TYPE:%s\n", name);
    }
}

/* 
 * return 0 if list is not free
 */

int ctrl_is_all_freed() {

    return !xorg_list_is_empty(&mem_list);
}


/* 
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
        _add_calloc( ptr, STR_OBJ, 1, strlen( ptr ) + 1 );
    }

    return ptr;
}
