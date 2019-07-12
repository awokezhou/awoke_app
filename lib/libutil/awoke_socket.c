#include <sys/un.h>

#include "awoke_socket.h"

static sock_map sk_map[] = 
{
	sk_map_item(SOCK_LOCAL, 		AF_LOCAL, 	SOCK_STREAM, 	0),
	sk_map_item(SOCK_INTERNET_TCP, 	AF_INET, 	SOCK_STREAM, 	0),
	sk_map_item(SOCK_INTERNET_UDP, 	AF_INET, 	SOCK_DGRAM, 	0),
	sk_map_item(SOCK_ETHERNET, 		AF_INET, 	SOCK_RAW,		0),
};
static const int sk_map_size = array_size(sk_map);

static inline sock_map *get_sock_proto(uint8_t type)
{
	int i;
	
	for (i=0; i<sk_map_size; i++) {
		sock_map *p = sk_map + i;
		if (p->ltype == type) {
			return p;
		}
	}

	return NULL;
}

bool awoke_sock_fd_read_ready(int fd)
{
    int read = 0;
    
    ioctl(fd, FIONREAD, &read);   

    if (read)
        return TRUE;
    else
        return FALSE;
}

int awoke_socket_set_nonblocking(int sockfd)
{
    if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK) == -1) {
        log_err("Can't set to non-blocking mode socket %i", sockfd);
        return -1;
    }

    fcntl(sockfd, F_SETFD, FD_CLOEXEC);

    return 0;
}

int awoke_socket_set_timeout(int sockfd, int timeout)
{
	struct timeval tm;

	tm.tv_sec = timeout;
	
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tm, sizeof(tm)) < 0) {
		 log_err("Can't set timemout");
		 return -1;
	}

	return 0;
}

int awoke_socket_accpet(int server_fd)
{
    int remote_fd;
    struct sockaddr sock_addr;
    socklen_t socket_size = sizeof(struct sockaddr);

    remote_fd = accept(server_fd, &sock_addr, &socket_size);

    return remote_fd;
}

int awoke_socket_accept_unix(int server_fd)
{
	int remote_fd;
	struct sockaddr_un sock_addr;
	socklen_t socket_size = sizeof(struct sockaddr);

	remote_fd = accept(server_fd, &sock_addr, &socket_size);

	return remote_fd;
}

static err_type awoke_socket_bind(int fd, const struct sockaddr *addr,
                   socklen_t addrlen, int backlog)
{
	int ret;

	ret = bind(fd, addr, addrlen);
    if(ret == -1) {
        log_err("error binding socket");
        return et_sock_bind;
    }

	ret = listen(fd, backlog);
    if(ret == -1) {
        return et_sock_listen;
    }

	return et_ok;
}

int awoke_socket_create(int domain, int type, int protocol)
{
    int fd;

#ifdef SOCK_CLOEXEC
    fd = socket(domain, type | SOCK_CLOEXEC, protocol);
#else
    fd = socket(domain, type, protocol);
    fcntl(fd, F_SETFD, FD_CLOEXEC);
#endif

    if (fd == -1) {
        return -1;
    }

    return fd;	
}

int awoke_socket_server(uint8_t type, const char *addr,
							    uint32_t port, bool reuse_port)
{
	int server_fd;
	socklen_t len;
	struct sockaddr *sk_addr;
	struct sockaddr_in servaddr; 
	sock_map *proto;
	err_type ret = et_ok;
	
	if (!(proto = get_sock_proto(type)))
		return -1;

	server_fd = awoke_socket_create(proto->family, proto->stype, proto->flag);
	if (server_fd < 0) {
		log_err("socket create error");
		return -1;
	}

	if (proto->ltype == SOCK_LOCAL) {
		unlink(addr);
		struct sockaddr_un unix_addr;
		unix_addr.sun_family = AF_LOCAL;
		strncpy(unix_addr.sun_path, addr, sizeof(unix_addr.sun_path));
		unlink(addr);
		sk_addr = (struct sockaddr *)&unix_addr;
		len = sizeof(unix_addr);
	} else if (proto->ltype == SOCK_ETHERNET) {
		return server_fd;
	} else {
		/* TCP or UDP */
	 	memset(&servaddr, 0, sizeof(servaddr));  
		servaddr.sin_family = proto->family; 
		if (!addr) {
			servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		} else {
			inet_aton(addr, &servaddr.sin_addr);
		}
		if (port > 0) {
			servaddr.sin_port = htons(port);
		} else {
			servaddr.sin_port = htons(SOCK_DEF_PORT);
		}
		sk_addr = (struct sockaddr *)&servaddr;
	}

	ret = awoke_socket_bind(server_fd, sk_addr, len, SOMAXCONN);
	if (et_ok != ret) {
		close(server_fd);
		return -1;
	}

	return server_fd;
}

err_type awoke_unix_message_send(char *addr, char *buff, int len, int *rfd, uint32_t flag)
{
	int fd;
	int rc;
	err_type ret;
	
	struct sockaddr_un server_addr;
	
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

	server_addr.sun_family = AF_LOCAL;
	strcpy(server_addr.sun_path, addr);

	rc = connect(fd, 
			 (struct sockaddr *)&server_addr,
			 sizeof(server_addr));
	if (rc < 0) {
		ret = et_sock_conn;
		goto err;
	}

	rc = write(fd, buff, len);
	if (rc < 0) {
		if (errno == EPIPE) {
			return et_sock_conn;
		} else {
			return et_sock_send;
		}
	} else if (rc != len) {
		return et_sock_send;
	}	

	if (mask_exst(flag, UNIX_SOCK_F_WAIT_REPLY)) {
		*rfd = fd;
	} else {
		close(fd);
	}
	return et_ok;
	
err:
	if (fd)
		close(fd);
	return ret;
}

err_type awoke_socket_recv_wait(int fd, uint32_t tm)
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

