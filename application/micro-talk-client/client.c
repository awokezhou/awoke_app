
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



err_type mtk_timer_send(struct _mtk_context *ctx, struct _mtk_work *work)
{
	mtk_voicefile_send(ctx, "test-recv-29.amr", 32*1024);
	return mtk_work_schedule(ctx, mtk_timer_send, 60*60*MTK_US_PER_SEC);
}


typedef enum {
	arg_none = 0,
	arg_dport,
	arg_nport,
	arg_imei,
	arg_sendfile,
	arg_recvfile,
	arg_chunksize,
	arg_fileid,
	arg_katimeout,
} arg_list;

int main(int argc, char **argv)
{
	int opt;
	int mode = 0;
	int fileid = 1;
	int loglevel = LOG_DBG;
	int chunksize = 1024*32;
	char *filepath = "test";
	char *serveraddr = "112.124.201.252";
	char *devimei = "0134653629865946";
	int nc_port = 8972;
	int dc_port = 8970;
	err_type ret;
	mtk_context *ctx;

	static const struct option long_opts[] = {
		{"loglevel",			required_argument,	NULL,	'l'},
		{"serveraddr",			required_argument,	NULL,	's'},
		{"dport",				required_argument,  NULL,   arg_dport},
		{"nport",				required_argument,  NULL,   arg_nport},
		{"imei",				required_argument,	NULL,   arg_imei},
		{"sendfile",			required_argument,  NULL,	arg_sendfile},
		{"recvfile",			required_argument,  NULL,	arg_recvfile},
		{"chunksize",			required_argument,  NULL,	arg_chunksize},
		{"fileid",				required_argument,  NULL,	arg_fileid},
		{NULL, 0, NULL, 0},
	};

	while ((opt = getopt_long(argc, argv, "l:s:?h-", long_opts, NULL)) != -1)
	{
		switch (opt) {

		case 'l':
			loglevel = atoi(optarg);
			break;

		case 's':
			serveraddr = optarg;
			break;

		case arg_dport:
			dc_port = atoi(optarg);
			break;

		case arg_nport:
			nc_port = atoi(optarg);
			break;

		case arg_imei:
			devimei = optarg;
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
			chunksize = atoi(optarg);
			break;

		case arg_fileid:
			fileid = atoi(optarg);
			break;
			
        default:
            break;
		}
	}

	awoke_log_init(loglevel, loglevel);
	
	ctx = mtk_context_create("Micro Talk Client", serveraddr, dc_port, nc_port, devimei);
	if (!ctx) {log_err("mtk context create error"); return 0;}

	log_info("%s start", ctx->name);
	
	mtk_work_schedule(ctx, mtk_nc_connect, 0);

	//mtk_work_schedule(ctx, mtk_timer_send, 60*MTK_US_PER_SEC);

	if (mode == 1)
		mtk_voicefile_send(ctx, filepath, chunksize);
	else if (mode == 2)
		mtk_voicefile_recv(ctx, filepath, fileid);
	/*
	else 
		//mtk_voicefile_recv(ctx, filepath, 1);
		//mtk_voicefile_send(ctx, "linux-c-api-ref.pdf", 1024*32);
		mtk_voicefile_send(ctx, "helloworld.txt", 1);*/
	
	
	mtk_voicefile_send(ctx, "test-recv-46.amr", 1024*32);
	mtk_voicefile_send(ctx, "test-recv-46.amr", 1024*32);

	/* service loop run */
	do {

		ret = mtk_service_run(ctx, 0);
		
	} while (!mtk_service_should_stop(ctx) &&
			 (ret == et_ok));
	
	log_notice("%s stop", ctx->name);

	return 0;
}

