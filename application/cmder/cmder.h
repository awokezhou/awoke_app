
#ifndef __CMDER_H__
#define __CMDER_H__



#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_macros.h"
#include "awoke_log.h"
#include "awoke_list.h"
#include "awoke_queue.h"
#include "awoke_buffer.h"
#include "cmder_protocol.h"


struct command_table;


typedef enum {
	MATCH_STRING = 1,
	MATCH_NUMBER,
	MATCH_BYTE,
	MATCH_WORD,
	MATCH_DWRD,
	MATCH_BYTES,
	MATCH_CUSTOM,
} command_match_type;

struct command_match {
	uint8_t type;
	int length;
	union {
		const char *string;
		unsigned int num;
		uint8_t byte;
		uint16_t word;
		uint16_t dwrd;
		uint8_t *bytes;
		void *struc;
	} val;
};

struct command {
	struct command_match match;
	err_type (*get)(struct command *);
	err_type (*set)(struct command *, void *buf, int);
	err_type (*exe)(struct command *);
	err_type (*help)(struct command *);
};

struct command_matcher {
	bool (*format)(void *buf, int len);
	bool (*match)(struct command_table *, void *buf, int len, struct command **cmd);
	uint8_t (*dispatch)(struct command_table *, void *buf, int len);
};

struct command_table {

	uint8_t tid;
	const char *name;
	
	int nr_commands;
	struct command *commands;
	
	struct command_matcher *matcher;

	awoke_list _head;
};

struct command_transceiver {
	char *port;
	int fd;
	uint8_t id;

	err_type (*rx)(struct command_transceiver *, void *);
	err_type (*tx)(struct command_transceiver *, void *, int );

	awoke_buffchunk *rxbuffer;

	awoke_list _head;
};

struct cmder {
	
	int nr_tabs;
	awoke_list tabs;
	
	int nr_protocols;
	awoke_list protocols;

	int nr_transceivers;
	awoke_list transceiver_list;
	
	int length;
	unsigned int datacome:1;
};

struct uartcmder {
	struct cmder base;

	awoke_minpq *rxqueue;
	awoke_minpq *txqueue;

	uint16_t gain;
	uint16_t exposure;
	uint8_t ae_enable;
	uint8_t ae_expo_en;
	uint8_t ae_gain_en;
	uint8_t goal_max;
	uint8_t goal_min;
	uint8_t ae_frame;

	uint8_t fps;
	uint8_t fps_min;
	uint8_t fps_max;
	uint8_t cl_test;
	uint8_t front_test;
	uint8_t cross_en;
	uint32_t cross_cp;
	uint8_t vinvs;
	uint8_t hinvs;

	uint16_t regaddr;
	uint8_t regvalue;

	awoke_buffchunk_pool *bpool_filechunk;
};

#define cmder_to_uartcmder(p)	container_of(p, struct uartcmder, cmder)

#define cmder_burst(...) 	logm_burst(LOG_M_CMDER, 	__VA_ARGS__)
#define cmder_trace(...) 	logm_trace(LOG_M_CMDER, 	__VA_ARGS__)
#define cmder_debug(...) 	logm_debug(LOG_M_CMDER, 	__VA_ARGS__)
#define cmder_info(...) 	logm_info(LOG_M_CMDER, 		__VA_ARGS__)
#define cmder_notice(...) 	logm_notice(LOG_M_CMDER,	__VA_ARGS__)
#define cmder_err(...) 		logm_err(LOG_M_CMDER, 		__VA_ARGS__)
#define cmder_warn(...) 	logm_warn(LOG_M_CMDER, 		__VA_ARGS__)
#define cmder_bug(...) 		logm_bug(LOG_M_CMDER, 		__VA_ARGS__)


#endif /* Luster@__CMDER_H__ */

