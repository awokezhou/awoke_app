
#ifndef __BENCHMARK_H__
#define __BENCHMARK_H__

#include "awoke_log.h"
#include "awoke_type.h"
#include "awoke_lock.h"
#include "awoke_error.h"
#include "awoke_event.h"
#include "awoke_queue.h"
#include "awoke_waitev.h"
#include "awoke_memory.h"
#include "awoke_worker.h"
#include "awoke_macros.h"
#include "awoke_string.h"
#include "awoke_http.h"
#include "awoke_buffer.h"
#include "awoke_package.h"
#include "awoke_string.h"
#include "awoke_printf.h"
#include "awoke_tree.h"
#include "vin_parser.h"
#include "condition_action.h"
#include <poll.h>

typedef enum {
	arg_none = 0,
	arg_waitev_test,
	arg_condaction_test,
	arg_event_channel,
	arg_worker,
	arg_pthread,
	arg_bitflag,
	arg_lock,
	arg_timer_worker,
	arg_queue_zero_filter,
	arg_vin_parse_test,
	arg_http_request_test,
	arg_sscanf_test,
	arg_buffchunk_test,
	arg_valist_test,
    arg_time_zero,
    arg_gpsdata_test,
    arg_queue_test,
    arg_minpq_test,
    arg_fifo_test,
    arg_btree_test,
    arg_md5_test,
    arg_socket_poll_test,
    arg_list_test,
    arg_logfile_cache_test,
    arg_filecache_test,
    arg_log_color_test,
    arg_log_cache_test,
    arg_macro_log_test,
    arg_eots_hd_test,
    arg_array_fifo_test,
    arg_fbkflow_test,
    arg_autofocus_test,
    arg_valuemem_test,
    arg_fibonacci_test,
    arg_version_test,
    arg_mblock_test,
    arg_kalman_test,
    arg_mae_test,
    arg_checksum_test,
    arg_imgsensor_test,
    arg_nvm_test,
    arg_epc_test,
    arg_sign_exten,
    arg_cameraconfig_test,
    arg_sensorconfig_test,
    arg_nucparamsgen_test,
    arg_buildmacros_test,
    arg_memmove_test,
<<<<<<< HEAD
    arg_attribute_test,
    arg_quickemb_test,
    arg_ptrfunc_test,
=======
    arg_strucp_test,
>>>>>>> 9f093788c86a002f47df2a51c71270239926b9d2
} benchmark_args;

typedef enum {
	arg_http_none = 0,
	arg_http_content_type,
	arg_http_authorization,
	arg_http_accept,
	arg_http_connection,
	arg_http_user_agent,
	arg_http_connect_keep,
} http_args;

typedef enum {
	arg_eots_none = 0,
	arg_eots_ecmd_input,
	arg_eots_ecmd_fix,
	arg_eots_ecmd_file,
} eots_args;

typedef err_type (*bencmark_func)(int argc, char *argv[]);



typedef struct _waitev_test_t {
	int count;
	int status;
} waitev_test_t;

typedef struct _event_channel_test_t {
	int count;
	awoke_event_loop *evl;
	awoke_event_pipech pipe_channel;
} event_channel_test_t;

typedef struct _pipech_message_header {
	uint16_t src;
	uint16_t dst;
	uint32_t msg_type;

	uint16_t f_event,
			 f_request,
			 f_response,
			 f_broadcast;

	uint32_t word;
	uint32_t data_len;

	struct _pipech_message_header *next;	
} pipech_msg_header;

typedef struct _lock_test_t {
	awoke_lock *mux_lock;
	awoke_lock *sem_lock;
	awoke_worker *wk;
} lock_test_t;


typedef struct _timer_worker_test_t {
	awoke_tmwkr *twk;
	int x;
} timer_worker_test_t;


#define bk_burst(...) 	logm_burst(LOG_M_BK, 	__VA_ARGS__)
#define bk_trace(...) 	logm_trace(LOG_M_BK, 	__VA_ARGS__)
#define bk_debug(...) 	logm_debug(LOG_M_BK, 	__VA_ARGS__)
#define bk_info(...) 	logm_info(LOG_M_BK, 	__VA_ARGS__)
#define bk_notice(...) 	logm_notice(LOG_M_BK, 	__VA_ARGS__)
#define bk_err(...) 	logm_err(LOG_M_BK, 		__VA_ARGS__)
#define bk_warn(...) 	logm_warn(LOG_M_BK, 	__VA_ARGS__)
#define bk_bug(...) 	logm_bug(LOG_M_BK, 		__VA_ARGS__)



#endif /* __BENCHMARK_H__ */
