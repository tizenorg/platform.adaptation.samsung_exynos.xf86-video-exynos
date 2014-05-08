/*
 * test_video_image.c
 *
 *  Created on: Oct 21, 2013
 *      Author: svoloshynov
 */
#include "test_video_image.h"
#include "exynos_driver.h"

#include "./xv/exynos_video_image.c"

#define PRINTVAL( v ) printf( " %d\n", v );

//***************************************** Defines ****************************************
//************************************* Fake functions *************************************
//*************************************** Unit tests ***************************************
//==========================================================================================
/* static Bool _exynosVideoImageCompareIppId (pointer structure, pointer element) */
START_TEST ( exynosVideoImageCompareIppId_idsIsNotEquals )
{
	EXYNOSIppPtr pIpp_data = ctrl_calloc( 1, sizeof( EXYNOSIpp ) );
	pIpp_data->prop_id = 1;
	unsigned int element = 2;
	fail_if( _exynosVideoImageCompareIppId( pIpp_data, &element ), "ids not equal 0 must returned" );
}
END_TEST;

START_TEST ( exynosVideoImageCompareIppId_idsIsEquals )
{
	EXYNOSIppPtr pIpp_data = ctrl_calloc( 1, sizeof( EXYNOSIpp ) );
	pIpp_data->prop_id = 1;
	unsigned int element = 1;
	fail_if( !_exynosVideoImageCompareIppId( pIpp_data, &element ), "ids are equal 1 must returned" );
}
END_TEST;
//==========================================================================================

//==========================================================================================
/* static Bool _exynosVideoImageCompareBufIndex (pointer structure, pointer element) */
START_TEST ( exynosVideoImageCompareBufIndex_indexessIsNotEquals )
{
	EXYNOSIppBuf* ippbufinfo = ctrl_calloc( 1, sizeof( EXYNOSIppBuf ) );
	ippbufinfo->index = 1;
	unsigned int index = 2;
	fail_if( _exynosVideoImageCompareBufIndex( ippbufinfo, &index ), "indexes not equal 0 must returned" );
}
END_TEST;

START_TEST ( exynosVideoImageCompareBufIndex_indexessIsEquals )
{
	EXYNOSIppBuf* ippbufinfo = ctrl_calloc( 1, sizeof( EXYNOSIppBuf ) );
	ippbufinfo->index = 1;
	unsigned int index = 1;
	fail_if( !_exynosVideoImageCompareBufIndex( ippbufinfo, &index ), "indexes are equal 1 must returned" );
}
END_TEST;
//==========================================================================================

//==========================================================================================
/* static Bool _exynosVideoImageCompareStamp (pointer structure, pointer element) */
START_TEST ( exynosVideoImageCompareStamp_serialNumbersIsNotEquals )
{
	PixmapPtr pPixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );
	pPixmap->drawable.serialNumber = 1;
	unsigned int serialNumber = 2;
	fail_if( _exynosVideoImageCompareStamp( pPixmap, &serialNumber ), "Serial numbers are not equal 0 must returned" );
}
END_TEST;

START_TEST ( exynosVideoImageCompareStamp_serialNumbersIsEquals )
{
	PixmapPtr pPixmap = ctrl_calloc( 1, sizeof( PixmapRec ) );
	pPixmap->drawable.serialNumber = 1;
	unsigned int serialNumber = 1;
	fail_if( !_exynosVideoImageCompareStamp( pPixmap, &serialNumber ), "Serial numbers are equal 1 must returned" );
}
END_TEST;
//==========================================================================================

//==========================================================================================
/* XF86ImagePtr exynosVideoImageGetImageInfo (int id) */
START_TEST ( exynosVideoImageGetImageInfo_idLessThenZero )
{
	int id = -1;
	fail_if( exynosVideoImageGetImageInfo( id ), "id less then zero, 0 must returned" );
}
END_TEST;

START_TEST ( exynosVideoImageGetImageInfo_CorrectInit )
{
	int id = FOURCC_YUY2;
	fail_if( !exynosVideoImageGetImageInfo( id ), "Correct Init, 1 must returned" );
}
END_TEST;
//==========================================================================================

//==========================================================================================
/* static Bool _exynosVideoImageIsZeroCopy (int id) */
START_TEST ( exynosVideoImageIsZeroCopy_ )
{
	int a[5];

	int c = sizeof( a ) / sizeof( a[0] );

	PRINTVAL( sizeof( a ) )
	PRINTVAL( sizeof( a[0] ) )
	PRINTVAL( c )
}
END_TEST;

//==========================================================================================

//******************************************************************************************
int test_video_image_suite ( run_stat_data* stat )
{
	Suite *s = suite_create ( "VIDEO_IMAGE.C" );
	/* Core test case */

	//_exynosVideoImageCompareIppId
	TCase *tc_exynosVideoImageCompareIppId = tcase_create ( "_exynosVideoImageCompareIppId" );
	tcase_add_test ( tc_exynosVideoImageCompareIppId, exynosVideoImageCompareIppId_idsIsNotEquals );
	tcase_add_test ( tc_exynosVideoImageCompareIppId, exynosVideoImageCompareIppId_idsIsEquals );
	suite_add_tcase ( s, tc_exynosVideoImageCompareIppId );

	//_exynosVideoImageCompareBufIndex
	TCase *tc_exynosVideoImageCompareBufIndex = tcase_create ( "_exynosVideoImageCompareBufIndex" );
	tcase_add_test ( tc_exynosVideoImageCompareBufIndex, exynosVideoImageCompareBufIndex_indexessIsNotEquals );
	tcase_add_test ( tc_exynosVideoImageCompareBufIndex, exynosVideoImageCompareBufIndex_indexessIsEquals );
	suite_add_tcase ( s, tc_exynosVideoImageCompareBufIndex );

	//_exynosVideoImageCompareStamp
	TCase *tc_exynosVideoImageCompareStamp = tcase_create ( "_exynosVideoImageCompareStamp" );
	tcase_add_test ( tc_exynosVideoImageCompareStamp, exynosVideoImageCompareStamp_serialNumbersIsNotEquals );
	tcase_add_test ( tc_exynosVideoImageCompareStamp, exynosVideoImageCompareStamp_serialNumbersIsEquals );
	suite_add_tcase ( s, tc_exynosVideoImageCompareStamp );

	//exynosVideoImageGetImageInfo
	TCase *tc_exynosVideoImageGetImageInfo = tcase_create ( "exynosVideoImageGetImageInfo" );
	tcase_add_test ( tc_exynosVideoImageGetImageInfo, exynosVideoImageGetImageInfo_idLessThenZero );
	tcase_add_test ( tc_exynosVideoImageGetImageInfo, exynosVideoImageGetImageInfo_CorrectInit );
	suite_add_tcase ( s, tc_exynosVideoImageGetImageInfo );

	//_exynosVideoImageIsZeroCopy
	TCase *tc_exynosVideoImageIsZeroCopy = tcase_create ( "_exynosVideoImageIsZeroCopy" );
	tcase_add_test ( tc_exynosVideoImageIsZeroCopy, exynosVideoImageIsZeroCopy_ );
	suite_add_tcase ( s, tc_exynosVideoImageIsZeroCopy );


	SRunner *sr = srunner_create(s);
	srunner_run_all(sr, CK_VERBOSE);

	stat->num_tests = srunner_ntests_run( sr );
	stat->num_pass = srunner_ntests_run( sr ) - srunner_ntests_failed( sr );
	stat->num_failure = srunner_ntests_failed( sr );

	return 0;
}
