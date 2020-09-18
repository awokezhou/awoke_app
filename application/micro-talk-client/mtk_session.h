
#ifndef __MTK_SESSION_H__
#define __MTK_SESSION_H__



#include "micro_talk.h"



typedef enum {
	MTK_SESSION_STATIC = 0,
	MTK_SESSION_DYNAMIC,
} mtk_session_type;

typedef enum {
	MTK_SESSION_NONE = 0,
	MTK_SESSION_REQ,
	MTK_SESSION_RSP,
	MTK_SESSION_FIN,
} mtk_session_state;


typedef struct _mtk_session {

	int id;
	char *name;
	mtk_channel_e channel;
	mtk_session_type type;

	mtk_session_state state;

	err_type (*req_process)(struct _mtk_connect *);
	err_type (*rsp_process)(struct _mtk_connect *, struct _mtk_response_msg *);

	void *data;
	void (*destory)(void *);
	
} mtk_session;


extern mtk_session chunkrecv;
extern mtk_session chunksend;
extern mtk_session getcmd;
int mtk_session_id_get(struct _mtk_context *ctx);
err_type mtk_session_start(struct _mtk_context *ctx, struct _mtk_work *work);
void mtk_session_free(struct _mtk_session *ss);
mtk_session *mtk_session_clone(struct _mtk_session *src);
err_type mtk_session_add(struct _mtk_context *ctx, struct _mtk_session *clone, void *data, 
	void (*destory)(void *));
err_type mtk_voicefile_recv(struct _mtk_context *ctx, const char *filepath, uint32_t fileid);
err_type mtk_voicefile_send(struct _mtk_context *ctx, const char *filepath, int chunksize);
#endif /* __MTK_SESSION_H__ */
