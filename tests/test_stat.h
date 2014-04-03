/*
 * test_stat.h
 *
 *  Created on: Nov 14, 2013
 *      Author: svoloshynov
 */

#ifndef TEST_STAT_H_
#define TEST_STAT_H_


typedef struct _run_stat_data{
	unsigned num_func;
    unsigned num_tested;
    unsigned num_nottested;
    unsigned num_cases;
	unsigned num_tests;
    unsigned num_pass;
    unsigned num_failure;
} run_stat_data;


#endif /* TEST_STAT_H_ */
