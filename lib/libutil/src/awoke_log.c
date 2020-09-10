#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "awoke_log.h"
#include "awoke_type.h"



/*
 * Config
 * AWOKE_LOG_WITH_COLOR
 * AWOKE_LOG_WITH_MODULE
 * AWOKE_LOG_TO_FILE
 */



#if defined(AWOKE_LOG_COLOR)
static awoke_log_levelmap levelmap[] = {
	{NULL,			LOG_NONE,	LOG_COLOR_WHITE_LIGHT,		LOG_COLOR_WHITE_LIGHT},
	{"burst",		LOG_BURST,	LOG_COLOR_WHITE_LIGHT,		LOG_COLOR_WHITE_LIGHT},
	{"trace",		LOG_TRACE,	LOG_COLOR_WHITE_LIGHT,		LOG_COLOR_WHITE_LIGHT},
	{"DEBUG", 		LOG_DBG,	LOG_COLOR_GREEN_LIGHT,		LOG_COLOR_GREEN_LIGHT},
	{"INFO", 		LOG_INFO,	LOG_COLOR_CYAN_LIGHT,		LOG_COLOR_CYAN_LIGHT},
	{"NOTICE",		LOG_NOTICE,	LOG_COLOR_YELLOW_LIGHT,		LOG_COLOR_YELLOW_LIGHT},
	{"WARN", 		LOG_WARN,	LOG_COLOR_PINK_LIGHT,		LOG_COLOR_PINK_LIGHT},
	{"ERROR", 		LOG_ERR,	LOG_COLOR_RED_LIGHT,		LOG_COLOR_RED_LIGHT},
	{"BUG", 		LOG_BUG,	LOG_COLOR_RED,				LOG_COLOR_RED},
};
#else
static awoke_log_levelmap levelmap[] = {
	{NULL,			LOG_NONE},
	{"burst",		LOG_BURST},
	{"trace",		LOG_TRACE},
	{"DEBUG",		LOG_DBG},
	{"INFO",		LOG_INFO},
	{"NOTICE",		LOG_NOTICE},
	{"WARN",		LOG_WARN},
	{"ERROR",		LOG_ERR},
	{"BUG", 		LOG_BUG},
};
#endif


static awoke_log_modulemap modulemap[] = {
	{NULL,			LOG_M_NONE},
	{"OS",			LOG_M_OS},
	{"EV",			LOG_M_EVENT},
	{"Queue",		LOG_M_QUEUE},
	{"HTTP",		LOG_M_HTTP},
	{"PKT",			LOG_M_PKT},
	{"worker",		LOG_M_WORKER},
	{"mc-client",	LOG_M_MC_CLIENT},
	{"mc-server",	LOG_M_MC_SERVER},
};

static awoke_log_context logctx = {
	.level = LOG_TRACE,
	.direction = LOG_D_STDOUT,
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
	return (test_bit(module, logctx.mbitmap));
}

static const char *module_string_get(uint16_t module)
{
	int size;
	awoke_log_modulemap *p, *h;
	
	h = modulemap;
	size = array_size(modulemap);

	array_foreach(h, size, p) {
		if (p->mbit == module)
			return p->string;
	}

	return modulemap[LOG_M_NONE].string;
}


void awoke_log_init(uint8_t level, uint16_t mmask)
{
	logctx.level = level;
}

void awoke_log_set_module(uint32_t mmask)
{
	awoke_set_bit(mmask, logctx.mbitmap);
}

static bool direction_stdout(void)
{
	return (logctx.direction == LOG_D_STDOUT);
}

static void _log_file_write(char *file, char *str, int len)
{
	FILE *fp;
	struct stat st;
	long long file_size;

	fp = fopen(file, "r");
	if (!fp) {
		fp = fopen(file, "w");
		fclose(fp);
	}

	if (stat(file, &st) < 0) {
		return;
	}

	file_size = (long long)st.st_size;

	if (file_size >= LOG_FILE_SIZE) {
		fp = fopen(file, "w+");
	} else {
		fp = fopen(file, "a+");
	}

	fwrite(str, len, 1, fp);
	fclose(fp);
}

#define extern_print(fmt, args...)	printf(fmt, ##args)

void awoke_log(int level, const char *func, int line, const char *format, ...)
{
	int len;
	char buff[1024];
    time_t now;
    va_list args;
    struct tm result;
    struct tm *current;
	char log_file[128] = {"\0"};
#if defined(AWOKE_LOG_COLOR)
	const char *reset_color = LOG_COLOR_RESET;
#endif

	if (!level_visible(level))
		return;

	awoke_log_levelmap *_levelmap = &levelmap[level];
	
	now = time(NULL);
	current = localtime_r(&now, &result);

	if (direction_stdout()) {		// print to stdout

		va_start(args, format);
		vsprintf(buff, format, args);
		va_end(args);
		
#if defined(AWOKE_LOG_COLOR)

	    extern_print("%s[%i/%02i/%02i %02i:%02i:%02i] [%s] [%s:%d]%s %s%s%s\n",
	    	_levelmap->header_color,
			current->tm_year + 1900,
			current->tm_mon + 1,
			current->tm_mday,
			current->tm_hour,
			current->tm_min,
			current->tm_sec,
        	_levelmap->string,
       		func, line, reset_color,
        	_levelmap->body_color, buff, reset_color);
#else
	    extern_print("[%i/%02i/%02i %02i:%02i:%02i] [%s] [%s:%d] %s\n", 
	    	current->tm_year + 1900,
			current->tm_mon + 1,
			current->tm_mday,
			current->tm_hour,
			current->tm_min,
			current->tm_sec,
			_levelmap->string,
			func, line, buff);
#endif
	    
	    fflush(stdout);
	} else {
		
		build_ptr bp = build_ptr_init(buff, 1024);

	
		build_ptr_format(bp, "[%i/%02i/%02i %02i:%02i:%02i] ", 
					     current->tm_year + 1900,
					     current->tm_mon + 1,
					     current->tm_mday,
					     current->tm_hour,
					     current->tm_min,
					     current->tm_sec);
		
		build_ptr_format(bp, "[%s] [%s:%d] ", _levelmap->string, func, line);
		
		va_start(args, format);
		len = vsnprintf(bp.p, bp.max, format, args);
		va_end(args);
		bp.p += len;
		bp.max -= len;
		bp.len += len;
		
		build_ptr_string(bp, "\n");

		sprintf(log_file, "%s%s", LOG_FILE_PATH, LOG_FILE_POSTFIX);
		_log_file_write(log_file, bp.head, bp.len);
	}
	
    return;
}

void awoke_logm(int level, uint32_t module, const char *func, int line, const char *format, ...)
{
	int len;
	char buff[1024];
    time_t now;
    va_list args;
    struct tm result;
    struct tm *current;
	char log_file[128] = {"\0"};
#if defined(AWOKE_LOG_COLOR)
	const char *reset_color = LOG_COLOR_RESET;
#endif

	printf("<debug> module:%d\r\n", module);

	if (!level_visible(level))
		return;

	if (!module_visible(module))
		return;

	printf("<debug> module:%d visible\r\n", module);

	awoke_log_levelmap *_levelmap = &levelmap[level];
	awoke_log_modulemap *_modulemap = &modulemap[module];
	
	now = time(NULL);
	current = localtime_r(&now, &result);

	if (direction_stdout()) {		// print to stdout

		va_start(args, format);
		vsprintf(buff, format, args);
		va_end(args);
		
#if defined(AWOKE_LOG_COLOR)
#if defined(AWOKE_LOG_TIME)
	    extern_print("%s[%i/%02i/%02i %02i:%02i:%02i] [%s] [%s] [%s:%d]%s %s%s%s\n",
	    	_levelmap->header_color,
			current->tm_year + 1900,
			current->tm_mon + 1,
			current->tm_mday,
			current->tm_hour,
			current->tm_min,
			current->tm_sec,
			_modulemap->string,
        	_levelmap->string,
       		func, line, reset_color,
        	_levelmap->body_color, buff, reset_color);
#else
		extern_print("%s[%s] [%s] [%s:%d]%s %s%s%s\n",
			_levelmap->header_color,
			_modulemap->string,
			_levelmap->string,
			func, line, reset_color,
			_levelmap->body_color, buff, reset_color);

#endif
#else
#if defined(AWOKE_LOG_TIME)
	    extern_print("[%i/%02i/%02i %02i:%02i:%02i] [%s] [%s] [%s:%d] %s\n", 
	    	current->tm_year + 1900,
			current->tm_mon + 1,
			current->tm_mday,
			current->tm_hour,
			current->tm_min,
			current->tm_sec,
			_modulemap->string,
			_levelmap->string,
			func, line, buff);
#else
		extern_print("[%s] [%s] [%s:%d] %s\n", 
			_modulemap->string,
			_levelmap->string,
			func, line, buff);
#endif
#endif
	    
	    fflush(stdout);
	} else {
		
		build_ptr bp = build_ptr_init(buff, 1024);

#if defined(AWOKE_LOG_TIME)
		build_ptr_format(bp, "[%i/%02i/%02i %02i:%02i:%02i] ", 
					     current->tm_year + 1900,
					     current->tm_mon + 1,
					     current->tm_mday,
					     current->tm_hour,
					     current->tm_min,
					     current->tm_sec);
#endif
		build_ptr_format(bp, "[%s] [%s:%d] ", _levelmap->string, func, line);
		
		va_start(args, format);
		len = vsnprintf(bp.p, bp.max, format, args);
		va_end(args);
		bp.p += len;
		bp.max -= len;
		bp.len += len;
		
		build_ptr_string(bp, "\n");

		sprintf(log_file, "%s%s", LOG_FILE_PATH, LOG_FILE_POSTFIX);
		_log_file_write(log_file, bp.head, bp.len);
	}
	
    return;	
}

void awoke_hexdump(int level, const char *func, int linenr, const void *vbuf, size_t len)
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
