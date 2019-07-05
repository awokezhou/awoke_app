

#include "sec_padding.h"

int sec_align(int in_len, int align)
{
	if (in_len == align)
		return align;
	else
		return (((in_len/align)+1)*align);
}

uint8_t *sec_string_padding(uint8_t *s, char c, int align, int *rlen) 
{
	int i;
	uint8_t *p;
	uint8_t *new_s;
	size_t len;
	size_t align_len;

	if ((!s) || (align <= 0))
        return NULL;
	
	len = strlen(s);
	if (len==align)
		align_len = align;
	else
		align_len = ((len/align)+1)*align;
	new_s = mem_alloc_z(align_len);
	p = new_s;
	uint8_t *end = &new_s[align_len-1];
	buf_push_stri_safe(*s, p, end, len);

	for (i=0; i<align_len-len; i++) {
		buf_push_byte_safe(c, p, end);
	}
	*rlen = align_len;
	return new_s;
}

