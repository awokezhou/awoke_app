
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

err_type awoke_tcp_connect_send(awoke_tcp_connect *c, void *buf, int len)
{
	int rc;
	awoke_network_io *io = awoke_network_io_get();

	if (!c || !buf || c->status != tcp_connected)
		return et_param;

	rc = io->send(c->sock, buf, len, 0);
	if (rc < 0) {
		log_err("send error, ret %d", rc);
		return et_sock_send;
	}

	return et_ok;
}

err_type awoke_tcp_connect_recv(awoke_tcp_connect *c, void *buf, int max_size)
{
	int rc;
	awoke_network_io *io = awoke_network_io_get();

	rc = io->recv(c->sock, buf, max_size, 0);
	if (rc < 0) {
		log_err("send error, ret %d", rc);
		return et_sock_recv;
	}

	return et_ok;
}

err_type awoke_tcp_connect_create(awoke_tcp_connect *c, const char *addr, uint16_t port)
{
	int fd;
	int ret;
	struct sockaddr_in local;
	awoke_network_io *io = awoke_network_io_get();
	
	if (!c || !addr) 
		return et_param;

	fd = io->socket_create(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		log_err("socket create error");
		return et_sock_creat;
	}

	c->sock = fd;
	c->addr.sin_family = AF_INET;
	c->addr.sin_port = htons(port);
	c->addr.sin_addr.s_addr = inet_addr(addr);

	tcp_connect_status_set(c, tcp_init);

	return et_ok;
}
