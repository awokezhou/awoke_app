
#include "bk_nvm.h"



static struct bk_nvm_block nvmblocks[] = {
	{
		.name = "userset",
		.size = 0x3000,
		.offset = 0x203000,
	},
	{
		.name = "factory",
		.size = 0x300,
		.offset = 0x204000,
	},
	{
		.name = "sensor",
		.size = 0x400,
		.offset = 0x205000,
	},
};

static err_type bk_nvm_blocks_add(struct bk_nvm_struct *m, struct bk_nvm_block *blocks, int nr)
{
	struct bk_nvm_block *n, *p, *head;

	head = blocks;

	array_foreach_start(head, nr, p) {
		
		n = mem_alloc_z(sizeof(struct bk_nvm_block));
		n->name = awoke_string_dup(p->name);
		n->size = p->size;
		n->offset = p->offset;
		n->id = m->nr_blocks;
		list_append(&n->_head, &m->blocks);
		m->nr_blocks++;
		
	} array_foreach_end();

	bk_trace("nvm blocks:");
	list_for_each_entry(p, &m->blocks, _head) {
		if (p->size >= 0x400) {
			bk_trace("  %s 0x%x 0x%xKB", p->name, p->offset, (p->size >> 10));
		} else {
			bk_trace("  %s 0x%x 0x%xB", p->name, p->offset, p->size);
		}
	}

	return et_ok;
}

static err_type nvm_sector_erase(struct bk_nvm_struct *m, uint32_t addr)
{
	bk_trace("sector erase:0x%x", addr);
	return et_ok;
}

static err_type nvm_write(struct bk_nvm_struct *m, uint32_t addr, void *buf, int len)
{
	return et_ok;
}

static err_type nvm_block_write(struct bk_nvm_struct *m, struct bk_nvm_block *block, void *buf, int len)
{
	uint32_t addr;
	
	if (len > block->size) {
		bk_err("write len:%d too big, blocksize:%d", len, block->size);
		return et_fail;
	}

	for (addr=block->offset; addr<(block->offset+len); addr+=m->sector_size) {
		nvm_sector_erase(m, addr);
	}

	return nvm_write(m, block->offset, buf, len);
}

static err_type bk_nvm_block_write(struct bk_nvm_struct *m, int num, void *buf, int len)
{
	struct bk_nvm_block *p;
	
	bk_debug("block[%d] write");

	list_for_each_entry(p, &m->blocks, _head) {

		if (p->id == num) {
			bk_trace("find block %s", p->name);
			return nvm_block_write(m, p, buf, len);
		}
	}

	return et_notfind;
}

err_type benchmark_nvm_test(int argc, char *argv[])
{
	int i;
	struct bk_nvm_struct nvm;
	uint8_t buffer[32] = {0x0};

	char *string = "EPCP";
	awoke_hexdump_trace(string, strlen(string));

	list_init(&nvm.blocks);
	nvm.nr_blocks = 0;
	nvm.sector_size = 4096;

	bk_nvm_blocks_add(&nvm, nvmblocks, array_size(nvmblocks));

	for (i=0; i<32; i++) {
		buffer[i] = i;
	}

	bk_nvm_block_write(&nvm, 0, buffer, 8193);

	return et_ok;
}

