
#include "qe_core.h"



#define __si_range(s, e) \
	static int si_start(void){return 0;}QE_EXPORT(si_start, s);\
	static int si_end(void){return 0;}QE_EXPORT(si_end, e);

#define __si_define(stage, s, e) \
	static int si_##stage##_start(void) {return 0;} QE_EXPORT(si_##stage##_start, s".end");\
	static int si_##stage##_end(void) {return 0;} QE_EXPORT(si_##stage##_end, e".end");\
	void qe_##stage##_init(void) \
	{\
		volatile const init_fn_t *pfn;\
		for (pfn=&__qe_init_si_##stage##_start; pfn<&__qe_init_si_##stage##_end; pfn++) {\
			(*pfn)();\
		}\
	}
	
__si_define(board, 		 "0", "1")
__si_define(prev, 		 "1", "2")
__si_define(device, 	 "2", "3")
__si_define(component, 	 "3", "4")
__si_define(env, 		 "4", "5")
__si_define(application, "5", "6")

__si_range("0", "6.end");


void qe_stage_initialize(void)
{
	volatile const init_fn_t *pfn;

	for (pfn=&__qe_init_si_start; pfn<&__qe_init_si_end; pfn++) {
		(*pfn)();
	}
}

