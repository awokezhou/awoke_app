#ifndef __AWOKE_LOG_H__
#define __AWOKE_LOG_H__


#include "awoke_string.h"

void _log_set(char *id, int mode, int level);
void awoke_log(int level, const char *func, int line, const char *format, ...);


#define LOG_BUG   	0x06
#define LOG_ERR   	0x05
#define LOG_WARN  	0x04
#define LOG_INFO  	0x03
#define LOG_DBG   	0x02

#define LOG_TEST	0x01		/* log print to stdout */
#define LOG_SYS		0x02		/* log write to logfile */

#define ANSI_GREEN      "\033[92m"
#define ANSI_BLUE       "\033[94m"
#define ANSI_RED        "\033[91m"
#define ANSI_MAGENTA    "\033[95m"
#define ANSI_YELLOW     "\033[93m"
#define ANSI_WHITE      "\033[97m"
#define ANSI_CYAN       "\033[36m"
#define ANSI_RESET      "\033[0m"


#define LOG_FILE_SIZE		8092
#define LOG_FILE_PATH		"/home/share/project/awoke_app/log/"
#define LOG_FILE_POSTFIX	".log"

#define log_debug(...)      awoke_log(LOG_DBG,    __func__, __LINE__, __VA_ARGS__)
#define log_info(...)       awoke_log(LOG_INFO,   __func__, __LINE__, __VA_ARGS__)
#define log_err(...)        awoke_log(LOG_ERR,    __func__, __LINE__, __VA_ARGS__)
#define log_warn(...)       awoke_log(LOG_WARN,   __func__, __LINE__, __VA_ARGS__)
#define log_bug(...)        awoke_log(LOG_BUG,    __func__, __LINE__, __VA_ARGS__)

#define log_level(level)			log_level_set(level)
#define log_mode(mode)				log_mode_set(mode)
#define log_id(id)					log_id_set(id)
#define log_set(id, model, level)	_log_set(id, model, level)


#endif /* __AWOKE_LOG_H__ */
