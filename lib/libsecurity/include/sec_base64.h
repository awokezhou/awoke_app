
#ifndef __SEC_BASE64_H__
#define __SEC_BASE64_H__

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


int sec_base64_encode( unsigned char *dst, size_t dlen, size_t *olen,
                   const unsigned char *src, size_t slen );
int sec_base64_decode( unsigned char *dst, size_t dlen, size_t *olen,
				  const unsigned char *src, size_t slen );
#endif /* __SEC_BASE64_H__ */
