
#ifndef __BK_NVM_H__
#define __BK_NVM_H__



#include "benchmark.h"



struct bk_nvm_block {
	int id;
	char *name;
	unsigned int size;
	unsigned int offset;
	awoke_list _head;
};

struct bk_nvm_struct {
	unsigned int pagesize;
	unsigned int sector_size;
	unsigned int nr_blocks;
	awoke_list blocks;
};



err_type benchmark_nvm_test(int argc, char *argv[]);



#endif /* __BK_NVM_H__ */

