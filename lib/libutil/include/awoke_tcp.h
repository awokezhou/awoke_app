
#ifndef __AWOKE_TCP_H__
#define __AWOKE_TCP_H__

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_macros.h"
#include "awoke_socket.h"



typedef enum {
	tcp_unavailable = 0,
	tcp_init,
	tcp_connected,
} awoke_tcp_connect_status;


/* -- tcp connect define --{ */
typedef struct _awoke_tcp_connect {
	int sock;								/* socket fd */
	uint16_t port;							/* tcp port */
	uint16_t flags;							
	awoke_tcp_connect_status status;
	int recv_len;
	struct sockaddr_in addr;
} awoke_tcp_connect;
/* }-- tcp connect define -- */

err_type awoke_tcp_do_connect(awoke_tcp_connect *c);
err_type awoke_tcp_connect_send(awoke_tcp_connect *c, void *buf, int len);
err_type awoke_tcp_connect_recv(awoke_tcp_connect *c, void *buf, int max_size);
err_type awoke_tcp_connect_create(awoke_tcp_connect *c, const char *addr, uint16_t port);
err_type awoke_tcp_connect_release(awoke_tcp_connect *c);
#endif /* __AWOKE_TCP_H__ */
