
#ifndef __MICRO_TALK_H__
#define __MICRO_TALK_H__



#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_queue.h"
#include "awoke_event.h"
#include "awoke_memory.h"
#include "awoke_package.h"
#include "awoke_buffer.h"



#define MTK_US_PER_SEC	((uint64_t)1000000)
#define MTK_MS_PER_SEC	((uint64_t)1000)
#define MTK_US_PER_MS	((uint64_t)1000)
#define MTK_NS_PER_US	((uint64_t)1000)

#define MTK_RSP_HEADER_LENGTH	4
#define MTK_REQ_HEADER_LENGTH	13

typedef enum {
	MTK_CHANNEL_NOTICE = 0,
	MTK_CHANNEL_DATA,
	MTK_CHANNEL_SIZEOF,
} mtk_channel_e;


typedef enum {

	MTK_CALLBACK_PROTOCOL_INIT,
	MTK_CALLBACK_PROTOCOL_DESTROY,

	MTK_CALLBACK_CONNECTION_REDIRECT,
	MTK_CALLBACK_CONNECTION_ESTABLISHED,
	MTK_CALLBACL_CONNECTION_RECEIVE,
	MTK_CALLBACL_CONNECTION_WRITABLE,
	MTK_CALLBACK_CONNECTION_ERROR,
	MTK_CALLBACK_CONNECTION_CLOSED,

} mtk_callback_reasons;

typedef struct _mtk_notice_msg {
	uint8_t data;
} mtk_notice_msg;

typedef enum {
	MTK_METHOD_PUT = 0,
	MTK_METHOD_GET,
	MTK_METHOD_ALARM,
} mtk_method_e;

typedef enum {
	MTK_LBL_COMMAND = 0,
	MTK_LBL_STATUS,
	MTK_LBL_MEDIA,
} mtk_lbl_e;

typedef struct _mtk_request_msg {
	uint8_t tag;
	uint8_t subtype;
	uint8_t checksum;
	uint16_t length;
	uint8_t token[8];
	void *subdata;
	int sublen;
	uint8_t *payload;
} mtk_request_msg;

typedef struct _mtk_response_msg {
	uint8_t code;
	uint16_t len;
	uint8_t *data;
	uint8_t checksum;
	int recvlen;
} mtk_response_msg;

typedef struct _mtk_put_request_msg {
	uint16_t chunk_nr;
	uint16_t chunk_index;
	uint32_t totoal_size;
	uint8_t md5value[16];
	uint32_t hash;
	uint8_t *data;
} mtk_put_request_msg;

typedef struct _mtk_connect {

	awoke_event event;

	int fd;
	uint8_t msgindex;
	uint8_t last_method;
	
	mtk_channel_e chntype;
	
	struct _awoke_queue *ring;
	struct _mtk_context *context;
	struct _mtk_session *session;
	struct _mtk_protocol_handle *protocol;

	awoke_buffchunk *spare_buff;

	uint64_t ka_timeout_us;

	err_type (*callback)(struct _mtk_connect *conn, mtk_callback_reasons reason,
	    void *user, void *in, size_t len);
		
	unsigned int need_do_redirect:1;
	unsigned int leave_wirtable_active:1;
	unsigned int f_keepalive:1;
	unsigned int f_connecting:1;
	unsigned int f_connected:1;

	void *user;
	
} mtk_connect;

typedef struct _mtk_client_connect_info {

	uint16_t port;
	uint8_t channel;
	const char *address;
	
	uint64_t ka_timeout_us;
	
	struct _mtk_context *context;
	struct _mtk_session *session;
	struct _mtk_protocol_handle *protocol;
	
	err_type (*callback)(struct _mtk_connect *conn, mtk_callback_reasons reason,
		    void *user, void *in, size_t len);

} mtk_client_connect_info;

typedef struct _mtk_context {

	char *name;

	int session_baseid;

	struct _awoke_minpq works_pq;

	struct _awoke_event_loop *evloop;

	struct _mtk_connect connects[MTK_CHANNEL_SIZEOF];

	struct _awoke_buffchunk_pool rx_pool;

	struct _awoke_buffchunk *rx_buff;

	unsigned int service_need_stop:1;
	
} mtk_context;

typedef struct _mtk_work {
	err_type (*handle)(struct _mtk_context *, struct _mtk_work *);
	uint64_t us;
	void *data;
} mtk_work;

typedef enum {
	MTK_CHUNKTYPE_FILE = 0,
	MTK_CHUNKTYPE_BUFF,
} mtk_chunktype;

typedef struct _mtk_chunkinfo {
	uint16_t crc;
	uint16_t hash;
	uint16_t index;
	uint32_t fileid;
	uint16_t chunknum;
	uint16_t chunksize;
	uint32_t totalsize;
	char *filepath;
	awoke_buffchunk *buffout;
	awoke_buffchunk buffin;
	mtk_chunktype type;
	awoke_minpq *chunk_pq;
} mtk_chunkinfo;

typedef struct _mtk_protocol_handle {
	const char *name;
	err_type (*handle_read)(struct _mtk_connect *);
	err_type (*handle_write)(struct _mtk_connect *);
	err_type (*handle_close)(struct _mtk_connect *);
} mtk_protocol_handle;



err_type mtk_client_connect_via_info(struct _mtk_client_connect_info *i);
struct _mtk_context *mtk_context_create(const char *name);

err_type mtk_work_schedule(struct _mtk_context *ctx, 
	err_type (*cb)(struct _mtk_context *, struct _mtk_work *), uint64_t us);
err_type mtk_retry_work_schedule(struct _mtk_context *ctx, 
	err_type (*cb)(struct _mtk_context *, struct _mtk_work *), uint64_t us);
err_type mtk_work_schedule_withdata(struct _mtk_context *ctx, 
	err_type (*cb)(struct _mtk_context *, struct _mtk_work *), uint64_t us, void *data);

bool mtk_service_should_stop(struct _mtk_context *ctx);
err_type mtk_service_run(struct _mtk_context *ctx, uint32_t timeout_ms);

err_type mtk_callback_on_writable(struct _mtk_connect *conn);

err_type mtk_connect_write(struct _mtk_connect *conn, uint8_t *buf, size_t len);
err_type mtk_connect_read(struct _mtk_connect *conn, uint8_t *buf, size_t len);
void mtk_connect_close(struct _mtk_connect *conn);

void mtk_connect_release(struct _mtk_connect *c);
bool mtk_connect_keepalive(struct _mtk_connect *c);
err_type mtk_connect_session_del(struct _mtk_connect *c);
err_type mtk_connect_session_add(struct _mtk_connect *c, struct _mtk_session *s);


err_type mtk_session_set(struct _mtk_connect *c, struct _mtk_session *s);

err_type mtk_chunkinfo_generate_from_buff(char *buff, int len, 
	int chunksize, struct _mtk_chunkinfo **r);
err_type mtk_chunkinfo_generate_from_file(const char *filepath, 
	int chunksize, struct _mtk_chunkinfo **r);

err_type mtk_chunkdata_merge(struct _mtk_chunkinfo *ci);
void mtk_chunkinfo_free(void *d);
err_type mtk_chunkdata_merge(struct _mtk_chunkinfo *ci);
err_type mtk_chunkdata_copy(struct _mtk_chunkinfo *ci, uint8_t *dst, uint32_t start, int size);

bool mtk_file_exist(const char *filepath);
uint16_t mtk_getbuffcrc(char *buff, int totalsize);
uint16_t mtk_getfilecrc(const char *filepath, int filesize);
int mtk_getfilesize(const char *filepath);


#endif /* __MICRO_TALK_H__ */

