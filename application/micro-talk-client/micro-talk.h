
#ifndef __MICRO_TALK_H__
#define __MICRO_TALK_H__



#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_queue.h"
#include "awoke_event.h"
#include "awoke_memory.h"
#include "awoke_package.h"



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

typedef struct _mtk_work {
	err_type (*handle)();
	uint64_t us;
} mtk_work;

typedef struct _mtk_protocol_handle {
	const char *name;
	err_type (*handle_read)(struct _mtk_context *, struct _mtk_connect *);
	err_type (*handle_write)(struct _mtk_context *, struct _mtk_connect *);
} mtk_protocol_handle;

typedef err_type
mtk_callback_function(struct _mtk_connect *conn, enum mtk_callback_reasons reason,
		    void *user, void *in, size_t len);

typedef struct _mtk_connect {
	awoke_event event;
	int fd;
	uint8_t msgindex;
	mtk_channel_e chntype;
	struct _awoke_queue *ring;
	struct _mtk_context *context;
	struct _mtk_protocol_handle *protocol;
	unsigned int need_do_redirect:1;
	unsigned int leave_wirtable_active:1;
	void *user;
} mtk_connect;

typedef struct _mtk_client_connect_info {
	struct _mtk_context *context;
	const char *address;
	uint16_t port;
	uint8_t channel;
	struct _mtk_protocol_handle *protocol;
} mtk_client_connect_info;

typedef struct _mtk_context {

	char *name;

	struct _awoke_minpq works_pq;

	struct _awoke_event_loop *evloop;

	err_type (*callback)(struct _mtk_connect *conn, mtk_callback_reasons reason,
		    void *user, void *in, size_t len);

	struct _mtk_connect connects[MTK_CHANNEL_SIZEOF];

	unsigned int service_need_stop:1;
	
} mtk_context;

typedef struct _mtk_service {
	struct _mtk_context *context;
};

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
#endif /* __MICRO_TALK_H__ */