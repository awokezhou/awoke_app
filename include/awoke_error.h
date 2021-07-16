#ifndef __AWOKE_ERROR_H__
#define __AWOKE_ERROR_H__


typedef enum 
{
    et_ok = 0,

	/* 1~10: special return value, means non-error */
	et_finish,					/* for waiting areas */
	et_unfinished,				/* for waiting areas */
	et_yield,					/* need to exit from current function scope */
	et_noneed,					/* don't need to do something */
	et_reboot,					/* system reboot */
	et_killed,					/* object be killed */

	/* 11~30: general error */
	err_common = 10,			/* general error */
	err_param,					/* function passed in parameter error, like ptr null */
	err_range,					/* value range error */
	err_oom,					/* out of memory */
	err_exist,					/* object already exists */
	err_notfind,				/* object not find */
	err_full,					/* storage structure is full */
	err_empty,					/* storage structure is empty */
	err_timeout,				/* general timeout */
	err_locked,					/* resource is locked */
	err_notready,				/* not ready do to something */
	err_notsupport,				/* not support */
	err_match,
	err_checksum,
	err_fail,
	err_open,
	err_send,
	err_unfinished,

	et_noeed,
    et_fail,
    et_param,
    et_nomem,
    et_mem_limit,
    et_exist,
    et_full,
    et_empty,
    et_notfind,
    et_encode,
    et_decode,
    et_sock_set,
    et_sock_creat,
    et_sock_bind,
    et_sock_conn,
    et_sock_accept,
    et_sock_listen,
    et_sock_send,
    et_sock_recv,
    et_evl_create,
    et_parser,
    et_response,
    et_file_open,
    et_file_read,
    et_file_send,
    et_file_mmap,
    et_sendmore,
    et_flow_pull,
    et_sem_create,
    et_sem_init,
    et_worker_mutex,
    et_worker_condition,
    et_aes_enc,

	et_pipe,

	et_locked,
	et_lock_init,
	et_lock_timeout,

	et_waitev_create,
	et_waitev_finish,

} err_type;


#endif /* __AWOKE_ERROR_H__ */
