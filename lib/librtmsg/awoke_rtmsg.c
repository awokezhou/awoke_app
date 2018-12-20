
#include <errno.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>


#include "awoke_rtmsg.h"
#include "awoke_log.h"
#include "awoke_error.h"
#include "awoke_string.h"
#include "awoke_memory.h"
#include "awoke_macros.h"

rtmsg_app_info app_info_array[] = 
{
	{app_none, 			"none", 		RA_F_NULL},
	{app_test_client,	"test_client",	RA_F_NULL},
	{app_test_server, 	"test_server", 	RA_F_NULL},
};
int app_array_size = sizeof(app_info_array)/sizeof(rtmsg_app_info);

rtmsg_app_info *get_app_array()
{
	return app_info_array;
}

int get_app_array_size()
{
	return app_array_size;
}

err_type rt_message_send(int, rtmsg_header *);

err_type rt_message_send_reply(rtmsg_handle *handle, 
									    rtmsg_header *header,
									    err_type code)
{
    rtmsg_header reply;

	reply.dst = header->src;
    reply.src = header->dst;
    reply.msg_type = header->msg_type;

	reply.f_request = 0;
    reply.f_response = 1;

	reply.word = code;

	return rt_message_send(handle->fd, &reply);
}

err_type rt_message_send(int fd, rtmsg_header *buff)
{
	int rc;
	uint32_t total_size;

	total_size = sizeof(rtmsg_header) + buff->data_len;

	rc = write(fd, buff, total_size);
	if (rc < 0) {
		if (errno == EPIPE) {
			return et_sock_conn;
		} else {
			return et_sock_send;
		}
	} else if (rc != total_size) {
		return et_sock_send;
	}

	return et_ok;
}

static err_type rt_message_wait_data(int fd, uint32_t tm)
{
	int rc;
	fd_set rfds;
	struct timeval tv;

	FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

	tv.tv_sec = tm/1000;
	tv.tv_usec = (tm%1000)*1000;

	rc = select(fd+1, &rfds, NULL, NULL, &tv);
	if ((rc == 1) && (FD_ISSET(fd, &rfds)))
		return et_ok;
	else
		return et_sock_recv;
}

err_type rt_message_recv(int fd, rtmsg_header **buff, uint32_t *timeout)
{
	int rc;
	rtmsg_header *header;
	err_type ret = et_ok;

	if (!buff)
		return et_fail;
	else
		*buff = NULL;

	if (timeout) {
		ret = rt_message_wait_data(fd, *timeout);
		if (et_ok != ret) {
			return ret;
		}
	}

	header = mem_alloc_z(sizeof(rtmsg_header));
	if (!header) {
		return et_nomem;
	}

	rc = read(fd, header, sizeof(rtmsg_header));
	if ((rc==0) || ((rc==-1) && (errno==131))) {
		ret = et_sock_conn;
		goto err;
	} else if (rc<0 || rc!=sizeof(rtmsg_header)) {
		ret = et_sock_recv;
		goto err;
	}

	if (header->data_len > 0) {
		char *data;
		uint32_t read_size = 0;
		uint32_t rest_size = header->data_len;

		header = mem_realloc(header, sizeof(rtmsg_header) + header->data_len);
		if (!header) {
			ret = et_nomem;
			goto err;
		}

		data = (char *)(header+1);
		do {
			if (timeout) {
				ret = rt_message_wait_data(fd, *timeout);
				if (et_ok != ret) {
					return ret;
				}
			}
			rc = read(fd, data, rest_size);
			if (rc < 0) {
				ret = et_sock_recv;
				goto err;
			} else {
				data += rc;
				read_size += rc;
				rest_size -= rc;
			}
		} while (read_size < header->data_len);
	}

	*buff = header;
	return ret;

err:
	if (header)
		mem_free(header);
	return ret;
}

err_type rt_message_init(rt_app_id rid, rtmsg_handle **msg_handle)
{
	int fd;
	int rc;
	rtmsg_header header;
	rtmsg_handle *handle;
	err_type ret = et_ok;
	struct sockaddr_un router_addr;

	handle = mem_alloc_z(sizeof(rtmsg_handle));
	if (!handle)
		return et_nomem;

	fd = socket(AF_LOCAL, SOCK_STREAM, 0);
	if (fd < 0) {
		ret = et_sock_creat;
		goto err;
	}

	rc = fcntl(fd, F_SETFD, FD_CLOEXEC);
	if (rc < 0) {
		ret = et_sock_set;
		goto err;
	}

	router_addr.sun_family = AF_LOCAL;
	strcpy(router_addr.sun_path, RTMSG_SOCK_ADDR);
	
	rc = connect(fd, 
				 (struct sockaddr *)&router_addr,
				 sizeof(router_addr));
	if (rc < 0) {
		ret = et_sock_conn;
		goto err;
	}

	memset(&header, 0x0, sizeof(header));
	header.src = rid;
	header.dst = app_message_router;
	header.msg_type = rtmsg_associate;
	header.f_event = 1;

	ret = rt_message_send(fd, &header);
	if (et_ok != ret) {
		log_err("send associate message to router error, ret %d", ret);
		goto err;
	}

	handle->fd = fd;
	handle->rid = rid;
	*msg_handle = handle;
	return et_ok;

err:
	if (handle) 
		mem_free(handle);
	if (fd)
		close(fd);
	return ret;
}

void rt_message_handle_clean(rtmsg_handle **handle)
{
	rtmsg_handle *p;

	if (!handle || !*handle)
		return;

	p = *handle;

	if (p->fd)
		close(p->fd);
	mem_free(p);
	p = NULL;
	return;
}

err_type awoke_rtmsg_send(rtmsg_handle *handle,
							    rtmsg_header *header)
{
	return rt_message_send(handle->fd, header);
}

err_type awoke_rtmsg_recv(rtmsg_handle *handle,
							    rtmsg_header *header)
{
	return rt_message_recv(handle->fd, header, NULL);
}

err_type awoke_rtmsg_recv_timeout(rtmsg_handle *handle,
									        rtmsg_header *header,
							    			uint32_t timeout)
{
	return rt_message_recv(handle->fd, header, &timeout);
}

err_type awoke_rtmsg_init(rt_app_id rid, rtmsg_handle **msg_handle)
{
	return rt_message_init(rid, msg_handle);
}

void awoke_rtmsg_handle_clean(rtmsg_handle **handle)
{
	return rt_message_handle_clean(handle);
}




