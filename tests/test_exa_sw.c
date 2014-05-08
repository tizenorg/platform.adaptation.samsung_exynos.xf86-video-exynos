/*
 * test_exa_sw.c
 *
 *  Created on: Oct 21, 2013
 *      Author: svoloshynov
 */

#include <pixmap.h>
#include "test_exa_sw.h"
#include "./accel/exynos_exa_sw.c"



START_TEST (_swBoxAdd_addIntersected_Success)
{
    BoxRec b1,b2;
    ExaBox* ret;
    int cnt_calloc;
    struct xorg_list list;
    xorg_list_init (&list);

    //intersected
    b1.x1=10;
    b1.y1=10;
    b1.x2=100;
    b1.y2=100;


    b2.x1=50;
    b2.y1=50;
    b2.x2=200;
    b2.y2=200;

    calloc_init();
    ret=_swBoxAdd(&list,&b1,&b2);
    fail_if (ret==0 ,"should add");
    fail_if (xorg_list_is_empty(&list) ,"should add to list");
    fail_if (ctrl_get_cnt_calloc()==0 ,"memory should allocate");
}
END_TEST;


START_TEST (_swBoxAdd_addNotIntersected_Failure)
{
    BoxRec b1,b2;
    ExaBox* ret;
    int cnt_calloc;
    struct xorg_list list;
    xorg_list_init (&list);

    //not intersected
    b1.x1=10;
    b1.y1=10;
    b1.x2=100;
    b1.y2=100;


    b2.x1=150;
    b2.y1=150;
    b2.x2=200;
    b2.y2=200;

    calloc_init();
    ret=_swBoxAdd(&list,&b1,&b2);

    fail_if (ret!=0 ,"should not add");
    fail_if (!xorg_list_is_empty(&list) ,"should not add to list");
    fail_if (ctrl_get_cnt_calloc()!=0 ,"memory should not allocate ");
}
END_TEST;



START_TEST (test_swBoxMerge)
{
/*TODO*/
}
END_TEST;

START_TEST (test_swBoxMove)
{
/*TODO*/
}
END_TEST;


START_TEST (test_swBoxRemoveAll)
{
/*TODO*/
}
END_TEST;

START_TEST (test_swBoxIsOne)
{
/*TODO*/
}
END_TEST;



START_TEST (EXYNOSExaSwUploadToScreen_Success)
{

}END_TEST;

START_TEST (EXYNOSExaSwDownloadFromScreen_Success)
{
    //EXYNOSExaSwDownloadFromScreen(PixmapPtr pSrc, int x, int y, int w, int h, char *dst, int dst_pitch)_;
}END_TEST;




int test_exynos_exa_sw_suite (run_stat_data* stat )
{
  Suite *s = suite_create ("EXA_SW");
  /* Core test case */
  TCase *texa_s_core = tcase_create ("_swBoxAdd");
  tcase_add_test (texa_s_core, _swBoxAdd_addIntersected_Success);
  tcase_add_test (texa_s_core, _swBoxAdd_addNotIntersected_Failure);
  tcase_add_test (texa_s_core, test_swBoxMerge);
  tcase_add_test (texa_s_core, test_swBoxRemoveAll);
  tcase_add_test (texa_s_core, test_swBoxIsOne);
  tcase_add_test (texa_s_core, EXYNOSExaSwUploadToScreen_Success);
  tcase_add_test (texa_s_core, EXYNOSExaSwDownloadFromScreen_Success);
  suite_add_tcase (s, texa_s_core);

  return 0;
}
