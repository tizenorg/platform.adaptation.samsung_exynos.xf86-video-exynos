/*
 * test_output.c
 *
 *  Created on: Nov 27, 2013
 *      Author: svoloshynov
 */
#include "test_output.h"

#include "./display/exynos_output.c"



START_TEST (test_createPropertyRange_BigLen_fail)
{
    const char test_name[]="RANGE_PROPERTY veryveryveryveryveryveryveryveryverylong string";
    int id=1245;
    drmModePropertyPtr prop=createPropertyRange(test_name,id,0,100);
    fail_if(prop == 0, "error of creating" );
    fail_if(strcmp(prop->name,test_name) == 0, "too long name" );
}
END_TEST;


START_TEST (test_createPropertyRange_success)
{
    const char test_name[]="RANGE_PROPERTY";
    int id=1245;
    drmModePropertyPtr prop=createPropertyRange(test_name,id,0,100);
    fail_if(prop == 0, "error of creating" );
    fail_if(strcmp(prop->name,test_name) != 0, "Name is not set!" );
    fail_if(prop->prop_id!= id, "ID is not set!" );
}
END_TEST;

START_TEST (test_createPropertyEnum_success)
{
    const char test_name[]="RANGE_ENUM";
    int id=1245;
    drmModePropertyPtr prop=createPropertyEnum(test_name,id);
    fail_if(prop == 0, "error of creating" );
    fail_if(strcmp(prop->name,test_name) != 0, "Name is not set!" );
    fail_if(prop->prop_id!= id, "ID is not set!" );
}
END_TEST;

static int doesPropContainEnum(drmModePropertyPtr prop,const char*str){
    int i;
    for(i=0;i<prop->count_enums;i++)
        if(strcmp(prop->enums[i].name,str)==0)
            return 1;
    return 0;
}


START_TEST (test_addEnumProperty_success)
{   const char test_name1[]="FIRST";
    const char test_name2[]="SECOND";
    drmModePropertyPtr prop=createPropertyEnum("RANGE_ENUM",134);
    addEnumProperty(prop,test_name1);
    fail_if(prop->count_enums != 1, "count is not valid" );
    fail_if(doesPropContainEnum(prop,test_name1) == 0, "enum is not added!" );

    addEnumProperty(prop,test_name2);
    fail_if(prop->count_enums != 2, "count is not valid" );
    fail_if(doesPropContainEnum(prop,test_name1) == 0, "lost of data" );
    fail_if(doesPropContainEnum(prop,test_name2) == 0, "enum is not added!" );
}
END_TEST;


int test_output_suite (run_stat_data* stat )
{


  Suite *s = suite_create ("OUTPUT.C");

  /* Core test case */
  TCase *tc_o_core = tcase_create ("test_createPropertyEnum");

  tcase_add_test (tc_o_core, test_createPropertyRange_success);
  tcase_add_test (tc_o_core, test_createPropertyEnum_success);
  tcase_add_test (tc_o_core, test_createPropertyRange_BigLen_fail);


  suite_add_tcase (s, tc_o_core);
  SRunner *sr = srunner_create( s );
  srunner_run_all( sr, CK_VERBOSE );

	stat->num_tests = srunner_ntests_run( sr );
	stat->num_pass = srunner_ntests_run( sr ) - srunner_ntests_failed( sr );
	stat->num_failure = srunner_ntests_failed( sr );

  return 0;
}
