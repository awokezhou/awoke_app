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

	fd = open("vector_file", O_WRONLY|O_CREAT, S_IRWXU|S_IRUSR|S_IXUSR|S_IROTH|S_IXOTH);
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
	int client_fd = -1;
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

void _fifo_curr_move()
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
#define pkt_push_dwrd_safe(data, p, end) 	_pkt_push_safe(data, p, 4, end)

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
	pkt_push_dwrd_safe(time, p ,end);
	pkt_push_dwrd_safe(time2, p ,end);
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

#define PT_HEADER_LEN	21
#define IOT_HEADER_LEN	4

typedef struct _pt_header {
    uint8_t type;
    uint8_t index;
    uint8_t ver;
    uint8_t dev;
    uint8_t manu;
    uint8_t initiator;
    uint8_t opt;
    bool needAck;
    uint8_t encrypt;
    uint16_t payloadlen;
    uint32_t time;
	uint8_t imei[8];

} pt_header;

typedef struct _iot_header {
    uint16_t id;
    uint16_t len;
} iot_header;

void xxx_pt_header(pt_header *hdr, int priv_len)
{
	uint8_t imei[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	hdr->type = 0x10;
	hdr->index = 1;
	hdr->ver = 0x0;
	hdr->dev = 0x02;
	hdr->encrypt = 0x0;
	hdr->manu = 0x01;
	hdr->initiator = 0;
	hdr->opt = 0x3;
	hdr->needAck = FALSE;
	hdr->payloadlen = priv_len;
	hdr->time = 1560390134;
	memcpy(hdr->imei, imei, 8);
}

int xxx_ptl_encode_pt_priv_data(uint8_t *buf)
{
	int i;
	uint8_t *pos = buf;
	uint8_t data = 0x25;

	uint16_t msg_id = 0x0600;
	//msg_id = htons(msg_id);
	pkt_push_word(msg_id, pos);

	for (i=0; i<128; i++) {
		pkt_push_byte(data, pos);
	}

	return (pos-buf);
}

void xxx_iot_header(iot_header *hdr, int priv_len)
{
	hdr->id = 0x43;
	hdr->len = priv_len;
}

void package_sample_header(uint8_t *buf, int offset, int priv_len,
								    void (*priv_pt_header)(pt_header *, int))
{
	pt_header header;
	uint8_t *p = buf + offset;
	
	priv_pt_header(&header, priv_len);
	//pt_header_encode(&header, p, PT_HEADER_LEN);
	return;
}

void package_iot_header(uint8_t *buf, int offset, int priv_len, 
							    void (*priv_iot_header)(iot_header *, int))
{
	iot_header header;
	uint8_t *p = buf + offset;
	
	priv_iot_header(&header, priv_len);
	//iot_header_encode(&header, p, IOT_HEADER_LEN);
}

int package_priv_data(uint8_t *buf, int len, int (*encode_fn)(uint8_t *))
{
	uint8_t *p = buf + len;
	return encode_fn(p);
}

void package_test()
{
	uint8_t original_buf[512];
	uint32_t priv_len;

	priv_len = package_priv_data(original_buf, IOT_HEADER_LEN+PT_HEADER_LEN, 
		xxx_ptl_encode_pt_priv_data);
	log_debug("priv_len %d", priv_len);

	package_iot_header(original_buf, 0, priv_len+PT_HEADER_LEN, xxx_iot_header);
	log_debug("package_iot_header");
	
	package_sample_header(original_buf, IOT_HEADER_LEN, priv_len, xxx_pt_header);
	log_debug("package_sample_header");

	int dump_len = priv_len+PT_HEADER_LEN+IOT_HEADER_LEN;
	pkt_dump(original_buf, dump_len);
}

typedef enum {
	ss_none = 0,
	ss_i2c,
	ss_uart
} ss_type;

typedef struct _ss_desc {
	ss_type type;
	int try_max;
	err_type (*init)();
	err_type (*read)(uint32_t *);
} ss_desc;

static err_type ss_i2c_init()
{
	log_debug("i2c init");
	return et_ok;
}

static err_type ss_i2c_read(uint32_t *data)
{
	log_debug("i2c read in");
	return et_fail;
}

static err_type ss_uart_init()
{
	log_debug("uart init in");
	return et_ok;
}

static err_type ss_uart_read(uint32_t *data)
{
	log_debug("uart read in");
	return et_ok;
}

struct _ss_desc i2c_desc = {
	.type = ss_i2c,
	.try_max = 5,
	.init = ss_i2c_init,
	.read = ss_i2c_read,
};

struct _ss_desc uart_desc = {
	.type = ss_uart,
	.try_max = 1,
	.init = ss_uart_init,
	.read = ss_uart_read,
};

static struct _ss_desc *g_desc;

static err_type ss_read()
{
	uint32_t data;
	return g_desc->read(&data);
}

static void ss_desc_set(struct _ss_desc *desc)
{
	log_debug("ss mode change to %d", desc->type);
	g_desc = desc;
}

static err_type ss_try_read()
{
	int i;
	err_type ret;

	log_debug("ss try read");
	
	for (i=0; i<g_desc->try_max; i++) {
		ret = ss_read();
		if (ret == et_ok) {
			log_debug("ss try read ok");
			return et_ok;
		}
	}

	log_debug("ss try read error");
	return et_fail;
}

static void ss_init()
{
	log_debug("ss init");
	ss_desc_set(&i2c_desc);

	g_desc->init();

	if (g_desc->type == ss_i2c)
		log_debug("ss type i2c");
	else if (g_desc->type == ss_uart)
		log_debug("ss type uart");
	
	if (ss_try_read() != et_ok) {
		ss_desc_set(&uart_desc);
	}
}

void sensors_support()
{	
	int i;
	err_type ret;
	
	ss_init();
	
	for (i=0; i<10; i++) {
		ret = ss_read();
		if (ret != et_ok) {
			log_debug("ss read error");
		}
	}
}

uint32_t __create_flags(int status)
{
	return (uint32_t)status;
}

#define TCP_SERVER	1
#define TCP_CLIENT	0
enum {
	TCP_NONE,
	TCP_CONNECT,
	TCP_ACK,
	TCP_ERR,
} STATUS;
#define tcp_clientflags(status)	\
	((TCP_CLIENT << 8) | __create_flags(status))

#define tcp_flagsstatus_set(flags, status) do {\
		flags &= 0xf0;\
		flags |= status;\
	} while(0)
	
#define tcp_flagsstatus(flags) ((flags) & 0x0f)

void flags_test()
{
	uint32_t flags;
	/*
	 * | server/client | ack | keep-alive | non-block | 
	 */
	flags = tcp_clientflags(TCP_NONE);
	log_debug("flags:0x%x", flags);
	tcp_flagsstatus_set(flags, TCP_CONNECT);
	uint8_t status = tcp_flagsstatus(flags);
	log_debug("status:%d", status);
}

int _smp_push(mem_ptr_t *ptr, int max, const char *fmt, ...)
{
	int len;
	va_list ap;
	max-= ptr->len;
	
	va_start(ap, fmt);
	len = vsnprintf(ptr->p, max, fmt, ap);
	max -= len;
	va_end(ap);

	ptr->len += len;
	ptr->p += len;

	log_debug("len:%d", len);
	
	return len;	
}

int smp_push_string(mem_ptr_t *ptr, char *string, int max)
{
	return _smp_push(ptr, max, "%s", string);
}

int smp_push_int(mem_ptr_t *ptr, int data, int max)
{
	return _smp_push(ptr, max, "%d", data);
}

int smp_push_hex(mem_ptr_t *ptr, uint32_t data, int max)
{
	return _smp_push(ptr, max, "%x", data);
}

build_ptr smp_ptr_iot_header(int len)
{
	char header[32];
	
	build_ptr bp = build_ptr_init(header, 32);

	build_ptr_hex(bp, 5055);
	build_ptr_number(bp, len);

	return bp;
}

build_ptr smp_ptr_pt_header(int len)
{
	char header[32];
	
	build_ptr bp = build_ptr_init(header, 32);

	build_ptr_hex(bp, 0x4055);
	build_ptr_number(bp, len);

	return bp;
}

build_ptr smp_ptr_data()
{
	char data[512];

	build_ptr bp = build_ptr_init(data, 512);

	build_ptr_number(bp, 22);
	build_ptr_hex(bp, 0x55313131);
	build_ptr_number(bp, 1531341425);
	build_ptr_number(bp, 22);
	build_ptr_number(bp, 22);
	build_ptr_number(bp, 22);

	return bp;
}

void smp_ptr_test()
{
	build_ptr data = smp_ptr_data();
	build_ptr pt_header = smp_ptr_pt_header(data.len);
	build_ptr iot_header = smp_ptr_iot_header(data.len+pt_header.len);

	log_debug("data:%*.s", data.len, data.p);
	log_debug("pt:%*.s", pt_header.len, pt_header.p);
	log_debug("iot:%*.s", iot_header.len, iot_header.p);
}

void address_type_test()
{	
	int len;
	char *host = "www.baidu.com";
	unsigned int a, b, c, d, port = 0;
	
	if (sscanf(host, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
		log_debug("ip address");
	} else {
		log_debug("hostname address");
	}
}


typedef struct _struct_t {
	int nr;
#define STATUS_MAX_NR 	3
	uint8_t status[STATUS_MAX_NR];
} status_t;

static void status_push(status_t *p, uint8_t s)
{
	int i;
	int offset = p->nr;

	if (offset == STATUS_MAX_NR) offset = 0;

	for (i=STATUS_MAX_NR-1; i>=1; i--) {
		p->status[i] = p->status[i-1];
	}
	p->status[0] = s;
}

void status_test()
{

	status_t status;
	uint8_t s1, s2, s3;

	s1 = 0x01;
	s2 = 0x02;
	s3 = 0x03;

	memset(&status, 0x0, sizeof(status_t));

	status_push(&status, s1);
	log_debug("status 0x%x 0x%x 0x%x", status.status[2], status.status[1], status.status[0]);
	status_push(&status, s2);
	log_debug("status 0x%x 0x%x 0x%x", status.status[2], status.status[1], status.status[0]);
	status_push(&status, s3);
	log_debug("status 0x%x 0x%x 0x%x", status.status[2], status.status[1], status.status[0]);
	status_push(&status, s2);
	log_debug("status 0x%x 0x%x 0x%x", status.status[2], status.status[1], status.status[0]);
	status_push(&status, s1);
	log_debug("status 0x%x 0x%x 0x%x", status.status[2], status.status[1], status.status[0]);
	status_push(&status, 0x23);
	log_debug("status 0x%x 0x%x 0x%x", status.status[2], status.status[1], status.status[0]);
	status_push(&status, 0x73);
	log_debug("status 0x%x 0x%x 0x%x", status.status[2], status.status[1], status.status[0]);
	status_push(&status, 0x74);
	log_debug("status 0x%x 0x%x 0x%x", status.status[2], status.status[1], status.status[0]);
	status_push(&status, 0x41);
	log_debug("status 0x%x 0x%x 0x%x", status.status[2], status.status[1], status.status[0]);
	status_push(&status, 0xad);
	log_debug("status 0x%x 0x%x 0x%x", status.status[2], status.status[1], status.status[0]);
}

void ipaddr_test()
{
	struct sockaddr_in sock_addr;
	uint8_t ipaddr[4];
	char ipaddr_str[16];

	sprintf(ipaddr_str, "8.8.8.8");
	log_debug("ipaddr_str %s", ipaddr_str);
	sock_addr.sin_addr.s_addr = inet_addr(ipaddr_str);
	log_debug("sock_addr %s", inet_ntoa(sock_addr.sin_addr));
	memcpy(ipaddr, &sock_addr.sin_addr, 4);
	log_debug("ipaddr %d.%d.%d.%d", ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
}

void awoke_list_test()
{
	typedef struct _lnode {
		int x;
		awoke_list _head;
	} lnode;

	awoke_list xlist;
	lnode *p;
	lnode *f;
	lnode *new1 = mem_alloc_z(sizeof(lnode));

	list_init(&xlist);

	new1->x = 2;
	list_prepend(&new1->_head, &xlist);

	lnode *new2 = mem_alloc_z(sizeof(lnode));
	new2->x = 3;
	list_prepend(&new2->_head, &xlist);

	list_for_each_entry(p, &xlist, _head) {
		log_debug("x %d", p->x);
	}

	list_for_each_entry_safe(p, f, &xlist, _head) {
		list_unlink(&p->_head);
		memset(p, 0x0, sizeof(lnode));
		//free(p);
		break;
	}	

	list_for_each_entry(p, &xlist, _head) {
		log_debug("x %d", p->x);
	}	
}

static int ctoi(char c)
{
    int i;

    i = (int)c - '0';
    if (i < 0 || i > 9)
    {
        i = 0;
    }
    return i;
}

int string_convert_BCD_to_str(char *out, const uint8_t *in, uint16_t b_len)
{
	int i;
    int out_len = 0;

    if (out == NULL || in == NULL || b_len == 0)
    {
        return -1;
    }

    for (i = 0; i < b_len/2; i++)
    {
        out_len += sprintf(out + out_len, "%u", (uint8_t)(in[i] & 0x0F));        // lower nibble
        out_len += sprintf(out + out_len, "%u", (uint8_t)(in[i] & 0xF0) >> 4);   // upper nibbl
    }
    // If it is an odd number add one digit more
    if ((b_len & 0x1) != 0)
    {
        out_len += sprintf(out + out_len, "%u", (uint8_t)(in[b_len/2] & 0x0F));  // lower nibble
    }
    out[out_len] = '\0'; // It is a string so add the null terminated character
    return out_len;
}

int string_convert_str_to_BCD(uint8_t *bin_data, const char *str_data, uint16_t bin_len)
{
	int i;
    int out_len;

    uint8_t digit1;
    uint8_t digit2;

    if (bin_data == NULL || str_data == NULL || bin_len == 0)
    {
        return -1;
    }

    for ( out_len = 0; out_len +1 < bin_len; out_len += 2)       //lint !e440  False positive complaint about bit_len not being modified.
    {
        if ((str_data[out_len] < '0') || (str_data[out_len] > '9') || (str_data[out_len + 1] < '0') || (str_data[out_len + 1] > '9'))
        {
            return -1;
        }

        digit1 = (uint8_t)ctoi(str_data[out_len]);
        digit2 = (uint8_t)ctoi(str_data[out_len + 1]);
        bin_data[out_len/2] = digit1 | ((digit2 << 4) & 0xFF);
    }
    // If it is an odd number add the last digit and a 0
    if ((bin_len & 0x1) != 0)
    {
        if ((str_data[bin_len - 1] < '0') || (str_data[bin_len - 1] > '9'))
        {
            return -1;
        }

        bin_data[bin_len/2] = (uint8_t)ctoi(str_data[bin_len - 1]); // just digit 1, digit2 = 0
    }
    return bin_len;
}

#include "awoke_package.h"

void bcd_test()
{
	
	uint8_t hex[8] = {0x68, 0x9, 0x6, 0x30, 0x20, 0x0, 0x62, 0x9};
	//uint8_t hex[16];
	char str[] = "869060030200269";
	char _str[32];
	string_convert_BCD_to_str(_str, hex, 15);
	//string_convert_str_to_BCD(hex, str, 15);
	log_debug("_str %s", _str);
	//pkg_dump(hex, 8);
}

void string_to_hex_test()
{
	char *str = "1223222222222222222";
	uint8_t hex[64] = {0x0};

	awoke_string_to_hex(str, hex, 20);
	pkg_dump(hex, 10);

	//uint8_t hex[8] = {0x23, 0x34, 0x45, 0x56, 0x67, 0x78, 0x89};
	//awoke_string_from_hex(hex, str, 8);
	//log_debug("str %s", str);
}

typedef enum {
	tx_alert = 1,
	tx_gps,
	tx_wifi,
	tx_cell,
} tx_type;

typedef struct _tx_unit {
	tx_type type;
	uint32_t flag;
	void *data;
	int data_len;
} tx_unit;

typedef struct _tx_queue {
	int curr;
#define QUEUE_F_RB	0x0002
	uint16_t flag;
#define TX_QUEUE_SIZE	8
	tx_unit _queue[TX_QUEUE_SIZE];
} tx_queue;


#define tx_queue_foreach(q, u)						\
		int __k;									\
		u = &q->_queue[q->curr];					\
													\
		for (__k = q->curr;							\
		 	(__k>=0);								\
		 	__k--,									\
		 	u = &q->_queue[__k])					\


bool tx_queue_empty(tx_queue *q)
{
	return ((q->curr+1) == 0);
}

bool tx_queue_full(tx_queue *q)
{
	return ((q->curr+1) == TX_QUEUE_SIZE);
}

void tx_queue_dq(tx_queue *q, tx_unit *u)
{
	int i;
	
	if (tx_queue_empty(q)) {
		memset(u, 0x0, sizeof(tx_unit));
		return;
	}

	memcpy(u, &q->_queue[0], sizeof(tx_unit));
	
	for (i=0; i<q->curr; i++) {
		q->_queue[i] = q->_queue[i+1];
	}
	memset(&q->_queue[q->curr], 0x0, sizeof(tx_unit));
	q->curr--;
	return;
}

void tx_queue_eq(tx_queue *q, tx_unit *u)
{
	if (tx_queue_full(q)) {
		if (mask_exst(q->flag, QUEUE_F_RB)) {
			tx_unit _front;
			tx_queue_dq(q, &_front);
		} else {
			return;
		}
	}
	
	q->curr = (q->curr+1)%TX_QUEUE_SIZE;
	memcpy(&q->_queue[q->curr], u, sizeof(tx_unit));
	return;
}

void tx_queue_init(tx_queue *q, uint16_t flag)
{
	memset(&q->_queue, 0x0, sizeof(tx_queue));

	q->curr = -1;
	mask_push(q->flag, flag);
}

void queue_dump(tx_queue *q)
{
	int max;
	int len;
	int size;
	tx_unit *unit;
	size = q->curr+1;
	char *pos;
	char dump[256] = {'\0'};
	
	if (!size)
		return;

	len = 0;	
	max = 256;
	pos = dump;
	
	tx_queue_foreach(q, unit) {
		len = snprintf(pos, max, "%d ", unit->type);
		pos += len;
		max -= len;
	}
	log_debug("queue size:%d, data: [%s]", size, dump);
	printf("\n");
}

void tx_queue_test()
{
	tx_unit u;
	tx_unit a, b, c, d;
	tx_queue queue;

	tx_queue_init(&queue, QUEUE_F_RB);

	a.type = 1;
	b.type = 2;
	c.type = 3;
	d.type = 4;

	queue_dump(&queue);

	log_debug("eq [3 1 2 2 2 2 2 4 3 2 1] --> ");
	tx_queue_eq(&queue, &a);
	tx_queue_eq(&queue, &b);
	tx_queue_eq(&queue, &c);
	tx_queue_eq(&queue, &d);
	tx_queue_eq(&queue, &b);
	tx_queue_eq(&queue, &b);
	tx_queue_eq(&queue, &b);
	tx_queue_eq(&queue, &b);
	tx_queue_eq(&queue, &b);
	tx_queue_eq(&queue, &a);
	tx_queue_eq(&queue, &c);
	
	queue_dump(&queue);
	
	tx_queue_dq(&queue, &u);
	log_debug("dq --> [%d] ", u.type);
	tx_queue_dq(&queue, &u);
	log_debug("dq --> [%d] ", u.type);
	tx_queue_dq(&queue, &u);
	log_debug("dq --> [%d] ", u.type);
	tx_queue_dq(&queue, &u);
	log_debug("dq --> [%d] ", u.type);
	tx_queue_dq(&queue, &u);
	log_debug("dq --> [%d] ", u.type);
	tx_queue_dq(&queue, &u);
	log_debug("dq --> [%d] ", u.type);
	tx_queue_dq(&queue, &u);
	log_debug("dq --> [%d] ", u.type);
	tx_queue_dq(&queue, &u);
	log_debug("dq --> [%d] ", u.type);
	tx_queue_dq(&queue, &u);
	log_debug("dq --> [%d] ", u.type);

	queue_dump(&queue);

	log_debug("eq [3 1 2 2] --> ");
	tx_queue_eq(&queue, &b);
	tx_queue_eq(&queue, &b);
	tx_queue_eq(&queue, &a);
	tx_queue_eq(&queue, &c);

	queue_dump(&queue);

	tx_queue_dq(&queue, &u);
	log_debug("dq --> [%d] ", u.type);

	queue_dump(&queue);

	log_debug("eq [3] --> ");
	tx_queue_eq(&queue, &c);

	queue_dump(&queue);
	
	tx_queue_dq(&queue, &u);
	log_debug("dq --> [%d] ", u.type);
	tx_queue_dq(&queue, &u);
	log_debug("dq --> [%d] ", u.type);
	tx_queue_dq(&queue, &u);
	log_debug("dq --> [%d] ", u.type);

	queue_dump(&queue);
}

typedef struct _rb_queue {
	int curr;
#define TX_QUEUE_SIZE	8
	tx_unit _queue[TX_QUEUE_SIZE];
} rb_queue;

bool rb_queue_empty(rb_queue *q)
{
	return ((q->curr+1) == 0);
}

bool rb_queue_full(rb_queue *q)
{
	return ((q->curr+1) == TX_QUEUE_SIZE);
}

void rb_queue_init(rb_queue *q)
{
	memset(&q->_queue, 0x0, sizeof(rb_queue));
	q->curr = -1;
}

void rb_queue_dq(rb_queue *q, tx_unit *u)
{
	int i;
	
	if (rb_queue_empty(q)) {
		memset(u, 0x0, sizeof(tx_unit));
		return;
	}

	memcpy(u, &q->_queue[0], sizeof(tx_unit));
	
	for (i=0; i<q->curr; i++) {
		q->_queue[i] = q->_queue[i+1];
	}
	memset(&q->_queue[q->curr], 0x0, sizeof(tx_unit));
	q->curr--;
	return;
}

void rb_queue_eq(rb_queue *q, tx_unit *u)
{	
	if (rb_queue_full(q)) {
		tx_unit _front;
		rb_queue_dq(q, &_front);		
	}

	q->curr = (q->curr+1)%TX_QUEUE_SIZE;
	memcpy(&q->_queue[q->curr], u, sizeof(tx_unit));
}

void rb_queue_test()
{
	rb_queue queue;
	tx_unit u;
	tx_unit a, b, c, d;

	a.type = 1;
	b.type = 2;
	c.type = 3;
	d.type = 4;
	
	rb_queue_init(&queue);

	log_debug("eq [2 3 3 4 3 2 1] --> ");
	rb_queue_eq(&queue, &a);
	rb_queue_eq(&queue, &b);
	rb_queue_eq(&queue, &c);
	rb_queue_eq(&queue, &d);
	rb_queue_eq(&queue, &c);
	rb_queue_eq(&queue, &c);
	rb_queue_eq(&queue, &b);

	queue_dump(&queue);

	log_debug("eq [3 1 1] --> ");
	rb_queue_eq(&queue, &a);
	rb_queue_eq(&queue, &a);
	rb_queue_eq(&queue, &c);

	queue_dump(&queue);

	rb_queue_dq(&queue, &u);
	log_debug("dq [%d] --> ", u.type);

	queue_dump(&queue);

	rb_queue_dq(&queue, &u);
	log_debug("dq [%d] --> ", u.type);
	rb_queue_dq(&queue, &u);
	log_debug("dq [%d] --> ", u.type);
	rb_queue_dq(&queue, &u);
	log_debug("dq [%d] --> ", u.type);
	rb_queue_dq(&queue, &u);
	log_debug("dq [%d] --> ", u.type);
	rb_queue_dq(&queue, &u);
	log_debug("dq [%d] --> ", u.type);
	rb_queue_dq(&queue, &u);
	log_debug("dq [%d] --> ", u.type);

	queue_dump(&queue);

	rb_queue_dq(&queue, &u);
	log_debug("dq [%d] --> ", u.type);
	rb_queue_dq(&queue, &u);
	log_debug("dq [%d] --> ", u.type);
	
	log_debug("dq [%d] --> ", u.type);
}

#include "sec_base64.h"
void base64_test()
{
	size_t len;
	uint8_t buff[128];
	char *str = "QEABASApCQMIdwAAaIAwMJBxFwYAAAAAABQAMAUAAAAAAAEAAAAAAAAAAEwAAK0jIw==";
	
	sec_base64_decode(buff, sizeof(buff), &len, str, strlen(str));

	pkg_dump(buff, len);
}
#include "cJSON.h"
void cjson_test()
{
	int remainder;
	int len;
	char *str = "[http] array:array\":[{\"ack\":1,\"expire_at\":1567933248,\"value\":\"10022B0A010001001D5d722";	
	char *p = str;
	char *str_remainder = NULL;
	char *end;
	
	str_remainder = strstr(p, "HTTP/1.1");
	if (!str_remainder)
		return;
	str_remainder += strlen("HTTP/1.1");
	sscanf(str_remainder, " %d[^ ]", &remainder);
	log_debug("state %d", remainder);
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

	//uint16_set();

	//package_test();

	//sensors_support();

	//flags_test();

	//smp_ptr_test();

	//address_type_test();

	//status_test();

	//ipaddr_test();

	//awoke_list_test();

	//signature_test();

	//bcd_test();

	string_to_hex_test();

	//tx_queue_test();

	//rb_queue_test();

	//base64_test();

	//cjson_test();

	return 0;
}

