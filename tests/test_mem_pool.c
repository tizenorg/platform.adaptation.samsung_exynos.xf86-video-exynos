/*
 * test_mem_pool_ipp.c
 *
 *  Created on: Oct 21, 2013
 *      Author: svoloshynov
 */
#include "test_mem_pool.h"
#include "mem.h"
#include "./util/exynos_mem_pool.c"

//------------------_exynosMemPoolCreateBufInfo---------------------

START_TEST( test_exynosMemPoolCreateBufInfo_NullPtr) {
    _exynosMemPoolCreateBufInfo(0, 0, 0, 0);
}
END_TEST;

START_TEST( test_exynosMemPoolCreateBufInfo_NullSize_failure) {
    ScrnInfoRec scr;
    prepare(&scr);
    int prev_mem = cnt_all_mem_obj();

    _exynosMemPoolCreateBufInfo(&scr, 0, 0, 0);

    fail_if(cnt_all_mem_obj() != prev_mem, "mem shouldn't be allocate");
}
END_TEST;

START_TEST( test_exynosMemPoolCreateBufInfo_BadID_failure) {
    ScrnInfoRec scr;
    prepare(&scr);
    int prev_mem = cnt_all_mem_obj();
    _exynosMemPoolCreateBufInfo(&scr, 1010, 20, 30);
    fail_if(cnt_all_mem_obj() != prev_mem, "mem shouldn't be allocate");
}
END_TEST;

START_TEST( test_exynosMemPoolCreateBufInfo_success) {
    int w = 20, h = 30, id = FOURCC_SR32;
    ScrnInfoRec scr;
    prepare(&scr);
    int prev_mem = cnt_all_mem_obj();
    EXYNOSBufInfoPtr bufinfo = _exynosMemPoolCreateBufInfo(&scr, id, w, h);

    fail_if(cnt_all_mem_obj() != 1 + prev_mem, "mem should be allocate");
    fail_if(bufinfo->width != w, "width don't set");
    fail_if(bufinfo->height != h, "height don't set");
    fail_if(bufinfo->fourcc != id, "fourcc don't set");
    fail_if(bufinfo->size == 0, "size don't set");
}
END_TEST;

//------------------_exynosMemPoolDestroyBufInfo---------------------

START_TEST( test_exynosMemPoolDestroyBufInfo_NullPtr) {
    _exynosMemPoolDestroyBufInfo(0);
}
END_TEST;

START_TEST( test_exynosMemPoolDestroyBufInfo_success) {
    int w = 20, h = 30, id = FOURCC_SR32;
    ScrnInfoRec scr;
    prepare(&scr);
    EXYNOSBufInfoPtr bufinfo = _exynosMemPoolCreateBufInfo(&scr, id, w, h);
    _exynosMemPoolDestroyBufInfo(bufinfo);
}
END_TEST;


//------------------exynosMemPoolDestroyBO---------------------
START_TEST( test_exynosMemPoolDestroyBO_NullPtr) {
    exynosMemPoolDestroyBO(0);
}
END_TEST;


START_TEST( test_exynosMemPoolDestroyBO_success) {
    int w = 20, h = 30, id = FOURCC_SR32;
    ScrnInfoRec scr;
    tbm_bo_handle *handles = calloc(4, sizeof(tbm_bo_handle));
    prepare(&scr);
    int prev_bo=ctrl_get_cnt_mem_obj(TBM_OBJ);
    int prev_calloc=ctrl_get_cnt_mem_obj(CALLOC_OBJ);
    EXYNOSBufInfoPtr bufinfo = exynosMemPoolCreateBO(&scr, id, w, h);
    //printf("bo - %i %i calloc %i %i",prev_bo,ctrl_get_cnt_mem_obj(TBM_OBJ),prev_calloc,ctrl_get_cnt_mem_obj(CALLOC_OBJ));
    exynosMemPoolDestroyBO(bufinfo);
    fail_if(prev_bo!=ctrl_get_cnt_mem_obj(TBM_OBJ), "num of bo was changed");
    fail_if(prev_calloc!=ctrl_get_cnt_mem_obj(CALLOC_OBJ), "memory leak");

}
END_TEST;
//------------------exynosMemPoolGetBufInfo--------------------

START_TEST( test_exynosMemPoolGetBufInfo_NullPtr) {
    EXYNOSBufInfoPtr bufinfo =exynosMemPoolGetBufInfo(0);
    fail_if(bufinfo!=0,"should be 0");
}
END_TEST;

START_TEST( test_exynosMemPoolGetBufInfo_success) {
    int w = 20, h = 30, id = FOURCC_SR32;
    ScrnInfoRec scr;
    tbm_bo_handle *handles = calloc(4, sizeof(tbm_bo_handle));
    prepare(&scr);

    EXYNOSBufInfoPtr bufinfo = exynosMemPoolCreateBO(&scr, id, w, h);
    bufinfo = exynosMemPoolGetBufInfo(bufinfo->tbos[0]);
    fail_if(bufinfo==0,"should be valid bufinfo");
}
END_TEST;
//------------------exynosMemPoolCreateBO----------------------
START_TEST( test_exynosMemPoolCreateBO_NullPtr) {
    EXYNOSBufInfoPtr bufinfo =exynosMemPoolCreateBO(0,0,0,0);
    fail_if(bufinfo!=0,"bufinfo should 0");
}
END_TEST;


START_TEST( test_exynosMemPoolCreateBO_zerosize) {
    int w = 20, h = 30, id = FOURCC_SR32;
    ScrnInfoRec scr;
    prepare(&scr);

    EXYNOSBufInfoPtr bufinfo = exynosMemPoolCreateBO(&scr, id, 0, h);
    fail_if(bufinfo!=0,"bufinfo should be 0");
}
END_TEST;

START_TEST( test_exynosMemPoolCreateBO_badid) {
    int w = 20, h = 30, id = 123;
    ScrnInfoRec scr;
    prepare(&scr);

    EXYNOSBufInfoPtr bufinfo = exynosMemPoolCreateBO(&scr, id, w, h);
    fail_if(bufinfo!=0,"bufinfo should be 0");
}
END_TEST;

START_TEST( test_exynosMemPoolCreateBO_success) {
    int w = 20, h = 30, id = FOURCC_SR32;
    void*cb_ptr;
    ScrnInfoRec scr;
    prepare(&scr);

    int prev_bo=ctrl_get_cnt_mem_obj(TBM_OBJ);
    EXYNOSBufInfoPtr bufinfo = exynosMemPoolCreateBO(&scr, id, w, h);
    fail_if(prev_bo==ctrl_get_cnt_mem_obj(TBM_OBJ), "num of bo was changed");
    tbm_bo_get_user_data(bufinfo->tbos[0],0,&cb_ptr);
    fail_if(cb_ptr==0, "_exynosMemPoolDestroyBufInfo was not set");
}
END_TEST;
//------------------exynosMemPoolAllocateBO--------------------
START_TEST( exynosMemPoolAllocateBO_NullPtr) {
    EXYNOSBufInfoPtr bufinfo =exynosMemPoolAllocateBO(0,0,0,0,0,0,0);
    fail_if(bufinfo!=0,"bufinfo should be 0");
}
END_TEST;


START_TEST( exynosMemPoolAllocateBO_zerosize) {
    int w = 20, h = 30, id = FOURCC_SR32;
    ScrnInfoRec scr;
    prepare(&scr);
    int prev_mem=cnt_all_mem_obj();
    EXYNOSBufInfoPtr bufinfo = exynosMemPoolAllocateBO(&scr, id, 0, h,0,0,0);
    fail_if(bufinfo!=0,"bufinfo should be 0");
    fail_if(cnt_all_mem_obj()!=prev_mem,"memory leak");
}
END_TEST;

START_TEST( exynosMemPoolAllocateBO_badid) {
    int w = 20, h = 30, id = FOURCC_SR32;
    ScrnInfoRec scr;
    prepare(&scr);

    EXYNOSBufInfoPtr bufinfo = exynosMemPoolAllocateBO(&scr, id, 0, h, 0, 0, 0);
    fail_if(bufinfo!=0,"bufinfo should be 0");
}
END_TEST;

START_TEST( exynosMemPoolAllocateBO_type_NullBoPtr) {
    int w = 20, h = 30, id = FOURCC_SR32;
    ScrnInfoRec scr;
    prepare(&scr);
    tbm_bo *tbos=0;
    unsigned int *names={0,0,0};
    int prev_mem=cnt_all_mem_obj();
    EXYNOSBufInfoPtr bufinfo = exynosMemPoolAllocateBO(&scr, id, w, h, tbos, &names, BUF_TYPE_LEGACY);
    fail_if(cnt_all_mem_obj()==prev_mem,"memory was not allocated");
}
END_TEST;

START_TEST( exynosMemPoolAllocateBO_type_NullNamesPtr) {
    int w = 20, h = 30, id = FOURCC_SR32;
    ScrnInfoRec scr;
    prepare(&scr);
    tbm_bo *tbos = { 0, 0, 0 };
    unsigned int *names;
    int prev_mem = cnt_all_mem_obj();
    EXYNOSBufInfoPtr bufinfo = exynosMemPoolAllocateBO(&scr, id, w, h, tbos,
            &names, BUF_TYPE_LEGACY);
    fail_if(cnt_all_mem_obj() == prev_mem, "memory was not allocated");
}
END_TEST;

START_TEST( exynosMemPoolAllocateBO_type_LEGACY_nullnames) {
    int w = 20, h = 30, id = FOURCC_SR32;
    ScrnInfoRec scr;
    prepare(&scr);
    tbm_bo *tbos = calloc(1, sizeof(tbm_bo));
    unsigned int names = { 0, 0, 0 };
    int prev_mem = cnt_all_mem_obj();
    EXYNOSBufInfoPtr bufinfo = exynosMemPoolAllocateBO(&scr, id, w, h, tbos,
            &names, BUF_TYPE_LEGACY);;
    fail_if(cnt_all_mem_obj() == prev_mem, "memory was not allocated");
}
END_TEST;

START_TEST( exynosMemPoolAllocateBO_type_DMABUF_nullnames) {
    int w = 20, h = 30, id = FOURCC_SR32;
    ScrnInfoRec scr;
    prepare(&scr);

    tbm_bo *tbos = calloc(1, sizeof(tbm_bo));
    unsigned int names = { 0, 0, 0 };

    EXYNOSBufInfoPtr bufinfo = exynosMemPoolAllocateBO(&scr, id, w, h, tbos,
            &names, BUF_TYPE_DMABUF);
}
END_TEST;

START_TEST( exynosMemPoolAllocateBO_type_VPIX_nullnames) {
    int w = 20, h = 30, id = FOURCC_SR32;
    ScrnInfoRec scr;
    prepare(&scr);

    tbm_bo *tbos = calloc(1, sizeof(tbm_bo));
    unsigned int names = { 0, 0, 0 };

    EXYNOSBufInfoPtr bufinfo = exynosMemPoolAllocateBO(&scr, id, w, h, tbos,
            &names, BUF_TYPE_VPIX);
}
END_TEST;

START_TEST( exynosMemPoolAllocateBO_type_LEGACY_success) {
    int w = 20, h = 30, id = FOURCC_SR32;
    ScrnInfoRec scr;
    prepare(&scr);
    tbm_bo *tbos = calloc(1, sizeof(tbm_bo));
    unsigned int names = { 2, 0, 0 };
    int prev_mem = cnt_all_mem_obj();
    EXYNOSBufInfoPtr bufinfo = exynosMemPoolAllocateBO(&scr, id, w, h, tbos,
            &names, BUF_TYPE_LEGACY);;
    fail_if(cnt_all_mem_obj() == prev_mem, "memory was not allocated");
}
END_TEST;

START_TEST( exynosMemPoolAllocateBO_type_DMABUF_success) {
    int w = 20, h = 30, id = FOURCC_SR32;
    ScrnInfoRec scr;
    prepare(&scr);

    tbm_bo *tbos = calloc(1, sizeof(tbm_bo));
    unsigned int names = { 2, 0, 0 };

    EXYNOSBufInfoPtr bufinfo = exynosMemPoolAllocateBO(&scr, id, w, h, tbos,
            &names, BUF_TYPE_DMABUF);
}
END_TEST;

START_TEST( exynosMemPoolAllocateBO_type_VPIX_success) {
    int w = 20, h = 30, id = FOURCC_SR32;
    ScrnInfoRec scr;
    prepare(&scr);

    tbm_bo *tbos = calloc(1, sizeof(tbm_bo));
    unsigned int names = { 2, 0, 0 };

    EXYNOSBufInfoPtr bufinfo = exynosMemPoolAllocateBO(&scr, id, w, h, tbos,
            &names, BUF_TYPE_VPIX);
}
END_TEST;

START_TEST( exynosMemPoolAllocateBO_type_UNKNOW) {
    int w = 20, h = 30, id = FOURCC_SR32;
    ScrnInfoRec scr;
    prepare(&scr);

    tbm_bo *tbos = calloc(1, sizeof(tbm_bo));
    unsigned int names = { 0, 0, 0 };
    int prev_mem = cnt_all_mem_obj();
    EXYNOSBufInfoPtr bufinfo = exynosMemPoolAllocateBO(&scr, id, w, h, tbos,
            &names, 5000);
    fail_if(bufinfo != 0, "bufinfo should not created");
    fail_if(cnt_all_mem_obj() != prev_mem, "memory leak");
}
END_TEST;



//------------------exynosMemPoolPrepareAccess---------------------

START_TEST( test_exynosMemPoolPrepareAccess_NullPtr) {
    exynosMemPoolPrepareAccess(0, 0, 0);
}
END_TEST;

START_TEST( test_exynosMemPoolPrepareAccess_success) {
    int w = 20, h = 30, id = FOURCC_SR32;
    ScrnInfoRec scr;
    tbm_bo_handle *handles = calloc(4, sizeof(tbm_bo_handle));
    prepare(&scr);

    EXYNOSBufInfoPtr bufinfo = exynosMemPoolCreateBO(&scr, id, w, h);
    int prev_map_cnt=cnt_map_bo(bufinfo->tbos[0]);
    exynosMemPoolPrepareAccess(bufinfo, handles, 0);
    fail_if(cnt_map_bo(bufinfo->tbos[0]) <= prev_map_cnt, "should be at least 1 map");
}
END_TEST;

//------------------exynosMemPoolFinishAccess---------------------

START_TEST( test_exynosMemPoolFinishAccess_NullPtr) {
    exynosMemPoolFinishAccess(0);
}
END_TEST;

START_TEST( test_exynosMemPoolFinishAccess_success) {
    int w = 20, h = 30, id = FOURCC_SR32;
    ScrnInfoRec scr;
    tbm_bo_handle *handles = calloc(4, sizeof(tbm_bo_handle));
    prepare(&scr);

    EXYNOSBufInfoPtr bufinfo = exynosMemPoolCreateBO(&scr, id, w, h);
    int prev_map_cnt=cnt_map_bo(bufinfo->tbos[0]);
    exynosMemPoolFinishAccess(bufinfo);
    fail_if(cnt_map_bo(bufinfo->tbos[0]) >= prev_map_cnt, "should be at least 1 unmap");
}
END_TEST;


//------------------exynosMemPoolFreeHandle---------------------------
//------------------exynosMemPoolGetGemBo---------------------------

START_TEST( test_exynosMemPoolGetGemBo_NullScr) {
    Bool ret = exynosMemPoolGetGemBo(0, 0, 0);
    fail_if(ret,"should be FALSE");
}
END_TEST;

START_TEST( test_exynosMemPoolGetGemBo_NullBo) {
    ScrnInfoRec scr;

    prepare(&scr);
    Bool ret = exynosMemPoolGetGemBo(&scr, 0, 0);
    fail_if(ret,"should be FALSE");
}
END_TEST;

START_TEST( test_exynosMemPoolGetGemBo_success) {
    ScrnInfoRec scr;
    tbm_bo bo;
    prepare(&scr);
    Bool ret = exynosMemPoolGetGemBo(&scr, 0, &bo);
    fail_if(!ret,"should be TRUE");
}
END_TEST;
//------------------exynosMemPoolGetGemHandle-----------------------
START_TEST( test_exynosMemPoolGetGemHandle_NullScr) {
    Bool ret = exynosMemPoolGetGemHandle(0, 0, 0,0);
    fail_if(ret,"should be FALSE");
}
END_TEST;

START_TEST( test_exynosMemPoolGetGemHandle_NullAddr) {
    ScrnInfoRec scr;

    prepare(&scr);
    Bool ret = exynosMemPoolGetGemHandle(&scr, 0, 0, 0);
    fail_if(ret,"should be FALSE");
}
END_TEST;

START_TEST( test_exynosMemPoolGetGemHandle_NullSize) {
    ScrnInfoRec scr;

    prepare(&scr);
    Bool ret = exynosMemPoolGetGemHandle(&scr, 10, 0, 0);
    fail_if(ret,"should be FALSE");
}
END_TEST;

START_TEST( test_exynosMemPoolGetGemHandle_NullHandle) {
    ScrnInfoRec scr;

    prepare(&scr);
    Bool ret = exynosMemPoolGetGemHandle(&scr, 10, 10, 0);
    fail_if(ret,"should be FALSE");
}
END_TEST;

START_TEST( test_exynosMemPoolGetGemHandle_success) {
    ScrnInfoRec scr;
    prepare(&scr);
    int handle=500;

    Bool ret = exynosMemPoolGetGemHandle(&scr, 10, 10, &handle);
    //printf("%i\n",handle);
    fail_if(!ret,"should be TRUE");
}
END_TEST;

void test_test(){
    int w = 20, h = 30, id = FOURCC_SR32;
        ScrnInfoRec scr;
        prepare(&scr);
        tbm_bo *tbos = calloc(1, sizeof(tbm_bo));
        unsigned int names = { 2, 0, 0 };
        int prev_mem = cnt_all_mem_obj();
        EXYNOSBufInfoPtr bufinfo = exynosMemPoolAllocateBO(&scr, id, w, h, tbos,
                &names, BUF_TYPE_LEGACY);;
}


//------------------exynosMemPoolCacheFlush---------------------------
//------------------exynosUtilClearNormalVideoBuffer------------------


int test_mem_pool(run_stat_data* stat) {
    Suite *s = suite_create("MEM_POOL.C");
    /* Core test case */
    TCase *tc_memp_core = tcase_create("_exynosMemPoolCreateBufInfo");

    tcase_add_test(tc_memp_core, test_exynosMemPoolCreateBufInfo_NullPtr);
    tcase_add_test(tc_memp_core,
            test_exynosMemPoolCreateBufInfo_NullSize_failure);
    tcase_add_test(tc_memp_core, test_exynosMemPoolCreateBufInfo_BadID_failure);
    tcase_add_test(tc_memp_core, test_exynosMemPoolCreateBufInfo_success);

    suite_add_tcase(s, tc_memp_core);

    TCase *tc_mem_destr = tcase_create("_exynosMemPoolDestroyBufInfo");

    tcase_add_test(tc_mem_destr, test_exynosMemPoolDestroyBufInfo_NullPtr);
    tcase_add_test(tc_mem_destr, test_exynosMemPoolDestroyBufInfo_success);
    suite_add_tcase(s, tc_mem_destr);


    TCase *tc_mem_destrbo = tcase_create("_exynosMemPoolDestroyBO");

    tcase_add_test(tc_mem_destrbo, test_exynosMemPoolDestroyBO_NullPtr);
    tcase_add_test(tc_mem_destrbo, test_exynosMemPoolDestroyBO_success);
    suite_add_tcase(s, tc_mem_destrbo);

    TCase *tc_mem_get_buf_info = tcase_create("_exynosGetBufInfo");

    tcase_add_test(tc_mem_get_buf_info, test_exynosMemPoolGetBufInfo_NullPtr);
    tcase_add_test(tc_mem_get_buf_info, test_exynosMemPoolGetBufInfo_success);
    suite_add_tcase(s, tc_mem_get_buf_info);

    TCase *tc_mem_create_bo = tcase_create("exynosMemPoolCreateBO");

    tcase_add_test(tc_mem_create_bo, test_exynosMemPoolCreateBO_NullPtr);
    tcase_add_test(tc_mem_create_bo, test_exynosMemPoolCreateBO_success);
    tcase_add_test(tc_mem_create_bo, test_exynosMemPoolCreateBO_badid);
    tcase_add_test(tc_mem_create_bo, test_exynosMemPoolCreateBO_zerosize);
    suite_add_tcase(s, tc_mem_create_bo);

    TCase *tc_mem_alloc_bo = tcase_create("exynosMemPoolAllocateBO");



    tcase_add_test(tc_mem_alloc_bo, exynosMemPoolAllocateBO_NullPtr);
    tcase_add_test(tc_mem_alloc_bo, exynosMemPoolAllocateBO_zerosize);
    tcase_add_test(tc_mem_alloc_bo, exynosMemPoolAllocateBO_badid);

    tcase_add_test(tc_mem_alloc_bo, exynosMemPoolAllocateBO_type_DMABUF_success);
    tcase_add_test(tc_mem_alloc_bo, exynosMemPoolAllocateBO_type_VPIX_success);
    tcase_add_test(tc_mem_alloc_bo, exynosMemPoolAllocateBO_type_LEGACY_nullnames);
    tcase_add_test(tc_mem_alloc_bo, exynosMemPoolAllocateBO_type_DMABUF_nullnames);
    tcase_add_test(tc_mem_alloc_bo, exynosMemPoolAllocateBO_type_VPIX_nullnames);
    tcase_add_test(tc_mem_alloc_bo, exynosMemPoolAllocateBO_type_UNKNOW);
    tcase_add_test(tc_mem_alloc_bo, exynosMemPoolAllocateBO_type_NullBoPtr);
    tcase_add_test(tc_mem_alloc_bo, exynosMemPoolAllocateBO_type_NullNamesPtr);

    tcase_add_test(tc_mem_alloc_bo, exynosMemPoolAllocateBO_type_LEGACY_success);
    suite_add_tcase(s, tc_mem_alloc_bo);

    TCase *tc_mem_pr_access = tcase_create("exynosMemPoolPrepareAccess");
    tcase_add_test(tc_mem_pr_access, test_exynosMemPoolPrepareAccess_NullPtr);
    tcase_add_test(tc_mem_pr_access, test_exynosMemPoolPrepareAccess_success);

    suite_add_tcase(s, tc_mem_pr_access);

    TCase *tc_mem_fin_access = tcase_create("exynosMemPoolFinishAccess");
    tcase_add_test(tc_mem_fin_access, test_exynosMemPoolFinishAccess_NullPtr);
    tcase_add_test(tc_mem_fin_access, test_exynosMemPoolFinishAccess_success);

    suite_add_tcase(s, tc_mem_fin_access);

    TCase *tc_mem_getgembo = tcase_create("exynosMemPoolGetGemBo");
    tcase_add_test(tc_mem_getgembo, test_exynosMemPoolGetGemBo_NullScr);
    tcase_add_test(tc_mem_getgembo, test_exynosMemPoolGetGemBo_NullBo);
    tcase_add_test(tc_mem_getgembo, test_exynosMemPoolGetGemBo_success);

    suite_add_tcase(s, tc_mem_getgembo);

    TCase *tc_mem_getgemhandle = tcase_create("exynosMemPoolGetGemHandle");
    tcase_add_test(tc_mem_getgemhandle, test_exynosMemPoolGetGemHandle_NullScr);
    tcase_add_test(tc_mem_getgemhandle, test_exynosMemPoolGetGemHandle_NullAddr);
    tcase_add_test(tc_mem_getgemhandle, test_exynosMemPoolGetGemHandle_NullSize);
    tcase_add_test(tc_mem_getgemhandle, test_exynosMemPoolGetGemHandle_NullHandle);
    tcase_add_test(tc_mem_getgemhandle, test_exynosMemPoolGetGemHandle_success);

    suite_add_tcase(s, tc_mem_getgemhandle);


    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_VERBOSE);
    stat->num_failure = srunner_ntests_failed(sr);
    stat->num_pass = srunner_ntests_run(sr);

	stat->num_tests = srunner_ntests_run( sr );
	stat->num_pass = srunner_ntests_run( sr ) - srunner_ntests_failed( sr );
	stat->num_failure = srunner_ntests_failed( sr );

    return 0;
}
