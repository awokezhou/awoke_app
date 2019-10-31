#ifndef __AWOKE_LOG_H__
#define __AWOKE_LOG_H__


void awoke_log(int level, const char *func, int line, const char *format, ...);


#define LOG_BUG   0x05
#define LOG_ERR   0x04
#define LOG_WARN  0x03
#define LOG_INFO  0x02
#define LOG_DBG   0x01

#define LOG_TEST	0x01		/* for log mode */
#define LOG_SYS		0x02

#define ANSI_GREEN      "\033[92m"
#define ANSI_BLUE       "\033[94m"
#define ANSI_RED        "\033[91m"
#define ANSI_MAGENTA    "\033[95m"
#define ANSI_YELLOW     "\033[93m"
#define ANSI_WHITE      "\033[97m"
#define ANSI_RESET      "\033[0m"

#define log_debug(...)      awoke_log(LOG_DBG,    __func__, __LINE__, __VA_ARGS__)
#define log_info(...)       awoke_log(LOG_INFO,   __func__, __LINE__, __VA_ARGS__)
#define log_err(...)        awoke_log(LOG_ERR,    __func__, __LINE__, __VA_ARGS__)
#define log_warn(...)       awoke_log(LOG_WARN,   __func__, __LINE__, __VA_ARGS__)
#define log_bug(...)        awoke_log(LOG_BUG,    __func__, __LINE__, __VA_ARGS__)

#define log_level(level)			log_level_set(level)
#define log_mode(mode)				log_mode_set(mode)



#endif /* __AWOKE_LOG_H__ */
