
#include "qe_quickemb.h"


static char heapbuffer[QE_CONFIG_HEAP_SIZE];

static err_type hw_board_init(void)
{
	qe_heap_init(heapbuffer, 2048);

	qe_assert_set_handle();

	qe_board_init();
}

err_type benchmark_quickemb_test(int argc, char *argv[])
{
	bk_debug("qe test in");

	hw_board_init();

	//application_init();
}
