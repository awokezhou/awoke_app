#ifndef __AWOKE_ERROR_H__
#define __AWOKE_ERROR_H__


typedef enum 
{
    et_ok = 0,
    et_fail,
    et_param,
    et_nomem,
    et_exist,
    et_sock_set,
    et_sock_creat,
    et_sock_bind,
    et_sock_conn,
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
} err_type;


#endif /* __AWOKE_ERROR_H__ */
