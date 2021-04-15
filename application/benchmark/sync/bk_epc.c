
#include "bk_epc.h"
#include "awoke_worker.h"



static void bk_epc_event_read(struct bk_epc_struct *epc)
{
	int i;

	if (awoke_bitmap_empty(&epc->bitmap[1], 2)) {
		bk_debug("bit 0-2 empty");
	} else {
		bk_debug("bit 0-2 not empty");
	}
	
	for (i=0; i<BK_EPC_SIZEOF; i++) {
		if (awoke_test_bit(i, epc->bitmap)) {
			bk_debug("event:%d", i);
			awoke_clear_bit(i, epc->bitmap);
		}
	}
}

static err_type mbp_work(void *context)
{
	awoke_worker_context *ctx = context;
	awoke_worker *wk = ctx->worker;
	struct bk_epc_struct *epc = wk->data;

	awoke_set_bit(BK_EPC_E_TESTC, epc->bitmap);

	bk_notice("mbp start work");

	while (!awoke_worker_should_stop(wk)) {

		bk_epc_event_read(epc);
		
		sleep(2);		/* idle */

	};

	return et_ok;
}

static err_type fpga_work(void *context)
{
	awoke_worker_context *ctx = context;
	awoke_worker *wk = ctx->worker;
	struct bk_epc_struct *epc = wk->data;

	sleep(2);
	bk_notice("fpga start work");

	while (!awoke_worker_should_stop(wk)) {

		//epc->testa = 15;
		//epc->testb = 16;
		
		//awoke_set_bit(BK_EPC_E_TESTA, epc->bitmap);
		//awoke_set_bit(BK_EPC_E_TESTB, epc->bitmap);
		//awoke_set_bit(BK_EPC_E_TESTC, epc->bitmap);

		//bk_debug("fpga write");
		
		sleep(5);		/* idle */

	};

	return et_ok;
}

void test_bitmap_clear(unsigned int *map, int start, int nr)
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

bool test_bitmap_empty(unsigned int *map, int start, int nr)
{
	unsigned int data, *p = map + BIT_WORD(start);
	const int size = start + nr;
	int bits_to_check = BITS_PER_LONG - (start % BITS_PER_LONG);
	unsigned int mask_to_check = BITMAP_FIRST_WORD_MASK(start);

	while (nr - bits_to_check >= 0) {
		data = *p;
		data &= mask_to_check;
		if (data)
			return FALSE;
		nr -= bits_to_check;
		bits_to_check = BITS_PER_LONG;
		mask_to_check = ~0UL;
		p++;
	}
	if (nr) {
		mask_to_check &= BITMAP_LAST_WORD_MASK(size);
		data = *p;
		data &= mask_to_check;
	}

	return (data==0);
}

static uint16_t bk_epc_version_make(uint8_t major, uint8_t minor)
{
	return ((major<<8)|minor);
}

static void bk_epc_header_init(struct bk_epc_struct *epc)
{
	struct bk_epc_header *h = &epc->header;

	h->marker = BK_EPC_MARKER;
	h->version = bk_epc_version_make(BK_EPC_MAJOR_VER, BK_EPC_MINOR_VER);
	h->width = 4;
	h->fdy = 1;
	h->mdy = 1;
}

static void bk_epc_header_dump(struct bk_epc_struct *epc)
{
	struct bk_epc_header *h = &epc->header;

	awoke_hexdump_trace(h, sizeof(struct bk_epc_header));

	bk_debug("");
	bk_debug("-------- EPC dump --------");
	bk_debug("marker:%s", (char *)&h->marker);
	bk_debug("version:v%d.%d", h->version>>8, h->version&0x0f);
	bk_debug("width:%d", h->width);
	bk_debug("FDY:%d", h->fdy);
	bk_debug("MDY:%d", h->mdy);
	bk_debug("--------------------------");
	bk_debug("");
}

err_type benchmark_epc_test(int argc, char *argv[])
{
	int i;
	struct bk_epc_struct epc;
	memset(&epc, 0x0, sizeof(struct bk_epc_struct));

	/*
	awoke_worker *mbp = awoke_worker_create("mbp", 1,
		WORKER_FEAT_CUSTOM_DEFINE, mbp_work, &epc);

	awoke_worker *fpga = awoke_worker_create("fpga", 1,
		WORKER_FEAT_CUSTOM_DEFINE, fpga_work, &epc);

	for (i=0; i<60; i++) {
		usleep(1000000);
	}*/

	bk_epc_header_init(&epc);
	bk_epc_header_dump(&epc);
		
	bk_debug("%d", test_bitmap_empty(&epc.bitmap, 70, 5));
	awoke_bitmap_set(epc.bitmap, 64, 16);
	bk_debug("%d", test_bitmap_empty(&epc.bitmap, 70, 5));
	awoke_bitdump_trace(epc.bitmap, BK_EPC_SIZEOF);

	test_bitmap_clear(epc.bitmap, 70, 16);
	bk_debug("%d", test_bitmap_empty(&epc.bitmap, 70, 5));
	awoke_bitdump_trace(epc.bitmap, BK_EPC_SIZEOF);

	//awoke_worker_stop(mbp);
	//awoke_worker_destroy(mbp);

	//awoke_worker_stop(fpga);
	//awoke_worker_destroy(fpga);

	return et_ok;
}
