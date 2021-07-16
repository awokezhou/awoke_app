
#include "qe_heap.h"



void qe_heap_init(uint32_t start, uint32_t len)
{
	bpool(start, len);
}

void qe_heap_clean(void)
{
	cleanup_bpool
}
