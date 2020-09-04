
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
	MTK_TAG_PUT = 0,
	MTK_TAG_GET,
	MTK_TAG_ALARM,
} mtk_tag_e;

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

typedef struct _mtk_work {
	err_type (*handle)();
	uint64_t us;
} mtk_work;

typedef struct _mtk_protocol_handle {
	const char *name;
	err_type (*handle_read)(struct _mtk_connect *);
	err_type (*handle_write)(struct _mtk_connect *);
} mtk_protocol_handle;

typedef err_type
mtk_callback_function(struct _mtk_connect *conn, mtk_callback_reasons reason,
		    void *user, void *in, size_t len);

typedef struct _mtk_connect {
	awoke_event event;
	int fd;
	uint8_t msgindex;
	mtk_channel_e chntype;
	struct _awoke_queue *ring;
	struct _mtk_context *context;
	struct _mtk_protocol_handle *protocol;

	uint64_t ka_timeout_us;
	
	unsigned int need_do_redirect:1;
	unsigned int leave_wirtable_active:1;
	unsigned int f_keepalive:1;
	void *user;
} mtk_connect;

typedef struct _mtk_client_connect_info {
	struct _mtk_context *context;
	const char *address;
	uint16_t port;
	uint8_t channel;
	uint64_t ka_timeout_us;
	struct _mtk_protocol_handle *protocol;
} mtk_client_connect_info;

typedef struct _mtk_context {

	char *name;

	struct _awoke_minpq works_pq;

	struct _awoke_event_loop *evloop;

	err_type (*callback)(struct _mtk_connect *conn, mtk_callback_reasons reason,
		    void *user, void *in, size_t len);

	struct _mtk_connect connects[MTK_CHANNEL_SIZEOF];

	struct _awoke_buffchunk_pool rx_pool;

	unsigned int service_need_stop:1;
	
} mtk_context;


err_type mtk_client_connect_via_info(struct _mtk_client_connect_info *i);
struct _mtk_context *mtk_context_create(void);
err_type mtk_work_schedule(struct _mtk_context *ctx, 
	err_type (*cb)(struct _awoke_minpq *q), uint64_t us);
err_type mtk_retry_work_schedule(struct _mtk_context *ctx, 
	err_type (*cb)(struct _awoke_minpq *q), uint64_t us);
bool mtk_service_should_stop(struct _mtk_context *ctx);
err_type mtk_service_run(struct _mtk_context *ctx, uint32_t timeout_ms);
err_type mtk_nc_connect(struct _mtk_context *ctx, struct _mtk_connect *conn, 
	struct _mtk_client_connect_info *i);
err_type mtk_callback_on_writable(struct _mtk_connect *conn);
err_type mtk_connect_write(struct _mtk_connect *conn, uint8_t *buf, size_t len);
void mtk_connect_release(struct _mtk_connect **c);
bool mtk_connect_keepalive(struct _mtk_connect *c);
#endif /* __MICRO_TALK_H__ */
