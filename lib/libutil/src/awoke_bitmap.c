
#include "awoke_bitmap.h"
#include "awoke_log.h"
#include "awoke_string.h"


bool awoke_bitmap_empty(const unsigned int *bitmap, int bits)
{
	int k, lim = bits/BITS_PER_LONG;
	for (k = 0; k < lim; ++k)
		if (bitmap[k])
			return 0;

	if (bits % BITS_PER_LONG)
		if (bitmap[k] & BITMAP_LAST_WORD_MASK(bits))
			return 0;

	return 1;
}

int awoke_bitmap_full(const unsigned int *bitmap, int bits)
{
	int k, lim = bits/BITS_PER_LONG;
	for (k = 0; k < lim; ++k)
		if (~bitmap[k])
			return 0;

	if (bits % BITS_PER_LONG)
		if (~bitmap[k] & BITMAP_LAST_WORD_MASK(bits))
			return 0;

	return 1;
}

void awoke_bitmap_set(unsigned int *map, int start, int nr)
{
	unsigned int *p = map + BIT_WORD(start);
	const int size = start + nr;
	int bits_to_set = BITS_PER_LONG - (start % BITS_PER_LONG);
	unsigned int mask_to_set = BITMAP_FIRST_WORD_MASK(start);

	while (nr - bits_to_set >= 0) {
		*p |= mask_to_set;
		nr -= bits_to_set;
		bits_to_set = BITS_PER_LONG;
		mask_to_set = ~0UL;
		p++;
	}
	if (nr) {
		mask_to_set &= BITMAP_LAST_WORD_MASK(size);
		*p |= mask_to_set;
	}
}

void awoke_bitmap_clear(unsigned int *map, int start, int nr)
{
	unsigned int *p = map + BIT_WORD(start);
	const int size = start + nr;
	int bits_to_clear = BITS_PER_LONG - (start % BITS_PER_LONG);
	unsigned int mask_to_clear = BITMAP_FIRST_WORD_MASK(start);

	while (nr - bits_to_clear >= 0) {
		*p &= ~mask_to_clear;
		nr -= bits_to_clear;
		bits_to_clear = BITS_PER_LONG;
		mask_to_clear = ~0UL;
		p++;
	}
	if (nr) {
		mask_to_clear &= BITMAP_LAST_WORD_MASK(size);
		*p &= ~mask_to_clear;
	}
}

void awoke_bitmap_dump(const unsigned int *bitmap, int size)
{
	int i, linenr;
	char buff[256] = {'\0'};
	build_ptr bp = build_ptr_init(buff, 256);
	
	log_debug("");
	log_debug("-- Bitmap dump --");
	log_debug("size:%d", size);
	log_debug("empty:%d", awoke_bitmap_empty(bitmap, size));
	log_debug("full:%d", awoke_bitmap_full(bitmap, size));
	linenr = size*3 + 5;

	for (i=0; i<linenr; i++) {
		build_ptr_string(bp, "-");
	}
	log_debug("%.*s", bp.len, bp.head);

	memset(buff, 0x0, 256);
	bp = build_ptr_make(buff, 256);
	build_ptr_string(bp, "bit: ");
	for (i=0; i<size; i++) {
		build_ptr_format(bp, "%2d ", i);
	}
	log_debug("%.*s", bp.len, bp.head);

	memset(buff, 0x0, 256);
	bp = build_ptr_make(buff, 256);
	for (i=0; i<linenr; i++) {
		build_ptr_string(bp, "-");
	}
	log_debug("%.*s", bp.len, bp.head);
	
	memset(buff, 0x0, 256);
	bp = build_ptr_make(buff, 256);
	build_ptr_string(bp, "set: ");
	for (i=0; i<size; i++) {
		build_ptr_format(bp, "%2d ", test_bit(i, bitmap));
	}

	log_debug("%.*s", bp.len, bp.head);

	memset(buff, 0x0, 256);
	bp = build_ptr_make(buff, 256);
	for (i=0; i<linenr; i++) {
		build_ptr_string(bp, "-");
	}

	build_ptr_string(bp, "\n");

	log_debug("%.*s", bp.len, bp.head);
}
