
#include <getopt.h>
#include "memtrace.h"
#include "bget.h"

static struct memtrace_context memtrace_ctx;


static inline unsigned int bkdrhash(const char *str)
{
	unsigned int seed = 131;
	unsigned int hash = 0;

	while (*str)
		hash = hash * seed + (*str++);

	return hash & 0x7FFFFFFF;
}

static struct memtrace_node *memtrace_node_find(const char *name)
{
	unsigned int index;
	struct memtrace_node *node, *pos;

	index = bkdrhash(name) % memtrace_ctx.hashsize;
	node = &memtrace_ctx.root[index];
	
	if (strncmp(node->name, name, MEMTRACE_NAME_SIZE) == 0) {
		return node;
	}

	list_foreach(pos, &node->list, list) {
		if (strncmp(pos->name, name, MEMTRACE_NAME_SIZE) == 0) {
			return pos;
		}
	}

	return NULL;
}

static void memtrace_node_init(struct memtrace_node *node, const char *name, uint8_t type)
{
	int name_len;
	struct memtrace_sequence *ps;

	node->type = type;
	name_len = (strlen(name) > MEMTRACE_NAME_SIZE) ? MEMTRACE_NAME_SIZE : strlen(name);
	memcpy(node->name, name, name_len);
	
	if ((type == MEMTRACE_COUNTER) || (type == MEMTRACE_VARIABLE)) {
		struct memtrace_number *pn;
		node->param = bget(sizeof(struct memtrace_number));
		if (node->param == NULL) {
			bk_err("alloc param err");
			return;
		}
		pn = (struct memtrace_number *)node->param;
		pn->num = 0;
	} else if (type == MEMTRACE_EVENT) {
		node->param = bget(sizeof(struct memtrace_sequence));
		ps = (struct memtrace_sequence *)node->param;
		memset(&ps->queue, 0x0, sizeof(struct awoke_ringq));
		ps->buffer = NULL;
	} else if (type == MEMTRACE_CUSTOM_EVENT) {
		struct memtrace_cev_seq *pcs;
		node->param = bget(sizeof(struct memtrace_cev_seq));
		pcs = (struct memtrace_cev_seq *)node->param;
		ps = (struct memtrace_sequence *)node->param;
		memset(&ps->queue, 0x0, sizeof(struct awoke_ringq));
		ps->buffer = NULL;
		list_init(&pcs->pinfo);
	} else if (type == MEMTRACE_LOG) {
		struct memtrace_buffer *pb;
		node->param = bget(sizeof(struct memtrace_buffer));
		pb = (struct memtrace_buffer *)node->param;
		pb->buffer = NULL;
	}
}

static unsigned int node_param_size(struct memtrace_node *node)
{
	unsigned size = 0;
	struct memtrace_sequence *ps;
	
	switch (node->type) {

	case MEMTRACE_COUNTER:
	case MEMTRACE_VARIABLE:
		size = sizeof(unsigned int);
		break;
	
	case MEMTRACE_EVENT:
	case MEMTRACE_CUSTOM_EVENT:
		ps = (struct memtrace_sequence *)node->param;
		size = ps->queue.depth;
		break;
	}
	
	return size;
}

static unsigned int total_param_size(void)
{
	unsigned int size;
	struct memtrace_node *n, *sub;
	
	array_foreach(memtrace_ctx.root, memtrace_ctx.hashsize, n) {
		if (n->type == MEMTRACE_NONE) {
			continue;
		}
		size += node_param_size(n);
		list_foreach(sub, &n->list, list) {
			size += node_param_size(sub);
		}
	};

	return size;
}

err_type memtrace_tracer_add(const char *name, unsigned int type)
{
	unsigned int index;
	struct memtrace_node *node, *n;
	
	node = memtrace_node_find(name);
	if (node != NULL) {
		bk_err("%s exist");
		return err_exist;
	}

	index = bkdrhash(name) % memtrace_ctx.hashsize;
	node = &memtrace_ctx.root[index];

	if (node->type != MEMTRACE_NONE) {
		/* we use zipper method to resolve hash collisions */
		n = bget(sizeof(struct memtrace_node));
		memtrace_node_init(n, name, type);
		list_append(&n->list, &node->list);
		memtrace_ctx.ntracers++;
		bk_debug("tracer[%d] %s append", index, name);
		return et_ok;
	}

	memtrace_node_init(node, name, type);
	memtrace_ctx.ntracers++;
	bk_debug("tracer[%d] %s add", index, name);
	return et_ok;
}

void memtrace_add_value(char *name, const char *val_name, unsigned int val_size)
{
	unsigned int namelen;
	struct memtrace_paraminfo *info;
	struct memtrace_cev_seq *pcs;
	struct memtrace_node *node = memtrace_node_find(name);
	if (node == NULL) {
		bk_err("%s not find");
		return;
	}

	if (node->type != MEMTRACE_CUSTOM_EVENT) {
		return;
	}

	pcs = (struct memtrace_cev_seq *)node->param;
	info = bget(sizeof(struct memtrace_paraminfo));
	namelen = (strlen(val_name) > MEMTRACE_NAME_SIZE) ? MEMTRACE_NAME_SIZE : strlen(val_name);
	memcpy(info->name, val_name, namelen);
	info->size = val_size;
	list_append(&info->list, &pcs->pinfo);
}

err_type memtrace_tracer_set_queue(const char *name, unsigned int size, unsigned int length)
{
	struct memtrace_node *node;
	struct memtrace_sequence *ps;

	node = memtrace_node_find(name);
	if ((node == NULL) || (node->param == NULL)) {
		bk_err("%s not find", name);
		return err_notfind;
	}

	if ((node->type != MEMTRACE_EVENT) && (node->type != MEMTRACE_CUSTOM_EVENT)) {
		return err_notsupport;
	}

	ps = (struct memtrace_sequence *)node->param;
	if (ps->buffer) {
		bk_err("%s's queue exist", name);
		return err_param;
	}
	if ((node->type == MEMTRACE_EVENT) && (size != sizeof(struct memtrace_event))) {
		bk_err("event size must be %s", sizeof(struct memtrace_event));
		return err_param;
	}
	ps->buffer = bget(size*length);
	if (ps->buffer == NULL) {
		bk_err("queue buffer alloc err");
		return err_oom;
	}
	awoke_ringq_init(&ps->queue, ps->buffer, size, length);
	
	bk_debug("%s create queue:%d %d", name, size, length);

	return et_ok;
}

err_type memtrace_record(const char *name, void *data)
{
	struct memtrace_node *node;
	struct memtrace_number *pn;
	struct memtrace_sequence *ps;

	node = memtrace_node_find(name);
	if (node == NULL) {
		bk_err("%s not find", name);
		return err_notfind;
	}

	switch (node->type) {

	case MEMTRACE_COUNTER:
		pn = (struct memtrace_number *)node->param;
		pn->num++;
		break;

	case MEMTRACE_VARIABLE:
		pn = (struct memtrace_number *)node->param;
		pn->num = *(uint32_t *)data;
		break;

	case MEMTRACE_EVENT:
	case MEMTRACE_CUSTOM_EVENT:
		ps = (struct memtrace_sequence *)node->param;
		if (awoke_ringq_full(&ps->queue)) {
			char *pe = bget(ps->queue.node_size);
			awoke_ringq_deq(&ps->queue, pe, 1);
			brel(pe);
		}
		awoke_ringq_enq(&ps->queue, data, 1);
		break;
	}

	return et_ok;
}

unsigned int memtrace_free(void)
{
	long totfree;
	long maxfree;
	long curalloc;
	long nget, nrel;
	bstats(&curalloc, &totfree, &maxfree, &nget, &nrel);
	bk_debug("alloc:%d totfree:%d maxfree:%d nget:%d nrel:%d",
		curalloc, totfree, maxfree, nget, nrel);
	return ((unsigned int)totfree);
}

err_type memtrace_init(unsigned int hashsize, char *buf, unsigned int bufsz)
{
	struct memtrace_node *pos;
	unsigned int hash_memsize;

	hash_memsize = sizeof(struct memtrace_node)*hashsize;
	if (hash_memsize > bufsz) {
		bk_err("need:%d give:%d", sizeof(struct memtrace_node)*hashsize, bufsz);
		return err_oom;
	}

	bpool(buf, bufsz);
	
	memtrace_ctx.buf = buf;
	memtrace_ctx.bufsz = bufsz;
	memtrace_ctx.hashsize = hashsize;
	if (hashsize > 0) {
		memtrace_ctx.root = bget(sizeof(struct memtrace_node)*hashsize);
		if (memtrace_ctx.root == NULL) {
			bk_err("hash mem alloc err");
			return err_oom;
		}

		array_foreach(memtrace_ctx.root, hashsize, pos) {
			list_init(&pos->list);
		};
	}


	bk_debug("memtrace hashsize:%d created at %d %d", hash_memsize, buf, bufsz);

	return et_ok;
}

void memtrace_log_init(unsigned int size)
{
	unsigned int free = memtrace_free();
	if (size > free) {
		bk_err("no mem");
		return;
	}

	memtrace_ctx.logbuf = bget(size);
	if (!memtrace_ctx.logbuf) {
		bk_err("alloc log err");
		return;
	}
	memtrace_ctx.log_bufsz = size;
	
	awoke_ringbuffer_init(&memtrace_ctx.rb_log, memtrace_ctx.logbuf, memtrace_ctx.log_bufsz);
}

void memtrace_log_write(char *str, unsigned int length)
{
	if (memtrace_ctx.logbuf == NULL) {
		return;
	}

	awoke_ringbuffer_write(&memtrace_ctx.rb_log, str, length);
}

void memtrace_log_export(char *buf)
{
	char *pos = (char *)buf;
	struct memtrace_export_log head;
	struct awoke_ringbuffer *rb = &memtrace_ctx.rb_log;
	
	/* export head */
	head.head = rb->head;
	head.tail = rb->tail;
	head.size = rb->length;
	pkg_push_bytes(head, pos, sizeof(struct memtrace_export_log));
	bk_debug("head:%d tail:%d", rb->head, rb->tail);

	/* export logbuf */
	pkg_push_stris(memtrace_ctx.logbuf, pos, memtrace_ctx.log_bufsz);
}

void memtrace_log_import(char *buf)
{
	char *pos = (char *)buf;
	struct memtrace_export_log head;
	struct awoke_ringbuffer *rb = &memtrace_ctx.rb_log;

	/* import head */
	pkg_pull_bytes(head, pos, sizeof(struct memtrace_export_log));
	bk_debug("head:%d tail:%d, size:%d", head.head, head.tail, head.size);

	memtrace_ctx.logbuf = bget(head.size);
	if (memtrace_ctx.logbuf == NULL) {
		bk_err("alloc err");
		return;
	}
	memcpy(memtrace_ctx.logbuf, pos, head.size);
	memtrace_ctx.log_bufsz = head.size;

	rb->head = head.head;
	rb->tail = head.tail;
	rb->count = head.size;
	rb->buf = memtrace_ctx.logbuf;
	rb->length = head.size;
}

unsigned int memtrace_get_log_export_size(void)
{
	unsigned int size = sizeof(struct memtrace_export_log) + memtrace_ctx.log_bufsz;
	return size;
}

static void node_clean(struct memtrace_node *node)
{
	struct memtrace_sequence *ps;
	struct memtrace_cev_seq *pcs, *temp;
	struct memtrace_paraminfo *pinfo;
	
	if ((node->type == MEMTRACE_COUNTER) || (node->type == MEMTRACE_VARIABLE)) {
		brel(node->param);
		node->param = NULL;
	} else if (node->type == MEMTRACE_EVENT) {
		ps = (struct memtrace_sequence *)node->param;
		brel(ps->buffer);
		brel(node->param);
	} else if (node->type == MEMTRACE_CUSTOM_EVENT) {
		pcs = (struct memtrace_cev_seq *)node->param;
		list_for_each_entry_safe(pinfo, temp, &pcs->pinfo, list) {
			list_unlink(&pinfo->list);
			brel(pinfo);
		}
		brel(pcs->seq.buffer);
		brel(node->param);
	}
}

void memtrace_clean(void)
{
	struct memtrace_node *n, *sub, *temp;

	/*
	 * MemTrace clean steps:
	 * 	 1. clean up conflicting nodes "sub" in hash zipper
	 *   2. remove "sub" node from hash zipper
	 *   3. free "sub" node memory
	 *   4. clean hash node "n"
	 *   5. free hash memory
	 *   6. free log memory
	 *
	 * Node clean steps:
	 *   1. free param memory
	 *	 	1.1 Counter and Variable only free param
	 *		1.2 Event need free queue
	 *		1.3 CustomEvent need free queue and paraminfo
	 */

	if (memtrace_ctx.hashsize > 0) {
	
		array_foreach(memtrace_ctx.root, memtrace_ctx.hashsize, n) {

			if (n->type == MEMTRACE_NONE)
				continue;

			list_for_each_entry_safe(sub, temp, &n->list, list) {
				node_clean(sub);
				list_unlink(&sub->list);
				brel(sub);
			}

			node_clean(n);
		};

		brel(memtrace_ctx.root);
	}

	awoke_ringbuffer_clear(&memtrace_ctx.rb_log);
	brel(memtrace_ctx.logbuf);
	memtrace_ctx.log_bufsz = 0;

	cleanup_bpool();
	
	memtrace_ctx.buf      = NULL;
	memtrace_ctx.root     = NULL;
	memtrace_ctx.bufsz    = 0;
	memtrace_ctx.hashsize = 0;
	memtrace_ctx.ntracers = 0;	
}

void number_test(void)
{
	int i;
	unsigned int data = 0;
	
	for (i=0; i<1000; i++) {
		srand(time(0));
		data = rand();
		memtrace_record("TaskACnt", NULL);
		memtrace_record("TaskBVar", &data);
	}
}

void event_test(void)
{
	int i;
	struct memtrace_event event;
	struct memtrace_ae ae;
	
	for (i=0; i<1825; i++) {
		event.time = i;
		srand(event.time);
		event.data = rand();
		memtrace_record("XferEvt", &event);

		ae.expo = i;
		ae.gain = 1000 - i;
		memtrace_record("AECev", &ae);
	}
}

unsigned int memtrace_get_export_size(void)
{
	unsigned int size = 0;
	struct memtrace_node *node;

	size += sizeof(struct memtrace_export_head);
	size += memtrace_ctx.ntracers * sizeof(struct memtrace_export_node);
	size += total_param_size();
	size += memtrace_ctx.ntracers * MEMTRACE_NAME_SIZE;

	return size;
}

static void node_export(struct memtrace_node *node, char *buf, 
	char **pos, char **pos_name, char **pos_param)
{
	char *event;
	struct memtrace_number *pn;
	struct memtrace_sequence *ps;
	struct memtrace_export_node expn;

	expn.type = node->type;
	expn.name_offset = *pos_name - (char *)buf;
	expn.name_length = MEMTRACE_NAME_SIZE;
	expn.param_offset = *pos_param - (char *)buf;
	expn.param_length = node_param_size(node);
	bk_debug("node %s noff:%d nlen:%d poff:%d plen:%d",
		node->name, expn.name_offset, expn.name_length,
		expn.param_offset, expn.param_length);
	pkg_push_bytes(expn, *pos, sizeof(expn));

	/* export node's param */
	if ((node->type == MEMTRACE_COUNTER) || (node->type == MEMTRACE_VARIABLE)) {
		pn = (struct memtrace_number *)node->param;
		pkg_push_dwrd(pn->num, *pos_param);
	} else if ((node->type == MEMTRACE_EVENT) || (node->type == MEMTRACE_CUSTOM_EVENT)) {
		ps = (struct memtrace_sequence *)node->param;
		while (!awoke_ringq_empty(&ps->queue)) {
			event = mem_alloc_z(ps->queue.node_size);
			awoke_ringq_deq(&ps->queue, event, 1);
			pkg_push_bytes(event, *pos_param, ps->queue.node_size);
			mem_free(event);
		};
	}

	/* export node's name */
	pkg_push_stris(node->name, *pos_name, MEMTRACE_NAME_SIZE);
}

void memtrace_export(void *buf, unsigned int len)
{
	char *pos, *pos_name, *pos_param;
	char *hash_start, *param_start, *name_start;
	unsigned int param_size;
	struct memtrace_node *node, *subnode;
	struct memtrace_export_node expn;
	struct memtrace_export_head head;

	/* calculate total params size */
	param_size = total_param_size();
	bk_debug("param size:%d", param_size);

	hash_start = (char *)buf + sizeof(struct memtrace_export_head);
	param_start = hash_start + sizeof(struct memtrace_export_node)*memtrace_ctx.ntracers;
	name_start = param_start + param_size;
	bk_debug("buf:0x%x", buf);
	bk_debug("head size:%d node size:%d", sizeof(struct memtrace_export_head), 
		sizeof(struct memtrace_export_node));
	bk_debug("hash start:%d", hash_start-(char *)buf);
	bk_debug("param start:%d", param_start-(char *)buf);
	bk_debug("name start:%d", name_start-(char *)buf);
		
	/* export construct */
	head.marker = 0xBBCCCCBB;
	head.version = 0x00000001;
	head.ntracers = memtrace_ctx.ntracers;
	head.hashsize = memtrace_ctx.hashsize;
	pos = (char *)buf;
	pkg_push_dwrd(head.marker, pos);
	pkg_push_dwrd(head.version, pos);
	pkg_push_dwrd(head.hashsize, pos);
	pkg_push_dwrd(head.ntracers, pos);

	/* export node */
	pos = hash_start;
	pos_param = param_start;
	pos_name = name_start;
	array_foreach(memtrace_ctx.root, memtrace_ctx.hashsize, node) {

		if (node->type == MEMTRACE_NONE) {
			continue;
		}

		node_export(node, buf, &pos, &pos_name, &pos_param);

		list_foreach(subnode, &node->list, list) {
			node_export(subnode, buf, &pos, &pos_name, &pos_param);
		}
	}
}

void memtrace_import(void *buf, unsigned int length)
{
	int i;
	char *pos;
	char name[MEMTRACE_NAME_SIZE] = {'\0'};
	char *hash_start, *param_start, name_start;
	struct memtrace_export_node expn;
	struct memtrace_export_head exph;

	pos = (char *)buf;
	pkg_pull_bytes(exph, pos, sizeof(struct memtrace_export_head));

	bk_debug("marker:0x%x version:0x%x hashsize:%d ntracers:%d",
		exph.marker, exph.version, exph.hashsize, exph.ntracers);

	hash_start = (char *)buf + sizeof(struct memtrace_export_head);

	pos = hash_start;
	for (i=0; i<exph.ntracers; i++) {
		pkg_pull_bytes(expn, pos, sizeof(struct memtrace_export_node));
		bk_debug("type:%d noff:%d nlen:%d poff:%d plen:%d", 
			expn.type, expn.name_offset, expn.name_length,
			expn.param_offset, expn.param_length);
		memcpy(name, (char *)buf+expn.name_offset, expn.name_length);
		bk_debug("name:%s", name);

		memtrace_tracer_add(name, expn.type);
		if (expn.type == MEMTRACE_EVENT) {
			memtrace_tracer_set_queue(name, sizeof(struct memtrace_event), 10);
		}
	}
}

void memtrace_dump(void)
{
	int i;
	struct memtrace_node *node;
	struct memtrace_number *pn;
	struct memtrace_sequence *ps;
	struct memtrace_event event;

	for (i=0; i<memtrace_ctx.hashsize; i++) {
		node = &memtrace_ctx.root[i];
		if (node->type != MEMTRACE_NONE) {
			if (node->type == MEMTRACE_COUNTER) {
				pn = (struct memtrace_number *)node->param;
				bk_debug("%s count:%d", node->name, pn->num);
			} else if (node->type == MEMTRACE_VARIABLE) {
				pn = (struct memtrace_number *)node->param;
				bk_debug("%s variable:%d", node->name, pn->num);
			} else if (node->type == MEMTRACE_EVENT) {
				ps = (struct memtrace_sequence *)node->param;
				bk_debug("%s event:%d", node->name, ps->queue.count);
				while (!awoke_ringq_empty(&ps->queue)) {
					awoke_ringq_deq(&ps->queue, &event, 1);
					bk_debug("	time:%d data:%d", event.time, event.data);
				}
			} else if (node->type == MEMTRACE_CUSTOM_EVENT) {
				ps = (struct memtrace_sequence *)node->param;
				bk_debug("%s custom event:%d", node->name, ps->queue.count);
				if (!strcmp(node->name, "AECev")) {
					struct memtrace_ae ae;
					while (!awoke_ringq_empty(&ps->queue)) {
						awoke_ringq_deq(&ps->queue, &ae, 1);
						bk_debug("	expo:%d gain:%d", ae.expo, ae.gain);
					}
				}
			}
		}
	}
}

int memtrace_getfilesize(const char *filepath)
{
	struct stat statbuff;

	if (stat(filepath, &statbuff) < 0) {
		return -1;
	} else {
		return statbuff.st_size;
	}
}

static void memtrace_test_log_export(char *file)
{
	int fd;
	char *trace_buf;
	char *export_buf;
	unsigned int trace_size = 1024*4;
	unsigned int export_size;

	char *log_strings1[] = {
		"A12345\r\n",
		"B123456789\r\n",
		"C123456789123456789\r\n",
		"D1234567891234567891234567891234567891234567891234567891234567891234\r\n",
		"E12345678912345678912345678912345678912345\r\n",
		"F123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123\r\n",
	};

	char *log_strings2[] = {
		"G123456789123\r\n",
		"H123456789123\r\n",
	};

	trace_buf = mem_alloc_z(trace_size);
	memtrace_init(0, trace_buf, trace_size);
	memtrace_log_init(256);

	for (int i=0; i<array_size(log_strings1); i++) {
		memtrace_log_write(log_strings1[i], strlen(log_strings1[i]));
	}

	export_size = memtrace_get_log_export_size();
	export_buf = mem_alloc_z(export_size);
	memtrace_log_export(export_buf);

	fd = open(file, O_WRONLY|O_CREAT, 0777);
	write(fd, export_buf, export_size);
	close(fd);

	memtrace_clean();

	mem_free(trace_buf);
	mem_free(export_buf);

	bk_debug("export size:%d to %s", export_size, file);
}

static void memtrace_test_log_import(char *file)
{
	int fd;
	char *trace_buf;
	char *import_buf;
	unsigned int trace_size = 1024*4;
	unsigned int import_size;
	
	trace_buf = mem_alloc_z(trace_size);
	memtrace_init(0, trace_buf, trace_size);

	import_size = memtrace_getfilesize(file);
	bk_debug("import size:%d", import_size);

	fd = open(file, O_RDONLY);
	if (fd) {
		import_buf = mem_alloc_z(import_size);
		read(fd, import_buf, import_size);
		memtrace_log_import(import_buf);
		awoke_hexdump_debug(memtrace_ctx.logbuf, memtrace_ctx.log_bufsz);
	}

	close(fd);
}

static void memtrace_test_log_append(char *file, int string_num)
{
	int fd;
	int import_size;
	char *trace_buf;
	char *export_buf;
	unsigned int trace_size = 1024*4;
	unsigned int logbuf_size = 1024;
	unsigned int export_size;
	
	trace_buf = mem_alloc_z(trace_size);
	memtrace_init(0, trace_buf, trace_size);

	char *log_strings[] = {
		"[0] None.\r\n",
		"[1] Hello World.\r\n",
		"[2] A fastener for locking together two toothed edges by means of a sliding tab.\r\n",
		"[3] Close with a zipper\r\n",
		"[4] GLUX9701BSI is a 1.3 Megapixel resolution BSI CMOS image sensor with 9.76μm x 9.76μm pixel size.\r\n",
		"[5] The sensor operates in electronic rolling shutter supporting two operation modes: HDR mode and Low Noise mode.\r\n",
		"[6] Users can choose which to operate in camera based on practical application’s requirement on frame rate, noise, dynamic range, power consumption, etc.\r\n",
		"[7] With two types of data output channels: a 4 channel pairs sub-LVDS and MIPI (CSI-2, D-PHY) running at 445.5 MHz, achieving frame rate of 30 fps to flexibly match either ISP or FPGA components being used in camera electronics.\r\n",
		"[8] Various operating functions are available including master mode, slave mode, standby mode, etc, making GLUX9701BSI useable for different imaging conditions.\r\n",
		"[9] Table 1 below lists the key specifications for the two operation modes.\r\n",
		"[10] The min.\r\n",
	};

	import_size = memtrace_getfilesize(file);
	if (import_size < 0) {
		bk_debug("first in");
		memtrace_log_init(logbuf_size);
	} else {
		bk_debug("import");
		fd = open(file, O_RDONLY);
		export_buf = mem_alloc_z(import_size);
		read(fd, export_buf, import_size);
		memtrace_log_import(export_buf);
		close(fd);
		mem_free(export_buf);
	}

	memtrace_log_write(log_strings[string_num], strlen(log_strings[string_num]));

	export_size = memtrace_get_log_export_size();
	bk_debug("exportsize:%d", export_size);
	export_buf = mem_alloc_z(export_size);
	memtrace_log_export(export_buf);
	fd = open(file, O_WRONLY|O_CREAT, 0777);
	write(fd, export_buf, export_size);
	close(fd);

	mem_free(trace_buf);
	mem_free(export_buf);
}

err_type benchmark_memtrace_test(int argc, char *argv[])
{
	int opt;
	int mode;
	int num_string = 0;
	char *export_file = NULL;
	char *import_file = NULL;

	static const struct option long_opts[] = {
		{"log-export",		required_argument,	NULL,	arg_memtrace_log_export},
		{"log-import", 		required_argument,	NULL,	arg_memtrace_log_import},
		{"log-append",		required_argument,	NULL,	arg_memtrace_log_append},
		{"log-string",		required_argument,	NULL,	arg_memtrace_log_string},
		{NULL, 0, NULL, 0}
	};
	
	while ((opt = getopt_long(argc, argv, "?h", long_opts, NULL)) != -1)
	{
		switch (opt)
		{
			case arg_memtrace_log_export:
				export_file = optarg;
				mode = MEMTRACE_TM_LOG_EXPORT;
				break;

			case arg_memtrace_log_import:
				import_file = optarg;
				mode = MEMTRACE_TM_LOG_IMPORT;
				break;

			case arg_memtrace_log_append:
				export_file = optarg;
				mode = MEMTRACE_TM_LOG_APPEND;
				break;

			case arg_memtrace_log_string:
				num_string = atoi(optarg);
				break;

			case '?':
			case 'h':
			default:
				break;
		}
	}

	switch (mode) {

	case MEMTRACE_TM_LOG_APPEND:
		memtrace_test_log_append(export_file, num_string);
		break;

	case MEMTRACE_TM_LOG_EXPORT:
		memtrace_test_log_export(export_file);
		break;

	case MEMTRACE_TM_LOG_IMPORT:
		memtrace_test_log_import(import_file);
		break;
	}

#if 0
	char *membuf = mem_alloc_z(1024*16);
	memtrace_init(128, membuf, 1024*16);

	memtrace_tracer_add("TaskACnt", MEMTRACE_COUNTER);
	memtrace_tracer_add("TaskBVar", MEMTRACE_VARIABLE);

	memtrace_tracer_add("XferEvt", MEMTRACE_EVENT);
	memtrace_tracer_set_queue("XferEvt", sizeof(struct memtrace_event), 10);

	memtrace_tracer_add("AECev", MEMTRACE_CUSTOM_EVENT);
	memtrace_add_value("AECev", "Expo", sizeof(uint32_t));
	memtrace_add_value("AECev", "Gain", sizeof(uint32_t));
	memtrace_tracer_set_queue("AECev", sizeof(struct memtrace_ae), 8);

	number_test();
	
	event_test();
	
	//memtrace_dump();

	export_size = memtrace_get_export_size();
	bk_debug("export size:%d", export_size);
	char *expbuf = mem_alloc_z(export_size);
	memtrace_export(expbuf, export_size);
	awoke_hexdump_debug(expbuf, export_size);

	memtrace_clean();
	memtrace_init(128, membuf, 1024*16);
	memtrace_import(expbuf, export_size);

	//memtrace_import(expbuf, export_size);

	mem_free(membuf);
	mem_free(expbuf);
#endif

	return et_ok;
}

