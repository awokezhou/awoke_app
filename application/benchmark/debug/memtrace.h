
#ifndef __MEMTRACE_H__
#define __MEMTRACE_H__



#include "benchmark.h"
#include "awoke_queue.h"


#define MEMTRACE_NAME_SIZE	16

typedef enum {
	MEMTRACE_NONE = 0,
	MEMTRACE_COUNTER,
	MEMTRACE_VARIABLE,
	MEMTRACE_EVENT,
	MEMTRACE_CUSTOM_EVENT,
	MEMTRACE_LOG
} memtrace_node_e;

struct memtrace_node {
	char name[MEMTRACE_NAME_SIZE];
	void *param;
	unsigned int type:8;
	unsigned int reserve:24;
	awoke_list list;
} PACKED_ALIGN_BYTE;

struct memtrace_context {
	
	char *buf;
	unsigned int bufsz;
	
	unsigned int ntracers;
	
	unsigned int hashsize;
	
	unsigned int export_size;
	
	struct memtrace_node *root;

	char *logbuf;
	unsigned int log_bufsz;
	struct awoke_ringbuffer rb_log;
};

struct memtrace_number {
	uint32_t num;
};

struct memtrace_sequence {
	struct awoke_ringq queue;
	char *buffer;
};

struct memtrace_paraminfo {
	char name[MEMTRACE_NAME_SIZE];
	unsigned int size;
	unsigned int dtype;
	awoke_list list;
};

struct memtrace_cev_seq {
	struct memtrace_sequence seq;
	awoke_list pinfo;
};

struct memtrace_buffer {
	struct awoke_ringbuffer *buffer;
};

struct memtrace_event {
	uint32_t time;
	uint32_t data;
};

struct memtrace_ae {
	uint32_t time;
	uint32_t expo;
	uint32_t gain;
};

struct memtrace_export_head {
	uint32_t marker;
	uint32_t version;
	uint32_t hashsize;
	uint32_t ntracers;
};

struct memtrace_export_node {
	uint32_t type:8;
	uint32_t reserve:24;
	uint32_t name_offset;
	uint32_t name_length;
	uint32_t param_offset;
	uint32_t param_length;
};

struct memtrace_export_log {
	unsigned int head;
	unsigned int tail;
	unsigned int size;
};

typedef enum {
	arg_memtrace_none = 0,
	arg_memtrace_log_export,
	arg_memtrace_log_import,
	arg_memtrace_log_append,
	arg_memtrace_log_string,
} memtrace_args;

typedef enum {
	MEMTRACE_TM_NONE = 0,
	MEMTRACE_TM_LOG_EXPORT,
	MEMTRACE_TM_LOG_IMPORT,
	MEMTRACE_TM_LOG_APPEND,
};

err_type benchmark_memtrace_test(int argc, char *argv[]);



#endif /* __MEMTRACE_H__ */

