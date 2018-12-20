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
#include "awoke_memory.h"
#include "awoke_log.h"
#include "awoke_error.h"

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

#define sk_map_item(ltype, family, stype, flag) {ltype, family, stype, flag}
typedef struct _sock_map{
	uint8_t ltype;
	int family;
	int stype;
	int flag;
}sock_map; 

int awoke_socket_server(uint8_t, const char *, uint32_t, bool);


#endif /* __AWOKE_SOCKET_H__ */

