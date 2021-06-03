#ifndef __AWOKE_ERROR_H__
#define __AWOKE_ERROR_H__


typedef enum 
{
    et_ok = 0,
    et_unfinished,
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

	err_param,
	err_exist,
	err_oom,
	err_notfind,
	err_open,
	err_fail,
	err_empty,
	err_send,
	err_match,
	err_unfinished,
	err_checksum,
	err_incomplete,
} err_type;


#endif /* __AWOKE_ERROR_H__ */
