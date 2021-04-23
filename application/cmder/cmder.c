
#include "cmder.h"
#include "awoke_log.h"
#include "awoke_memory.h"
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "proto-litetalk.h"
#include "awoke_package.h"
#include "awoke_socket.h"
#include "uart.h"


#if defined(CMDER_TEST_STRING_COMMAND)
static err_type hello_get(struct command *cmd);
static err_type hello_set(struct command *cmd, void *buf, int len);
static err_type hello_exec(struct command *cmd);
#endif

#if defined(CMDER_TEST_BYTE_COMMAND)
static err_type world_exec(struct command *cmd);
static err_type bytes1_exec(struct command *cmd);
static err_type bytes2_exec(struct command *cmd);
bool matcher_bytes(struct command_table *tab, void *buf, int len, struct command **cmd);
#endif

#if defined(CMDER_TEST_ECMD_COMMAND)
static err_type ecmd_handler_dfg(struct command *cmd);
static err_type ecmd_handler_zoom(struct command *cmd);
bool ecmd_format_check(void *buf, int len);
bool ecmd_xmatch(struct command_table *tab, void *buf, int len, struct command **cmd);
#endif

#if defined(CMDER_TEST_STRING_COMMAND)
uint8_t macher_string_dispatch(struct command *cmd, void *buf, int len);
bool matcher_string(struct command_table *tab, void *buf, int len, struct command **cmd);

#endif
static err_type litetalk_service_callback(struct cmder_protocol *proto, uint8_t reasons, 
	void *user, void *in, unsigned int length);









#define match_string_make(str)		{MATCH_STRING,	.length=strlen(str),		.val.string=str}
#define match_bytes_make(array)		{MATCH_BYTES,	.length=array_size(array),	.val.bytes=array}
#define match_ecmd_make(e)			{MATCH_CUSTOM,	.length=1,					.val.struc=(void *)&e}


#if defined(CMDER_TEST_STRING_COMMAND)
static struct command test_string_commands[] = {
	{match_string_make("hello"),	hello_get,	hello_set,	hello_exec},
	{match_string_make("world"),	NULL,	NULL,	world_exec},
};

static struct command_matcher string_matcher = {
	.match = matcher_string,
	.dispatch = macher_string_dispatch,
};

#endif

#if defined(CMDER_TEST_BYTE_COMMAND)
static uint8_t testbytes[2][3] = {
	{0x11, 0x22, 0x33},
	{0x21, 0x22, 0x23},
};

static struct command test_bytes_commands[] = {
	{match_bytes_make(testbytes[0]), NULL, NULL, bytes1_exec},
	{match_bytes_make(testbytes[1]), NULL, NULL, bytes2_exec},
};

static struct command_matcher bytes_matcher = {
	.match = matcher_bytes,
};
#endif

#if defined(CMDER_TEST_ECMD)
struct ecmd_match {
	uint8_t code;
	int need;
};

static struct ecmd_match ecmdsx[] = {
	{.code=0x37, .need=6},
	{.code=0x46, .need=6},
};

static struct command test_ecmds[] = {
	{match_ecmd_make(ecmdsx[0]), NULL, NULL, ecmd_handler_dfg},
	{match_ecmd_make(ecmdsx[1]), NULL, NULL, ecmd_handler_zoom},
};

static struct command_matcher ecmd_matcher = {
	.format = ecmd_format_check,
	.match = ecmd_xmatch,
};

#endif


#if defined(CMDER_TEST_STRING_COMMAND)
bool matcher_string(struct command_table *tab, void *buf, int len, struct command **cmd)
{
	bool find = FALSE;
	struct command *p;
	const char *string = (const char *)buf;

	array_foreach_start(tab->commands, tab->nr_commands, p) {

		if (!strncmp(string, p->match.val.string, p->match.length)) {
			find = TRUE;
			break;
		}
		
	} array_foreach_end();

	if (find) {
		*cmd = p;
	}

	return find;
}

uint8_t macher_string_dispatch(struct command *cmd, void *buf, int len)
{
	const char *string = (const char *)buf;

	if (strlen(string) < cmd->match.length) {
		return 99;
	}

	if (strlen(string) == cmd->match.length) {
		//cmder_debug("exec");
		return 0;
	}

	if ((string[cmd->match.length] == '=') && (string[cmd->match.length+1]== '?')) {
		//cmder_debug("get");
		return 1;
	}

	if (string[cmd->match.length] == '=') {
		//cmder_debug("set");
		return 2;
	}

	if (string[cmd->match.length] == '?') {
		//cmder_debug("help");
		return 3;
	}

	return 99;
}
#endif

#if defined(CMDER_TEST_BYTE_COMMAND)
bool matcher_bytes(struct command_table *tab, void *buf, int len, struct command **cmd)
{
	int i;
	bool find = FALSE;
	struct command *p;
	uint8_t *pos = (uint8_t *)buf;

	array_foreach_start(tab->commands, tab->nr_commands, p) {

		for (i=0; i<p->match.length; i++) {
			if (pos[i] != p->match.val.bytes[i]) {
				break;
			}
		}

		if (i != p->match.length) {
			continue;
		} else {
			find = TRUE;
			break;
		}
	} array_foreach_end();

	if (find) {
		*cmd = p;
	}

	return find;
}
#endif

#if defined(CMDER_TEST_ECMD_COMMAND)
bool ecmd_format_check(void *buf, int len)
{
	int length;
	uint16_t prefix;
	uint8_t *head = (uint8_t *)buf;
	uint8_t *pos = (uint8_t *)buf;
	bool find_postfix = FALSE;
	
	if (len < 6) {
		return FALSE;
	}
	
	prefix = (head[0]<<8)|head[1];
	if (prefix != 0x8101) {
		return FALSE;
	}

	/* find postfix */
	while (pos != (head+len)) {
		if (*pos == 0xff) {
			find_postfix = TRUE;
			break;
		}
		pos++;
	}

	if (!find_postfix) {
		return FALSE;
	}

	length = pos-head+1;
	if (length < 6) {
		return FALSE;
	}

	return TRUE;
}

bool ecmd_xmatch(struct command_table *tab, void *buf, int len, struct command **cmd)
{
	bool find = FALSE;
	struct command *p;
	struct ecmd_match *m;
	uint8_t *pos = (uint8_t *)buf;

	array_foreach_start(tab->commands, tab->nr_commands, p) {

		m = (struct ecmd_match *)p->match.val.struc;
		if ((m->code == pos[2]) && (m->need == len)) {
			find = TRUE;
			break;
		}
	} array_foreach_end();

	if (find) {
		*cmd = p;
	}

	return find;
}
#endif

#if defined(CMDER_TEST_STRING_COMMAND)
static err_type hello_exec(struct command *cmd)
{
	cmder_notice("hello exec");
	return et_ok;
}

static err_type hello_get(struct command *cmd)
{
	cmder_notice("hello get");
	return et_ok;
}

static err_type hello_set(struct command *cmd, void *buf, int len)
{
	return et_ok;
}
#endif

#if defined(CMDER_TEST_BYTE_COMMAND)
static err_type world_exec(struct command *cmd)
{
	cmder_notice("world");
	return et_ok;
}

static err_type bytes1_exec(struct command *cmd)
{
	cmder_notice("bytes1");
	return et_ok;
}

static err_type bytes2_exec(struct command *cmd)
{
	cmder_notice("bytes2");
	return et_ok;
}
#endif

#if defined(CMDER_TEST_ECMD_COMMAND)
static err_type ecmd_handler_dfg(struct command *cmd)
{
	cmder_notice("dfg");
	return et_ok;
}

static err_type ecmd_handler_zoom(struct command *cmd)
{
	cmder_notice("zoom");
	return et_ok;
}
#endif

#if defined(CMDER_TEST_STRING_COMMAND)
static err_type test_string_tx(struct cmder *c)
{
	return et_ok;	
}
#endif

#if defined(CMDER_TEST_BYTE_COMMAND)
static err_type test_bytes_tx(struct cmder *c)
{
	return et_ok;	
}
#endif

static err_type uart_rx(struct command_transceiver *tcvr, void *data)
{
	err_type ret;
	int c;
	awoke_buffchunk *chunk, *buffer;
	struct cmder_protocol *proto;
	struct uartcmder *uc = (struct uartcmder *)data;

	buffer = tcvr->rxbuffer;

	c = read(tcvr->fd, buffer->p+buffer->length, buffer->size);
	if (c > 0) {
		awoke_hexdump_trace(buffer->p+buffer->length, c);
		buffer->length += c;
		cmder_trace("read %d total:%d", c, buffer->length);
		
		if (uc->base.nr_protocols) {
			list_foreach(proto, &uc->base.protocols, _head) {
				ret = proto->match(proto, buffer->p, buffer->length);
				if (ret == err_unfinished) {
					return et_ok;
				}
			}
		}
		if (et_ok != ret) {
			return et_ok;
		}

		chunk = awoke_buffchunk_create(buffer->length);
		if (!chunk) {
			cmder_err("buffer create error");
			return err_oom;
		}
		chunk->id = tcvr->id;
		awoke_buffchunk_copy(chunk, buffer);
		awoke_buffchunk_clear(tcvr->rxbuffer);
		awoke_minpq_insert(uc->rxqueue, &chunk, tcvr->id);
	}

	return et_ok;
}

static err_type uart_tx(struct command_transceiver *tcvr, void *buf, int len)
{
	int c;

	//awoke_hexdump_trace(buf, len);

	c = write(tcvr->fd, buf, len);
	if (c <= 0) {
		cmder_err("tx error");
		return err_send;
	}

	return et_ok;
}

static err_type tcp_rx(struct command_transceiver *tcvr, void *data)
{
	int n, maxfd;
	err_type ret = et_ok;
	int c;
	awoke_buffchunk *chunk, *buffer;
	struct timeval tv;
	fd_set readfds;
	struct cmder_protocol *proto;
	struct uartcmder *uc = (struct uartcmder *)data;

	buffer = tcvr->rxbuffer;

	FD_ZERO(&readfds);

	FD_SET(tcvr->fd, &readfds);
	if (tcvr->connfd > 0) {
		FD_SET(tcvr->connfd, &readfds);
		maxfd = tcvr->connfd + 1;
	} else {
		maxfd = tcvr->fd + 1;
	}

    tv.tv_sec = 0;
    tv.tv_usec = 1000;

	n = select(maxfd, &readfds, NULL, NULL, &tv);
	//cmder_debug("select n:%d", n);
	if (n <= 0) {
		return et_ok;
	}

	if ((tcvr->connfd > 0) && (FD_ISSET(tcvr->connfd, &readfds))) {
		c = recv(tcvr->connfd, buffer->p+buffer->length, buffer->size, 0);
		if (c > 0) {
			//awoke_hexdump_trace(buffer->p+buffer->length, c);
			buffer->length += c;
			cmder_trace("read %d total:%d", c, buffer->length);

			if (uc->base.nr_protocols) {
				list_foreach(proto, &uc->base.protocols, _head) {
					ret = proto->match(proto, buffer->p, buffer->length);
					if (ret == err_unfinished) {
						return et_ok;
					}
				}
			}
			if (ret != et_ok) {
				return et_ok;
			}
	
			chunk = awoke_buffchunk_create(buffer->length);
			if (!chunk) {
				cmder_err("buffer create error");
				return err_oom;
			}
			chunk->id = tcvr->id;
			awoke_buffchunk_copy(chunk, buffer);
			awoke_buffchunk_clear(tcvr->rxbuffer);
			awoke_minpq_insert(uc->rxqueue, &chunk, tcvr->id);
		}
	} else if (FD_ISSET(tcvr->fd, &readfds)) {
		if (tcvr->connfd > 0) {
			cmder_warn("shutdown old:%d", tcvr->connfd);
			close(tcvr->connfd);
		}
		tcvr->connfd = awoke_socket_accpet(tcvr->fd);
		cmder_debug("new accept:%d", tcvr->connfd);
	}

	return et_ok;
}

static err_type tcp_tx(struct command_transceiver *tcvr, void *buf, int len)
{
	int c;

	//awoke_hexdump_trace(buf, len);

	c = send(tcvr->connfd, buf, len, 0);
	if (c <= 0) {
		cmder_err("tx error");
		return err_send;
	}

	return et_ok;
}

struct command_transceiver *cmder_find_tcvr_byid(struct cmder *c, uint8_t id)
{
	struct command_transceiver *p;

	list_foreach(p, &c->transceiver_list, _head) {
		if (p->id == id) {
			return p;
		}
	}

	return NULL;
}

static err_type cmder_uart_tcvr_claim(struct cmder *c, uint8_t id)
{
	int fd;
	bool find = FALSE;
	struct command_transceiver *p;
	
	list_foreach(p, &c->transceiver_list, _head) {
		if (p->id == id) {
			find = TRUE;
			break;
		}
	}

	if (!find) {
		cmder_err("tcvr[%d] not find", id);
		return err_notfind;
	}

	if (p->fd != -1) {
		cmder_err("tcvr[%d] already open", id);
		return et_noeed;
	}

	fd = uart_claim(p->port, 115200, 8, 1, 'N');
	if (fd < 0) {
		cmder_err("tcvr[%d] open error", id);
		return err_open;
	}

	p->fd = fd;

	cmder_debug("tcvr[%d] claim ok", id);
	return et_ok;
}

static err_type cmder_tcp_tcvr_claim(struct cmder *c, uint8_t id)
{
	int fd;
	struct command_transceiver *p;
	
	list_foreach(p, &c->transceiver_list, _head) {
		if (p->id == id) {
			break;
		}
	}

	fd = awoke_socket_server(SOCK_INTERNET_TCP, "localhost", 5567, FALSE);
	if (fd < 0) {
		cmder_err("socket create error");
		return et_sock_creat;
	}
	
 	p->fd = fd;
	p->connfd = -1;

	cmder_debug("tcvr[%d:%d] claim ok", id, fd);
	
	return et_ok;
}

static err_type cmder_tcvr_register(struct cmder *c, uint8_t id, const char *port, 
	err_type (*rx)(struct command_transceiver *, void *), 
	err_type (*tx)(struct command_transceiver *, void *, int ),
	int bufsize)
{
	struct command_transceiver *p, *n;

	if (!c || !port || !rx || !tx) {
		return err_param;
	}

	list_foreach(p, &c->transceiver_list, _head) {
		if (p->id == id) {
			return err_exist;
		}
	}

	n = mem_alloc_z(sizeof(struct command_transceiver));
	if (!n) {return err_oom;}

	n->fd = -1;
	n->id = id;
	n->rx = rx;
	n->tx = tx;
	n->port = awoke_string_dup(port);

	if (bufsize) {
		n->rxbuffer = awoke_buffchunk_create(bufsize);
		if (!n->rxbuffer) {
			mem_free(n);
			return err_oom;
		}
	}

	c->nr_transceivers++;
	list_append(&n->_head, &c->transceiver_list);

	cmder_debug("tcvr(%d-%s) registed", id, port);

	return et_ok;
}

static err_type cmder_tab_register(struct cmder *c, const char *name, struct command *commands, 
	int size, struct command_matcher *matcher)
{
	struct command_table *p, *n;
		
	if (!c || !name || !commands || !matcher) {
		return et_param;
	}

	list_for_each_entry(p, &c->tabs, _head) {
		if (!strcmp(name, p->name)) {
			return et_exist;
		}
	}

	n = mem_alloc_z(sizeof(struct command_table));
	if (!n) {cmder_err("command table alloc error");return et_nomem;}

	n->name = awoke_string_dup(name);
	n->matcher = matcher;
	n->commands = commands;
	n->nr_commands = size;

	list_append(&n->_head, &c->tabs);

	return et_ok;
}

static void cmder_static_init(struct uartcmder *c)
{
	struct cmder_config *cfg = &c->config;
	
	cfg->ae_enable = 0;
	cfg->ae_expo_enable = 0;
	cfg->ae_gain_enable = 0;
	cfg->gain = 0;
	cfg->gain_min = 0;
	cfg->gain_max = 500;
	cfg->expo = 1600;
	cfg->goal_max = 85;
	cfg->goal_min = 65;
	cfg->ae_frame = 4;

	cfg->fps = 100;
	cfg->expo_min = 936/cfg->fps + 7;
	cfg->expo_max = 936*1053/cfg->fps + 7;
	cfg->fps_min = 1;
	cfg->fps_max = 125;
	cfg->cltest_enable = 0;
	cfg->frtest_enable = 0;
	cfg->crossview_enable = 0;
	cfg->crossview_point.x = 640;
	cfg->crossview_point.y = 540;
	cfg->vinvs = 0;
	cfg->hinvs = 0;
}

static err_type cmder_init(struct uartcmder *c, const char *port)
{
	struct command_transceiver *transceiver;

	list_init(&c->base.tabs);
	c->base.nr_tabs = 0;

	list_init(&c->base.protocols);
	c->base.nr_protocols = 0;

	list_init(&c->base.transceiver_list);
	c->base.nr_transceivers = 0;

#if defined(CMDER_TEST_STRING_COMMAND)
	cmder_tab_register(&c->base, "string-tab", test_string_commands,
		array_size(test_string_commands), &string_matcher);
#endif

#if defined(CMDER_TEST_BYTE_COMMAND)
	cmder_tab_register(&c->base, "bytes-tab", test_bytes_commands,
		array_size(test_bytes_commands), &bytes_matcher);
#endif

#if defined(CMDER_TEST_ECMD_COMMAND)
	cmder_tab_register(&c->base, "ecmds", test_ecmds,
		array_size(test_ecmds), &ecmd_matcher);*/
#endif

#if defined(CMDER_TCVR_UART)
	cmder_tcvr_register(&c->base, 0, port, uart_rx, uart_tx, 1024);
#endif

	cmder_tcvr_register(&c->base, 0, "localhost", tcp_rx, tcp_tx, 1024);

	/*
	swift_protocol.callback = switf_service_callback;
	list_append(&swift_protocol._head, &c->base.protocols);
	swift_protocol.callback(&swift_protocol, SWIFT_CALLBACK_PROTOCOL_INIT, NULL, NULL, 0);
	*/

	litetalk_protocol.callback = litetalk_service_callback;
	list_append(&litetalk_protocol._head, &c->base.protocols);
	c->base.nr_protocols++;
	litetalk_protocol.context = c;
	litetalk_protocol.callback(&litetalk_protocol, 
		LITETALK_CALLBACK_PROTOCOL_INIT, NULL, NULL, 0x0);
	
	c->rxqueue = awoke_minpq_create(sizeof(awoke_buffchunk *), 8, NULL, 0x0);
	c->txqueue = awoke_minpq_create(sizeof(awoke_buffchunk *), 8, NULL, 0x0);

	cmder_static_init(c);

#if defined(CMDER_TCVR_UART)
	cmder_uart_tcvr_claim(&c->base, 0);
#endif

	cmder_tcp_tcvr_claim(&c->base, 0);

	c->flashchunk = awoke_buffchunk_create(1024*64);
	if (!c->flashchunk) {
		cmder_err("alloc flashbuff error");
	} else {
		cmder_debug("flash space: %d bytes", c->flashchunk->size);
	}
	
	return et_ok;
}

static err_type cmder_command_recv(struct uartcmder *uc)
{
	err_type ret;
	struct cmder *base = &uc->base;
	struct command_transceiver *tcvr;
	
	list_foreach(tcvr, &base->transceiver_list, _head) {
		//cmder_debug("tcvr[%d]", tcvr->id);
		ret = tcvr->rx(tcvr, uc);
		if (ret != et_ok) {
			cmder_err("rx error:%d", ret);
			return ret;
		}
	}

	return et_ok;
}

static struct command *command_get(struct uartcmder *uc, awoke_buffchunk *chunk, uint8_t *operate)
{
	struct cmder *base = &uc->base;
	struct command *cmd;
	struct command_table *table;
	struct command_matcher *matcher;

	awoke_hexdump_trace(chunk->p, chunk->length);
 
	list_for_each_entry(table, &base->tabs, _head) {
		cmder_trace("table:%s tid:%d chunkid:%d", table->name, table->tid, chunk->id);

		if (table->tid != chunk->id) {
			//ucmder_trace("not match tid");
			continue;
		}
		
		matcher = table->matcher;
		if (matcher->format) {
			if (!matcher->format(chunk->p, chunk->length)) {
				continue;
			}
		}
		if (matcher->match(table, chunk->p, chunk->length, &cmd)) {
			//cmder_info("command matched");
			if (matcher->dispatch) {
				*operate = matcher->dispatch(table, chunk->p, chunk->length);
			} else {
				*operate = 0;
			}
			return cmd;
		}
	}

	return NULL;
}

static err_type cmder_command_exec(struct uartcmder *uc)
{
	err_type ret;
	int prior_nouse;
	uint8_t operation;
	awoke_buffchunk *chunk, *tx;
	struct cmder *c = &uc->base;
	struct command *cmd;

	if (awoke_minpq_empty(uc->rxqueue)) {
		return et_ok;
	}

	awoke_minpq_delmin(uc->rxqueue, &chunk, &prior_nouse);

	if (!c->nr_tabs) {
		awoke_buffchunk_free(&chunk);
		return et_ok;
	}
		
	cmd = command_get(c, chunk, &operation);
	if (!cmd) {
		cmder_trace("command not find");
		awoke_minpq_insert(uc->rxqueue, &chunk, 0);
		return et_ok;
	}

	tx = awoke_buffchunk_create(256);
	tx->id = chunk->id;
	
	switch (operation) {

	case 0:
		if (cmd->exe) {ret = cmd->exe(cmd);}
		break;

	case 1:
		if (cmd->get) {ret = cmd->get(cmd);}
		break;

	case 2:
		if (cmd->set) {ret = cmd->set(cmd, chunk->p, chunk->length);}
		break;

	case 3:
		if (cmd->help) {ret = cmd->help(cmd);}
		break;
	}
	
	if (!cmd) {
		cmder_err("exec error:%d", ret);
		awoke_buffchunk_free(&chunk);
		awoke_buffchunk_free(&tx);
		return ret;
	}

	awoke_buffchunk_free(&chunk);

	awoke_minpq_insert(uc->txqueue, &tx, 0);
	
	return et_ok;
}

static err_type cmder_command_send(struct uartcmder *uc)
{
	err_type ret;
	int nouse;
	awoke_buffchunk *chunk;
	struct command_transceiver *tcvr;

	if (awoke_minpq_empty(uc->txqueue)) {
		return et_ok;
	}

	awoke_minpq_delmin(uc->txqueue, &chunk, &nouse);
	tcvr = cmder_find_tcvr_byid(&uc->base, chunk->id);

	//cmder_trace("data(%d) need send", chunk->length);
	ret = tcvr->tx(tcvr, chunk->p, chunk->length);
	if (ret != et_ok) {
		cmder_err("send error:%d", ret);
		awoke_buffchunk_free(&chunk);
		return ret;
	}
	//cmder_trace("data(%d) send ok", chunk->length);

	awoke_buffchunk_free(&chunk);

	return et_ok;
}

static err_type litk_stream_write_file(const char *filename, const *mode, void *buf, int length)
{
	FILE *fp;
	
	fp = fopen(filename, mode);
	if (!fp) {
		cmder_err("open file error");
		return err_fail;
	}

	fwrite(buf, length, 1, fp);

	fclose(fp);

	return et_ok;
}

static err_type litk_stream_write(struct uartcmder *uc, uint32_t addr,
	uint8_t *buf, int len)
{
	return et_ok;
}

static err_type normal_checksum(uint8_t *buf, int len, uint8_t mask)
{
	uint8_t *pos;
	uint8_t checksum8;
	uint32_t marker;
	uint32_t checksum32, sum32;

	pos = buf;

	if (mask_exst(mask, CMDER_CHECKSUM_MARKER)) {
		pkg_pull_dwrd(marker, pos);
		if (marker != USER_MARK) {
			cmder_err("marker error:0x%x", marker);
			return err_param;
		}
	}

	if (mask_exst(mask, CMDER_CHECKSUM_NOR8)) {
		checksum8 = awoke_checksum_8(buf, len-1);
		if (checksum8 != buf[len-1]) {
			cmder_err("checksum error:0x%x 0x%x", buf[len-1], checksum8);
			return err_checksum;
		}
	} else if (mask_exst(mask, CMDER_CHECKSUM_NOR32)) {
		checksum32 = awoke_checksum_32(buf, len-4);
		pos = &(buf[len-4]);
		pkg_pull_dwrd(sum32, pos);
		sum32 = awoke_htonl(sum32);
		if (checksum32 != sum32) {
			cmder_err("checksum error:0x%x 0x%x", sum32, checksum32);
			return err_checksum;
		}
	}

	return et_ok;	
}

static err_type litk_stream_merge(struct cmder_protocol *proto, struct litk_private *priv)
{
	err_type ret;
	awoke_buffchunk *merge;
	struct uartcmder *uc = proto->context;
	struct _awoke_buffchunk_pool *pool = &priv->streampool;

	merge = awoke_buffchunk_pool2chunk(pool);
	
	switch (priv->streaminfo.media) {

	case LITETALK_MEDIA_DEFCONFIG:
		cmder_info("media: defconfig");
		ret = normal_checksum((uint8_t *)merge->p, merge->length, 
			CMDER_CHECKSUM_MARKER|CMDER_CHECKSUM_NOR8);
		if (ret != et_ok) {
			cmder_err("checkerr:%d", ret);
			break;
		}
		ret = litk_stream_write(uc, 0x00001000, (uint8_t *)merge->p, merge->length);
		break;

	case LITETALK_MEDIA_USRCONFIG:
		cmder_info("media: usrconfig");
		ret = normal_checksum((uint8_t *)merge->p, merge->length, 
			CMDER_CHECKSUM_MARKER|CMDER_CHECKSUM_NOR8);
		if (ret != et_ok) {
			cmder_err("checkerr:%d", ret);
			break;
		}
		ret = litk_stream_write(uc, 0x00002000, (uint8_t *)merge->p, merge->length);
		break;

	case LITETALK_MEDIA_SENSORCFG:
		cmder_info("media: sensorcfg");
		ret = normal_checksum((uint8_t *)merge->p, merge->length, 
			CMDER_CHECKSUM_MARKER|CMDER_CHECKSUM_NOR8);
		if (ret != et_ok) {
			cmder_err("checkerr:%d", ret);
			break;
		}
		ret = litk_stream_write(uc, 0x00003000, (uint8_t *)merge->p, merge->length);
		break;

	case LITETALK_MEDIA_DPCPARAMS:
		cmder_info("media: dpcparams");
		ret = normal_checksum((uint8_t *)merge->p, merge->length, 
			CMDER_CHECKSUM_NOR8);
		if (ret != et_ok) {
			cmder_err("checkerr:%d", ret);
			break;
		}
		ret = litk_stream_write(uc, 0x00004000, (uint8_t *)merge->p, merge->length);
		break;

	case LITETALK_MEDIA_NUCPARAMS:

		/*
		 * start: 0x00010000
		 * sectors: 1280 4096
		 * end: 0x0050F000
		 */
		
		cmder_info("media: nucparams");
		cmder_info("addr:0x%x size:0x%x", uc->nucaddr, merge->length);
		if (merge->length == 0x1000) {
			uc->nuc_checksum += awoke_checksum_32((uint8_t *)merge->p, merge->length);
		} else {
			uc->nuc_checksum += awoke_checksum_32((uint8_t *)merge->p, merge->length-4);
		}
		break;

	default:
		cmder_err("unknown media:%d", priv->streaminfo.media);
		break;
	}

	awoke_buffchunk_pool_clean(pool);
	
	return et_ok;
}

static err_type litetalk_stream_process(struct cmder_protocol *proto, 
	void *user, void *in, int length)
{
	int remain;
	uint8_t code = 0;
	err_type ret = et_ok;
	awoke_buffchunk *rbx, *rsp;
	struct litk_streaminfo *info, *p;
	struct uartcmder *uc = proto->context;
	struct litk_private *priv = proto->private;
	struct _awoke_buffchunk_pool *pool = &priv->streampool;

	p = &priv->streaminfo;
	info = (struct litk_streaminfo *)user;

stream_transmit_start:
	if (!p->id) {
		/* streaminfo init */
		cmder_debug("stream[%d] init: media:%d totalsize:%d", 
			info->id, info->media, info->totalsize);
		memcpy(p, info, sizeof(struct litk_streaminfo));
		p->recvbytes = 0;
		uc->nucaddr = 0x0;
		uc->nuc_checksum = 0x0;
	} else if ((p->id != info->id) || (p->media != info->media)) {
		cmder_warn("stream[%d] break", p->id);
		awoke_buffchunk_pool_clean(pool);
		goto stream_transmit_start;
	}


	/* if recv old index, drop */
	if ((p->index) && (p->index != (info->index-1))) {
		cmder_err("recv old index");
		code = LITETALK_CODE_ERROR;
		goto  stream_transmit_ack;
	} else {
		p->index = info->index;
	}


	remain = pool->maxsize - pool->size;
	if (remain < info->length) {
		cmder_debug("pool not enough, merge");
		if (!uc->nucaddr) {
			uc->nucaddr = 0x00010000;
		}
		ret = litk_stream_merge(proto, priv);
		uc->nucaddr += 0x1000;
	}

	/* chunk recv */
	rbx = awoke_buffchunk_create(info->length);	
	awoke_buffchunk_write(rbx, in, info->length, TRUE);
	awoke_buffchunk_pool_chunkadd(pool, rbx);
	p->recvbytes += rbx->length;
		
	cmder_info("stream[%d:%d] length:%d %d/%d",
		p->id,
		p->index, 
		length,
		p->recvbytes,
		p->totalsize);


	/* if pool size not enough or receive finish, merge it */
	/*
	remain = pool->maxsize - pool->size;
	if (remain < info->length) {
		cmder_debug("pool not enough, merge");
		if (!uc->nucaddr) {
			uc->nucaddr = 0x00010000;
		}
		ret = litk_stream_merge(proto, priv);
		uc->nucaddr += 0x1000;
	} else if (p->recvbytes == p->totalsize) {
		cmder_debug("stream[%d] finish, merge", p->id);
		ret = litk_stream_merge(proto, priv);
		cmder_debug("stream[%d] clean", p->id);
		memset(p, 0x0, sizeof(struct litk_streaminfo));
		cmder_info("nuc checksum:0x%x", uc->nuc_checksum);
	}*/

	if (p->recvbytes == p->totalsize) {
		cmder_debug("stream[%d] finish, merge", p->id);
		ret = litk_stream_merge(proto, priv);
		cmder_debug("stream[%d] clean", p->id);
		memset(p, 0x0, sizeof(struct litk_streaminfo));
		cmder_info("nuc checksum:0x%x", uc->nuc_checksum);
	}
	code = (uint8_t)ret;


stream_transmit_ack:
	rsp = awoke_buffchunk_create(32);
	litetalk_build_stream_ack(rsp, info->id, info->index, code);
	rsp->id = 0;
	awoke_minpq_insert(uc->txqueue, &rsp, 0);
	
	return et_ok;
}

static err_type litetalk_filedown_finish_process(struct cmder_protocol *proto, 
	void *user, void *in, int length)
{
	int c;
	FILE *fp;
	awoke_buffchunk *chunk, *fc;
	struct uartcmder *uc = proto->context;

	if (!uc->bpool_filechunk) {
		uc->bpool_filechunk = awoke_buffchunk_pool_create(8092);
	}

	chunk = awoke_buffchunk_create(length);
	memcpy(chunk->p, in, length);
	chunk->length = length;
	awoke_buffchunk_pool_chunkadd(uc->bpool_filechunk, chunk);
	fc = awoke_buffchunk_pool2chunk(uc->bpool_filechunk);
	awoke_buffchunk_pool_free(&uc->bpool_filechunk);

	fp = fopen("test.txt", "w+");
	if (!fp) {
		cmder_err("open file error");
		return err_fail;
	}

	c = fwrite(fc->p, fc->length, 1, fp);

	fclose(fp);

	return et_ok;
}

static err_type litetalk_command_process(struct cmder_protocol *proto, 
	void *user, void *in, int length)
{
	err_type ret;
	awoke_buffchunk *txpkt;
	struct litetalk_cmd *p;
	struct litetalk_cmdinfo *info;
	struct litk_private *pri = proto->private;
	struct uartcmder *uc = proto->context;

	info = (struct litetalk_cmdinfo *)user;
	txpkt = awoke_buffchunk_create(256);

	cmder_trace("info->cmdtype %d", info->cmdtype);

	array_foreach(pri->cmdlist, pri->cmdlist_nr, p) {
		if (info->cmdtype == p->type) {
			if (mask_exst(info->flag, LITETALK_CMD_F_WRITE)) {
				ret = p->set(proto, info, in, length);
				if (ret != et_ok) {
					cmder_err("cmd error:%d", ret);
					txpkt->length = litetalk_build_cmderr(txpkt->p, info, ret);
					break;
				}
				ret = p->get(proto, info, txpkt);
			} else {
				ret = p->get(proto, info, txpkt);
			}
			break;
		}
	};
	
	txpkt->id = 0;
	awoke_minpq_insert(uc->txqueue, &txpkt, 0);

	return et_ok;
}

static err_type ltk_sensor_reg_set(struct cmder_protocol *proto, 
	struct litetalk_cmdinfo *info, void *in, int length)
{
	uint8_t *pos = (uint8_t *)in;
	struct uartcmder *uc = proto->context;

	pkg_pull_word(uc->regaddr, pos);
	uc->regaddr = htons(uc->regaddr);
	pkg_pull_byte(uc->regvalue, pos);

	cmder_debug("addr:0x%x value:%d", uc->regaddr, uc->regvalue);

	return et_ok;
}

static err_type ltk_sensor_reg_get(struct cmder_protocol *proto, 
	struct litetalk_cmdinfo *info, awoke_buffchunk *chunk)
{
	uint8_t code = 0;
	uint8_t *head = (uint8_t *)chunk->p + LITETALK_HEADERLEN;
	uint8_t *pos = head;
	uint16_t addr;
	struct uartcmder *uc = proto->context;

	pkg_push_byte(info->cmdtype, pos);
	pkg_push_byte(code, pos);

	addr = htons(uc->regaddr);
	pkg_push_word(addr, pos);
	pkg_push_byte(uc->regvalue, pos);

	litetalk_pack_header(chunk->p, LITETALK_CATEGORY_COMMAND, pos-head);

	chunk->length = pos-head+LITETALK_HEADERLEN;

	return et_ok;
}

static err_type ltk_exposure_set(struct cmder_protocol *proto, 
	struct litetalk_cmdinfo *info, void *in, int length)
{
	uint8_t *pos = (uint8_t *)in;
	struct uartcmder *uc = proto->context;
	struct cmder_config *cfg = &uc->config;
	struct ltk_exposure exposure;

	memset(&exposure, 0x0, sizeof(exposure));

	pkg_pull_word(exposure.gain, pos);
	exposure.gain = awoke_htons(exposure.gain);
	pos += 4;	// skip gain min/max
	pkg_pull_dwrd(exposure.expo, pos);
	exposure.expo = awoke_htonl(exposure.expo);
	pos += 8; 	// skip expo min/max
	pkg_pull_byte(exposure.ae_enable, pos);
	pkg_pull_byte(exposure.goal_max, pos);
	pkg_pull_byte(exposure.goal_min, pos);
	pkg_pull_byte(exposure.ae_frame, pos);

	cmder_info("");
	cmder_info("---- exposure set ----");
	cmder_info("gain:%d min:%d max:%d", exposure.gain, 
		exposure.gain_min, exposure.gain_max);
	cmder_info("expo:%d min:%d max:%d", exposure.expo, 
		exposure.expo_min, exposure.expo_max);
	cmder_info("goal min:%d max:%d", exposure.goal_min, exposure.goal_max);
	cmder_info("ae enable:%d", exposure.ae_enable);
	cmder_info("ae frame:%d", exposure.ae_frame);
	cmder_info("---- exposure set ----");
	cmder_info("");


	if (exposure.gain != cfg->gain) {
		cfg->gain = exposure.gain;
	}

	if (exposure.expo != cfg->expo) {
		cfg->expo = exposure.expo;
	}

	if (cfg->ae_enable != exposure.ae_enable) {
		cfg->ae_enable = exposure.ae_enable;
	}

	
	return et_ok;
}

static err_type ltk_exposure_get(struct cmder_protocol *proto, 
	struct litetalk_cmdinfo *info, awoke_buffchunk *chunk)
{
 	uint8_t code = 0;
	uint8_t *head = (uint8_t *)chunk->p + LITETALK_HEADERLEN;
	uint8_t *pos = head;
	struct ltk_exposure exposure;
	struct uartcmder *uc = proto->context;
	struct cmder_config *cfg = &uc->config;

	exposure.gain = awoke_htons(cfg->gain);
	exposure.gain_min = awoke_htons(cfg->gain_min);
	exposure.gain_max = awoke_htons(cfg->gain_max);
	exposure.expo = awoke_htonl(cfg->expo);
	exposure.expo_min = awoke_htonl(cfg->expo_min);
	exposure.expo_max = awoke_htonl(cfg->expo_max);
	exposure.ae_enable = cfg->ae_enable;
	exposure.goal_max = cfg->goal_max;
	exposure.goal_min = cfg->goal_min;
	exposure.ae_frame = cfg->ae_frame;

	cmder_info("");
	cmder_info("---- exposure get ----");
	cmder_info("gain:%d min:%d max:%d", cfg->gain, cfg->gain_min, cfg->gain_max);
	cmder_info("expo:%d min:%d max:%d", cfg->expo, cfg->expo_min, cfg->expo_max);
	cmder_info("ae_enable:%d", cfg->ae_enable);
	cmder_info("goal_max:%d", cfg->goal_max);
	cmder_info("goal_min:%d", cfg->goal_min);
	cmder_info("ae_frame:%d", cfg->ae_frame);
	cmder_info("---- exposure get ----");
	cmder_info("");

	pkg_push_byte(info->cmdtype, pos);
	pkg_push_byte(code, pos);
	pkg_push_word(exposure.gain, pos);
	pkg_push_word(exposure.gain_min, pos);
	pkg_push_word(exposure.gain_max, pos);
	pkg_push_dwrd(exposure.expo, pos);
	pkg_push_dwrd(exposure.expo_min, pos);
	pkg_push_dwrd(exposure.expo_max, pos);
	pkg_push_byte(exposure.ae_enable, pos);
	pkg_push_byte(exposure.goal_max, pos);
	pkg_push_byte(exposure.goal_min, pos);
	pkg_push_byte(exposure.ae_frame, pos);


	litetalk_pack_header(chunk->p, LITETALK_CATEGORY_COMMAND, pos-head);

	chunk->length = pos-head+LITETALK_HEADERLEN;

	awoke_hexdump_trace(chunk->p, chunk->length);

	return et_ok;
}

static err_type ltk_display_set(struct cmder_protocol *proto, 
	struct litetalk_cmdinfo *info, void *in, int length)
{
	uint8_t *pos = (uint8_t *)in;
	struct uartcmder *uc = proto->context;
	struct ltk_display display;
	struct cmder_config *cfg = &uc->config;

	cmder_info("display set");

	pkg_pull_byte(display.fps, pos);
	pkg_pull_byte(display.fps_min, pos);
	pkg_pull_byte(display.fps_max, pos);
	pkg_pull_byte(display.cl_test, pos);
	pkg_pull_byte(display.front_test, pos);
	pkg_pull_byte(display.cross_en, pos);
	pkg_pull_dwrd(display.cross_cp, pos);
	display.cross_cp = htonl(display.cross_cp);
	pkg_pull_byte(display.vinvs, pos);
	pkg_pull_byte(display.hinvs, pos);

	//cmder_debug("fps:%d", display.fps);
	//cmder_debug("fpsmin:%d", display.fps_min);
	//cmder_debug("fpsmax:%d", display.fps_max);
	//cmder_debug("CL test:%d", display.cl_test);
	//cmder_debug("Front test:%d", display.front_test);
	//cmder_debug("cross en:%d", display.cross_en);
	//cmder_debug("cross(%d,%d):%d", display.cross_cp>>16, display.cross_cp&0xffff);
	//cmder_debug("invs:%d %d", display.vinvs, display.hinvs);

	if (cfg->fps != display.fps) {
		cfg->fps = display.fps;
		cfg->expo_min = 936/display.fps + 7;
		cfg->expo_max = 936*1053/display.fps + 7;
		cmder_debug("fps:%d expo min:%d max:%d", display.fps, cfg->expo_min, cfg->expo_max);
	}
	
	cfg->cltest_enable = display.cl_test;
	cfg->frtest_enable = display.front_test;
	cfg->crossview_enable= display.cross_en;
	cfg->crossview_point.x = display.cross_cp>>16;
	cfg->crossview_point.y = display.cross_cp & 0xffff;
	cfg->vinvs = display.vinvs;
	cfg->hinvs = display.hinvs;

	return et_ok;
}

static err_type ltk_display_get(struct cmder_protocol *proto, 
	struct litetalk_cmdinfo *info, awoke_buffchunk *chunk)
{
	int length;
	uint8_t code = 0;
	uint8_t *head = chunk->p + LITETALK_HEADERLEN;
	uint8_t *pos = head;
	uint8_t cltest_enable;
	uint8_t frtest_enable;
	uint8_t cross_en;
	uint32_t cross_cp;
	struct uartcmder *uc = proto->context;
	struct cmder_config *cfg = &uc->config;

	cmder_info("display get");

	pkg_push_byte(info->cmdtype, pos);
	pkg_push_byte(code, pos);

	pkg_push_byte(cfg->fps, pos);
	pkg_push_byte(cfg->fps_min, pos);
	pkg_push_byte(cfg->fps_max, pos);
	cltest_enable = cfg->cltest_enable;
	pkg_push_byte(cltest_enable, pos);
	frtest_enable = cfg->frtest_enable;
	pkg_push_byte(frtest_enable, pos);
	cross_en = cfg->crossview_enable;
	pkg_push_byte(cross_en, pos);
	cross_cp = cfg->crossview_point.x << 16 | cfg->crossview_point.y;
	pkg_push_dwrd(cross_cp, pos);
	pkg_push_byte(cfg->vinvs, pos);
	pkg_push_byte(cfg->hinvs, pos);

	litetalk_pack_header(chunk->p, LITETALK_CATEGORY_COMMAND, pos-head);

	chunk->length = pos-head+LITETALK_HEADERLEN;

	awoke_hexdump_trace(chunk->p, chunk->length);

	return et_ok;	
}

static struct litetalk_cmd litetalk_cmd_array[] = {
	{LITETALK_CMD_SENSOR_REG, ltk_sensor_reg_set, ltk_sensor_reg_get},
	{LITETALK_CMD_EXPOSURE, ltk_exposure_set, ltk_exposure_get},
	{LITETALK_CMD_DISPLAY, ltk_display_set, ltk_display_get},
};

static err_type litetalk_service_callback(struct cmder_protocol *proto, uint8_t reasons, 
	void *user, void *in, unsigned int length)
{
	struct litk_private *priv;
	
	switch (reasons) {

	case LITETALK_CALLBACK_PROTOCOL_INIT:
		priv = mem_alloc_z(sizeof(struct litk_private));
		priv->cmdlist = litetalk_cmd_array;
		priv->cmdlist_nr = array_size(litetalk_cmd_array);
		proto->private = priv;
		awoke_buffchunk_pool_init(&priv->streampool, 4096);
		break;

	case LITETALK_CALLBACK_COMMAND:
		//cmder_info("COMMAND");
		litetalk_command_process(proto, user, in, length);
		break;

	case LITETALK_CALLBACK_STREAM_DOWNLOAD:
		//cmder_info("filedown");
		litetalk_stream_process(proto, user, in, length);
		break;

	case LITETALK_CALLBACK_FILE_DOWNLOAD_FINISH:
		//cmder_info("filedown FIN");
		litetalk_filedown_finish_process(proto, user, in, length);
		break;
		
	default:
		break;
	}

	return et_ok;
}

static err_type cmder_protocol_poll(struct uartcmder *uc)
{
	int prior;
	err_type ret;
	struct cmder *c = &uc->base;
	struct cmder_protocol *proto;
	awoke_buffchunk *chunk;
	
	if (awoke_minpq_empty(uc->rxqueue)) {
		return et_ok;
	}

	awoke_minpq_delmin(uc->rxqueue, &chunk, &prior);

	if (!c->nr_protocols) {
		cmder_debug("no protocols");
		awoke_buffchunk_free(&chunk);
		return et_ok;
	}

	//cmder_debug("chunk length:%d", chunk->length);
	//awoke_hexdump_trace(chunk->p, chunk->length);

	list_foreach(proto, &c->protocols, _head) {
		//cmder_debug("protocol:%s", proto->name);
		ret = proto->match(proto, chunk->p, chunk->length);
		if (ret != et_ok) {
			break;
		}
		proto->read(proto, chunk->p, chunk->length);
		break;
	}

	awoke_buffchunk_free(&chunk);

	return et_ok;
}

#if defined(CMDER_TEST_STRING_COMMAND)
static int ascii_to_integer(char *string)
{
	int value = 0;

	while (*string >= '0' && *string <= '9') {
		value *= 10;
		value += *string - '0';
		string++;
	}

	if (*string != '\0')
		value = 0;
	return value;
}

static void string_parse(char *str, int type, void *pval)
{
	char *p;
	char *token = ",";
	int value;

	if (strstr(str, "=")) {
		p = strtok(str, "=");
		cmder_debug("p:%s", p);
	}
	
	p = strtok(NULL, token);
	//cmder_debug("p:%s", p);
	value = ascii_to_integer(p);
	memcpy(pval, &value, 1);
	//cmder_debug("value:%d", value);
}
#endif

#if defined(CMDER_TEST_BYTE_COMMAND)
static void testbuf_tx(struct uartcmder *uc, uint8_t *buf, int len)
{
	int c;
	struct cmder *base = &uc->base;
	struct command_transceiver *tcvr = list_entry_first(&base->transceiver_list, 
		struct command_transceiver, _head);
	cmder_debug("tcvr[%d]", tcvr->id);

	c = write(tcvr->fd, buf, len);
	cmder_debug("c:%d", c);
}
#endif

int main (int argc, char *argv[])
{
	err_type ret;
	struct uartcmder *cmder;
	const char *serialport = "/dev/ttyS11";

	awoke_log_init(LOG_TRACE, LOG_M_ALL);
	
	cmder = mem_alloc_z(sizeof(struct uartcmder));
	if (!cmder) {
		cmder_err("cmder alloc error");
		return -1;
	}

	cmder_trace("cmder alloc ok");

	ret = cmder_init(cmder, serialport);
	if (ret != et_ok) {
		cmder_err("cmder init error");
		return -1;
	}

	//uint8_t testbuf[7] = {0x56, 0x56, 0x2, 0x1, 0x1, 0x0, 0x5};
	cmder_trace("cmder init ok");

	//testbuf_tx(cmder, testbuf, 7);

	while (1) {
		
		cmder_command_recv(cmder);
		//cmder_command_exec(cmder);
		//cmder_swift_test_run(cmder);
		cmder_protocol_poll(cmder);
		cmder_command_send(cmder);
		usleep(100);
	}

	return 0;
}
