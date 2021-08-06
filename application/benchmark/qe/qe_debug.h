
#ifndef __QE_DEBUG_H__
#define __QE_DEBUG_H__



#include "qe_config.h"
#include "qe_def.h"




#ifndef QE_LOG_BZ
#define QE_LOG_BZ	256
#endif


#if (QE_LOG_WITH_MDL == 1)
void qelog(qe_uint8_t level, const char *mname, const char *func, int line, const char *fmt, ...);
void qe_log_mdl_add(const char *name, qe_uint8_t level);
void qe_log_mdl_del(const char *mname);
void qe_log_mdl_enable(const char *mname, qe_bool_t en);
#else
void qelog(qe_uint8_t level, const char *func, int line, const char *fmt, ...);
#endif

#if (QE_LOG_WITH_TIME == 1)
void qe_log_set_timestr(void (*timestr)(char *buf, int size));
#endif

void qe_log_set_output(void (*output)(char *buf, int len));
void qe_hexdump(qe_uint8_t level, const char *func, int linenr, const void *vbuf, qe_size_t len);


/**
 * Log Color define
 */
#if (QE_LOG_WITH_COLOR == 1)
#define QE_LOG_COLOR_RESET				"\033[0m"
#define QE_LOG_COLOR_RED				"\033[31m"
#define QE_LOG_COLOR_GREEN				"\033[32m"
#define QE_LOG_COLOR_YELLOW				"\033[33m"
#define QE_LOG_COLOR_BLUE				"\033[34m"
#define QE_LOG_COLOR_PINK				"\033[35m"
#define QE_LOG_COLOR_CYAN				"\033[36m"
#define QE_LOG_COLOR_WHITE				"\033[37m"
#define QE_LOG_COLOR_RED_LIGHT			"\033[91m"
#define QE_LOG_COLOR_GREEN_LIGHT		"\033[92m"
#define QE_LOG_COLOR_YELLOW_LIGHT		"\033[93m"
#define QE_LOG_COLOR_BLUE_LIGHT			"\033[94m"
#define QE_LOG_COLOR_PINK_LIGHT			"\033[95m"
#define QE_LOG_COLOR_CYAN_LIGHT			"\033[96m"
#define QE_LOG_COLOR_WHITE_LIGHT		"\033[97m"
#endif

/**
 * Log Level define
 */
typedef enum {
	QE_LOG_NONE = 0,
	QE_LOG_BURST,
	QE_LOG_TRACE,
	QE_LOG_DEBUG,
	QE_LOG_INFO,
	QE_LOG_NOTICE,
	QE_LOG_WARN,
	QE_LOG_ERR,
	QE_LOG_BUG,
	QE_LOG_DISABLE,	
} qe_log_level_e;

struct qe_log_level {
	const char *string;
	qe_uint8_t level;
#if (QE_LOG_WITH_COLOR == 1)
	qe_bool_t use_color;
	const char *color;
#endif
};

#if (QE_LOG_WITH_MDL == 1)
struct qe_log_mdl {
	char name[QE_NAME_MAX];
	qe_bool_t enable;
	qe_uint8_t level;
	qe_list_t list;
};
#endif

/* Log Context */
struct qe_log_context {
	qe_uint8_t level;
	void (*output)(char *buf, int len);
#if (QE_LOG_WITH_TIME == 1)
	void (*timestr)(char *buf, int size);
#endif
};


#if (QE_LOG_WITH_MDL == 1)
#define qelog_burst(module, ...)	qelog(QE_LOG_BURST, 	module, __FILE__, __LINE__, __VA_ARGS__)
#define qelog_trace(module, ...)	qelog(QE_LOG_TRACE, 	module, __FILE__, __LINE__, __VA_ARGS__)
#define qelog_debug(module, ...)	qelog(QE_LOG_DEBUG, 	module, __FILE__, __LINE__, __VA_ARGS__)
#define qelog_info(module, ...)		qelog(QE_LOG_INFO, 		module, __FILE__, __LINE__, __VA_ARGS__)
#define qelog_notice(module, ...)	qelog(QE_LOG_NOTICE,	module, __FILE__, __LINE__, __VA_ARGS__)
#define qelog_err(module, ...)		qelog(QE_LOG_ERR, 		module, __FILE__, __LINE__, __VA_ARGS__)
#define qelog_warn(module, ...)		qelog(QE_LOG_WARN, 		module, __FILE__, __LINE__, __VA_ARGS__)
#define qelog_bug(module, ...)		qelog(QE_LOG_BUG, 		module, __FILE__, __LINE__, __VA_ARGS__)

#define sys_trace(...)				qelog(QE_LOG_TRACE,		"sys",	__FILE__, __LINE__, __VA_ARGS__)
#define sys_debug(...)				qelog(QE_LOG_TRACE,		"sys",	__FILE__, __LINE__, __VA_ARGS__)
#define sys_info(...)				qelog(QE_LOG_TRACE,		"sys",	__FILE__, __LINE__, __VA_ARGS__)
#define sys_notice(...)				qelog(QE_LOG_TRACE,		"sys",	__FILE__, __LINE__, __VA_ARGS__)
#define sys_err(...)				qelog(QE_LOG_TRACE,		"sys",	__FILE__, __LINE__, __VA_ARGS__)
#define sys_warn(...)				qelog(QE_LOG_TRACE,		"sys",	__FILE__, __LINE__, __VA_ARGS__)

#else
#define qelog_burst(...)			qelog(QE_LOG_BURST, 	__FILE__, __LINE__, __VA_ARGS__)
#define qelog_trace(...)			qelog(QE_LOG_TRACE, 	__FILE__, __LINE__, __VA_ARGS__)
#define qelog_debug(...)			qelog(QE_LOG_DEBUG, 	__FILE__, __LINE__, __VA_ARGS__)
#define qelog_info(...)				qelog(QE_LOG_INFO, 		__FILE__, __LINE__, __VA_ARGS__)
#define qelog_notice(...)			qelog(QE_LOG_NOTICE,	__FILE__, __LINE__, __VA_ARGS__)
#define qelog_err(...)				qelog(QE_LOG_ERR, 		__FILE__, __LINE__, __VA_ARGS__)
#define qelog_warn(...)				qelog(QE_LOG_WARN, 		__FILE__, __LINE__, __VA_ARGS__)
#define qelog_bug(...)				qelog(QE_LOG_BUG, 		__FILE__, __LINE__, __VA_ARGS__)
#endif

#define qe_hexdump_burst(...)		qe_hexdump(LOG_BURST, 	__FILE__, __LINE__, __VA_ARGS__)
#define qe_hexdump_trace(...)		qe_hexdump(LOG_TRACE, 	__FILE__, __LINE__, __VA_ARGS__)
#define qe_hexdump_debug(...)		qe_hexdump(LOG_DBG, 	__FILE__, __LINE__, __VA_ARGS__)
#define qe_hexdump_info(...)		qe_hexdump(LOG_INFO, 	__FILE__, __LINE__, __VA_ARGS__)
#define qe_hexdump_notice(...)		qe_hexdump(LOG_NOTICE, 	__FILE__, __LINE__, __VA_ARGS__)
#define qe_hexdump_err(...)			qe_hexdump(LOG_ERR, 	__FILE__, __LINE__, __VA_ARGS__)
#define qe_hexdump_warn(...)		qe_hexdump(LOG_WARN, 	__FILE__, __LINE__, __VA_ARGS__)
#define qe_hexdump_bug(...)			qe_hexdump(LOG_BUG, 	__FILE__, __LINE__, __VA_ARGS__)

#endif /* __QE_DEBUG_H__ */
