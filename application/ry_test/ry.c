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

#include "ry.h"

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
#include "awoke_package.h"


static int ry_queue_size(ry_queue *q);



ry_store *g_store = NULL;

static void ry_queue_dump(ry_queue *q)
{
	int max;
	int len;
	int size;
	ry_dataunit *du;
	char *pos;
	char dump[256] = {'\0'};

	size = ry_queue_size(q);
	if (!size)
		return;

	len = 0;	
	max = 256;
	pos = dump;

	ry_queue_foreach(q, du) {
		len = snprintf(pos, max, "%d ", du->pressure);
		pos += len;
		max -= len;
	}
	log_debug("queue size:%d, data: [%s]", size, dump);
}

static bool ry_queue_empty(ry_queue *q)
{
	return ((q->curr+1) == 0);
}

static bool ry_queue_full(ry_queue *q)
{
	return ((q->curr+1) == RY_QUEUE_SIZE);
}

static int ry_queue_size(ry_queue *q)
{
	return (q->curr+1);
}

static err_type ry_queue_deq(ry_queue *q, ry_dataunit *u)
{
	int i;
	
	if (ry_queue_empty(q)) {
		memset(u, 0x0, sizeof(ry_dataunit));
		return et_fail;
	}

	memcpy(u, &q->_queue[0], sizeof(ry_dataunit));
	
	for (i=0; i<q->curr; i++) {
		q->_queue[i] = q->_queue[i+1];
	}
	memset(&q->_queue[q->curr], 0x0, sizeof(ry_dataunit));
	q->curr--;
	return et_ok;
}

static err_type ry_queue_enq(ry_queue *q, ry_dataunit *u)
{
	if (ry_queue_full(q)) {
		if (mask_exst(q->flag, RY_QUEUE_F_RB)) {
			ry_dataunit _front;
			ry_queue_deq(q, &_front);
		} else {
			return et_fail;
		}
	}

	q->curr = (q->curr+1)%RY_QUEUE_SIZE;
	memcpy(&q->_queue[q->curr], u, sizeof(ry_dataunit));
	return et_ok;
}

static void ry_queue_init(ry_queue *q, uint16_t flag)
{
	memset(&q->_queue, 0x0, sizeof(ry_dataunit));
	q->curr = -1;
	mask_push(q->flag, flag);
}

static err_type ry_store_push(ry_wpdata data)
{
	ry_dataunit du;

	du.batt_level = 0x1221;
	du.batt_percent = 60;

	du.pressure = data.data;

	memcpy(&du.occur_time, localtime(&data.collecttime), sizeof(struct tm));

	return ry_queue_enq(&g_store->du_queue, &du);
}

static ry_wpdata ry_wpdata_make(time_t time, uint16_t value)
{
	ry_wpdata ret;
	ret.collecttime = time;
	ret.data = value;
	return ret;
}

static uint16_t ry_version(uint8_t v_major, uint8_t v_user)
{
	uint16_t version;

	version = (v_major << 8) | v_user;

	return version;
}

static void ry_datetime_get(ry_msg_header *hdr)
{
	struct tm *date;
	time_t now;
	now = time(NULL);
	date = localtime(&now);
	memcpy(&hdr->date, date, sizeof(struct tm));
	
	log_info("date: %d-%d-%d %d:%d:%d", 
		date->tm_year+1900,
		date->tm_mon+1,
		date->tm_mday,
		date->tm_hour,
		date->tm_min,
		date->tm_sec);
}

static void ry_imei_get(ry_msg_header *hdr)
{
	uint8_t imei[8] = {0x25, 0x13, 0x56, 0x22, 0x33, 0x24, 0x65, 0x38};

	memcpy(hdr->imei, imei, 8);
}

static ry_message *ry_msg_create(uint8_t order_type)
{
	ry_message *msg = NULL;
	
	msg = mem_alloc_z(sizeof(ry_message));
	if (!msg) {
		log_err("nomem");
		return NULL;
	}
	
	msg->header.prefix = RY_PREFIX;
	msg->header.postfix = RY_POSTFIX;

	msg->header.version = ry_version(RY_VER_MAJOR, RY_VER_USER);

	msg->header.manufacturer = 0x1234;
	msg->header.dev_type = 0x2222;
	msg->header.command = 0x0;
	msg->header.f_ack = 0x0;

	ry_datetime_get(&msg->header);

	ry_imei_get(&msg->header);

	msg->dtunit.order_type = order_type;
	msg->dtunit.type_flag = RY_TYPEF_WLPS;
	msg->dtunit.component_code = RY_CCODE;
	
	switch (order_type) {
		case RY_ORDER_RP_MONITOR:
			log_debug("[wpm] monitor report message");
			/* do nothing */
			break;
		case RY_ORDER_RP_ADDRESS:
			log_debug("[wpm] address report message");
			/* reserve */
			break;
		case RY_ORDER_RP_TIMESAPN:
			log_debug("[wpm] time span report message");
			/* reserve */
			break;
		case RY_ORDER_RP_CHECKFREQ:
			log_debug("[wpm] self-checking frequency report message");
			/* reserve */
			break;
		case RY_ORDER_RP_COLLSAPN:
			log_debug("[wpm] collect span report message");
			/* reserve */
			break;
		case RY_ORDER_RP_PRESSLOW:
			log_debug("[wpm] lower pressure limit report message");
			/* reserve */
			break;
		case RY_ORDER_RP_PRESSHIG:
			log_debug("[wpm] upper pressure limit report message");
			/* reserve */
			break;
		default:
			break;
	}		

	return msg;
}

static err_type ry_msg_du_monitor_package(ry_message *msg, uint8_t **ppos, 
		uint8_t *end)
{
	err_type ret;
	uint8_t *head = *ppos;
	uint8_t *pos = head;
	ry_dataunit *m = NULL;
	ry_msg_dtunit *du = &msg->dtunit;
	
	ry_queue_foreach(&(g_store->du_queue), m) {

		log_debug("m->pressure %d", m->pressure);

		// type flag 
		uint16_t type_flag = htons(du->type_flag);
		pkg_push_word_safe(type_flag, pos, end);

		// component code
		uint16_t component_code = htons(du->component_code);
		pkg_push_word_safe(component_code, pos, end);
	
		// battery level
		uint16_t batt_level = htons(m->batt_level);
		pkg_push_word_safe(batt_level, pos, end);

		// rssi percent
		uint16_t rssi_percent = htons(m->rssi_percent);
		pkg_push_word_safe(rssi_percent, pos, end);

		// rssi 
		uint16_t rssi = htons(m->rssi);
		pkg_push_word_safe(rssi, pos, end);	

		// snr
		uint16_t snr = htons(m->snr);
		pkg_push_word_safe(snr, pos, end);	

		// ecl
		pkg_push_byte_safe(m->ecl, pos, end);

		// battery percent
		pkg_push_byte_safe(m->batt_percent, pos, end);	

		// pressure
		uint16_t pressure = htons(m->pressure);
		pkg_push_word_safe(pressure, pos, end);	

		// system state
		pkg_push_byte_safe(m->sys_state, pos, end);	

		// occur time
		pkg_push_byte_safe(m->occur_time.tm_sec, pos, end);
		pkg_push_byte_safe(m->occur_time.tm_min, pos, end);
		pkg_push_byte_safe(m->occur_time.tm_hour, pos, end);
		pkg_push_byte_safe(m->occur_time.tm_mday, pos, end);
		pkg_push_byte_safe(m->occur_time.tm_mon, pos, end);
		pkg_push_byte_safe(m->occur_time.tm_year, pos, end);	
	}

	msg->dtunit_len = 1 + pos - head;
	return et_ok;
}

static err_type ry_msg_header_package(ry_message *msg)
{
	uint8_t *head = msg->original_buf;
	uint8_t *pos = head;
	uint8_t *end = head + 1024;
	ry_msg_header *h = &msg->header;
	
	// prefix
	uint16_t prefix = htons(h->prefix);
	pkg_push_word_safe(prefix, pos, end);	

	// version 
	uint16_t version = htons(h->version);
	pkg_push_word_safe(version, pos, end);

	// time
	pkg_push_byte_safe(h->date.tm_sec, pos, end);
	pkg_push_byte_safe(h->date.tm_min, pos, end);
	pkg_push_byte_safe(h->date.tm_hour, pos, end);
	pkg_push_byte_safe(h->date.tm_mday, pos, end);
	pkg_push_byte_safe(h->date.tm_mon, pos, end);
	pkg_push_byte_safe(h->date.tm_year, pos, end);					

	// manufacturer
	uint16_t manufacturer = htons(h->manufacturer);
	pkg_push_word_safe(manufacturer, pos, end);

	// imei
	pkg_push_bytes_safe(h->imei, pos, end, 8);

	// device type
	uint16_t dev_type = htons(h->dev_type);
	pkg_push_word_safe(dev_type, pos, end);

	// command
	pkg_push_byte_safe(h->command, pos, end);

	// ack flag
	pkg_push_byte_safe(h->f_ack, pos, end);

	// data unit len
	uint16_t dtunit_len = htons(msg->dtunit_len);
	pkg_push_word_safe(dtunit_len, pos, end);
	
	// checksum
	pos += msg->dtunit_len;
	log_debug("now at %d", pos-head);
	h->checksum = 0x99;
	pkg_push_byte_safe(h->checksum, pos, end);

	// postfix
	uint16_t postfix = htons(h->postfix);
	pkg_push_word_safe(postfix, pos, end);

	msg->original_len = pos - head;
	
	return et_ok;
}

static err_type ry_msg_dtunit_package(ry_message *msg)
{
	err_type ret;
	uint8_t *pos = msg->original_buf + RY_DU_OFFSET;
	uint8_t *end = &msg->original_buf[RY_PKT_SIZE-1]; 
	ry_msg_dtunit *du = &msg->dtunit;

	// order type
	pkg_push_byte_safe(du->order_type, pos, end);


	switch (du->order_type) {
		case RY_ORDER_RP_MONITOR:
			ret = ry_msg_du_monitor_package(msg, &pos, end);
			break;
		case RY_ORDER_RP_ADDRESS:
		case RY_ORDER_RP_TIMESAPN:
		case RY_ORDER_RP_CHECKFREQ:
		case RY_ORDER_RP_COLLSAPN:
		case RY_ORDER_RP_PRESSLOW:
		case RY_ORDER_RP_PRESSHIG:
			ret = et_ok;
			break;
		default:
			break;
	}

	log_info("data unit len %d", msg->dtunit_len);

	return ret;
}

err_type ry_construst_msg()
{
	err_type ret;
	ry_message *msg = NULL;

	msg = ry_msg_create(RY_ORDER_RP_MONITOR);
	if (!msg) {
		log_err("ry message create error");
		return et_nomem;
	}

	ret = ry_msg_dtunit_package(msg);
	if (ret != et_ok) {
		log_err("[wpm] ry data unit package error, ret %d", ret);
		goto err;
	}

	ret = ry_msg_header_package(msg);
	if (ret != et_ok) {
		log_err("[wpm] ry header package error, ret %d", ret);
		goto err;
	}

	pkg_dump(msg->original_buf, msg->original_len+10);
	mem_free(msg);
	return et_ok;

err:
	mem_free(msg);
	return ret;
}

err_type ry_push_data()
{
	ry_wpdata d1, d2, d3, d4;

	d1 = ry_wpdata_make(time(NULL), 1);
	d2 = ry_wpdata_make(time(NULL), 2);
	d3 = ry_wpdata_make(time(NULL), 3);
	d4 = ry_wpdata_make(time(NULL), 4);

	ry_store_push(d1);
	ry_store_push(d2);
	ry_store_push(d3);
	ry_store_push(d4);

	ry_queue_dump(&g_store->du_queue);
	 
	return et_ok;
}

err_type ry_store_init()
{
	g_store = mem_alloc_z(sizeof(ry_store));
	if (!g_store) {
		log_err("alloc store error");
		return et_nomem;
	}
	memset(g_store, 0x0, sizeof(ry_store));
	ry_queue_init(&g_store->du_queue, RY_QUEUE_F_RB);

	return et_ok;
}

int main(int argc, char **argv)
{
	log_level(LOG_DBG);
	log_mode(LOG_TEST);

	ry_store_init();

	ry_push_data();

	ry_construst_msg();

	return 0;
}

