
#ifndef __RY_H__
#define __RY_H__

#define RY_QUEUE_SIZE	15

#define ry_queue_foreach(q, u)						\
		int __k;									\
		u = &(q)->_queue[(q)->curr];				\											
													\
		for (__k = (q)->curr;						\
		 	(__k>=0);								\
		 	__k--,									\
		 	u = &(q)->_queue[__k])					\
		 	
typedef struct _ry_wpdata {
    time_t collecttime;
    uint16_t data;
} ry_wpdata;

typedef struct _ry_dataunit {
	uint16_t batt_level;
	uint16_t rssi;			 
	uint16_t rssi_percent;			 
	uint16_t rsrq;			/* sgnal quality level */
	uint16_t snr;			/* SNR */
	uint8_t batt_percent;
	uint8_t ecl;			/* ECL */
	uint16_t pressure;
	uint8_t sys_state;
	struct tm occur_time;
} ry_dataunit;

typedef struct _ry_queue {		/* FIFO standard queue */
	int curr;
#define RY_QUEUE_F_RB	0x0002			/* queue flag rollback */
	uint16_t flag;
	ry_dataunit _queue[RY_QUEUE_SIZE];
} ry_queue;

typedef struct _ry_store {
	ry_queue du_queue;			/* data unit queue */
} ry_store;


#define RY_PREFIX		0x4040
#define RY_POSTFIX		0x2323
#define RY_VER_MAJOR	0x01
#define RY_VER_USER		0x02
#define RY_PKT_SIZE		1024
#define RY_DU_OFFSET	26
#define RY_TYPEF_WLPS	0x3005
#define RY_CCODE		0x0000

#define RY_ORDER_RP_MONITOR		0x0
#define RY_ORDER_RP_ADDRESS		0x1
#define RY_ORDER_RP_TIMESAPN	0x2
#define RY_ORDER_RP_CHECKFREQ	0x3
#define RY_ORDER_RP_COLLSAPN	0x4
#define RY_ORDER_RP_PRESSLOW	0x5
#define RY_ORDER_RP_PRESSHIG	0x6

typedef struct _ry_msg_dtunit {
	uint8_t order_type;
	uint16_t type_flag;
	uint16_t component_code;
	union {
		//wpm_ptl_ry_msg_du_rp_monitor monitor;
		//wpm_ptl_ry_msg_du_rp_address address;
		//wpm_ptl_ry_msg_du_rp_timespan timespan;
		//...
	};
	
} ry_msg_dtunit;

typedef struct _ry_msg_header {
	uint16_t prefix;
	uint16_t postfix;
	uint16_t version;
	struct tm date;
	uint16_t manufacturer;
	char imei[8];
	uint16_t dev_type;
	uint8_t command;
	uint8_t f_ack;
	uint8_t checksum;
} ry_msg_header;

typedef struct _ry_message {
	ry_msg_header header;
	ry_msg_dtunit dtunit;
	int dtunit_len;
	int original_len;
	uint8_t original_buf[1024];
} ry_message;

#endif /* __RY_H__ */