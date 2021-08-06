

#include "qe_core.h"
#include <stdarg.h>


#if (QE_LOG_WITH_COLOR == 1)
static struct qe_log_level qe_level_tab[] = {
	{QE_NULL,	QE_LOG_NONE,		QE_FALSE, 	QE_LOG_COLOR_RESET},
	{"b",		QE_LOG_BURST,		QE_FALSE, 	QE_LOG_COLOR_RESET},
	{"t",		QE_LOG_TRACE,		QE_FALSE, 	QE_LOG_COLOR_WHITE},
	{"D",		QE_LOG_DEBUG,		QE_TRUE,	QE_LOG_COLOR_GREEN_LIGHT},
	{"I",		QE_LOG_INFO,		QE_TRUE,	QE_LOG_COLOR_CYAN_LIGHT},
	{"N",		QE_LOG_NOTICE,		QE_TRUE,	QE_LOG_COLOR_YELLOW_LIGHT},
	{"W",		QE_LOG_WARN,		QE_TRUE,  	QE_LOG_COLOR_PINK_LIGHT},
	{"E",		QE_LOG_ERR,			QE_TRUE,  	QE_LOG_COLOR_RED_LIGHT},
	{"B",		QE_LOG_BUG,			QE_TRUE,  	QE_LOG_COLOR_RED},
};
#else
static struct qe_log_level qe_level_tab[] = {
	{QE_NULL,	QE_LOG_NONE},
	{"b",		QE_LOG_BURST},
	{"t",		QE_LOG_TRACE},
	{"D",		QE_LOG_DEBUG},
	{"I",		QE_LOG_INFO},
	{"N",		QE_LOG_NOTICE},
	{"W",		QE_LOG_WARN},
	{"E",		QE_LOG_ERR},
	{"B", 		QE_LOG_BUG},
};
#endif

#if (QE_LOG_WITH_MDL == 1)
static qe_list_t qe_log_module_list = QE_LIST_OBJECT_INIT(qe_log_module_list);

struct qe_log_mdl *qe_log_mdl_get(const char *mname)
{
	struct qe_log_mdl *mdl;
	struct qe_list_node *node = QE_NULL;

	qe_list_foreach(node, &(qe_log_module_list)) {
		mdl = qe_list_entry(node, struct qe_log_mdl, list);
		if (qe_strncmp(mdl->name, mname, QE_NAME_MAX) == 0) {
			return mdl;
		}
	}

	return QE_NULL;
}

void qe_log_mdl_add(const char *name, qe_uint8_t level)
{
	struct qe_log_mdl *mdl = qe_malloc(sizeof(struct qe_log_mdl));
	qe_assert(mdl != QE_NULL);

	mdl->enable = QE_TRUE;
	mdl->level = level;
	qe_strncpy(mdl->name, name, QE_NAME_MAX);

	qe_list_append(&mdl->list, &qe_log_module_list);
}

void qe_log_mdl_del(const char *mname)
{
	struct qe_log_mdl *mdl;
	struct qe_list_node *node = QE_NULL, *temp = QE_NULL;

	qe_list_foreach_safe(node, temp, &qe_log_module_list) {
		mdl = qe_list_entry(node, struct qe_log_mdl, list);
		if (qe_strncmp(mdl->name, mname, QE_NAME_MAX) == 0) {
			qe_list_unlink(&mdl->list);
			qe_free(mdl);
			break;
		}
	}
}

void qe_log_mdl_enable(const char *mname, qe_bool_t en)
{
	struct qe_log_mdl *mdl;
	struct qe_list_node *node = QE_NULL;

	qe_list_foreach(node, &(qe_log_module_list)) {
		mdl = qe_list_entry(node, struct qe_log_mdl, list);
		if (qe_strncmp(mdl->name, mname, QE_NAME_MAX) == 0) {
			mdl->enable = en;
			break;
		}
	}	
}
#endif

static struct qe_log_context logctx = {
	.level = QE_LOG_TRACE,
	.output = QE_NULL,
	.timestr = QE_NULL,
};

#if (QE_LOG_WITH_TIME == 1)
void qe_log_set_timestr(void (*timestr)(char *buf, int len))
{
	logctx.timestr = timestr;
}
#endif

static qe_bool_t level_visible(qe_uint8_t level)
{
	return (logctx.level <= level);
}

void qe_log_set_output(void (*output)(char *buf, int len))
{
	logctx.output = output;
}

#if (QE_LOG_WITH_MDL == 1)
void qelog(qe_uint8_t level, const char *mname, const char *func, int line, const char *fmt, ...)
#else
void qelog(qe_uint8_t level, const char *func, int line, const char *fmt, ...)
#endif
{
	qe_int32_t n;
	va_list args;
	char buffer[QE_LOG_BZ] = {'\0'};
#if (QE_LOG_WITH_MDL == 1)
	char *m_string;
	struct qe_log_mdl *mp = qe_log_mdl_get(mname);
	if (!mp) {
		m_string = (char *)mname;
		goto __level_check;
	}
	if (!mp->enable) {return;}
	if (level < mp->level) {return;}
	m_string = mp->name;
#endif

__level_check:

	if (!level_visible(level)) {return;}

	qe_strb_t strb = qe_strb_init(buffer, QE_LOG_BZ);
	struct qe_log_level *lp = &qe_level_tab[level];

#if (QE_LOG_WITH_COLOR == 1)
	/* color start */
	if (lp->use_color) {
		qe_strb_format(strb, "%s", lp->color);
	}
#endif

#if (QE_LOG_WITH_TIME == 1)
	if (logctx.timestr) {
		char t_string[32];
		logctx.timestr(t_string, 32);
		qe_strb_format(strb, "[%s] ", t_string);
	}
#endif

#if (QE_LOG_WITH_MDL == 1)
	qe_strb_format(strb, "[%s] [%s] [%s:%d] ",
		m_string, lp->string, func, line);
#else
	qe_strb_format(strb, "[%s] [%s:%d] ", lp->string, func, line);
#endif

	va_start(args, fmt);
	n = qe_vsnprintf(strb.p, strb.max, fmt, args);
	va_end(args);

	strb.p += n;
	strb.max -= n;
	strb.len += n;

#if (QE_LOG_WITH_COLOR == 1)
	/* color end */
	if (lp->use_color) {
		qe_strb_format(strb, "%s", QE_LOG_COLOR_RESET);
	}
#endif

	qe_strb_string(strb, "\n");

	if (logctx.output)
		logctx.output(strb.head, strb.len);
}

void qe_hexdump(qe_uint8_t level, const char *func, int linenr, const void *vbuf, qe_size_t len)
{
	unsigned int n;
	unsigned char *buf = (unsigned char *)vbuf;
	
	if (!level_visible(level))
		return;

	if (!len || !vbuf)
		return;

#if (QE_LOG_WITH_MDL == 1)
	qelog(level, "pkt", func, linenr, "");
#else
	qelog(level, func, linenr, "");
#endif

	for (n = 0; n < len;) {
		unsigned int start = n, m;
		char line[80], *p = line;

		p += qe_snprintf(p, 10, "%04X: ", start);
		for (m = 0; m < 16 && n < len; m++)
			p += qe_snprintf(p, 5, "%02X ", buf[n++]);

		while (m++ < 16)
			p += qe_snprintf(p, 5, "   ");

		p += qe_snprintf(p, 6, "   ");
		for (m = 0; m < 16 && (start + m) < len; m++) {
			if (buf[start + m] >= ' ' && buf[start + m] < 127)
				*p++ = buf[start + m];
			else
				*p++ = '.';
		}

		while (m++ < 16)
			*p++ = ' ';

		*p = '\0';
		
#if (QE_LOG_WITH_MDL == 1)
		qelog(level, "pkt", func, linenr, "%s", line);
#else
		qelog(level, func, linenr, "%s", line);
#endif
		(void)line;
	}	

#if (QE_LOG_WITH_MDL == 1)
	qelog(level, "pkt", func, linenr, "");
#else
	qelog(level, func, linenr, "");
#endif
}

