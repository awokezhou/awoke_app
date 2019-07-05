#include <stdarg.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "awoke_log.h"
#include "awoke_type.h"

static int sg_log_level = LOG_ERR;
static int sg_log_mode  = LOG_SYS;
	
void log_level_set(int level)
{
	if (level > LOG_BUG || level < LOG_DBG)
		return;
	sg_log_level = level;
}

static int log_level_get()
{
	return sg_log_level;
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

void awoke_log(int level, const char *func, int line, const char *format, ...)
{
    time_t now;
    va_list args;
    struct tm result;
    struct tm *current;
    const char *header_title;
    const char *header_color;
    //const char *white_color = ANSI_WHITE;
    const char *reset_color = ANSI_RESET;

	if (log_level_invalid(level))
		return;

    switch (level)
    {
        case LOG_BUG:
            header_title = "BUG";
            header_color = ANSI_YELLOW;
            break;
            
        case LOG_DBG:
            header_title = "DEBUG";
            header_color = ANSI_GREEN;            
            break;
            
        case LOG_ERR:
            header_title = "ERROR";
            header_color = ANSI_RED;               
            break;
            
        case LOG_INFO:
            header_title = "INFO";
            header_color = ANSI_BLUE;  
            break;
            
        case LOG_WARN:
            header_title = "WARN";
            header_color = ANSI_MAGENTA;
            break;

        default:
            header_title = "BUG";
            header_color = ANSI_RED;
            break;
    }

    // only print colors to a terminal
    if (!isatty(STDOUT_FILENO)) {
        header_color = "";
    }

    now = time(NULL);

    current = localtime_r(&now, &result);
    printf(" [%i/%02i/%02i %02i:%02i:%02i] ",
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

	
	
    return;
}

