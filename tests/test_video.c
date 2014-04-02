/*
 * test_video_ipp.c
 *
 *  Created on: Oct 21, 2013
 *      Author: svoloshynov
 */
#include "test_video_ipp.h"

#include "./xv/exynos_video.c"



START_TEST (test_)
{

}
END_TEST;



int test_video_suite (run_stat_data* stat)
{
  Suite *s = suite_create ("VIDEO.C");
  /* Core test case */
  TCase *tc_v_core = tcase_create ("exynosVideoInit");

  tcase_add_test (tc_v_core, test_);

  suite_add_tcase (s, tc_v_core);

  return 0;
}
