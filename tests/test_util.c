/*
 * test_util.c
 *
 *  Created on: Oct 21, 2013
 *      Author: svoloshynov
 */
#include "test_util.h"

#include "./util/exynos_util.c"



START_TEST (test_)
{

}
END_TEST;



int test_util_suite (run_stat_data* stat )
{
  Suite *s = suite_create ("UTIL.C");

  /* Core test case */
  TCase *tc_u_core = tcase_create ("exynosUtilBoxInBox");

  tcase_add_test (tc_u_core, test_);

  suite_add_tcase (s, tc_u_core);

  return 0;
}
