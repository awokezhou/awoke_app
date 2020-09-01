#ifndef __AWOKE_LOG_H__
#define __AWOKE_LOG_H__


#include "awoke_macros.h"
#include "awoke_string.h"
#include "awoke_bitmap.h"

#define AWOKE_LOG_COLOR

void awoke_log(int level, const char *func, int line, const char *format, ...);
void awoke_logm(int level, uint32_t module, const char *func, int line, const char *format, ...);
void awoke_log_init(uint8_t level, uint16_t mmask);
void awoke_log_set_module(uint32_t mmask);
void awoke_hexdump(int level, const char *func, int linenr, const void *vbuf, size_t len);


#if defined(AWOKE_LOG_COLOR)
#define LOG_COLOR_RESET			"\033[0m"
#define LOG_COLOR_RED			"\033[31m"
#define LOG_COLOR_GREEN			"\033[32m"
#define LOG_COLOR_YELLOW		"\033[33m"
#define LOG_COLOR_BLUE			"\033[34m"
#define LOG_COLOR_PINK			"\033[35m"
#define LOG_COLOR_CYAN			"\033[36m"
#define LOG_COLOR_WHITE			"\033[37m"
#define LOG_COLOR_RED_LIGHT		"\033[91m"
#define LOG_COLOR_GREEN_LIGHT	"\033[92m"
#define LOG_COLOR_YELLOW_LIGHT	"\033[93m"
#define LOG_COLOR_BLUE_LIGHT	"\033[94m"
#define LOG_COLOR_PINK_LIGHT	"\033[95m"
#define LOG_COLOR_CYAN_LIGHT	"\033[96m"
#define LOG_COLOR_WHITE_LIGHT	"\033[97m"
#endif



/* -- Log Level --{*/
typedef enum {
	LOG_NONE = 0,
	LOG_BURST,
	LOG_TRACE,
	LOG_DBG,
	LOG_INFO,
	LOG_NOTICE,
	LOG_WARN,
	LOG_ERR,
	LOG_BUG,
} awoke_log_level;

typedef struct _awoke_log_levelmap {
	const char *string;
	awoke_log_level level;
#if defined(AWOKE_LOG_COLOR)
	const char *header_color;
	const char *body_color;
#endif
} awoke_log_levelmap;
/*}-- Log Level -- */


/* -- Log Module --{*/
#define LOG_M_MAXLEN	6

typedef enum {
	LOG_M_NONE = 0,
	LOG_M_OS,
	LOG_M_EVENT,
	LOG_M_QUEUE,
	LOG_M_HTTP,
	LOG_M_PKT,
	LOG_M_WORKER,
	LOG_M_MC_CLIENT,
	LOG_M_MC_SERVER,
	LOG_M_SIZEOF,
} awoke_log_module;

typedef struct _awoke_log_modulemap {
	const char *string;
	unsigned int mbit;
} awoke_log_modulemap;
/*}-- Log Module -- */


/* -- Log Direction --{*/
typedef enum {
	LOG_D_STDOUT = 1,
	LOG_D_FILE,
} awoke_log_direction;
/*}-- Log Direction -- */


/* -- Log Context --{*/
typedef struct _awoke_log_context {
	awoke_log_level level;
	awoke_log_direction direction;
	awoke_bitmap(mbitmap, LOG_M_SIZEOF);
} awoke_log_context;
/*}-- Log Context -- */

#define LOG_FILE_SIZE		8092
#define LOG_FILE_PATH		"/home/share/project/awoke_app/log/"
#define LOG_FILE_POSTFIX	".log"

#define log_burst(...)		awoke_log(LOG_BURST,  __func__, __LINE__, __VA_ARGS__)
#define log_trace(...)		awoke_log(LOG_TRACE,  __func__, __LINE__, __VA_ARGS__)
#define log_debug(...)      awoke_log(LOG_DBG,    __func__, __LINE__, __VA_ARGS__)
#define log_info(...)       awoke_log(LOG_INFO,   __func__, __LINE__, __VA_ARGS__)
#define log_notice(...)     awoke_log(LOG_NOTICE, __func__, __LINE__, __VA_ARGS__)
#define log_err(...)        awoke_log(LOG_ERR,    __func__, __LINE__, __VA_ARGS__)
#define log_warn(...)       awoke_log(LOG_WARN,   __func__, __LINE__, __VA_ARGS__)
#define log_bug(...)        awoke_log(LOG_BUG,    __func__, __LINE__, __VA_ARGS__)

#define awoke_hexdump_burst(...)	awoke_hexdump(LOG_BURST, 	__func__, __LINE__, __VA_ARGS__)
#define awoke_hexdump_trace(...)	awoke_hexdump(LOG_TRACE, 	__func__, __LINE__, __VA_ARGS__)
#define awoke_hexdump_debug(...)	awoke_hexdump(LOG_DBG, 		__func__, __LINE__, __VA_ARGS__)
#define awoke_hexdump_info(...)		awoke_hexdump(LOG_INFO, 	__func__, __LINE__, __VA_ARGS__)
#define awoke_hexdump_notice(...)	awoke_hexdump(LOG_NOTICE, 	__func__, __LINE__, __VA_ARGS__)
#define awoke_hexdump_err(...)		awoke_hexdump(LOG_ERR, 		__func__, __LINE__, __VA_ARGS__)
#define awoke_hexdump_warn(...)		awoke_hexdump(LOG_WARN, 	__func__, __LINE__, __VA_ARGS__)
#define awoke_hexdump_bug(...)		awoke_hexdump(LOG_BUG, 		__func__, __LINE__, __VA_ARGS__)







#endif /* __AWOKE_LOG_H__ */
