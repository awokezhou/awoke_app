
#include "bk_quickemb.h"
#include "qe_core.h"
#include "qe_debug.h"

static char heapbuffer[2048];

void bk_quickemb_assert_hook(const char *ex, const char *func, qe_size_t line)
{
	bk_bug("Assert(%s) %s:%d", ex, func, line);
}

void bk_quickemb_log_output(char *buf, int len)
{
	printf("%.*s", len, buf);
}

void bk_quickemb_log_timestr(char *buf, int size)
{
	time_t now;
	struct tm date;
	
	
	now = time(NULL);
	localtime_r(&now, &date);

	qe_strb_t strb = qe_strb_init(buf, size);
	
	qe_strb_format(strb, "%i/%02i/%02i %02i:%02i:%02i",
		date.tm_year + 1900,
		date.tm_mon + 1,
		date.tm_mday,
		date.tm_hour,
		date.tm_min,
		date.tm_sec);
}

static err_type hw_board_init(void)
{
	qe_heap_init(heapbuffer, 2048);

	qe_log_mdl_add("sys", QE_LOG_TRACE);
	qe_log_mdl_add("drv", QE_LOG_TRACE);
	qe_log_mdl_add("pkt", QE_LOG_TRACE);
	qe_log_mdl_add("app", QE_LOG_TRACE);

	qe_log_set_output(bk_quickemb_log_output);
	qe_log_set_timestr(bk_quickemb_log_timestr);

	qe_hexdump_notice(heapbuffer, 32);

	qelog_trace("app", "hello world");

	qe_assert_set_hook(bk_quickemb_assert_hook);

	//qe_board_init();
	qe_stage_initialize();
	return qe_ok;
}

err_type benchmark_quickemb_test(int argc, char *argv[])
{
	hw_board_init();
	
	return et_ok;
}
