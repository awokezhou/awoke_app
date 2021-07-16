
#include "qe_board.h"


void qe_board_init(void)
{
	volatile const qe_initfn_t *pfn;

	for (pfn = &__rt_init_rti_board_start; pfn < &__rt_init_rti_board_end; pfn++) {
        (*pfn)();
    }
}
