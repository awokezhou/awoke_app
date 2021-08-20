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
	et_oneed,

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
	err_incomplete,
	err_sock_creat,
} err_type;


#endif /* __AWOKE_ERROR_H__ */
