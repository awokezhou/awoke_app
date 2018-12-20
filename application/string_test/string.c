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
#include "awoke_socket.h"
#include "awoke_event.h"

#include "zw_quaternion.h"

#define STR_ALARM_A	"43"
#define STR_ALARM_B	"sensor_horizontal_adjust"
#define T_ALARM_A 8
#define T_ALARM_B 12

int get_alarm_type(char *buff)
{
	char s[]="sensor_horizontal_adjust#";
	char *delim="#";
	char *type;
	char *data;
	
	type = strtok(s, delim);
	printf("type %s\n", type);
	data = strtok(NULL, delim);
	printf("data %s\n", data);

	if (!strcmp(STR_ALARM_A, type))
		return T_ALARM_A;
	else if (!strcmp(STR_ALARM_B, type))
		return T_ALARM_B;
	return 0;
}

err_type vector_file_read(vector *vec)
{
	int fd;
	int len;
	char buff[1024] = {0x0};
	
	fd = open("vector_file", O_RDONLY);
	if (fd < 0) {
		log_err("open file error");
		return et_file_open;
	}

	len = read(fd, buff, sizeof(buff));
	if (len < 0) {
		close(fd);
		return et_file_read;
	}

	sscanf(buff, "%lf,%lf,%lf", &vec->x, &vec->y, &vec->z);
	log_debug("buff %lf, %lf, %lf", vec->x, vec->y, vec->z);
	close(fd);
	return et_ok;	
}

unsigned char u8WaitCnt = 0;

void uchartest()
{
	uint8_t count;

	while (u8WaitCnt < 12) {
		sleep(5);
		log_debug("count %d", u8WaitCnt);
		u8WaitCnt++;
		log_debug("count %d", u8WaitCnt);
	}
}

#define region_foreach(block, size, c) 				\
		int __i;									\
		c = &block[0];								\
													\
		for (__i = 0;								\
			__i < size;								\
			__i++, 									\
			c = &block[__i])						\		
	

void mem_map_test()
{
	#define PAGE_SHIFT 	12
	#define PAGE_OFFSET 0
	#define PHYS_OFFSET	0xc0000000
	#define	__phys_to_pfn(paddr)	((unsigned long)((paddr) >> PAGE_SHIFT))
	#define __phys_to_virt(x)		((x) - PHYS_OFFSET + PAGE_OFFSET)
	typedef struct _memblock_region {
		uint32_t base;
		uint32_t eng;
	}memblock_region;

	memblock_region regs[] = {
		{0x80000000, 0x829fffff},
		{0x87800000, 0x87bfffff},
		{0x88000000, 0x8fffffff},
	};

	int regs_size = array_size(regs);

	memblock_region *reg;

	region_foreach(regs, regs_size, reg) {
		log_debug("reg base 0x%x, eng 0x%x", reg->base, reg->eng);
		uint32_t pfn = __phys_to_pfn(reg->base);
		uint32_t vir = __phys_to_virt(reg->base);
		uint32_t len = reg->eng - reg->base;
		log_debug("pfn 0x%x, vir 0x%x, len %d", pfn, vir, len);
	}
}

err_type vector_file_write(vector *vec)
{
	int fd;
	int len;
	char buff[1024] = {"\n"};

	fd = open("vector_file", O_WRONLY|O_CREAT);
	if (fd < 0) {
		log_err("open file error");
		return et_file_open;
	}

	sprintf(buff, "%lf, %lf, %lf", vec->x, vec->y, vec->z);
	log_debug("buff %s", buff);
	len = write(fd, buff, 1024);
	if (len < 0) {
		log_err("write file error");
		return et_file_send;
	}

	close(fd);
	return et_ok;
}

void event_test()
{
	log_debug("event_test in");
	err_type ret = et_ok;
	awoke_event *event;
	awoke_event_loop *evl;
	int client_fd;
	awoke_event listener;
	awoke_event conn;

	int fd = awoke_socket_server(SOCK_INTERNET_TCP, "172.16.79.132", 8837, FALSE);
	if (fd < 0) {
		log_debug("socket server create error");
		return;
	}
	log_debug("socket server create ok, %d %d ", fd, SOCK_INTERNET_TCP);

	evl = awoke_event_loop_create(EVENT_QUEUE_SIZE);
    if (!evl)
    {
        log_err("event loop create error");
        return et_evl_create;
    }
	log_debug("event loop create ok");

    awoke_event_add(evl, fd, EVENT_LISTENER, EVENT_READ, &listener);
	log_debug("event add ok");


	while (1) {
		char buff[1024];
		awoke_event_wait(evl, 1000);
		awoke_event_foreach(event, evl) {
			
			if (!awoke_event_ready(event, evl))
                continue;

			if (event->type == EVENT_LISTENER) {
				log_debug("EVENT_LISTENER");
				client_fd = awoke_socket_accpet(fd);
				ret = awoke_event_add(evl, client_fd, EVENT_CONNECTION, EVENT_READ, &conn);
				if (awoke_unlikely(et_ok != ret)) {
					log_err("event add error, ret %d", ret);
					goto err;
				}
			} else if (event->type == EVENT_CONNECTION) {
				log_debug("EVENT_CONNECTION");
				int rc = recv(client_fd, buff, 1024, 0);
				log_debug("buff %s, size %d", buff, rc);
			} else if (event->type == EVENT_NOTIFICATION) {
				/**/
			} else if (event->type == EVENT_CUSTOM) {
				/**/
			}
		}
	}

err:
	close(client_fd);
	close(fd);
	return;
}

void kernel_modules_test()
{
	FILE *fp;
	char line[1024];	

	fp = fopen("/proc/modules", "r");
	if (!fp) {
		log_err("open modules file error");
		return;	
	}

	while (!feof(fp)) {
		fgets(line, 1024, fp);
		log_debug("line: %s", line);
	}

	fclose(fp);
	return;
}

err_type sgd_dot(int j[][3], int *d, int *g)
{
	g[0] = j[0][0]*d[0] + j[0][1]*d[1] + j[0][2]*d[2]; 
	g[1] = j[1][0]*d[0] + j[1][1]*d[1] + j[1][2]*d[2]; 
	g[2] = j[2][0]*d[0] + j[2][1]*d[1] + j[2][2]*d[2]; 
	g[3] = j[3][0]*d[0] + j[3][1]*d[1] + j[3][2]*d[2]; 
	return et_ok;
}

void mat_transpose()
{
	int i, j;
	int mat_a[3][4] = {{1,2,3,4}, {5,6,7,8}, {9,10,11,12}};
	int mat_b[4][3] = {0x0};
	int mat_d[3] = {1,2,3};	
	int mat_x[4] = {0x0};

	printf("[");
	for (i=0; i<3; i++) {
		printf("[");	
		for (j=0; j<4; j++) {
			printf("%d ", mat_a[i][j]);
		}
		printf("]\n");
	}
	printf("]\n");

	for (i=0; i<3; i++) {
		for (j=0; j<4; j++) {
			mat_b[j][i] = mat_a[i][j];
		}
	}

	printf("[");
	for (i=0; i<4; i++) {
		printf("[");	
		for (j=0; j<3; j++) {
			printf("%d ", mat_b[i][j]);
		}
		printf("]\n");
	}
	printf("]\n");

	sgd_dot(mat_b, mat_d, mat_x);

}

void list_dot(double a[][2], double *b, double *c)
{
	c[0] = a[0][0]*b[0] + a[0][1]*b[1];
	c[1] = a[1][0]*b[0] + a[1][1]*b[1];
}

int main(int argc, char **argv)
{
	log_level(LOG_DBG);
	log_mode(LOG_TEST);

	//uchartest();

	//mem_map_test();

	//event_test();

	//kernel_modules_test();

	//mat_transpose();
	
	return 0;
}

