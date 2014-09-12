#ifndef FAKE_TBM_H_
#define FAKE_TBM_H_

#include "fake_mem.h"

extern void *find_any_tbo();
extern void ctrl_print_mem();
extern unsigned ctrl_get_cnt_calloc();

typedef struct _tbm_bo {
    int ref_cnt; /**< ref count of bo */
    int map_cnt;
    int size;
    void* ptr;
    void* user_data;
} tbo_item;

void print_tbm(tbo_item*bo);


#endif /* FAKE_TBM_H_ */
