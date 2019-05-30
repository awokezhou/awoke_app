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

void sn_test()
{
	FILE *fp;

	fp = fopen("mifiinfo", "r");
	if (!fp) {
		log_err("file not exist");
		return;
	}

	while (!feof(fp)) {
		char line[1024];
		char serial_no[128];
		fgets(line, 1024, fp);
		if (strstr(line, "<serial_no>")) {
			sscanf(line, "%*[^>]>%[^<]", serial_no);
			log_debug("serial_no %s", serial_no);
		}
	}

	fclose(fp);
	return;
}

void snr_none(char *snr, int length)
{
	int i;
	int len = 0;
	int max_len = length;

	len += snprintf(snr, max_len, "50,50");

	for (i=0; i<49; i++) {
		len += snprintf(&(snr[len]), max_len-len, ",50,50");
	}

	return;
}

void snr_none_test()
{
	char snr[1024] = {0x0};

	snr_none(snr, 1024);

	log_debug("snr : %s, size %d", snr, strlen(snr));

	return;
}

typedef struct _fifo {
	int a;
	int b;
	uint8_t flag;
#define FIFO_F_NONE	0x00
#define FIFO_F_PUSH	0x01
#define FIFO_SIZE 	8
}fifo;
fifo xfifo[FIFO_SIZE];
fifo *pfifo;

#define fifo_foreach(block, size, c) 				\
		int __i;									\
		c = &block[0];								\
													\
		for (__i = 0;								\
			__i < size;								\
			__i++, 									\
			c = &block[__i])						\		


void fifo_init()
{
	fifo *p;
	
	fifo_foreach(xfifo, FIFO_SIZE, p) {
		p->a = 0;
		p->b = 0;
		p->flag = FIFO_F_NONE;
	}
	
	pfifo = xfifo;
}

bool fifo_full()
{
	return ((pfifo == &xfifo[FIFO_SIZE-1]) && (pfifo->flag == FIFO_F_PUSH));
}

bool fifo_end()
{
	return (pfifo == &xfifo[FIFO_SIZE-1]);
}

fifo *fifo_curr()
{
	return pfifo;
}

bool _fifo_curr_move()
{
	pfifo++;
}

bool fifo_curr_move()
{
	if (fifo_end()) {
		log_debug("fifo end");
		return FALSE;
	}
	else
		_fifo_curr_move();
	return TRUE;
}

void fifo_dump()
{
	fifo *p;
	log_debug("fifo dump -------->");
	fifo_foreach(xfifo, FIFO_SIZE, p) {
		log_debug("a %d, b %d", p->a, p->b);
	}
	log_debug("fifo dump <--------\n");
}

void fifo_push(int a, int b)
{
	fifo *curr;
	
	if (fifo_full()) {
		log_debug("fifo full");
		fifo_init();
	}

	curr = fifo_curr();
	curr->a = a;
	curr->b = b;
	curr->flag = FIFO_F_PUSH;
	fifo_curr_move();
	log_debug("push %d %d ok", a, b);
}

void fifo_test()
{
	fifo_init();
	fifo_dump();
	fifo_push(1, 2);
	fifo_push(3, 4);
	fifo_dump();
	fifo_push(5, 6);
	fifo_push(7, 8);
	fifo_dump();
	fifo_push(9, 10);
	fifo_push(11, 12);
	fifo_dump();
	fifo_push(13, 14);
	fifo_push(15, 16);
	fifo_dump();
	fifo_push(17, 18);
	fifo_push(19, 20);
	fifo_dump();
	fifo_push(21, 22);
	fifo_push(23, 24);
	fifo_push(25, 26);
	fifo_push(27, 28);
	fifo_push(29, 30);
	fifo_push(31, 32);
	fifo_dump();
	fifo_push(33, 34);
	fifo_push(35, 36);
	fifo_dump();
}

void int_test()
{
	double k = 2;
	int a = 6;
	int b = 10;
	log_debug("a %d b %d", a*(1-k), b*k);
}

#define pkt_dump(data, len) do {\
		int __i;\
		for (__i=0; __i<len; __i++) {\
			if (!(__i%16)) {\
				printf("\n");\
			}\
			printf("0x%2x ", data[__i]);\
		}\
		printf("\n");\
	}while(0)

#define _pkt_push(data, p, bytes) do {\
		memcpy(p, (char *)&data, bytes);\
		p+= bytes;\
	}while(0)
#define pkt_push_byte(data, p) 		_pkt_push(data, p, 1)
#define pkt_push_word(data, p) 		_pkt_push(data, p, 2)
#define pkt_push_dword(data, p) 	_pkt_push(data, p, 4)

#define _pkt_push_safe(data, p, bytes, end) do {\
		if ((end - p) >= bytes)\
			memcpy(p, (char *)&data, bytes);\
		p+= bytes;\
	}while(0)
#define pkt_push_byte_safe(data, p, end) 	_pkt_push_safe(data, p, 1, end)
#define pkt_push_word_safe(data, p, end) 	_pkt_push_safe(data, p, 2, end)
#define pkt_push_dword_safe(data, p, end) 	_pkt_push_safe(data, p, 4, end)

void pkg_test()
{
	uint8_t packet[12];

	char *head = packet;
	char *end = &packet[12-1];
	char *p = head;

	uint8_t type = 0x25;
	uint16_t port = 0x8837;
	int32_t time = 0x13324525;
	int32_t time2 = 0x89989656;
	int32_t time3 = 0x77657567;
	
	pkt_push_byte_safe(type, p ,end);
	pkt_push_word_safe(port, p ,end);
	pkt_push_dword_safe(time, p ,end);
	pkt_push_dword_safe(time2, p ,end);
	pkt_dump(packet, p-head);
	log_debug("packet len %d", p-head);
}

static int16_t alarm_create(uint8_t status, uint8_t type)
{
	return (status << type);
}

void uint16_test()
{
	uint8_t alarm_type = alarm_create(0x1, 0x05);
	log_debug("%d", alarm_type);
}

void pgd_test()
{
	#define pgd_index(addr)		((addr) >> 21)
	
}

void sensor_threshold_set()
{
	uint16_t thd = 1025;

	uint8_t thds;

	thds = thd >> 4;

	log_debug("thds %d", thds);
}

void undef_test()
{
	typedef struct _report {
	#define F_ACK	0x0001
	#define F_PUSH	0x0002
		uint32_t flag;
		uint8_t buf[1024];
	#undef F_ACK
	#undef F_PUSH
	}report;

	report rp;
}

typedef struct _test_data {
	char *name;
	char buff[1024];
	int data;
}test_data;

test_data data_list[] = {
	{"abc", {0x0}, 5},
	{"adc", {0x0}, 5},
};

void bt_name_set_test()
{
	log_debug("data name %s", data_list[0].name);
	data_list[0].data = 25;

	char data[128];
	char *line = awoke_string_dup("AT+SBTNAME=12353");

	log_debug("bt_name_set_test in");

	if (NULL != strstr(line, "AT+SBTNAME")) {
		log_debug("find set command, %s", line);
		char *dt = strtok(line, "=");
		if (dt) {
			char *dt = strtok(NULL, "=");
			log_debug("data %s", dt);
			memcpy(data, dt, strlen(dt));
			log_debug("data %s", data);
		} else {
			log_err("strtok error");
		}		
	}

	if (line)
		free(line);
}

void timestamp_json_test()
{
	char buff[256];
	time_t now;
	long ts;
	ts = time(&now);

	sprintf(buff, "%ld", ts);
	log_debug("buff %s, ts %ld", buff, ts);
}

void uint16_set()
{
	uint16_t shork = 12880;
	uint8_t value = 0;

	shork = (shork-10000);

	log_debug("shork %d", shork);

	
	value = shork >> 4;
	log_debug("value %d", value);	
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

	//sn_test();

	//snr_none_test();

	//fifo_test();

	//int_test();

	//pkg_test();

	//uint16_test();

	//pgd_test();

	//sensor_threshold_set();

	//undef_test();

	//bt_name_set_test();

	//timestamp_json_test();

	uint16_set();
	
	return 0;
}

