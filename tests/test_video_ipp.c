/*
 * test_video_ipp.c
 *
 *  Created on: Oct 21, 2013
 *      Author: svoloshynov
 */
#include "test_video_ipp.h"

#include "./xv/exynos_video_ipp.c"

#define PRINTVAL( v ) printf( "\n %d \n", v );

//******************************************* Unit tests ***************************************************
//==========================================================================================================
/*
int exynosVideoIppSetProperty (
					  int fd,
                      enum drm_exynos_ipp_cmd cmd,
                      int prop_id,
                      struct drm_exynos_ipp_config *src,
                      struct drm_exynos_ipp_config *dst,
                      int wb_hz,
                      int protect,
                      int csc_range) */
START_TEST ( exynosVideoIppSetProperty_fbNegative )
{
	int fd = -1;
	enum drm_exynos_ipp_cmd cmd;
	int prop_id;
	struct drm_exynos_ipp_config src;
	struct drm_exynos_ipp_config dst;
	int wb_hz;
	int protect;
	int csc_range;

	int res = 0;

	res = exynosVideoIppSetProperty( fd, cmd, prop_id, &src, &dst, wb_hz, protect, csc_range );

	fail_if( res != -1, "wrong initialization -1 must returned" );
}
END_TEST;

START_TEST ( exynosVideoIppSetProperty_srcNull )
{
	int fd = 1;
	enum drm_exynos_ipp_cmd cmd;
	int prop_id;
	struct drm_exynos_ipp_config src;
	struct drm_exynos_ipp_config dst;
	int wb_hz;
	int protect;
	int csc_range;

	int res = 0;

	res = exynosVideoIppSetProperty( fd, cmd, prop_id, NULL, &dst, wb_hz, protect, csc_range );

	fail_if( res != -1, "wrong initialization -1 must returned" );
}
END_TEST;

START_TEST ( exynosVideoIppSetProperty_dstNull )
{
	int fd = 1;
	enum drm_exynos_ipp_cmd cmd;
	int prop_id;
	struct drm_exynos_ipp_config src;
	struct drm_exynos_ipp_config dst;
	int wb_hz;
	int protect;
	int csc_range;

	int res = 0;

	res = exynosVideoIppSetProperty( fd, cmd, prop_id, &src, NULL, wb_hz, protect, csc_range );

	fail_if( res != -1, "wrong initialization -1 must returned" );
}
END_TEST;

START_TEST ( exynosVideoIppSetProperty_srcOpsIdIsNotEXYNOS_DRM_OPS_SRC)
{
	int fd = 1;
	enum drm_exynos_ipp_cmd cmd;
	int prop_id;
	struct drm_exynos_ipp_config src;
	struct drm_exynos_ipp_config dst;
	int wb_hz;
	int protect;
	int csc_range;

	src.ops_id = EXYNOS_DRM_OPS_DST;

	int res = 0;

	res = exynosVideoIppSetProperty( fd, cmd, prop_id, &src, &dst, wb_hz, protect, csc_range );

	fail_if( res != -1, "wrong initialization -1 must returned" );
}
END_TEST;

START_TEST ( exynosVideoIppSetProperty_srcOpsIdIsNotEXYNOS_DRM_OPS_DST)
{
	int fd = 1;
	enum drm_exynos_ipp_cmd cmd;
	int prop_id;
	struct drm_exynos_ipp_config src;
	struct drm_exynos_ipp_config dst;
	int wb_hz;
	int protect;
	int csc_range;

	dst.ops_id = EXYNOS_DRM_OPS_SRC;

	int res = 0;

	res = exynosVideoIppSetProperty( fd, cmd, prop_id, &src, &dst, wb_hz, protect, csc_range );

	fail_if( res != -1, "wrong initialization -1 must returned" );
}
END_TEST;

START_TEST ( exynosVideoIppSetProperty_cmdNegative )
{
	int fd = 1;
	enum drm_exynos_ipp_cmd cmd;
	int prop_id;
	struct drm_exynos_ipp_config src;
	struct drm_exynos_ipp_config dst;
	int wb_hz;
	int protect;
	int csc_range;

	src.ops_id = EXYNOS_DRM_OPS_SRC;
	dst.ops_id = EXYNOS_DRM_OPS_DST;

	cmd = 0;

	int res = 0;

	res = exynosVideoIppSetProperty( fd, cmd, prop_id, &src, &dst, wb_hz, protect, csc_range );

	fail_if( res != -1, "wrong initialization -1 must returned" );
}
END_TEST;

START_TEST ( exynosVideoIppSetProperty_correctInit )
{
	int fd = 1;
	enum drm_exynos_ipp_cmd cmd;
	int prop_id;
	struct drm_exynos_ipp_config src;
	struct drm_exynos_ipp_config dst;
	int wb_hz = 2;
	int protect = 3;
	int csc_range = 4;

	extern struct drm_exynos_ipp_property stored_property;

	src.ops_id = EXYNOS_DRM_OPS_SRC;
	dst.ops_id = EXYNOS_DRM_OPS_DST;

	int res = 0;

	res = exynosVideoIppSetProperty( fd, cmd, prop_id, &src, &dst, wb_hz, protect, csc_range );

	fail_if( res != 1, "correct initialization 1 must returned" );

	fail_if( stored_property.refresh_rate != wb_hz, "must be equal 1" );
	fail_if( stored_property.protect != protect, "must be equal 2" );
	fail_if( stored_property.range != csc_range, "must be equal 3" );
}
END_TEST;
//==========================================================================================================

//==========================================================================================================
/* Bool exynosVideoIppQueueBuf (int fd, struct drm_exynos_ipp_queue_buf *buf) */
START_TEST ( exynosVideoIppQueueBuf_fdNegative )
{
	int fd;
	struct drm_exynos_ipp_queue_buf buf;
	Bool res;

	fd = -1;

	res = exynosVideoIppQueueBuf( fd, &buf );

	fail_if( res != FALSE, "FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoIppQueueBuf_bufNull )
{
	Bool res;
	int fd;

	fd = 1;

	//Segmentation fault must not occur
	res = exynosVideoIppQueueBuf( fd, NULL );

	fail_if( res != FALSE, "FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoIppQueueBuf_bufHandleIsZero )
{
	Bool res;
	int fd;
	struct drm_exynos_ipp_queue_buf buf;

	fd = 1;
	buf.handle[0] = 0;

	res = exynosVideoIppQueueBuf( fd, &buf );

	fail_if( res != FALSE, "FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoIppQueueBuf_correctInit )
{
	Bool res;
	int fd;
	struct drm_exynos_ipp_queue_buf buf;

	fd = 1;
	buf.handle[0] = 1;

	res = exynosVideoIppQueueBuf( fd, &buf );

	fail_if( res != TRUE, "TRUE must returned, correct initialization" );
}
END_TEST;
//==========================================================================================================

//==========================================================================================================
/* Bool exynosVideoIppCmdCtrl (int fd, struct drm_exynos_ipp_cmd_ctrl *ctrl) */
START_TEST ( exynosVideoIppCmdCtrl_fdNegative )
{
	int fd;
	struct drm_exynos_ipp_cmd_ctrl ctrl;
	Bool res;

	fd = -1;

	res = exynosVideoIppCmdCtrl( fd, &ctrl );

	fail_if( res != FALSE, "FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoIppCmdCtrl_bufNull )
{
	Bool res;
	int fd;

	fd = 1;

	//Segmentation fault must not occur
	res = exynosVideoIppCmdCtrl( fd, NULL );

	fail_if( res != FALSE, "FALSE must returned" );
}
END_TEST;

START_TEST (exynosVideoIppCmdCtrl_bufHandleIsZero )
{
	Bool res;
	int fd;
	struct drm_exynos_ipp_cmd_ctrl ctrl;

	fd = 1;
	ctrl.prop_id = 0;

	res = exynosVideoIppCmdCtrl( fd, &ctrl );

	fail_if( res != FALSE, "FALSE must returned" );
}
END_TEST;

START_TEST ( exynosVideoIppCmdCtrl_correctInit )
{
	Bool res;
	int fd;
	struct drm_exynos_ipp_queue_buf ctrl;

	fd = 1;
	ctrl.prop_id = 1;

	res = exynosVideoIppCmdCtrl( fd, &ctrl );

	fail_if( res != TRUE, "TRUE must returned, correct initialization" );
}
END_TEST;
//==========================================================================================================

//**********************************************************************************************************
int test_video_ipp_suite (run_stat_data* stat)
{
	Suite *s = suite_create ("VIDEO_IPP.C");
	/* Core test case */
	TCase *tc_exynosVideoIppSetProperty = tcase_create ( "exynosVideoIppSetProperty" );
	TCase *tc_exynosVideoIppQueueBuf    = tcase_create ( "exynosVideoIppQueueBuf" );
	TCase *tc_exynosVideoIppCmdCtrl     = tcase_create ( "exynosVideoIppCmdCtrl" );


    //exynosVideoIppSetProperty
	tcase_add_test ( tc_exynosVideoIppSetProperty, exynosVideoIppSetProperty_fbNegative );
	tcase_add_test ( tc_exynosVideoIppSetProperty, exynosVideoIppSetProperty_srcNull );
	tcase_add_test ( tc_exynosVideoIppSetProperty, exynosVideoIppSetProperty_dstNull );
	tcase_add_test ( tc_exynosVideoIppSetProperty, exynosVideoIppSetProperty_srcOpsIdIsNotEXYNOS_DRM_OPS_SRC );
	tcase_add_test ( tc_exynosVideoIppSetProperty, exynosVideoIppSetProperty_srcOpsIdIsNotEXYNOS_DRM_OPS_DST );
	tcase_add_test ( tc_exynosVideoIppSetProperty, exynosVideoIppSetProperty_cmdNegative );
	tcase_add_test ( tc_exynosVideoIppSetProperty, exynosVideoIppSetProperty_correctInit );

	//exynosVideoIppQueueBuf
	tcase_add_test ( tc_exynosVideoIppQueueBuf, exynosVideoIppQueueBuf_fdNegative );
	tcase_add_test ( tc_exynosVideoIppQueueBuf, exynosVideoIppQueueBuf_bufNull );
	tcase_add_test ( tc_exynosVideoIppQueueBuf, exynosVideoIppQueueBuf_bufHandleIsZero );
	tcase_add_test ( tc_exynosVideoIppQueueBuf, exynosVideoIppQueueBuf_correctInit );

	//exynosVideoIppCmdCtrl
	tcase_add_test ( tc_exynosVideoIppCmdCtrl, exynosVideoIppCmdCtrl_fdNegative );
	tcase_add_test ( tc_exynosVideoIppCmdCtrl, exynosVideoIppCmdCtrl_bufNull );
	tcase_add_test ( tc_exynosVideoIppCmdCtrl, exynosVideoIppCmdCtrl_bufHandleIsZero );
	tcase_add_test ( tc_exynosVideoIppCmdCtrl, exynosVideoIppCmdCtrl_correctInit );

	suite_add_tcase ( s, tc_exynosVideoIppSetProperty );
	suite_add_tcase ( s, tc_exynosVideoIppQueueBuf );
	suite_add_tcase ( s, tc_exynosVideoIppCmdCtrl );


	SRunner *sr = srunner_create( s );
	srunner_run_all( sr, CK_VERBOSE );

    stat->num_failure=srunner_ntests_failed(sr);
    stat->num_pass=srunner_ntests_run(sr) - srunner_ntests_failed(sr);

	stat->num_tests = srunner_ntests_run( sr );
	stat->num_pass = srunner_ntests_run( sr ) - srunner_ntests_failed( sr );
	stat->num_failure = srunner_ntests_failed( sr );

	stat->num_tested = 3;
	stat->num_nottested = 0;

	return 0;
}
