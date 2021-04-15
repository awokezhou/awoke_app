
#ifndef __AWOKE_BITMAP_H__
#define __AWOKE_BITMAP_H__



#include "awoke_type.h"
#include "awoke_error.h"



#define BITS_PER_BYTE		8
#define BITS_PER_LONG		32
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define BIT_WORD(nr)		((nr) / BITS_PER_LONG)
#define bits_to_long(nr)	DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(int))

#define awoke_bitmap(name,bits) \
	unsigned int name[bits_to_long(bits)]

#define BITMAP_FIRST_WORD_MASK(start) (~0UL << ((start) % BITS_PER_LONG))
#define BITMAP_LAST_WORD_MASK(nbits)					\
(									\
	((nbits) % BITS_PER_LONG) ?					\
		(1UL<<((nbits) % BITS_PER_LONG))-1 : ~0UL		\
)

#if defined(AWOKE_HAVE_ATOMIC)
static inline void ____atomic_set_bit(unsigned int bit, volatile unsigned int *p)
{
	unsigned int mask = 1UL << (bit & 31);

	p += bit >> 5;

	raw_local_irq_save(flags);
	*p |= mask;
	raw_local_irq_restore(flags);
}

static inline void ____atomic_clear_bit(unsigned int bit, volatile unsigned int *p)
{
	unsigned int flags;
	unsigned int mask = 1UL << (bit & 31);

	p += bit >> 5;

	raw_local_irq_save(flags);
	*p &= ~mask;
	raw_local_irq_restore(flags);
}

static inline void ____atomic_change_bit(unsigned int bit, volatile unsigned int *p)
{
	unsigned int flags;
	unsigned int mask = 1UL << (bit & 31);

	p += bit >> 5;

	raw_local_irq_save(flags);
	*p ^= mask;
	raw_local_irq_restore(flags);
}
#else
static inline void _set_bit(unsigned int bit, volatile unsigned int *p)
{
	unsigned int mask = 1UL << (bit & 31);

	p += bit >> 5;

	*p |= mask;
}

static inline void _clear_bit(unsigned int bit, volatile unsigned int *p)
{
	unsigned int mask = 1UL << (bit & 31);

	p += bit >> 5;

	*p &= ~mask;
}

static inline void _change_bit(unsigned int bit, volatile unsigned int *p)
{
	unsigned int mask = 1UL << (bit & 31);

	p += bit >> 5;

	*p ^= mask;
}
#endif

static inline int test_bit(int nr, const volatile unsigned int *addr)
{
	return 1UL & (addr[BIT_WORD(nr)] >> (nr & (BITS_PER_LONG-1)));
}

#if defined(AWOKE_HAVE_ATOMIC)
#if defined(AWOKE_ARM_PLATFORM)
#define awoke_bitop(name,nr,p)			\
	(__builtin_constant_p(nr) ? ____atomic_##name(nr, p) : _##name(nr,p))
#else
#define awoke_bitop(name,nr,p)		_##name(nr,p)
#endif
#else
#define awoke_bitop(name,nr,p)		_##name(nr,p)
#endif


#define awoke_set_bit(bit, map)		awoke_bitop(set_bit, bit, map)
#define awoke_clear_bit(bit, map)	awoke_bitop(clear_bit, bit, map)
#define awoke_change_bit(bit, map)	awoke_bitop(change_bit, bit, map)
#define awoke_test_bit(bit, map)	test_bit(bit, map)



bool awoke_bitmap_empty(const unsigned int *bitmap, int bits);
void awoke_bitmap_dump(const unsigned int *bitmap, int size);
void awoke_bitmap_clear(unsigned int *map, int start, int nr);
void awoke_bitmap_set(unsigned int *map, int start, int nr);
int awoke_bitmap_full(const unsigned int *bitmap, int bits);
#endif /* __AWOKE_BITMAP_H__ */
