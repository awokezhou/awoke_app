
#ifndef __PROTO_LITETALK_H__
#define __PROTO_LITETALK_H__



#define LITETALK_HEADERLEN	10

#define LITETALK_MARK		0xBBCCCCBB

typedef enum {
	LITETALK_CALLBACK_PROTOCOL_INIT = 0,
	LITETALK_CALLBACK_COMMAND,
	LITETALK_CALLBACK_STREAM_DOWNLOAD,
	LITETALK_CALLBACK_FILE_DOWNLOAD,
	LITETALK_CALLBACK_FILE_DOWNLOAD_FINISH,
};

typedef enum {
	LITETALK_CATEGORY_COMMAND = 0,
	LITETALK_CATEGORY_STREAM,
	LITETALK_CATEGORY_LOG,
	LITETALK_CATEGORY_ALARM,
};

typedef enum {
	LITETALK_CMD_SENSOR_REG = 5,
	LITETALK_CMD_EXPOSURE = 6,
	LITETALK_CMD_DISPLAY = 7,
};

typedef enum {
	LITETALK_MEDIA_DEFCONFIG = 1,
	LITETALK_MEDIA_USRCONFIG = 2,
	LITETALK_MEDIA_SENSORCFG = 3,
} litetalk_media;

struct litetalk_streaminfo {
	uint8_t media;
	uint32_t totalsize;
	uint8_t streamid;
	uint8_t index;
	uint16_t length;
};

struct litetalk_cmd {
	uint8_t type;
	err_type (*set)();
	err_type (*get)();
	awoke_list _head;
}; 

struct litetalk_private {
	struct litetalk_cmd *cmdlist;
	int cmdlist_nr;
	uint32_t streamid;
	uint32_t streamidx;
	awoke_buffchunk_pool *stream_dbpool;
};

struct litetalk_cmdinfo {
	uint8_t cmdtype;
#define LITETALK_CMD_F_WRITE	0x01
	uint8_t flag;
	uint32_t value;
};

struct ltk_exposure {
	uint16_t gain;
	uint16_t gain_min;
	uint16_t gain_max;
	uint16_t expo;
	uint16_t expo_min;
	uint16_t expo_max;
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
awoke_buffchunk *litetalk_filedown_ack(uint8_t code);
int litetalk_build_cmderr(void *buf, struct litetalk_cmdinfo *, 
	uint8_t code);
err_type litetalk_build_stream_ack(awoke_buffchunk *p, 
	uint8_t streamid, uint8_t index, uint8_t code);
#endif /* __PROTO_LITETALK_H__ */