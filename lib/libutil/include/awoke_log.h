#ifndef __AWOKE_LOG_H__
#define __AWOKE_LOG_H__



#include "awoke_list.h"
#include "awoke_macros.h"
#include "awoke_string.h"



/* -- feature macros --{*/
#define AWOKE_CONFIG_LOG_TIME
#define AWOKE_CONFIG_LOG_COLOR
/*}-- feature macros -- */



#define AWOKE_LOG_BUFF_MAXLEN	256



void awoke_log(uint8_t level, const char *func, int line, const char *format, ...);
void awoke_logm(uint8_t level, uint32_t module, const char *func, int line, const char *format, ...);
void awoke_log_init(uint8_t level, uint32_t mmask);
void awoke_log_set_module(uint32_t mmask);
void awoke_hexdump(uint8_t level, const char *func, int linenr, const void *vbuf, size_t len);



/*
 * Log Color {
 */
#if defined(AWOKE_CONFIG_LOG_COLOR)
#define LOG_COLOR_RESET				"\033[0m"
#define LOG_COLOR_RED				"\033[31m"
#define LOG_COLOR_GREEN				"\033[32m"
#define LOG_COLOR_YELLOW			"\033[33m"
#define LOG_COLOR_BLUE				"\033[34m"
#define LOG_COLOR_PINK				"\033[35m"
#define LOG_COLOR_CYAN				"\033[36m"
#define LOG_COLOR_WHITE				"\033[37m"
#define LOG_COLOR_RED_LIGHT			"\033[91m"
#define LOG_COLOR_GREEN_LIGHT		"\033[92m"
#define LOG_COLOR_YELLOW_LIGHT		"\033[93m"
#define LOG_COLOR_BLUE_LIGHT		"\033[94m"
#define LOG_COLOR_PINK_LIGHT		"\033[95m"
#define LOG_COLOR_CYAN_LIGHT		"\033[96m"
#define LOG_COLOR_WHITE_LIGHT		"\033[97m"
#endif
/*
 * Log Color }
 */



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
#if defined(AWOKE_CONFIG_LOG_COLOR)
	bool use_color;
	const char *color;
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

#define LOG_M_OFFSET(n) 	(0x00000001 << (n))

#define LOG_M_NONE			0x00000000
#define LOG_M_SYS			LOG_M_OFFSET(0)
#define LOG_M_DRV			LOG_M_OFFSET(1)
#define LOG_M_PKT			LOG_M_OFFSET(2)
#define LOG_M_LIB			LOG_M_OFFSET(3)

#define LOG_M_BK			LOG_M_OFFSET(8)
#define LOG_M_MK			LOG_M_OFFSET(9)
#define LOG_M_CMDER			LOG_M_OFFSET(10)
#define LOG_M_ALL			0xFFFFFFFF


typedef struct _awoke_log_modulemap {
	const char *string;
	uint32_t mmask;
	uint8_t mlevel;
} awoke_log_modulemap;
/*}-- Log Module -- */


/* -- Log Direction --{*/
typedef enum {
	LOG_D_STDOUT = 1,
	LOG_D_FILE,
	LOG_D_UART,
	LOG_D_EXTERNAL_INTERFACE,
} awoke_log_direction;
/*}-- Log Direction -- */


/* -- Log Context --{*/
typedef struct _hqlog_filecache_item {
	int id;
	awoke_list _head;
} log_filecache_item;

typedef struct _hqlog_filecache {
	char *name;
	int maxsize;
	int cachenum;
	int cachemax;
	awoke_list caches;
	uint8_t init_finish:1;
} log_filecache;

typedef struct _awoke_log_context {
	uint8_t level;
	uint8_t direction;
	uint32_t mmask;
	void (*external_interface)(char *, int );
	log_filecache fc;
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

#define logm_burst(module, ...)		awoke_logm(LOG_BURST, 	module, __func__, __LINE__, __VA_ARGS__)
#define logm_trace(module, ...)		awoke_logm(LOG_TRACE, 	module, __func__, __LINE__, __VA_ARGS__)
#define logm_debug(module, ...)		awoke_logm(LOG_DBG, 	module, __func__, __LINE__, __VA_ARGS__)
#define logm_info(module, ...)		awoke_logm(LOG_INFO, 	module, __func__, __LINE__, __VA_ARGS__)
#define logm_notice(module, ...)	awoke_logm(LOG_NOTICE,	module, __func__, __LINE__, __VA_ARGS__)
#define logm_err(module, ...)		awoke_logm(LOG_ERR, 	module, __func__, __LINE__, __VA_ARGS__)
#define logm_warn(module, ...)		awoke_logm(LOG_WARN, 	module, __func__, __LINE__, __VA_ARGS__)
#define logm_bug(module, ...)		awoke_logm(LOG_BUG, 	module, __func__, __LINE__, __VA_ARGS__)


#define awoke_hexdump_burst(...)	awoke_hexdump(LOG_BURST, 	__func__, __LINE__, __VA_ARGS__)
#define awoke_hexdump_trace(...)	awoke_hexdump(LOG_TRACE, 	__func__, __LINE__, __VA_ARGS__)
#define awoke_hexdump_debug(...)	awoke_hexdump(LOG_DBG, 		__func__, __LINE__, __VA_ARGS__)
#define awoke_hexdump_info(...)		awoke_hexdump(LOG_INFO, 	__func__, __LINE__, __VA_ARGS__)
#define awoke_hexdump_notice(...)	awoke_hexdump(LOG_NOTICE, 	__func__, __LINE__, __VA_ARGS__)
#define awoke_hexdump_err(...)		awoke_hexdump(LOG_ERR, 		__func__, __LINE__, __VA_ARGS__)
#define awoke_hexdump_warn(...)		awoke_hexdump(LOG_WARN, 	__func__, __LINE__, __VA_ARGS__)
#define awoke_hexdump_bug(...)		awoke_hexdump(LOG_BUG, 		__func__, __LINE__, __VA_ARGS__)


#define awoke_bitdump_burst(...)	awoke_bitdump(LOG_BURST, 	__func__, __LINE__, __VA_ARGS__)
#define awoke_bitdump_trace(...)	awoke_bitdump(LOG_TRACE, 	__func__, __LINE__, __VA_ARGS__)
#define awoke_bitdump_debug(...)	awoke_bitdump(LOG_DBG, 		__func__, __LINE__, __VA_ARGS__)
#define awoke_bitdump_info(...)		awoke_bitdump(LOG_INFO, 	__func__, __LINE__, __VA_ARGS__)
#define awoke_bitdump_notice(...)	awoke_bitdump(LOG_NOTICE, 	__func__, __LINE__, __VA_ARGS__)
#define awoke_bitdump_err(...)		awoke_bitdump(LOG_ERR, 		__func__, __LINE__, __VA_ARGS__)
#define awoke_bitdump_warn(...)		awoke_bitdump(LOG_WARN, 	__func__, __LINE__, __VA_ARGS__)
#define awoke_bitdump_bug(...)		awoke_bitdump(LOG_BUG, 		__func__, __LINE__, __VA_ARGS__)

#define lib_burst(...)				awoke_logm(LOG_BURST, 	LOG_M_LIB, __func__, __LINE__, __VA_ARGS__)
#define lib_trace(...)				awoke_logm(LOG_TRACE, 	LOG_M_LIB, __func__, __LINE__, __VA_ARGS__)
#define lib_debug(...)				awoke_logm(LOG_DBG, 	LOG_M_LIB, __func__, __LINE__, __VA_ARGS__)
#define lib_info(...)				awoke_logm(LOG_INFO, 	LOG_M_LIB, __func__, __LINE__, __VA_ARGS__)
#define lib_notice(...)				awoke_logm(LOG_NOTICE, 	LOG_M_LIB, __func__, __LINE__, __VA_ARGS__)
#define lib_err(...)				awoke_logm(LOG_ERR, 	LOG_M_LIB, __func__, __LINE__, __VA_ARGS__)
#define lib_warn(...)				awoke_logm(LOG_WARN, 	LOG_M_LIB, __func__, __LINE__, __VA_ARGS__)
#define lib_bug(...)				awoke_logm(LOG_BUG, 	LOG_M_LIB, __func__, __LINE__, __VA_ARGS__)



#endif /* __AWOKE_LOG_H__ */
