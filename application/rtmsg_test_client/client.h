#ifndef __CLIENT_H__
#define __CLIENT_H__


#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <sys/wait.h>  
#include <semaphore.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/mman.h>
#include <sys/sem.h>

#include "awoke_rtmsg.h"
#include "awoke_event.h"
#include "awoke_list.h"
#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_log.h"
#include "awoke_socket.h"
#include "awoke_macros.h"




#endif /* __CLIENT_H__ */
