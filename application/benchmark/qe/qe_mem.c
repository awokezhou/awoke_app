

#include "qe_core.h"
#include "bget.h"
#include "benchmark.h"


enum qemem_type {
	QE_MEM_EXTERNAL,
	QE_MEM_STATIC,
};

struct qemem_context {
	enum qemem_type type;
};

static struct qemem_context memctx = {
	.type = QE_MEM_EXTERNAL,
};

struct qemem_external_api {
	void* (*malloc)(qe_size_t size);
	void* (*zalloc)(qe_size_t size);
	void* (*realloc)(void *buf, qe_size_t size);
	void  (*free)(void *ptr);
};

static struct qemem_external_api eapi = {
	.malloc  = QE_NULL,
	.zalloc  = QE_NULL,
	.realloc = QE_NULL,
	.free    = QE_NULL,
};

void *qe_malloc(qe_size_t size)
{
	if (memctx.type == QE_MEM_EXTERNAL) {
		if (eapi.malloc != QE_NULL) {
			return eapi.malloc(size);
		}
		return QE_NULL;
	} else if (memctx.type == QE_MEM_STATIC) {
		return bget(size);
	}

	return QE_NULL;
}

void *qe_zalloc(qe_size_t size)
{
	if (memctx.type == QE_MEM_EXTERNAL) {
		if (eapi.zalloc != QE_NULL) {
			return eapi.zalloc(size);
		}
		return QE_NULL;
	} else if (memctx.type == QE_MEM_STATIC) {
		return bgetz(size);
	}

	return QE_NULL;
}

void *qe_realloc(void *buf, qe_size_t size)
{
	qe_assert(buf != QE_NULL);
	
	if (memctx.type == QE_MEM_EXTERNAL) {
		if (eapi.zalloc != QE_NULL) {
			return eapi.realloc(buf, size);
		}
		return QE_NULL;
	} else if (memctx.type == QE_MEM_STATIC) {
		return bgetr(buf, size);
	}

	return QE_NULL;
}

void qe_free(void *ptr)
{
	if (memctx.type == QE_MEM_EXTERNAL) {
		if (eapi.free != QE_NULL) {
			return eapi.free(ptr);
		}
	} else if (memctx.type == QE_MEM_STATIC) {
		return brel(ptr);
	}
}

void qe_heap_init(void *start, qe_size_t size)
{
	bpool(start, size);
	memctx.type = QE_MEM_STATIC;
	bk_debug("heap:0x%x %d", start, size);
}

void qe_mem_set_eapi(void* (*malloc)(qe_size_t size), 
							   void* (*zalloc)(qe_size_t size),
							   void* (*realloc)(void *buf, qe_size_t size),
							   void  (*free)(void *ptr))
{
	eapi.free    = free;
	eapi.malloc  = malloc;
	eapi.zalloc  = zalloc;
	eapi.realloc = realloc;
	memctx.type  = QE_MEM_EXTERNAL;
}


