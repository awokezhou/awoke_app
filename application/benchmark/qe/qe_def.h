
#ifndef __QE_DEF_H__
#define __QE_DEF_H__



typedef int (*qe_initfn_t)(void);

#define QE_SECTION(x)	 __attribute__((section(x)))

#define QE_INIT_EXPORT(fn, level) \
	__attribute__((used)) const qe_initfn_t __qe_init_##fn QE_SECTION(".qei_fn."level) = fn
#define QE_BOARD_EXPORT(fn)           QE_INIT_EXPORT(fn, "1")


#define QE_NAME_MAX	8

#endif /* __QE_DEF_H__ */

