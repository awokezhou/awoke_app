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


#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_memory.h"
#include "awoke_list.h"
#include "awoke_macros.h"
#include "awoke_log.h"
#include "awoke_vector.h"
#include "awoke_string.h"

typedef struct _listener {
	uint16_t port;
	char *name;
	awoke_list _head;
}listener;

int listener_reg(const uint16_t port, const char *name, awoke_list *list)
{
	listener *e;
	listener *n;

	list_for_each_entry(e, list, _head) {
		if (!strcmp(e->name, name))
			return 0;
	}

	n = mem_alloc_z(sizeof(listener));
	if (!n)
		return 0;

	n->port = port;
	n->name = awoke_string_dup(name);
	list_append(&n->_head, list);
	return 0;
}

int main(int argc, char **argv) 
{
	listener *e;
	listener *t;
	
	awoke_list test_list;
	list_init(&test_list);

	listener_reg(22, "ssh", &test_list);
	listener_reg(23, "ftp", &test_list);
	listener_reg(80, "http", &test_list);

	list_for_each_entry(e, &test_list, _head) {
		log_debug("port %d, name %s", e->port, e->name);
	}

	listener *first = list_entry_first(&test_list, listener, _head);
	log_debug("first port %d, name %s", first->port, first->name);

	listener *last = dlist_entry_last(&test_list, listener, _head);
	log_debug("last port %d, name %s", last->port, last->name);

	return 0;
}
