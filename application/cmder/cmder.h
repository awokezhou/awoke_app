
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



#define CMDER_CHECKSUM_NOR8		0x01
#define CMDER_CHECKSUM_NOR32	0x02
#define CMDER_CHECKSUM_MARKER	0x10

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
	int connfd;
	uint16_t netport;
	uint8_t id;

	err_type (*rx)(struct command_transceiver *, void *);
	err_type (*tx)(struct command_transceiver *, void *, int );

	awoke_buffchunk *rxbuffer;

	awoke_list _head;
};

struct cmder_2dpoint {
	uint16_t x;
	uint16_t y;
};

struct cmder_config {
	uint32_t mark;

	uint16_t goal;
	uint16_t goal_min;
	uint16_t goal_max;
	
	uint32_t expo;
	uint32_t expo_min;
	uint32_t expo_max;

	uint16_t gain;
	uint16_t gain_min;
	uint16_t gain_max;

	uint8_t fps;
	uint8_t fps_min;
	uint8_t fps_max;

	uint8_t hinvs;
	uint8_t vinvs;

	uint8_t ae_frame;

	int32_t tec_target;
	uint32_t tecwork_freq;

	uint16_t iff_th;
	uint16_t iff_div;

	uint32_t ae_enable:1;
	uint32_t ae_expo_enable:1;
	uint32_t ae_gain_enable:1;
	uint32_t hdr_enable:1;
	uint32_t cst_enable:1;
	uint32_t dfg_enable:1;
	uint32_t nrd_enable:1;
	uint32_t ehc_enable:1;
	uint32_t shp_enable:1;
	uint32_t zoom_enable:1;
	uint32_t epc_testbit:1;
	uint32_t inversion_enable:1;
	uint32_t crossview_enable:1;
	uint32_t config_dump:1;
	uint32_t epc_info_dump:1;
	uint32_t tec_enable:1;
	uint32_t cltest_enable:1;
	uint32_t frtest_enable:1;
	uint32_t dpc_enable:1;
	uint32_t psnu_enable:1;
	uint32_t iff_enable:1;
	uint32_t gamm_en:1;
	uint32_t rsv01:10;

	struct cmder_2dpoint crossview_point;

	uint32_t iffparam;
	
	uint8_t checksum;
}PACKED_ALIGN_BYTE;

struct cmder {
	
	int nr_tabs;
	awoke_list tabs;
	
	int nr_protocols;
	awoke_list protocols;

	struct cmder_protocol *proto;

	int nr_transceivers;
	awoke_list transceiver_list;
	
	int length;
	unsigned int datacome:1;
};

struct uartcmder {
	struct cmder base;

	uint8_t nuc_checksum;
	uint32_t nucaddr;
	awoke_buffchunk *flashchunk;

	awoke_minpq *rxqueue;
	awoke_minpq *txqueue;

	uint16_t regaddr;
	uint8_t regvalue;

	struct cmder_config config;

	awoke_minpq workqueue;

	uint8_t temp_cap_en;

	awoke_buffchunk_pool *bpool_filechunk;
};

struct cmder_work {
	err_type (*handle)(struct uartcmder *, struct cmder_work *);
	uint32_t ms;
	void *data;
	uint8_t flags;
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

