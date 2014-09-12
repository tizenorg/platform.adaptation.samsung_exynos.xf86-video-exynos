#include "../../src/sec.h"

#include "fake_tbm.h"
#include "fake_mem.h"


tbm_bo tbm_bo_alloc(tbm_bufmgr bufmgr, int size, int flags) {
    void *ret_ptr = calloc(1, size);
    tbo_item* bo=calloc(1,sizeof(tbo_item)) ;
    bo->ptr=ret_ptr;
    bo->ref_cnt=1;
    bo->size=size;
    bo->user_data=0;
    _add_calloc( bo, TBM_OBJ, 1, sizeof( tbo_item ) );
    return bo;
}

tbm_bufmgr tbm_bufmgr_init   (int fd)
{
	return 0;
}

void tbm_bufmgr_deinit (tbm_bufmgr bufmgr)
{

}

static tbo_item*_find_bo(tbm_bo bo) {
    calloc_obj_item* co = find_mem_obj(bo);
    tbo_item* need_bo = co->calloc_object;
    return need_bo;
}

int tbm_get_ref( tbm_bo bo )
{
	tbo_item* need_bo=_find_bo(bo);
	return need_bo->ref_cnt;
}

int tbm_get_map( tbm_bo bo )
{
	tbo_item* need_bo=_find_bo(bo);
	return need_bo->map_cnt;
}

tbm_bo tbm_bo_import(tbm_bufmgr bufmgr, unsigned int key) {
    tbo_item* bo= find_any_tbo();
    return bo;//maybe i'll do it more rightly.
}

unsigned int tbm_bo_export(tbm_bo bo) {
	return 11;
}

tbm_bo tbm_bo_ref(tbm_bo bo) {
    tbo_item* need_bo=_find_bo(bo);
    need_bo->ref_cnt++;
    return bo;
}

void tbm_bo_unref(tbm_bo bo)
{
    tbo_item* need_bo=_find_bo(bo);
    need_bo->ref_cnt--;
    if (need_bo->ref_cnt == 0){
        //autodeleting
        free(need_bo->ptr);
        ctrl_free(need_bo);
    }
}


tbm_bo_handle tbm_bo_map( tbm_bo bo, int device, int opt )
{
    tbo_item* need_bo=_find_bo(bo);
    need_bo->map_cnt++;
	return (tbm_bo_handle)need_bo->ptr;
}

int tbm_bo_unmap( tbm_bo bo )
{
    tbo_item* need_bo=_find_bo(bo);
    need_bo->map_cnt--;
    return 0;
}

tbm_bo_handle tbm_bo_get_handle(tbm_bo bo, int device) {
    tbo_item* need_bo=_find_bo(bo);
    return (tbm_bo_handle)need_bo->ptr;
}

int tbm_bo_size(tbm_bo bo) {
    tbo_item* need_bo=_find_bo(bo);
    return need_bo->size;
}

int tbm_bo_add_user_data(tbm_bo bo, unsigned long key, tbm_data_free data_free_func)
{
    tbo_item* need_bo=_find_bo(bo);
    return (int)(size_t)(need_bo->user_data=(void*)data_free_func);
}

unsigned cnt_map_bo(tbm_bo bo) {
    tbo_item* need_bo=_find_bo(bo);
    return need_bo->map_cnt;
}

/////////////////////////////
/*TODO wrong realization*/
static void* bo_data_gl;
/////////////////////////////

int tbm_bo_set_user_data( tbm_bo bo, unsigned long key, void* data )
{
	bo_data_gl = data; // receive poiter to bo user data (_renderBoCreate() function)
	return 1;
}

int tbm_bo_get_user_data(tbm_bo bo, unsigned long key, void** data)
{
    tbo_item* need_bo=_find_bo(bo);
    *data=need_bo->user_data;

   	return 0;
}

void print_tbm(tbo_item*bo){
    printf(" obj (%x) ref_cnt(%i) map_cnt(%i) size(%i)",(unsigned int)(size_t)bo->ptr,bo->ref_cnt,bo->map_cnt,bo->size);
}

int tbm_bo_swap (tbm_bo bo1, tbm_bo bo2)
{
	return 0;
}


//-------------------------------------------------------------------------------

static tbm_bo_handle hndl_tbm;     // this variable is returned by tbm_bo_map() and tbm_bo_get_handle()

// sets field 'ptr' of global variable 'hndl_tbm' to use in later
void set_hndl_tbm_ptr( void* ptr )
{
    hndl_tbm.ptr = ptr;
}

unsigned get_tbm_check_refs(){
    return ctrl_get_cnt_calloc(TBM_OBJ);
}

unsigned get_tbm_map_cnt( void )
{
    return ctrl_get_cnt_calloc(TBM_OBJ_MAP);
}

// returns pointer to global variable 'bo_data_gl',
// that WAS set in tbm_bo_set_user_data() function
/*TODO wrong realization*/
void* get_bo_data( void )
{
	return bo_data_gl;
}

#if 0
// returns tbm_bo_map/tbm_bo_unmap calls counter
int get_tbm_map_cnt( void )
{
	return tbm_map_cnt;
}



#endif


void tbm_bo_forTests( tbm_bo bo, unsigned int val )
{
    tbo_item* need_bo=_find_bo(bo);
    need_bo->user_data= (void*)(size_t)val;
}

void tbm_test(){
    tbm_bo bo1=  tbm_bo_alloc(0,100,0);
    tbm_bo bo2=  tbm_bo_alloc(0,100,0);
    tbm_bo_ref(bo1);
    ctrl_print_mem();
    tbm_bo bo3=tbm_bo_import(0,0);
    printf("import-%p\n",bo3);
    tbm_bo_ref(bo2);
    tbm_bo_ref(bo1);
    tbm_bo_map(bo1,0,0);
    ctrl_print_mem();
    tbm_bo_unref(bo1);
    tbm_bo_unref(bo1);
    ctrl_print_mem();
    tbm_bo_unref(bo1);
    ctrl_print_mem();
}
