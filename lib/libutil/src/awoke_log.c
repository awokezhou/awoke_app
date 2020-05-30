#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "awoke_log.h"
#include "awoke_type.h"

static int sg_log_level = LOG_DBG;
static int sg_log_mode  = LOG_TEST;
static char *sg_log_id  = NULL;
	
void log_level_set(int level)
{
	if (level > LOG_BUG || level < LOG_TEST)
		return;
	sg_log_level = level;
}

static int log_level_get()
{
	return sg_log_level;
}

static char *log_id_get()
{
	return sg_log_id;
}

void log_id_set(char *id)
{
	if (!id)
		return;
	sg_log_id = id;
}

static bool log_id_invalid()
{
	return (!log_id_get());
}

static bool log_level_invalid(int level)
{
	return (log_level_get() > level);
}

void log_mode_set(int mode)
{
	if (mode > LOG_SYS || mode < LOG_TEST)
		return;	
	sg_log_mode = mode;
}

static int log_mode_get()
{
	return sg_log_mode;
}

static bool log_mode_test()
{
	return (log_mode_get()==LOG_TEST);
}

void _log_set(char *id, int mode, int level)
{
	log_id_set(id);
	log_mode_set(mode);
	log_level_set(level);
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

void awoke_log(int level, const char *func, int line, const char *format, ...)
{
	int len;
    time_t now;
    va_list args;
    struct tm result;
    struct tm *current;
	char buff[1024] = {"\0"};
    const char *header_title;
    const char *header_color;
	char log_file[128] = {"\0"};
    //const char *white_color = ANSI_WHITE;
    const char *reset_color = ANSI_RESET;

	if (log_level_invalid(level))
		return;

    switch (level)
    {
        case LOG_BUG:
            header_title = "BUG  ";
            header_color = ANSI_YELLOW;
            break;
            
        case LOG_DBG:
            header_title = "DEBUG";
            header_color = ANSI_GREEN;            
            break;

		case LOG_TEST:
			header_title = "TEST ";
			header_color = ANSI_CYAN;
			break;
			
        case LOG_ERR:
            header_title = "ERROR";
            header_color = ANSI_RED;               
            break;
            
        case LOG_INFO:
            header_title = "INFO ";
            header_color = ANSI_BLUE;  
            break;
            
        case LOG_WARN:
            header_title = "WARN ";
            header_color = ANSI_MAGENTA;
            break;

        default:
            header_title = "BUG  ";
            header_color = ANSI_RED;
            break;
    }

	now = time(NULL);
	current = localtime_r(&now, &result);

	if (log_mode_test()) {		// test mode print to stdout
	
	    // only print colors to a terminal
	    if (!isatty(STDOUT_FILENO)) {
	        header_color = "";
	    }

		printf("[%i/%02i/%02i %02i:%02i:%02i] ",
			current->tm_year + 1900,
			current->tm_mon + 1,
			current->tm_mday,
			current->tm_hour,
			current->tm_min,
			current->tm_sec);

	    printf("[%s%s%s] [%s%s:%d %s] ",
	        header_color, header_title, 
	        reset_color, header_color,
	        func, line, reset_color);

		va_start(args, format);
	    vprintf(format, args);
	    va_end(args);

	    printf("%s\n", reset_color);
	    fflush(stdout);
	} else {

		if (log_id_invalid())
			return;
		
		build_ptr bp = build_ptr_init(buff, 1024);

	
		build_ptr_format(bp, "[%i/%02i/%02i %02i:%02i:%02i] ", 
					     current->tm_year + 1900,
					     current->tm_mon + 1,
					     current->tm_mday,
					     current->tm_hour,
					     current->tm_min,
					     current->tm_sec);
		
		build_ptr_format(bp, "[%s] [%s:%d] ", header_title, func, line);
		
		va_start(args, format);
		len = vsnprintf(bp.p, bp.max, format, args);
		va_end(args);
		bp.p += len;
		bp.max -= len;
		bp.len += len;
		
		build_ptr_string(bp, "\n");

		sprintf(log_file, "%s%s%s", LOG_FILE_PATH, log_id_get(), LOG_FILE_POSTFIX);
		_log_file_write(log_file, bp.head, bp.len);
	}
	
    return;
}

