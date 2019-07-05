
#ifndef __SEC_PADDING_H__
#define __SEC_PADDING_H__

#include "awoke_log.h"
#include "awoke_type.h"
#include "awoke_memory.h"
#include "awoke_macros.h"
#include "awoke_package.h"

int sec_align(int in_len, int align);
int sec_bytes_padding(uint8_t *buf, int buf_len, uint8_t c, int align, 
	uint8_t **out);

#endif /* __SEC_PADDING_H__ */
