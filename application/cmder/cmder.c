
#include "cmder.h"
#include "awoke_log.h"
#include "awoke_memory.h"
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "proto-swift.h"
#include "proto-litetalk.h"
#include "awoke_package.h"
#include "uart.h"


static err_type hello_get(struct command *cmd);
static err_type hello_set(struct command *cmd, void *buf, int len);
static err_type hello_exec(struct command *cmd);
static err_type world_exec(struct command *cmd);
static err_type bytes1_exec(struct command *cmd);
static err_type bytes2_exec(struct command *cmd);
static err_type ecmd_handler_dfg(struct command *cmd);
static err_type ecmd_handler_zoom(struct command *cmd);
static err_type switf_service_callback(void *context, uint8_t reasons, void *user, void *in, unsigned int length);
uint8_t macher_string_dispatch(struct command *cmd, void *buf, int len);
static err_type litetalk_service_callback(void *context, uint8_t reasons, void *user, void *in, unsigned int length);


bool matcher_string(struct command_table *tab, void *buf, int len, struct command **cmd);
bool matcher_bytes(struct command_table *tab, void *buf, int len, struct command **cmd);
bool ecmd_format_check(void *buf, int len);
bool ecmd_xmatch(struct command_table *tab, void *buf, int len, struct command **cmd);



#define match_string_make(str)		{MATCH_STRING,	.length=strlen(str),		.val.string=str}
#define match_bytes_make(array)		{MATCH_BYTES,	.length=array_size(array),	.val.bytes=array}
#define match_ecmd_make(e)			{MATCH_CUSTOM,	.length=1,					.val.struc=(void *)&e}


static struct command test_string_commands[] = {
	{match_string_make("hello"),	hello_get,	hello_set,	hello_exec},
	{match_string_make("world"),	NULL,	NULL,	world_exec},
};


static uint8_t testbytes[2][3] = {
	{0x11, 0x22, 0x33},
	{0x21, 0x22, 0x23},
};

static struct command test_bytes_commands[] = {
	{match_bytes_make(testbytes[0]), NULL, NULL, bytes1_exec},
	{match_bytes_make(testbytes[1]), NULL, NULL, bytes2_exec},
};

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

static struct command_matcher string_matcher = {
	.match = matcher_string,
	.dispatch = macher_string_dispatch,
};

static struct command_matcher bytes_matcher = {
	.match = matcher_bytes,
};

static struct command_matcher ecmd_matcher = {
	.format = ecmd_format_check,
	.match = ecmd_xmatch,
};

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

static err_type test_string_tx(struct cmder *c)
{
	return et_ok;	
}

static err_type test_bytes_tx(struct cmder *c)
{
	return et_ok;	
}

static err_type uart_rx(struct command_transceiver *tcvr, void *data)
{
	int readsize, c;
	awoke_buffchunk *chunk, *buffer;
	struct uartcmder *uc = (struct uartcmder *)data;

	buffer = tcvr->rxbuffer;

	c = read(tcvr->fd, buffer->p, buffer->size);
	if (c > 0) {
		cmder_trace("read %d", c);
		buffer->length = c;
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

static err_type cmder_tcvr_claim(struct cmder *c, uint8_t id)
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

	fd = uart_claim(p->port, 9600, 8, 1, 'N');
	if (fd < 0) {
		cmder_err("tcvr[%d] open error", id);
		return err_open;
	}

	p->fd = fd;

	cmder_debug("tcvr[%d] claim ok", id);
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
	c->ae_enable = 1;
	c->ae_expo_en = 1;
	c->ae_gain_en = 1;
	c->gain = 0;
	c->exposure = 1600;
	c->goal_max = 85;
	c->goal_min = 65;
	c->ae_frame = 4;

	c->fps = 100;
	c->fps_min = 0;
	c->fps_max = 125;
	c->cl_test = 0;
	c->front_test = 0;
	c->cross_en = 0;
	c->cross_cp = (640<<16)|540;
	c->vinvs = 0;
	c->hinvs = 0;
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

	cmder_tab_register(&c->base, "string-tab", test_string_commands,
		array_size(test_string_commands), &string_matcher);

	/*
	cmder_tab_register(&c->base, "bytes-tab", test_bytes_commands,
		array_size(test_bytes_commands), &bytes_matcher);

	cmder_tab_register(&c->base, "ecmds", test_ecmds,
		array_size(test_ecmds), &ecmd_matcher);*/

	cmder_tcvr_register(&c->base, 0, port, uart_rx, uart_tx, 1024);

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

	cmder_tcvr_claim(&c->base, 0);

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

	cmder_trace("data(%d) need send", chunk->length);
	ret = tcvr->tx(tcvr, chunk->p, chunk->length);
	if (ret != et_ok) {
		cmder_err("send error:%d", ret);
		awoke_buffchunk_free(&chunk);
		return ret;
	}
	cmder_trace("data(%d) send ok", chunk->length);

	awoke_buffchunk_free(&chunk);

	return et_ok;
}

static err_type litetalk_stream_process(struct cmder_protocol *proto, 
	void *user, void *in, int length)
{
	awoke_buffchunk *chunk, *txpkt;
	struct litetalk_streaminfo *sinfo;
	struct uartcmder *uc = proto->context;
	struct litetalk_private *priv = proto->private;

	sinfo = (struct litetalk_streaminfo *)user;

create_pool:
	if (!priv->stream_dbpool) {
		priv->stream_dbpool = awoke_buffchunk_pool_create(sinfo->totalsize);
		if (!priv->stream_dbpool) {
			cmder_err("download pool create error");
			return err_oom;
		}
		priv->streamid = sinfo->streamid;
		priv->streamidx = 0;
		cmder_info("download[%d] create, totalsize:%d", 
			priv->streamid, sinfo->totalsize);
	} else {
		if (priv->streamid != sinfo->streamid) {
			cmder_warn("previous download[%d] break");
			awoke_buffchunk_pool_free(&priv->stream_dbpool);
			priv->stream_dbpool = NULL;
			goto create_pool;
		}
	}

	if ((priv->streamidx != 0) && (priv->streamidx != (sinfo->index-1))) {
		cmder_err("need:%d but give:%d", priv->streamidx, sinfo->index);
	} else {
		priv->streamidx = sinfo->index;
	}

	chunk = awoke_buffchunk_create(sinfo->length);
	awoke_buffchunk_write(chunk, in, sinfo->length, TRUE);
	awoke_buffchunk_pool_chunkadd(priv->stream_dbpool, chunk);
	
	cmder_debug("download[%d-%d] length:%d %d/%d",
		priv->streamid,
		priv->streamidx, 
		length,
		priv->stream_dbpool->length,
		priv->stream_dbpool->maxsize);

	if (priv->stream_dbpool->length == priv->stream_dbpool->maxsize) {
		cmder_info("download[%d] finish, merge...", priv->streamid);
		awoke_buffchunk *merge = awoke_buffchunk_pool2chunk(priv->stream_dbpool);
		awoke_buffchunk_pool_free(&priv->stream_dbpool);
		priv->stream_dbpool = NULL;
		awoke_hexdump_trace(merge->p, merge->length);
		awoke_buffchunk_free(&merge);
	}

	txpkt = awoke_buffchunk_create(32);
	litetalk_build_stream_ack(txpkt, priv->streamid, priv->streamidx, 0);
	txpkt->id = 0;
	awoke_minpq_insert(uc->txqueue, &txpkt, 0);
	
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
	uint32_t value;
	awoke_buffchunk *txpkt;
	struct litetalk_cmd *p;
	struct litetalk_cmdinfo *info;
	struct litetalk_private *pri = proto->private;
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
	uint8_t *head = chunk->p + LITETALK_HEADERLEN;
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
	struct ltk_exposure expo;

	pkg_pull_word(expo.gain, pos);
	expo.gain = htons(expo.gain);

	pkg_pull_word(expo.exposure, pos);
	expo.exposure = htons(expo.exposure);	

	pkg_pull_byte(expo.ae_enable, pos);
	pkg_pull_byte(expo.goal_max, pos);
	pkg_pull_byte(expo.goal_min, pos);
	pkg_pull_byte(expo.ae_frame, pos);

	cmder_debug("ae_enable:0x%x", expo.ae_enable);
	
	uc->gain = expo.gain;
	uc->exposure = expo.exposure;
	uc->ae_enable = expo.ae_enable & 0x1;
	uc->ae_expo_en = (expo.ae_enable & 0x2) >> 1;
	uc->ae_gain_en = (expo.ae_enable & 0x4) >> 2;
	uc->goal_max = expo.goal_max;
	uc->goal_min = expo.goal_min;
	uc->ae_frame = expo.ae_frame;
	
	return et_ok;
}

static err_type ltk_exposure_get(struct cmder_protocol *proto, 
	struct litetalk_cmdinfo *info, awoke_buffchunk *chunk)
{
	int length;
	uint8_t code = 0;
	uint8_t *head = chunk->p + LITETALK_HEADERLEN;
	uint8_t *pos = head;
	struct uartcmder *uc = proto->context;
	struct ltk_exposure expo;

	expo.gain = uc->gain;
	expo.exposure = uc->exposure;
	expo.ae_enable = uc->ae_enable | uc->ae_expo_en<<1 | uc->ae_gain_en<<2;
	expo.goal_max = uc->goal_max;
	expo.goal_min = uc->goal_min;
	expo.ae_frame = uc->ae_frame;

	cmder_debug("gain:%d", uc->gain);
	cmder_debug("exposure:%d", uc->exposure);
	cmder_debug("ae_enable:%d", uc->ae_enable);
	cmder_debug("goal_max:%d", uc->goal_max);
	cmder_debug("goal_min:%d", uc->goal_min);
	cmder_debug("ae_frame:%d", uc->ae_frame);

	pkg_push_byte(info->cmdtype, pos);
	pkg_push_byte(code, pos);

	expo.gain = htons(expo.gain);
	pkg_push_word(expo.gain, pos);

	expo.exposure = htons(expo.exposure);
	pkg_push_word(expo.exposure, pos);

	pkg_push_byte(expo.ae_enable, pos);
	pkg_push_byte(expo.goal_max, pos);
	pkg_push_byte(expo.goal_min, pos);
	pkg_push_byte(expo.ae_frame, pos);

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

	cmder_debug("fps:%d", display.fps);
	cmder_debug("fpsmin:%d", display.fps_min);
	cmder_debug("fpsmax:%d", display.fps_max);
	cmder_debug("CL test:%d", display.cl_test);
	cmder_debug("Front test:%d", display.front_test);
	cmder_debug("cross en:%d", display.cross_en);
	cmder_debug("cross(%d,%d):%d", display.cross_cp>>16, display.cross_cp&0xffff);
	cmder_debug("invs:%d %d", display.vinvs, display.hinvs);

	uc->fps = display.fps;
	uc->cl_test = display.cl_test;
	uc->front_test = display.front_test;
	uc->cross_en = display.cross_en;
	uc->cross_cp = display.cross_cp;
	uc->vinvs = display.vinvs;
	uc->hinvs = display.hinvs;

	return et_ok;
}

static err_type ltk_display_get(struct cmder_protocol *proto, 
	struct litetalk_cmdinfo *info, awoke_buffchunk *chunk)
{
	int length;
	uint8_t code = 0;
	uint8_t *head = chunk->p + LITETALK_HEADERLEN;
	uint8_t *pos = head;
	uint32_t cross_cp;
	struct uartcmder *uc = proto->context;

	cmder_info("display get");

	pkg_push_byte(info->cmdtype, pos);
	pkg_push_byte(code, pos);

	pkg_push_byte(uc->fps, pos);
	pkg_push_byte(uc->fps_min, pos);
	pkg_push_byte(uc->fps_max, pos);
	pkg_push_byte(uc->cl_test, pos);
	pkg_push_byte(uc->front_test, pos);
	pkg_push_byte(uc->cross_en, pos);
	cross_cp = htonl(uc->cross_cp);
	pkg_push_dwrd(cross_cp, pos);
	pkg_push_byte(uc->vinvs, pos);
	pkg_push_byte(uc->hinvs, pos);

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

static err_type litetalk_service_callback(void *context, uint8_t reasons, 
	void *user, void *in, unsigned int length)
{
	struct cmder_protocol *proto = context;
	struct litetalk_private *priv;
	
	switch (reasons) {

	case LITETALK_CALLBACK_PROTOCOL_INIT:
		priv = mem_alloc_z(sizeof(struct litetalk_private));
		priv->cmdlist = litetalk_cmd_array;
		priv->cmdlist_nr = array_size(litetalk_cmd_array);
		proto->private = priv;
		break;

	case LITETALK_CALLBACK_COMMAND:
		cmder_info("COMMAND");
		litetalk_command_process(proto, user, in, length);
		break;

	case LITETALK_CALLBACK_STREAM_DOWNLOAD:
		cmder_info("filedown");
		litetalk_stream_process(proto, user, in, length);
		break;

	case LITETALK_CALLBACK_FILE_DOWNLOAD_FINISH:
		cmder_info("filedown FIN");
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
	awoke_hexdump_trace(chunk->p, chunk->length);

	list_foreach(proto, &c->protocols, _head) {
		//cmder_debug("protocol:%s", proto->name);
		if (proto->match(proto, chunk->p, chunk->length)) {
			//cmder_debug("%s match", proto->name);
			return proto->read(proto, chunk->p, chunk->length);
		}
	}

	awoke_buffchunk_free(&chunk);

	return et_ok;
}

struct cmder_swift_testmap {
	int count;
	void (*handle)(void *buf);
};

static void cmder_swift_test_build_connect_package(void *buffer)
{
	uint8_t *pos = buffer;
	uint32_t marker = 0x12345678;
	uint8_t byte = 0x00;

	pkg_push_dwrd(marker, pos);
	pkg_push_byte(byte, pos);
}

static void cmder_swift_test_build_command_package(void *buffer)
{
	uint8_t *pos = buffer;
	uint32_t marker = 0x12345678;
	uint8_t byte = 0x01;

	pkg_push_dwrd(marker, pos);
	pkg_push_byte(byte, pos);
}

static void cmder_swift_test_build_filestm_package(void *buffer)
{
	uint8_t *pos = buffer;
	uint32_t marker = 0x12345678;
	uint8_t byte = 0x02;

	pkg_push_dwrd(marker, pos);
	pkg_push_byte(byte, pos);
}

static void cmder_swift_test_build_disconnect_package(void *buffer)
{
	uint8_t *pos = buffer;
	uint32_t marker = 0x12345678;
	uint8_t byte = 0x03;

	pkg_push_dwrd(marker, pos);
	pkg_push_byte(byte, pos);
}

static err_type cmder_swift_test_run(struct uartcmder *uc)
{
	static int count = 0;
	awoke_buffchunk *chunk;
	
	struct cmder_swift_testmap testmap[] = {
		{0, cmder_swift_test_build_connect_package},
		{1, cmder_swift_test_build_command_package},
		{2, cmder_swift_test_build_filestm_package},
		{3, cmder_swift_test_build_disconnect_package},
	};

	if (count > 3) {
		count = 0;
	}

	chunk = awoke_buffchunk_create(256);
	
	testmap[count++].handle(chunk->p);

	return et_ok;
}

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

int main (int argc, char *argv[])
{
	err_type ret;
	uint8_t type, level;
	struct uartcmder *cmder;
	const char *serialport = "/dev/ttyS1";
	
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
		usleep(1000);
	}

	return 0;
}
