
#include "recorder.h"
#include "awoke_log.h"
#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_worker.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/soundcard.h>


typedef struct _recorder_context {
	int fd;
} recorder_context;

err_type recorder_init(recorder_context *ctx)
{
	int r, fd, rate=22050, format=AFMT_U8, channels=0;

	fd = open("/dev/dsp", O_WRONLY);
	if (!fd) {
		log_err("open device error");
		return et_file_open;
	}

	r = ioctl(fd, SNDCTL_DSP_STEREO, &channels);
	if (r < 0) {
		log_err("set channel error");
		close(fd);
		return et_fail;
	}

	if (channels != 0) {
		log_debug("only support single channel");
	}

	r = ioctl(fd, SNDCTL_DSP_SETFMT, &format);
	if (r < 0) {
		log_err("set format error");
		close(fd);
		return et_fail;
	}

	r = ioctl(fd, SNDCTL_DSP_SPEED, &rate);
	if (r < 0) {
		log_err("set rate error");
		close(fd);
		return et_fail;
	}

	
}

char buf[32];

err_type recorder_run(recorder_context *ctx, awoke_worker *wk)
{
	int status;
	err_type ret;
	
	do {
		
		log_info("Please say:");
		status = read(ctx->fd, buf, sizeof(buf));
	
	} while (!awoke_worker_should_stop(wk) &&
			 (ret == et_ok));

	log_notice("%s stop (ret:%d)", ret);

	return ret;
}

err_type recorder_worker(void *d)
{	
	recorder_context context;
	awoke_worker_context *ctx = (awoke_worker_context *)d;
	awoke_worker *wk = ctx->worker;

	recorder_init(&context);

	recorder_run(&context, wk);
}
