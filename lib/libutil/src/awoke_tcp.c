
#include "awoke_log.h"
#include "awoke_tcp.h"

static void tcp_connect_status_set(awoke_tcp_connect *c, awoke_tcp_connect_status status)
{
	c->status = status;
}

err_type awoke_tcp_connect_release(awoke_tcp_connect *c)
{
	awoke_network_io *io = awoke_network_io_get();
	
	if (c->status == tcp_unavailable)
		return et_ok;
	io->socket_close(c->sock);

	tcp_connect_status_set(c, tcp_unavailable);

	return et_ok;
}

err_type awoke_tcp_do_connect(awoke_tcp_connect *c)
{
	int ret;
	awoke_network_io *io = awoke_network_io_get();

	if (c->status == tcp_connected)
		return et_ok;

	ret = io->connect(c->sock, (struct sockaddr*)&(c->addr), sizeof(struct sockaddr_in));
	if (ret < 0) {
		log_err("connect error, ret %d");
		return et_sock_conn;
	}

	tcp_connect_status_set(c, tcp_connected);

	return et_ok;
}

err_type awoke_tcp_connect_send(awoke_tcp_connect *c, char *buf, int buf_size, 
	int *send_size)
{
	int ss;
	awoke_network_io *io = awoke_network_io_get();

	if (!c || !buf || c->status != tcp_connected)
		return et_param;

	ss = io->send(c->sock, buf, buf_size, 0);
	if (ss < 0) {
		log_err("send error, ret %d", ss);
		return et_sock_send;
	}

	if (send_size != NULL)
		*send_size = ss;

	return et_ok;
}

bool awoke_tcp_recv_finish(awoke_tcp_connect *c)
{
	bool data_ready = awoke_sock_fd_read_ready(c->sock);
	return (!data_ready);
}

err_type awoke_tcp_connect_recv(awoke_tcp_connect *c, char *buf, int max_size, 
	int *read_size, bool block)
{
	int rs;
	awoke_network_io *io = awoke_network_io_get();

	if (block) {
		rs = io->read(c->sock, buf, max_size);
	} else {
		rs = io->recv(c->sock, buf, max_size, 0);
	}

	if (rs < 0) {
		log_err("send error, ret %d", rs);
		return et_sock_recv;
	}  

	if (read_size != NULL)
		*read_size = rs;

	return et_ok;
}

err_type awoke_tcp_connect_create(awoke_tcp_connect *c, const char *addr, uint16_t port)
{
	int fd;
	awoke_network_io *io = awoke_network_io_get();
	
	if (!c || !addr) 
		return et_param;

	fd = io->socket_create(PF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		log_err("socket create error");
		return et_sock_creat;
	}

	c->sock = fd;
	c->addr.sin_family = PF_INET;
	c->addr.sin_port = htons(port);
	c->addr.sin_addr.s_addr = inet_addr(addr);

	log_trace("port:%d", port);

	tcp_connect_status_set(c, tcp_init);

	return et_ok;
}
