
#include "micro_talk.h"
#include "mtk_client.h"
#include "mtk_session.h"

#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/socket.h>
#include "awoke_string.h"


err_type mtk_voicefile_recv(struct _mtk_context *ctx, const char *filepath, uint32_t fileid)
{
	mtk_session *ss;
	mtk_chunkinfo *ci;

	ci = mem_alloc_trace(sizeof(mtk_chunkinfo), "chunkinfo");
	if (!ci) {
		log_err("alloc chunkinfo error");
		return et_nomem;
	}
	ci->index = 0;
	ci->fileid = fileid;
	if (filepath)
		ci->filepath = awoke_string_dup(filepath);
	ci->type = MTK_CHUNKTYPE_FILE;

	ss = mtk_session_clone(&chunkrecv);
	ss->id = mtk_session_id_get(ctx);
	ss->data = ci;
	ss->state = MTK_SESSION_REQ;
	ss->destory = mtk_chunkinfo_free;

	return mtk_work_schedule_withdata(ctx, mtk_session_start, 1*MTK_US_PER_MS, ss);
}

err_type mtk_voicebuff_recv(struct _mtk_context *ctx)
{
	mtk_session *ss;
	mtk_chunkinfo *ci;

	ci = mem_alloc_trace(sizeof(mtk_chunkinfo), "chunkinfo");
	if (!ci) {
		log_err("alloc chunkinfo error");
		return et_nomem;
	}
	ci->fileid = 1;
	ci->index = 0;
	ci->type = MTK_CHUNKTYPE_BUFF;

	ss = mtk_session_clone(&chunkrecv);
	ss->id = mtk_session_id_get(ctx);
	ss->data = ci;
	ss->state = MTK_SESSION_REQ;
	ss->destory = mtk_chunkinfo_free;

	return mtk_work_schedule_withdata(ctx, mtk_session_start, 1*MTK_US_PER_MS, ss);
}

err_type mtk_voicefile_send(struct _mtk_context *ctx, const char *filepath, int chunksize)
{
	err_type ret;
	mtk_session *ss;
	mtk_chunkinfo *ci;

	log_trace("file %s", filepath);

	ret = mtk_chunkinfo_generate_from_file(filepath, chunksize, &ci);
	if (ret != et_ok) {
		log_err("generate chunkinfo error:%d", ret);
		return ret;
	}

	ss = mtk_session_clone(&chunksend);
	ss->id = mtk_session_id_get(ctx);
	ss->data = ci;
	ss->state = MTK_SESSION_REQ;
	ss->destory = mtk_chunkinfo_free;

	return mtk_work_schedule_withdata(ctx, mtk_session_start, 1*MTK_US_PER_MS, ss);	
}

typedef enum {
	arg_none = 0,
	arg_sendfile,
	arg_recvfile,
	arg_chunksize,
	arg_fileid,
} arg_list;

int main(int argc, char **argv)
{
	int opt;
	int mode = 0;
	int fileid = 1;
	int chunksize = 1024*32;
	int loglevel = LOG_TRACE;
	char *filepath = "test";
	err_type ret;
	mtk_context *ctx;

	static const struct option long_opts[] = {
		{"loglevel",			required_argument,	NULL,	'l'},
		{"sendfile",			required_argument,  NULL,	arg_sendfile},
		{"recvfile",			required_argument,  NULL,	arg_recvfile},
		{"chunksize",			required_argument,  NULL,	arg_chunksize},
		{"fileid",				required_argument,  NULL,	arg_fileid},
		{NULL, 0, NULL, 0},
	};

	while ((opt = getopt_long(argc, argv, "l:s:r:?h-", long_opts, NULL)) != -1)
	{
		switch (opt) {

		case 'l':
			loglevel = atoi(optarg);
			break;

		case arg_sendfile:
			mode = 1;
			filepath = optarg;
			break;

		case arg_recvfile:
			mode = 2;
			filepath = optarg;
			break;

		case arg_chunksize:
			chunksize = 1024*atoi(optarg);
			break;

		case arg_fileid:
			fileid = atoi(optarg);
			break;
			
        default:
            break;
		}
	}

	awoke_log_init(loglevel, loglevel);
	
	ctx = mtk_context_create("Micro Talk Client");
	if (!ctx) {log_err("mtk context create error"); return 0;}

	log_info("%s start", ctx->name);
	
	//mtk_work_schedule(ctx, mtk_nc_connect, 0);
	//mtk_work_schedule_withdata(ctx, dc_normal_get, 1, &ctx->connects[MTK_CHANNEL_DATA]);
	//mtk_work_schedule(ctx, dc_file_send, 1*MTK_US_PER_SEC);
	//mtk_work_schedule(ctx, dc_file_recv, 1*MTK_US_PER_SEC);
	//mtk_voicefile_recv(ctx, 0);

	if (mode == 1)
		mtk_voicefile_send(ctx, filepath, chunksize);
	else if (mode == 2)
		mtk_voicefile_recv(ctx, filepath, fileid);
	else 
		mtk_voicefile_recv(ctx, filepath, 1);
		//mtk_voicefile_send(ctx, "linux-c-api-ref.pdf", 1024*32);
		//mtk_voicefile_send(ctx, "helloworld.txt", 1);
	
	
	/* service loop run */
	do {

		ret = mtk_service_run(ctx, 0);
		
	} while (!mtk_service_should_stop(ctx) &&
			 (ret == et_ok));
	
	log_notice("%s stop", ctx->name);

	return 0;
}

