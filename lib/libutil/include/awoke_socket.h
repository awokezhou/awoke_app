#ifndef __AWOKE_SOCKET_H__
#define __AWOKE_SOCKET_H__

#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>


#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_macros.h"
#include "awoke_memory.h"

#define SOCK_LOCAL			0x01
#define SOCK_INTERNET_TCP	0x02
#define SOCK_INTERNET_UDP   0x03
#define SOCK_ETHERNET		0x04

#ifndef SOMAXCONN
#define SOMAXCONN				5
#endif

#ifndef SOCK_DEF_PORT
#define SOCK_DEF_PORT		8998
#endif

#define UNIX_SOCK_F_NONE		0x0000
#define UNIX_SOCK_F_WAIT_REPLY	0x0002

#define sk_map_item(ltype, family, stype, flag) {ltype, family, stype, flag}
typedef struct _sock_map{
	uint8_t ltype;
	int family;
	int stype;
	int flag;
}sock_map; 


/* -- network io define --{ */
typedef struct _awoke_network_io {
	char *name;
	int call_nr;
	int	(*accept)(int, struct sockaddr *, int *);
	int (*bind)(int, struct sockaddr *, int);
	int (*connect)(int, struct sockaddr *, int);
	int (*recv)(int, void *, int, unsigned int);
	int (*send)(int, const void *, int, unsigned int);
	int (*read)(int, void *, int);
	int (*socket_create)(int, int, int);
	int (*socket_close)(int);
} awoke_network_io;
/* }-- network io define -- */




int awoke_socket_server(uint8_t, const char *, uint32_t, bool);
int awoke_socket_accept_unix(int);
int awoke_socket_accpet(int);
int awoke_socket_set_nonblocking(int);
bool awoke_sock_fd_read_ready(int);
int awoke_socket_create(int, int, int);
err_type awoke_unix_message_send(char *, char *, int, int *, uint32_t);
err_type awoke_socket_recv_wait(int fd, uint32_t tm);
awoke_network_io *awoke_network_io_get();


#endif /* __AWOKE_SOCKET_H__ */

