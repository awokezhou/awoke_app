#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "awoke_log.h"
#include "awoke_type.h"
#include "awoke_memory.h"
#include "awoke_printf.h"



#if defined(AWOKE_CONFIG_LOG_COLOR)
static awoke_log_levelmap levelmap[] = {
	{NULL,		LOG_NONE,		FALSE, LOG_COLOR_RESET},
	{"b",		LOG_BURST,		FALSE, LOG_COLOR_RESET},
	{"t",		LOG_TRACE,		FALSE, LOG_COLOR_WHITE},
	{"D", 		LOG_DBG,		TRUE,  LOG_COLOR_GREEN_LIGHT},
	{"I", 		LOG_INFO,		TRUE,  LOG_COLOR_CYAN_LIGHT},
	{"N",		LOG_NOTICE,		TRUE,  LOG_COLOR_YELLOW_LIGHT},
	{"W", 		LOG_WARN,		TRUE,  LOG_COLOR_PINK_LIGHT},
	{"E", 		LOG_ERR,		TRUE,  LOG_COLOR_RED_LIGHT},
	{"B", 		LOG_BUG,		TRUE,  LOG_COLOR_RED},
};
#else
static awoke_log_levelmap levelmap[] = {
	{NULL,		LOG_NONE},
	{"b",		LOG_BURST},
	{"t",		LOG_TRACE},
	{"D",		LOG_DBG},
	{"I",		LOG_INFO},
	{"N",		LOG_NOTICE},
	{"W",		LOG_WARN},
	{"E",		LOG_ERR},
	{"B", 		LOG_BUG},
};
#endif

static awoke_log_modulemap modulemap[] = {
	{NULL,			LOG_M_NONE,	LOG_NONE},
	{"sys",			LOG_M_SYS,	LOG_NONE},
	{"drv",			LOG_M_DRV,	LOG_NONE},
	{"pkt",			LOG_M_PKT,	LOG_NONE},
	{"lib",			LOG_M_LIB,	LOG_NONE},
	{"bk",			LOG_M_BK,		LOG_TRACE},
	{"mk",			LOG_M_MK,		LOG_TRACE},
	{"cmd",			LOG_M_CMDER,	LOG_TRACE},
};

static awoke_log_context logctx = {
	.level = LOG_TRACE,
	.direction = LOG_D_STDOUT,
	.mmask = LOG_M_ALL,
	.fc = {
		.name = "awoke.log",
		.maxsize = 8*1024,		/* 64KB */
		.cachenum = 0,
		.cachemax = 3,
		.init_finish = 0,
	},
};

awoke_log_context *hqlog_context_get(void)
{
	return &logctx;
}

static awoke_log_level level_get(void)
{
	return logctx.level;
}

static bool level_visible(uint8_t level)
{
	return (level_get() <= level);
}

static bool module_visible(uint32_t module)
{
	if (mask_exst(logctx.mmask, module))
		return TRUE;
	return FALSE;
}

static awoke_log_modulemap *module_get(uint32_t module)
{
	int size;
	awoke_log_modulemap *p, *h;

	h = modulemap;
	size = array_size(modulemap);
	array_foreach_start(h, size, p) {
		if (p->mmask == module)
			return p;
	} array_foreach_end();

	return NULL;
}

static const char *module_string_get(uint32_t module)
{
	int size;
	awoke_log_modulemap *p, *h;

	h = modulemap;
	size = array_size(modulemap);

	array_foreach_start(h, size, p) {
		if (p->mmask == module)
			return p->string;
	} array_foreach_end();

	return modulemap[LOG_M_NONE].string;
}

void awoke_log_init(uint8_t level, uint32_t mmask, uint8_t direct)
{
	logctx.level = level;
	logctx.mmask = mmask;
	logctx.direction = direct;
}

void awoke_log_external_interface(void (*handle)(uint8_t, uint32_t, char *, int, void*), void *data)
{
	logctx.external_interface = handle;
	logctx.external_data = data;
}

static bool log_file_exist(const char *filepath)
{
	FILE *fp;

	fp = fopen(filepath, "r");
	if (!fp) {
		return FALSE;
	}

	fclose(fp);
	return TRUE;
}

static int log_file_size(const char *filepath)
{
	struct stat statbuff;

	if (stat(filepath, &statbuff) < 0) {
		return -1;
	} else {
		return statbuff.st_size;
	}	
}

static void log_filecahce_item_add(log_filecache *fc, int id)
{
	log_filecache_item *ni;
	char name[32] = {'\0'};
	
	ni = mem_alloc_z(sizeof(log_filecache_item));
	if (!ni) {
		log_err("alloc filecache item error");
		return;
	}

	sprintf(name, "%s.%d", fc->name, id);
	rename(fc->name, name);
	ni->id = id;
	list_append(&ni->_head, &fc->caches);
	fc->cachenum++;
}

static bool log_filecache_full(log_filecache *fc)
{
	int size;

	size = log_file_size(fc->name);

	if (size >= fc->maxsize) {
		return TRUE;
	}

	return FALSE;
}

static void log_filecahce_init(log_filecache *fc)
{
	int i;
	char name[32] = {'\0'};
	
	/* init cache list */
	list_init(&fc->caches);

	/* record all existed cache file */
	for (i=fc->cachemax; i>0; i--) {

		memset(name, 0x0, 32);
		sprintf(name, "%s.%d", fc->name, i);

		if (log_file_exist(name)) {
			log_filecahce_item_add(fc, i);
		}
	}
	
	fc->init_finish = 1;
}

static void log_filecahce_update(log_filecache *fc)
{
	log_filecache_item *i;
	char name[32] = {'\0'};
	char sname[32] = {'\0'};

	/* if cache full, delete last */
	if (fc->cachenum >= fc->cachemax) {
		i = list_entry_first(&fc->caches, log_filecache_item, _head);
		list_unlink(&i->_head);
		sprintf(name, "%s.%d", fc->name, i->id);
		mem_free(i);
		remove(name);
	}

	list_for_each_entry(i, &fc->caches, _head) {
		memset(name, 0x0, 32);
		sprintf(sname, "%s.%d", fc->name, i->id);
		i->id++;
		sprintf(name, "%s.%d", fc->name, i->id);
		rename(sname, name);
	}

	log_filecahce_item_add(fc, 1);
}

static void log_filecache_write(log_filecache *fc, char *message, int length)
{
	FILE *fp;

	fp = fopen(fc->name, "a+");
	if (!fp) {
		fp = fopen(fc->name, "w");
		if (!fp) {
			return;
		}
	}

	fwrite(message, length, 1, fp);
	fclose(fp);
}

static void awoke_log_file_write(log_filecache *fc, char *message, int length)
{
	
	/* check and do init */
	if (!fc->init_finish) {
		log_filecahce_init(fc);
	}

	/* if file full, dump to cache */
	if (log_filecache_full(fc)) {
		log_filecahce_update(fc);
	}

	log_filecache_write(fc, message, length);
}

#define extern_print(fmt, args...)	printf(fmt, ##args)

void awoke_log(uint8_t level, const char *func, int line, const char *format, ...)
{
	int n;
	va_list args;
	time_t now;
	struct tm date;
	char *logcontext;
	int logcontext_length;
	char buff[AWOKE_LOG_BUFF_MAXLEN] = {'\0'};

	if (!level_visible(level))
		return;

	build_ptr bp = build_ptr_init(buff, AWOKE_LOG_BUFF_MAXLEN);

	awoke_log_levelmap *map = &levelmap[level];


#if defined(AWOKE_CONFIG_LOG_COLOR)
	/* color start */
	if (map->use_color) {
		build_ptr_format(bp, "%s", map->color);
	}
#endif

#if defined(AWOKE_CONFIG_LOG_TIME)
	now = time(NULL);
	localtime_r(&now, &date);
	build_ptr_format(bp, "[%i/%02i/%02i %02i:%02i:%02i] ",
		date.tm_year + 1900,
		date.tm_mon + 1,
		date.tm_mday,
		date.tm_hour,
		date.tm_min,
		date.tm_sec);
#endif

	logcontext = bp.p;

	build_ptr_format(bp, "[%s] [%s:%d] ", map->string, func, line);

	va_start(args, format);
	//n = vsnprintf(bp.p, bp.max, format, args);
	n = awoke_vsnprintf(bp.p, bp.max, format, args);
	va_end(args);

	bp.p += n;
	bp.max -= n;
	bp.len += n;
	logcontext_length = n;
	
#if defined(AWOKE_CONFIG_LOG_COLOR)
	/* color end */
	if (map->use_color) {
		build_ptr_format(bp, "%s", LOG_COLOR_RESET);
	}
#endif

	build_ptr_string(bp, "\n");

	switch (logctx.direction) {

	case LOG_D_STDOUT:
		//printf("%.*s", (int)bp.len, (char *)bp.head);
		awoke_printf("%.*s", (int)bp.len, (char *)bp.head);
		break;

	case LOG_D_FILE:
		awoke_log_file_write(&logctx.fc, (char *)bp.head, (int)bp.len);
		break;

	case LOG_D_STDOUT_EX:
		awoke_printf("%.*s", (int)bp.len, (char *)bp.head);
		break;

	default:
		break;
	}
}

void awoke_logm(uint8_t level, uint32_t module, const char *func, int line, const char *format, ...)
{
	int n;
    va_list args;
	time_t now;
    struct tm date;
	char *logcontext;
	const char *mstring;
	int logcontext_length;
	char buff[AWOKE_LOG_BUFF_MAXLEN] = {'\0'};

	awoke_log_modulemap *mmap = module_get(module);
	if (!mmap)
		return;

	if ((level != LOG_NONE) && (level < mmap->mlevel)) {
		return;
	} else if (!level_visible(level)) {
		return;
	}
	
	if (!module_visible(module)) {
		log_err("module not visible");
		return;
	}

	build_ptr bp = build_ptr_init(buff, AWOKE_LOG_BUFF_MAXLEN);

	awoke_log_levelmap *lmap = &levelmap[level];
	mstring = module_string_get(module);


#if defined(AWOKE_CONFIG_LOG_COLOR)
	/* color start */
	if (lmap->use_color && (logctx.direction != LOG_D_FILE)) {
		build_ptr_format(bp, "%s", lmap->color);
	}
#endif

#if defined(AWOKE_CONFIG_LOG_TIME)
	now = time(NULL);
	localtime_r(&now, &date);
	build_ptr_format(bp, "[%i/%02i/%02i %02i:%02i:%02i] ",
		date.tm_year + 1900,
		date.tm_mon + 1,
		date.tm_mday,
		date.tm_hour,
		date.tm_min,
		date.tm_sec);
#endif

	build_ptr_format(bp, "[%s] [%s] ", mstring, lmap->string);

	logcontext = bp.p;

	build_ptr_format(bp, "[%s:%d] ", func, line);
	
	va_start(args, format);
	//n = vsnprintf(bp.p, bp.max, format, args);
	n = awoke_vsnprintf(bp.p, bp.max, format, args);
	va_end(args);

	bp.p += n;
	bp.max -= n;
	bp.len += n;

	logcontext_length = bp.p - logcontext;

#if defined(AWOKE_CONFIG_LOG_COLOR)
	/* color end */
	if (lmap->use_color && (logctx.direction != LOG_D_FILE)) {
		build_ptr_format(bp, "%s", LOG_COLOR_RESET);
	}
#endif

	build_ptr_string(bp, "\n");

	switch (logctx.direction) {

	case LOG_D_STDOUT:
		//printf("%.*s", (int)bp.len, (char *)bp.head);
		awoke_printf("%.*s", (int)bp.len, (char *)bp.head);
		break;

	case LOG_D_FILE:
		awoke_log_file_write(&logctx.fc, (char *)bp.head, (int)bp.len);
		break;

	case LOG_D_EXTERNAL_INTERFACE:
		logctx.external_interface(level, module, bp.head, bp.len, logctx.external_data);
		break;

	case LOG_D_STDOUT_EX:
		awoke_printf("%.*s", (int)bp.len, (char *)bp.head);
		logctx.external_interface(level, module, logcontext, logcontext_length, logctx.external_data);
		break;
		
	default:
		break;
	}
}

void awoke_hexdump(uint8_t level, const char *func, int linenr, const void *vbuf, size_t len)
{
	unsigned int n;
	unsigned char *buf = (unsigned char *)vbuf;
	
	if (!level_visible(level))
		return;

	if (!len || !vbuf)
		return;

	awoke_log(level, func, linenr, "");

	for (n = 0; n < len;) {
		unsigned int start = n, m;
		char line[80], *p = line;

		p += snprintf(p, 10, "%04X: ", start);

		for (m = 0; m < 16 && n < len; m++)
			p += snprintf(p, 5, "%02X ", buf[n++]);
		while (m++ < 16)
			p += snprintf(p, 5, "   ");

		p += snprintf(p, 6, "   ");

		for (m = 0; m < 16 && (start + m) < len; m++) {
			if (buf[start + m] >= ' ' && buf[start + m] < 127)
				*p++ = buf[start + m];
			else
				*p++ = '.';
		}

		while (m++ < 16)
			*p++ = ' ';

		*p = '\0';
		awoke_log(level, func, linenr, "%s", line);
		(void)line;
	}	

	awoke_log(level, func, linenr, "");
}

void awoke_bitdump(uint8_t level, const char *func, int linenr, const void *vbuf, size_t len)
{
	unsigned int n;
	uint32_t *buf = (uint32_t *)vbuf;

	if (!level_visible(level))
		return;

	if (!len || !vbuf)
		return;

	awoke_log(level, func, linenr, "");

	for (n=0; n<len; n+=32) {
		unsigned int start = n, m;
		char line[80], *p = line;

		p += snprintf(p, 10, "%d:\t", start);
		for (m = 0; m < 32 && n < len; m++)
			p += snprintf(p, 5, "%d ", (buf[n/32]&(1<<m))>0);
		while (m++ < 32)
			p += snprintf(p, 5, "   ");
		
		p += snprintf(p, 5, "   ");
		*p = '\0';
		awoke_log(level, func, linenr, "%s", line);
		(void)line;
	}

	awoke_log(level, func, linenr, "");
}
