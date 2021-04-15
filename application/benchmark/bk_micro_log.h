
#ifndef __BK_MICRO_LOG_H__
#define __BK_MICRO_LOG_H__

#define BK_MICRO_LOG_MAX	64

static char bk_micro_log_buffer[BK_MICRO_LOG_MAX] = {'0'};
static int bk_micro_log_length = 0;
static int bk_micro_log_count = 0;

#define bk_micro_log_info(string)	bk_micro_log_add("N:"string"\n");
#define bk_micro_log_debug(string)	bk_micro_log_add("D:"string"\n");
#define bk_micro_log_err(string)	bk_micro_log_add("E:"string"\n");

#define bk_microlog_info(string)	bk_microlog_push("N:"string"\n");
#define bk_microlog_debug(string)	bk_microlog_push("D:"string"\n");
#define bk_microlog_err(string)		bk_microlog_push("E:"string"\n");



#define BK_MICROLOG_COUNT_BASE	48

#define bk_push_char(c, p)	do {\
		memcpy(p, &c, 1);\
		p+=1;\
	}while(0)

#define bk_push_string(data, p, bytes) do {\
					memcpy(p, data, bytes);\
					p+= bytes;\
				}while(0)

typedef struct _bk_microlog {

#define BK_MICROLOG_BUFFMAX	64
	char buffer[BK_MICROLOG_BUFFMAX];

	int length;

	uint8_t count;

	uint8_t f_init:1;
	
} bk_microlog;

static bk_microlog mlog = {
	.count = 0,
	.length = 0,
	.f_init = 0,
};

static void bk_microlog_init(struct _bk_microlog *m)
{
	int i;
	m->f_init = 1;

	for (i=0; i<BK_MICROLOG_BUFFMAX; i++) {
		m->buffer[i] = 0x0;	
	}
}

static void bk_microlog_push(const char *message)
{
	int insize, remain;
	char *pos, *head, c_count;
	char c_ctrl[3] = {0x0a, 0x0a, 0x0a};
	
	if (!mlog.f_init) {
		bk_microlog_init(&mlog);
	}

	c_count = BK_MICROLOG_COUNT_BASE + (mlog.count++)%10;

	insize = strlen(message) + 1;
	remain = BK_MICROLOG_BUFFMAX - mlog.length - 1;

	head = mlog.buffer;
	pos = head + mlog.length;

	if (remain >= insize) {
		bk_push_char(c_count, pos);
		bk_push_string(message, pos, insize-1);
		mlog.length += insize;
	} else {
		if (remain < 4) {
			bk_push_string(c_ctrl, pos, remain);
			pos = head;
			bk_push_char(c_count, pos);
			bk_push_string(message, pos, insize-1);
			mlog.length = insize;
		} else {
			bk_push_char(c_count, pos);
			bk_push_string(message, pos, remain-2);
			bk_push_string(c_ctrl, pos, 1);
			pos = head;
			bk_push_string(message+remain-2, pos, insize-remain+1);
			mlog.length = insize-remain+1;
		}
	}

	if (mlog.length >= (BK_MICROLOG_BUFFMAX-1)) {
		log_trace("length = 0");
		mlog.length = 0;
	}
}

static void bk_microlog_print(void)
{
	log_trace("length:%d", mlog.length);
	printf("%s", mlog.buffer);
}

void bk_micro_log_add(char *msg)
{
	int insize;
	int remain;
	int addlen;
	char *pos, *head;

	char ctrl[3] = {0x0a, 0x0a, 0x0a};
	char num = 48 + bk_micro_log_count++%10;
	insize = strlen(msg)+1;
	remain = BK_MICRO_LOG_MAX - bk_micro_log_length-1;
	log_debug("insize:%d remain:%d", insize, remain);
	
	head = bk_micro_log_buffer;
	pos = head + bk_micro_log_length;

	if (remain >= insize) {
		memcpy(pos, &num, 1);
		pos += 1;
		memcpy(pos, msg, insize-1);
		bk_micro_log_length += insize;
	} else {
		if (remain < 4) {
			memcpy(pos, ctrl, remain);
			pos = head;
			memcpy(pos, &num, 1);
			pos += 1;
			memcpy(pos, msg, insize-1);
			bk_micro_log_length = insize;
		} else {
			memcpy(pos, &num, 1);
			pos += 1;
			addlen = remain-2;
			memcpy(pos, msg, remain-2);
			pos += addlen;
			memcpy(pos, ctrl, 1);
			memcpy(head, msg+addlen, insize-1-addlen);
			bk_micro_log_length = insize-1-addlen;
		}
	}

	if (bk_micro_log_length >= (BK_MICRO_LOG_MAX-1)) {
		bk_micro_log_length = 0;
		log_trace("resize length 0");
	}
}

void bk_micro_log_print(void)
{
	log_info("print---%d", bk_micro_log_length);
	printf("%s", bk_micro_log_buffer);
}

void bk_micro_log_printhex(void)
{
	awoke_hexdump_trace(mlog.buffer, BK_MICRO_LOG_MAX);
}

#endif /* __BK_MICRO_LOG_H__ */
