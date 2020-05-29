
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
	uint16_t magic = 0x1133;
	id = (&magic - magic);
	log_debug("id:%d", id);
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

int awoke_buffchunk_sizelimit()
{
	return AWOKE_BUFFCHUNK_LIMIT;
}

void awoke_buffchunk_dump(struct _awoke_buffchunk *chunk)
{
	log_debug("\n");
	log_debug(">>> buffchunk dump:");
	log_debug("------------------------");
	log_debug("size:%d", chunk->size);
	log_debug("length:%d", chunk->length);
	log_debug("p:%.*s", chunk->length, chunk->p);
	log_debug("------------------------");
	log_debug("\n");
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
	
	log_debug("\n");
	log_debug(">>> buffchunk pool dump:");
	log_debug("------------------------");
	log_debug("chunk number:%d", pool->chunknr);
	log_debug("size:%d", pool->size);
	log_debug("length:%d", pool->length);

	list_for_each_entry(c, &pool->chunklist, _head) {
		log_debug("  chunk[%d] length:%d size:%d", c->id, c->length, c->size);
	}
	
	log_debug("------------------------");
	log_debug("\n");	
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
			log_debug("chunk[%d] exist", chunk->id);
			return et_exist;
		}
	}

	list_append(&chunk->_head, &pool->chunklist);
	pool->chunknr++;
	pool->size += chunk->size;

	log_debug("chunk[%d] add to pool", chunk->id);
	
	return et_ok;
}
