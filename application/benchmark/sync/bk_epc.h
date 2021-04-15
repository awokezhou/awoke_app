
#ifndef __BK_EPC_H__
#define __BK_EPC_H__



#include "benchmark.h"
#include "awoke_bitmap.h"



#define BK_EPC_SIZEOF		128
#define BK_EPC_WR_OFFSET	2

#define BK_EPC_E_TESTA	0
#define BK_EPC_E_TESTB	1
#define BK_EPC_E_TESTC	2
#define BK_EPC_E_TESTD	3

#define BK_EPC_MARKER		0x45504350
#define BK_EPC_MAJOR_VER	1
#define BK_EPC_MINOR_VER	0

struct bk_epc_header {

	uint32_t marker;		/* defined as 'MBP_EPC_MARKER' */

	uint16_t version;		/* EPC Protocol Version bit[15:8]:major bit[7:0]:minor */
	uint16_t width:4;		/* data width, 1:8bit 2:16bit 4:32bit 8:64bit */
	uint16_t fdy:1;			/* FPGA ready */
	uint16_t mdy:1;			/* MicroBlaze ready */
	uint16_t rsv1:10;		/* reserve */

	uint16_t data_length;	/* Data field's bytes */
	uint16_t bmap_length;	/* Bitmap field's bytes */

	uint32_t rsv2[5];		/* reserve */
};


struct bk_epc_struct {

	struct bk_epc_header header;

	uint32_t testb;

	uint32_t testc;

	uint32_t testd;

	awoke_bitmap(bitmap, BK_EPC_SIZEOF);
};



err_type benchmark_epc_test(int argc, char *argv[]);



#endif /* __BK_EPC_H__ */
