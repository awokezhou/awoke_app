
#include "awoke_buffer.h"
#include "awoke_log.h"

bool awoke_buffchunk_dynamic(struct _awoke_buffchunk *chunk)
{
#ifdef AWOKE_BUFFCHUNK_FIXED
	return ((chunk->p != NULL) && (chunk->p != chunk->_fixed));
#else
	return (chunk->p != NULL);
#endif
}

uint16_t buffchunk_id(struct _awoke_buffchunk *chunk)
{
	uint16_t id;
	uint16_t magic = 0x1371;
	int addr1 = (int)&magic;
	int addr2 = (int)&chunk->id;
	id = ((addr1>>23)^magic) + ((magic>>5)^addr2);
	log_trace("id:%d", id);
	return id;

}

struct _awoke_buffchunk *awoke_buffchunk_create(int size)
{
	int allocsize;
	awoke_buffchunk *chunk;

	/*
	 * Create a clean buffchunk with of the specified size. If the 
	 * buffchunk fixed and the specified size is smaller than the 
	 * fixed size(AWOKE_BUFFCHUNK_FIXED_SIZE), buffchunk size will be 
	 * the fixed size, otherwise it will be alloc as required.
	 */

	if (size<=0 || size>AWOKE_BUFFCHUNK_LIMIT)
		return NULL;

#ifdef AWOKE_BUFFCHUNK_FIXED
	if (size <= AWOKE_BUFFCHUNK_FIXED_SIZE) {
		allocsize = 0;
	} else {
		allocsize = size;
	}
#else
	allocsize = size;
#endif

	chunk = mem_alloc_z(sizeof(awoke_buffchunk));
	if (!chunk)
		return NULL;

	if (allocsize > 0) {
		chunk->p = mem_alloc_z(allocsize);
		if (!chunk->p) {
			mem_free(chunk);
			return NULL;
		}
		chunk->size = allocsize;
	}

	chunk->length = 0;
#ifdef AWOKE_BUFFCHUNK_FIXED
	if (allocsize == 0) {
		chunk->p = chunk->_fixed;
		chunk->size = AWOKE_BUFFCHUNK_FIXED_SIZE;
	}	
#endif

	chunk->id = buffchunk_id(chunk);

	return chunk;
}

static err_type buffchunk_init(struct _awoke_buffchunk *chunk)
{
	/*
	 * Buffchunk initialization, used for structure reference.
	 * Created or resized buffchunk can't be initialized.
	 */

	chunk->length = 0;

#ifdef AWOKE_BUFFCHUNK_FIXED
	chunk->p = chunk->_fixed;
	chunk->size = AWOKE_BUFFCHUNK_FIXED_SIZE;
	memset(chunk->_fixed, 0x0, AWOKE_BUFFCHUNK_FIXED_SIZE);
#else
	chunk->p = NULL;
	chunk->size = 0;
	chunk->length = 0;
#endif
	
	return et_ok;	
}

err_type awoke_buffchunk_init(struct _awoke_buffchunk *chunk)
{	
	err_type ret;

	ret =  buffchunk_init(chunk);
	if (ret != et_ok)
		return ret;
	
	chunk->id = buffchunk_id(chunk);
	return et_ok;
}

void awoke_buffchunk_clean(struct _awoke_buffchunk *chunk)
{
	if (awoke_buffchunk_dynamic(chunk)) {
		mem_free(chunk->p);
		chunk->p = NULL;
	}
	
	buffchunk_init(chunk);
}

void awoke_buffchunk_clear(struct _awoke_buffchunk *chunk)
{
	memset(chunk->p, 0x0, chunk->size);
	chunk->length = 0;
}

void awoke_buffchunk_free(struct _awoke_buffchunk **p_chunk)
{
	awoke_buffchunk *p;

	if (!p_chunk || !*p_chunk)
		return;

	p = *p_chunk;

	if (p->p) {
		mem_free(p->p);
		p->p = NULL;
	}

	mem_free(p);
	p = NULL;
}

err_type awoke_buffchunk_write(struct _awoke_buffchunk *chunk, const char *data, 
	int length, bool trybest)
{
	int remain;
	int copylen;
	
	if (!data || length<=0) {
		log_err("data invalid");
		return et_param;
	}

	remain = awoke_buffchunk_remain(chunk);

	if (!remain) {
		log_err("chunk full");
		return et_full;
	}

	if (!trybest && (length > remain)) {
		log_err("chunk not enough");
		return et_full;
	}

	if (trybest && (length > remain)) {
		copylen = remain;
	} else {
		copylen = length;
	}

	memcpy(chunk->p+chunk->length, data, copylen);
	chunk->length += copylen;
	return et_ok;
}

err_type awoke_buffchunk_resize(struct _awoke_buffchunk *chunk, 
	int resize, bool trybest)
{
	int _size;
	char *realloc;

	/*
	 * Resize buffchunk to the specified size. The new size can be larger 
	 * or smaller than the original size. If <trybest> is not set, we ensure
	 * that the resized buffchunk retains the data content or return error. 
	 * But if <trybest> is set, the alloc memory size will be guaranteed first, 
	 * and then more data will be kept as much as possible. 
	 * 
	 * When the system is out of memory, setting the <trybest> can try to 
	 * allocate some memory space instead of returning failure. The allocation 
	 * size will be smaller than the expected value. The return size depends on 
	 * the system capacity and calculation method. At present, shift operation 
	 * is used, which is a quadratic exponential decline.
	 */

	if (trybest) {
		_size = min(resize, AWOKE_BUFFCHUNK_LIMIT);
	} else {
		if (resize > AWOKE_BUFFCHUNK_LIMIT) {
			log_err("size limit %d", AWOKE_BUFFCHUNK_LIMIT);
			return et_mem_limit;
		} else if (resize < chunk->length) {
			log_err("resize not enough, length %d", chunk->length);
			return et_fail;
		}
		_size = resize;
	}
	
	if (awoke_buffchunk_dynamic(chunk)) {

		realloc = mem_realloc(chunk->p, _size);
		if (!realloc) {
			goto mem_error;
		}

		chunk->p = realloc;

		if ((chunk->length > _size) && trybest) {
			chunk->length = _size;
		}
	
	} else {

		chunk->p = mem_alloc_z(_size);
		if (!chunk->p) {
			goto mem_error;
		}

#ifdef AWOKE_BUFFCHUNK_FIXED
		memcpy(chunk->p, chunk->_fixed, chunk->length);
#endif

	}

	chunk->size = _size;
	return et_ok;

mem_error:
	if (trybest) {
		_size = _size >> 1;
		return awoke_buffchunk_resize(chunk, _size, trybest);
	}

	return et_nomem;
}

const char *awoke_buffchunk_version()
{
	return AWOKE_BUFFCHUNK_VERSION;
}

int awoke_buffchunk_remain(struct _awoke_buffchunk *chunk)
{
	return (chunk->size - chunk->length);
}

int awoke_buffchunk_sizelimit()
{
	return AWOKE_BUFFCHUNK_LIMIT;
}

err_type awoke_buffchunk_copy(struct _awoke_buffchunk *dst, struct _awoke_buffchunk *src)
{
	if (src->length > dst->size)
		return err_empty;

	memcpy(dst->p, src->p, src->length);
	dst->length = src->length;
	return et_ok;
}

void awoke_buffchunk_dump(struct _awoke_buffchunk *chunk)
{
	log_trace("");
	log_trace(">>> buffchunk dump:");
	log_trace("------------------------");
	log_trace("id:%d", chunk->id);
	log_trace("size:%d", chunk->size);
	log_trace("length:%d", chunk->length);
	log_trace("------------------------");
	log_trace("");
}

struct _awoke_buffchunk_pool *awoke_buffchunk_pool_create(int maxsize)
{
	awoke_buffchunk_pool *pool;

	pool = mem_alloc_z(sizeof(awoke_buffchunk_pool));
	if (!pool)
		return NULL;

	pool->size = 0;
	pool->length = 0;
	pool->chunknr = 0;
	pool->maxsize = min(AWOKE_BUFFCHUNK_POOL_LIMIT, maxsize);
	list_init(&pool->chunklist);

	return pool;
}

err_type awoke_buffchunk_pool_init(struct _awoke_buffchunk_pool *pool, int maxsize)
{
	pool->size = 0;
	pool->length = 0;
	pool->chunknr = 0;
	pool->maxsize = min(AWOKE_BUFFCHUNK_POOL_LIMIT, maxsize);
	list_init(&pool->chunklist);

	return et_ok;
}

void awoke_buffchunk_pool_clean(struct _awoke_buffchunk_pool *pool)
{
	awoke_buffchunk *chunk, *temp;

	list_for_each_entry_safe(chunk, temp, &pool->chunklist, _head) {
		list_unlink(&chunk->_head);
		awoke_buffchunk_free(&chunk);
	}

	pool->size = 0;
	pool->length = 0;
	pool->chunknr = 0;
}

void awoke_buffchunk_pool_free(struct _awoke_buffchunk_pool **p_pool)
{
	awoke_buffchunk_pool *p;
	awoke_buffchunk *chunk, *temp;

	if (!p_pool || !*p_pool)
		return;

	p = *p_pool;

	list_for_each_entry_safe(chunk, temp, &p->chunklist, _head) {
		list_unlink(&chunk->_head);
		awoke_buffchunk_free(&chunk);
	}

	mem_free(p);
	p = NULL;
}

void awoke_buffchunk_pool_dump(struct _awoke_buffchunk_pool *pool)
{
	awoke_buffchunk *c;
	
	log_trace("");
	log_trace(">>> buffchunk pool dump:");
	log_trace("------------------------");
	log_trace("chunk number:%d", pool->chunknr);
	log_trace("size:%d", pool->size);
	log_trace("length:%d", pool->length);

	list_for_each_entry(c, &pool->chunklist, _head) {
		log_trace("  chunk[%d] length:%d size:%d", c->id, c->length, c->size);
	}
	
	log_trace("------------------------");
	log_trace("");	
}

err_type awoke_buffchunk_pool_chunkadd(struct _awoke_buffchunk_pool *pool, 
	struct _awoke_buffchunk *chunk)
{
	int available;
	awoke_buffchunk *c;

	if (!pool || !chunk)
		return et_param;

	available = pool->maxsize - pool->size;
	if (available < chunk->size) {
		log_err("available:%d, chunk too big", available);
		return et_mem_limit;
	}

	list_for_each_entry(c, &pool->chunklist, _head) {
		if (c->id == chunk->id) {
			log_trace("chunk[%d] exist", chunk->id);
			return et_exist;
		}
	}

	list_append(&chunk->_head, &pool->chunklist);
	pool->chunknr++;
	pool->size += chunk->size;
	pool->length += chunk->length;

	log_trace("chunk[%d] add to pool", chunk->id);
	
	return et_ok;
}

struct _awoke_buffchunk *awoke_buffchunk_pool_chunkget(struct _awoke_buffchunk_pool *pool)
{
	awoke_buffchunk *chunk;
	
	if (!pool || !pool->chunknr) {
		return NULL;
	}

	chunk = list_entry_first(&pool->chunklist, awoke_buffchunk, _head);
	if (!chunk) {
		return NULL;
	}

	return chunk;
}

struct _awoke_buffchunk *awoke_buffchunk_pool2chunk(struct _awoke_buffchunk_pool *pool)
{
	awoke_buffchunk *_c, *chunk;

	if (!pool || (!pool->chunknr) || (!pool->length)) {
		log_err("pool empty");
		return NULL;
	}

	chunk = awoke_buffchunk_create(pool->length);
	if (!chunk) {
		log_err("create chunk error");
		return NULL;
	}

	list_for_each_entry(_c, &pool->chunklist, _head) {
		if (_c->length == 0)
			continue;
		if (awoke_buffchunk_remain(chunk) < _c->length) {
			log_err("remain not enough");
			break;
		}
		awoke_buffchunk_write(chunk, _c->p, _c->length, FALSE);
	}

	return chunk;
}

