
#ifndef __PROTO_LITETALK_H__
#define __PROTO_LITETALK_H__



#define LITETALK_HEADERLEN			10
#define LITETALK_STREAM_HEADERLEN	9

#define USER_MARK			0xBBCCCCBB
#define LITETALK_MARK		0x13356789

typedef enum {
	LITETALK_CALLBACK_PROTOCOL_INIT = 0,
	LITETALK_CALLBACK_COMMAND,
	LITETALK_CALLBACK_STREAM_DOWNLOAD,
	LITETALK_CALLBACK_FILE_DOWNLOAD,
	LITETALK_CALLBACK_FILE_DOWNLOAD_FINISH,
	LITETALK_CALLBACK_CMDI_REQUEST,
} litetalk_callback_reson;

typedef enum {
	LITETALK_CATEGORY_COMMAND = 0,
	LITETALK_CATEGORY_STREAM,
	LITETALK_CATEGORY_LOG,
	LITETALK_CATEGORY_ALARM,
	LITETALK_CATEGORY_CMDI = 8,
} litetalk_category;

typedef enum {
	LITETALK_CMD_SENSOR_REG = 5,
	LITETALK_CMD_EXPOSURE = 6,
	LITETALK_CMD_DISPLAY = 7,
	LITETALK_CMD_ISP = 8,
	LITETALK_CMD_TEMPCTL = 9,
	LITETALK_CMD_TEMP_CAP = 10,
} litetalk_cmd_t;

typedef enum {
	LITETALK_MEDIA_DEFCONFIG = 1,
	LITETALK_MEDIA_USRCONFIG = 2,
	LITETALK_MEDIA_SENSORCFG = 3,
	LITETALK_MEDIA_DPCPARAMS = 4,
	LITETALK_MEDIA_NUCPARAMS = 5,
} litetalk_media;

typedef enum {
	LITETALK_CMDI_GET_REQUEST = 0x0,
	LITETALK_CMDI_GET_RESPONSE,
	LITETALK_CMDI_SET_REQUEST,
	LITETALK_CMDI_SET_RESPONSE,
} litetalk_cmdi_type;

typedef enum {
	LITETALK_CODE_SUCCESS = 0,
	LITETALK_CODE_ERROR,
} litetalk_code;

typedef enum {
	LITETALK_STREAM_ST_DOWNLOAD = 0x01,
	LITETALK_STREAM_ST_ACK = 0x02,
} litetalk_stream_subtype;

struct litk_streaminfo {
	uint8_t id;
	uint8_t media;
	uint16_t index;
	uint16_t length;
	uint32_t totalsize;
	uint32_t recvbytes;
};

struct litetalk_cmd {
	uint8_t type;
	err_type (*set)();
	err_type (*get)();
	awoke_list _head;
};

struct litetalk_cmdi_item {
	uint32_t address;
	err_type (*set)();
	err_type (*get)();
	awoke_list _head;
};

struct litk_private {
	int recvbyte;
	int cmdlist_nr;
	int cmdilist_nr;
	uint32_t sector_addr;
	struct litetalk_cmd *cmdlist;
	struct litetalk_cmdi_item *cmdilist;
	awoke_buffchunk *bigdata;
	awoke_buffchunk_pool streampool;
	struct litk_streaminfo streaminfo;
};

struct litetalk_cmdinfo {
	uint8_t cmdtype;
#define LITETALK_CMD_F_WRITE	0x01
	uint8_t flag;
	uint32_t value;
};

struct litetalk_cmdireq {
	uint8_t dtype;
	uint8_t subtype;
	uint8_t requestid;
	uint16_t datalen;
	uint32_t address[2];
};

struct ltk_exposure {
	uint16_t gain;
	uint16_t gain_min;
	uint16_t gain_max;
	uint32_t expo;
	uint32_t expo_min;
	uint32_t expo_max;
	uint8_t ae_enable;
	uint8_t goal_max;
	uint8_t goal_min;
	uint8_t ae_frame;
};

struct ltk_display {
	uint8_t fps;
	uint8_t fps_min;
	uint8_t fps_max;
	uint8_t cl_test;
	uint8_t front_test;
	uint8_t cross_en;
	uint32_t cross_cp;
	uint8_t vinvs;
	uint8_t hinvs;
};

extern struct cmder_protocol litetalk_protocol;
int litetalk_build_cmderr(void *buf, struct litetalk_cmdinfo *, 
	uint8_t code);
err_type litetalk_pack_header(uint8_t *buf, uint8_t category, int length);
err_type litetalk_build_stream_ack(awoke_buffchunk *p, 
	uint8_t streamid, uint16_t index, uint8_t code);
err_type litetalk_build_log(awoke_buffchunk *p, uint8_t level, uint32_t src, uint8_t *context, int length);
#endif /* __PROTO_LITETALK_H__ */