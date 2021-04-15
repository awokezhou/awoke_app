
#include "bk_eots_hd.h"
#include "benchmark.h"


static int ecmd_handler_dfg(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_zoom(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_graymode(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_mirror(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_adjopt(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_expotime(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_gain(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_opt_target(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_cst(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_nrd(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_ehc(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_shp(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_cross_view(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_cross_move(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_hdr(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_cfg_save(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_restore(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_self_inspection(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_wdr(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_fps(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_rsl(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_heartbeat(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_power(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_auto_focus(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_opt_dfg(struct _ecmd_entry *e, uint8_t *p);
static int ecmd_handler_opt_view(struct _ecmd_entry *e, uint8_t *p);
//static int ecmd_handler_opt_focus(struct _ecmd_entry *e, void *d);
//static int ecmd_handler_focal_length(struct _ecmd_entry *e, void *d);
//static int ecmd_handler_low_delay(struct _ecmd_entry *e, void *d);



static void xyrect_from_center(struct _bk_eots_xy_point c, uint16_t w, uint16_t h, 
	struct _bk_eots_xy_rect *r);



static struct _ecmd_entry ecmd_table[] = {
	{0x37,	6,	ecmd_handler_dfg},
	{0x46, 	6,	ecmd_handler_zoom},
	{0x63,	6,	ecmd_handler_graymode},
	{0x66,	6,	ecmd_handler_mirror},
	{0x39,	6,	ecmd_handler_adjopt},
	{0x4a,	7,	ecmd_handler_expotime},
	{0x4c, 	6,	ecmd_handler_gain},
	{0x0d,	6,  ecmd_handler_opt_target},
	{0x17,	6,  ecmd_handler_cst},
	{0x53,	6,  ecmd_handler_nrd},
	{0x5b,	6,  ecmd_handler_ehc},
	{0x5c,	6,  ecmd_handler_shp},
	{0x67,	6,  ecmd_handler_cross_view},
	{0x68,	8,  ecmd_handler_cross_move},
	{0x02,	6,  ecmd_handler_hdr},
	{0x03,	6,  ecmd_handler_cfg_save},
	{0x04,	6,  ecmd_handler_restore},
	{0x05,	6,  ecmd_handler_self_inspection},
	{0x06,	6,  ecmd_handler_wdr},
	{0x09,	6,  ecmd_handler_fps},
	{0x07,	6,  ecmd_handler_rsl},
	{0x11,	6,  ecmd_handler_heartbeat},
	{0x12, 	6,  ecmd_handler_power},
	{0x70, 	6,  ecmd_handler_auto_focus},
	{0x01,	6,  ecmd_handler_opt_dfg},
	{0x08,	6,  ecmd_handler_opt_view},
	//{0x0a,	6,  ecmd_handler_opt_focus},
	//{0x0b,	6,  ecmd_handler_focal_length},
	//{0x10,	6,  ecmd_handler_low_delay},
};

static int ecmd_handler_dfg(struct _ecmd_entry *e, uint8_t *p)
{
	uint32_t level = 0;
	
	if (!e || !p || (p[0] != 0x02)) {
		bk_err("dfg param");
		return -1;
	}

	if (p[1] == 0x00) {
		/* close */
		bk_info("dfg disable");
	} else {

		switch (p[1]) {

		case 0x01:
			bk_info("dfg low level");
			break;

		case 0x02:
			bk_info("dfg middle level");
			break;

		case 0x03:
			bk_info("dfg high level");
			break;

		default:
			bk_err("dfg level not support");
			return -1;
		}
		
		level = p[1] - 1;
		bk_debug("dfg enable, level:%d", level);
	}

	return 0;
}

static int ecmd_handler_zoom(struct _ecmd_entry *e, uint8_t *p)
{
	uint32_t zoom;

	if (!e || !p || (p[0] != 0x00)) {
		bk_err("zoom param");
		return -1;
	}

	switch (p[1]) {

	case 0x01:
		bk_info("zoom 1");
		break;

	case 0x02:
		bk_info("zoom 2");
		break;

	case 0x04:
		bk_info("zoom 4");
		break;

	default:
		bk_err("zoom not support");
		return -1;
	}
	
	zoom = p[1];

	bk_debug("zoom:%d", zoom);

	return 0;
}

static int ecmd_handler_graymode(struct _ecmd_entry *e, uint8_t *p)
{
	uint32_t mode;

	if (!e || !p || (p[0] != 0x00)) {
		bk_err("gray param");
		return -1;
	}

	switch (p[1]) {

	case 0x00:
		bk_info("gray close");
		mode = 0;
		break;

	case 0x04:
		bk_info("gray open");
		mode = 1;
		break;

	default:
		bk_err("gray not support");
		return -1;
	}

	bk_debug("graymode:%d", mode);

	return 0;
}

static int ecmd_handler_mirror(struct _ecmd_entry *e, uint8_t *p)
{
	bk_trace("mirror");

	return 0;
}

static int ecmd_handler_adjopt(struct _ecmd_entry *e, uint8_t *p)
{
	uint16_t w, h;
	uint32_t aec_tlpoint, aec_brpoint;
	bk_eots_xy_rect rect;
	bk_eots_xy_point center = XYPOINT_2D_CENTER;
	
	if (!e || !p) {
		bk_err("adjopt param");
		return -1;
	}

	if (p[0] == 0x00) {
		/* mode setting, 0:auto 3:manual */
		if (p[1] == 0x00) {
			bk_info("auto");
		} else if (p[1] == 0x03) {
			bk_info("manual");
		} else {
			bk_err("mode error");
			return -1;
		}
	} else {

		switch (p[0]) {

		case 0x01:
			bk_info("200*200");
			w = 200;
			h = 200;
			break;

		case 0x02:
			bk_info("500*500");
			w = 500;
			h = 500;
			break;

		case 0x03:
			bk_info("global");
			w = 800;
			h = 800;
			break;

		default:
			bk_err("area error");
			return -1;
		}
	
		xyrect_from_center(center, w, h, &rect);
		bk_debug("tl x:%d y:%d br x:%d y:%d", rect.tl.x, rect.tl.y, rect.br.x, rect.br.y);
		aec_tlpoint = (rect.tl.x) | ((rect.tl.y)<<16);
		aec_brpoint = (rect.br.x) | ((rect.br.y)<<16);
		bk_debug("aec tl:0x%x br:0x%x", aec_tlpoint, aec_brpoint);
	}
	return 0;
}

static int ecmd_handler_expotime(struct _ecmd_entry *e, uint8_t *p)
{
	uint16_t param;
	uint32_t expo_max = 16570;
	
	if (!e || !p) {
		bk_err("expo param");
		return -1;
	}

	param = (p[1]<<8)|p[2];

	if (p[0] == 0x00) {
		if (param > expo_max) {
			bk_err("setting time range error");
			return -1;
		}
		bk_info("expo time setting %d(ms)", param);
	} else {

		if ((param<1) || (param>1000)) {
			bk_err("+/- time range error");
			return -1;
		}
		
		switch (p[0]) {

		case 0x01:
			bk_info("expo++ %d(ms)", param);
			break;

		case 0x02:
			bk_info("expo-- %d(ms)", param);
			break;

		default:
			bk_err("expo error");
			return -1;
		}
	
	}

	return 0;
}

static int ecmd_handler_gain(struct _ecmd_entry *e, uint8_t *p)
{
	bool need_set = FALSE;

	if (!e || !p || (p[1] != 0x00)) {
		bk_err("gain param");
		return -1;
	}

	if (p[0] == 0x01) {		/* increase */
		bk_info("gain++");
		need_set = TRUE;
	} else if (p[0] == 0x02) {
		bk_info("gain--");
		need_set = TRUE;
	} else {
		bk_err("gain error");
		return -1;
	}

	if (need_set) {
		bk_info("gain set");
	}

	return 0;
}

static int ecmd_handler_opt_target(struct _ecmd_entry *e, uint8_t *p)
{
	if (!e || !p || (p[0] != 0x00)) {
		bk_err("param target");
		return -1;
	}

	switch (p[1]) {

	case 0x00:
		bk_info("target reset");
		break;

	case 0x02:
		bk_info("target++");
		break;

	case 0x03:
		bk_info("target--");
		break;

	default:
		bk_err("target err");
		return -1;
	}

	return 0;	
}

static int ecmd_handler_cst(struct _ecmd_entry *e, uint8_t *p)
{
	if (!e || !p || (p[0] != 0x00)) {
		bk_err("param cst");
		return -1;
	}

	if (p[1] > 2) {
		bk_err("cst error");
		return -1;
	}

	bk_info("cst level:%d", p[1]);

	return 0;
}

static int ecmd_handler_nrd(struct _ecmd_entry *e, uint8_t *p)
{
	uint32_t nrd_en;
	
	if (!e || !p || (p[0] != 0x00)) {
		bk_err("param nrd");
		return -1;
	}

	if (p[1] > 1) {
		bk_err("nrd error");
		return -1;
	}
	
	nrd_en = (p[1]==1) ? 1 : 0;

	bk_info("nrd en:%d", nrd_en);

	return 0;	
}

static int ecmd_handler_ehc(struct _ecmd_entry *e, uint8_t *p)
{
	uint32_t level = 0;
	uint32_t ehc_en;
	
	if (!e || !p || (p[0] != 0x00)) {
		bk_err("param ehc");
		return -1;
	}

	if (p[1] > 3) {
		bk_err("ehc range");
		return -1;
	}

	ehc_en = (p[1]==0) ? 0 : 1;
	
	if (ehc_en) {
		level = p[1];
		bk_info("ehc en");
		bk_info("ehc level:%d", level);
	} else {
		bk_info("ehc disable");
	}

	return 0;
}

static int ecmd_handler_shp(struct _ecmd_entry *e, uint8_t *p)
{
	uint32_t level = 0;
	uint32_t shp_en;

	if (!e || !p || (p[0] != 0x00)) {
		bk_err("param shp");
		return -1;
	}

	if (p[1] > 3) {
		bk_err("shp error");
		return -1;
	}

	shp_en = (p[1]==3) ? 0 : 1;
	
	if (shp_en) {
		level = p[1];
		bk_info("shp en");
		bk_info("shp level:%d", level);
	} else {
		bk_info("shp disable");
	}

	return 0;
}

static int ecmd_handler_cross_view(struct _ecmd_entry *e, uint8_t *p)
{
	uint32_t cursor_en;
	
	if (!e || !p || (p[0] != 0x00)) {
		bk_err("param nrd");
		return -1;
	}

	switch (p[1]) {

	case 0x02:
		bk_info("cross open");
		cursor_en = 1;
		break;

	case 0x03:
		cursor_en = 0;
		bk_info("cross close");
		break;		

	default:
		bk_err("cross error");
		return -1;
	}

	bk_debug("cursor en:%d", cursor_en);

	return 0;
}

static int ecmd_handler_cross_move(struct _ecmd_entry *e, uint8_t *p)
{
	uint16_t ud, lr;
	uint32_t cursor_xy;
	bk_eots_xy_point cross_xy = xypoint_get(800, 800);
	
	if (!e || !p) {
		bk_err("param nrd");
		return -1;
	}

	ud = p[1];		/* up down move step */
	lr = p[3];		/* left right move step */

	if (ud>0x32) {
		bk_err("cross up down range");
		return -1;
	}

	if (lr>0x32) {
		bk_err("cross left right range");
		return -1;
	}

	if (p[0] == 1) {		/* up */
		cross_xy.y -= ud;
		bk_info("move up");
	} else if (p[0] == 2) {	/* down */
		cross_xy.y += ud;
		bk_info("move down");
	} else if (p[0] > 2) {
		bk_err("cross ud error");
		return -1;
	}

	if (p[2] == 1) {		/* left */
		cross_xy.x -= lr;
		bk_info("move left");
	} else if (p[2] == 2) {
		cross_xy.x += lr;	/* right */
		bk_info("move right");
	} else if (p[2] > 2) {
		bk_err("cross lr error");
		return -1;
	}

	bk_info("cursor x:%d y:%d", cross_xy.x, cross_xy.y);

	cursor_xy = (cross_xy.x) | ((cross_xy.y)<<8);

	bk_info("cursor_xy:0x%x", cursor_xy);
	
	return 0;
}

static int ecmd_handler_hdr(struct _ecmd_entry *e, uint8_t *p)
{
	if (!e || !p || (p[0] != 0x00)) {
		bk_err("param hdr");
		return -1;
	}

	switch (p[1]) {

	case 0x02:
		bk_info("hdr enable");
		break;

	case 0x03:
		bk_info("hdr disable");
		break;

	default:
		bk_err("hdr error");
		return -1;
	}

	return 0;
}

static int ecmd_handler_cfg_save(struct _ecmd_entry *e, uint8_t *p)
{
	if (!e || !p || (p[0] != 0x00) || (p[1] != 0x00)) {
		bk_err("param cfg save");
		return -1;
	}

	bk_notice("not support");

	return 0;	
}

static int ecmd_handler_restore(struct _ecmd_entry *e, uint8_t *p)
{
	if (!e || !p || (p[0] != 0x00) || (p[1] != 0x00)) {
		bk_err("param restore");
		return -1;
	}

	bk_notice("not support");

	return 0;	
}

static int ecmd_handler_self_inspection(struct _ecmd_entry *e, uint8_t *p)
{
	if (!e || !p || (p[0] != 0x00) || (p[1] != 0x00)) {
		bk_err("param nrd");
		return -1;
	}

	bk_notice("not support");

	return 0;
}

static int ecmd_handler_wdr(struct _ecmd_entry *e, uint8_t *p)
{
	if (!e || !p || (p[0] != 0x00)) {
		bk_err("param wdr");
		return -1;
	}

	switch (p[1]) {

	case 0x02:
		bk_info("wdr enable");
		break;

	case 0x03:
		bk_info("wdr disable");
		break;

	default:
		bk_err("wdr error");
		return -1;
	}

	return 0;
}

static int ecmd_handler_fps(struct _ecmd_entry *e, uint8_t *p)
{
	uint32_t fps_select;
	
	if (!e || !p || (p[0] != 0x00)) {
		bk_err("param fps");
		return -1;
	}

	switch (p[1]) {

	case 0x02:
		bk_info("fps use default 30hz");
		fps_select = 30;
		break;

	case 0x03:
		bk_info("fps use 25hz");
		fps_select = 25;
		break;

	case 0x04:
		bk_info("fps use 50hz");
		fps_select = 50;
		break;

	case 0x05:
		bk_info("fps use 60hz");
		fps_select = 60;
		break;

	default:
		bk_err("fps error");
		return -1;
	}

	bk_debug("fps select:%d", fps_select);

	return 0;
}

static int ecmd_handler_rsl(struct _ecmd_entry *e, uint8_t *p)
{
	if (!e || !p || (p[0] != 0x00)) {
		bk_err("param rsl");
		return -1;
	}

	switch (p[1]) {

	case 0x01:
		bk_info("1080p");
		break;

	case 0x02:
		bk_info("720p");
		break;

	default:
		bk_err("rsl error");
		return -1;
	}

	return 0;
}

static int ecmd_handler_heartbeat(struct _ecmd_entry *e, uint8_t *p)
{
	if (!e || !p || (p[0] != 0x00)) {
		bk_err("param heartbeat");
		return -1;
	}

	switch (p[1]) {

	case 0x01:
		bk_info("heartbeat enable");
		break;

	case 0x02:
		bk_info("heartbeat disable");
		break;

	default:
		bk_err("heartbeat error");
		return -1;
	}

	return 0;
}

static int ecmd_handler_power(struct _ecmd_entry *e, uint8_t *p)
{
	if (!e || !p || (p[0] != 0x00)) {
		bk_err("param power");
		return -1;
	}

	switch (p[1]) {

	case 0x01:
		bk_info("power off");
		break;

	case 0x00:
		bk_info("power on");
		break;

	default:
		bk_err("power error");
		return -1;
	}	
	
	return 0;	
}

static int ecmd_handler_auto_focus(struct _ecmd_entry *e, uint8_t *p)
{
	bk_notice("not support");

	return 0;	
}

static int ecmd_handler_opt_dfg(struct _ecmd_entry *e, uint8_t *p)
{
	bk_notice("not support");

	return 0;	
}

static int ecmd_handler_opt_view(struct _ecmd_entry *e, uint8_t *p)
{
	if (!e || !p || (p[0] != 0x00)) {
		bk_err("param nrd");
		return -1;
	}

	bk_notice("not support");

	return 0;
}

static void ecmd_ack_build(struct _ecmd_ack *ack)
{
	ack->header = 0x6090;

	ack->focus_hi = 1;
	ack->focus_lo = 2;

	ack->view_hi = 3;
	ack->view_lo = 4;

	ack->cross_x = 5;
	ack->cross_y = 6;

	ack->expo = 7;

	ack->gain = 8;

	ack->algctl_dfg = 1;
	ack->algctl_gray = 0;
	ack->algctl_ehc = 1;
	ack->algctl_aec = 0;
	ack->algctl_zm2 = 1;
	ack->algctl_zm4 = 0;
	ack->algctl_hdr = 1;
	ack->algctl_rsv = 1;

	ack->algsta_aea = 3;
	ack->algsta_fps = 1;
	ack->algsta_mir = 2;
	ack->algsta_res = 0;
	ack->algsta_rsv = 0;

	ack->seinsp_img = 0;
	ack->seinsp_opf = 1;
	ack->seinsp_ope = 0;
	ack->seinsp_zmr = 1;
	ack->seinsp_fmr = 1;
	ack->seinsp_rsv = 0;
}

static uint8_t ecmd_ack_checkbyte(uint8_t *p, int len)
{
	int i;
	uint8_t byte = 0x0;
	
	if (!p || (len<=0))
		return 0;
	
	for (i=0; i<len; i++) {
		byte ^= p[i];
	}

	return byte;
}
static void ecmd_response(void)
{
	ecmd_ack ack;
	memset(&ack, 0x0, sizeof(ack));
	bk_debug("sizeof(ack):%d", sizeof(ack));
	ecmd_ack_build(&ack);
	ack.checkbyte = ecmd_ack_checkbyte((uint8_t *)&ack, sizeof(ack)-1);
	awoke_hexdump_trace(&ack, sizeof(ack));
}

static struct _ecmd_entry *ecmd_get_command(ecmd_item item)
{
	int size;
	ecmd_entry *head, *p;

	head = ecmd_table;
	size = array_size(ecmd_table);
	
	array_foreach(head, size, p) {
		if ((p->code == item.code) && (item.length == p->cmdlen)) {
			bk_trace("find code:0x%x", p->code);
			return p;
		}
	}

	bk_trace("command not find");
	return NULL;
}

static int ecmd_run_command(struct _ecmd_entry *e, uint8_t *p)
{
	if (!e) {
		return 0;
	}

	if (!e->handler) {
		bk_err("handler null");
		return 0;
	}

	bk_trace("subcode: 0x%x 0x%x", p[0], p[1]);

	e->handler(e, p);

	ecmd_response();

	return e->cmdlen;
}

static int ecmd_process(uint8_t *buf, int length)
{
	uint8_t *pos=buf;
	ecmd_item item;
	bool find_postfix = FALSE;

	item.prefix = (buf[0]<<8)|buf[1];
	item.code = buf[2];
	
	while (pos != (buf+length)) {
		if (*pos == 0xff) {
			find_postfix = TRUE;
			bk_trace("find postfix");
			break;
		}
		pos++;
	}

	if (!find_postfix) {
		bk_trace("postfix notfind");
		return 0;
	}
	
	item.length = pos-buf+1;
	bk_trace("item length:%d code:0x%x", item.length, item.code);
	if (!item.length)
		return 0;
	
	return ecmd_run_command(ecmd_get_command(item), &buf[3]);
}

static int scan_next(uint8_t *buf, int max, uint16_t icmd_header, 
	uint16_t ecmd_header)
{
	uint16_t header;
	uint8_t *pos = buf+1;
	uint8_t *end = buf+max-1;

	if (max <= 2) {
		return max;
	}

	while (pos != end) {
		header = (pos[0]<<8)|pos[1];
		if ((header==icmd_header) || (header==ecmd_header)) {
			bk_debug("header find");
			break;
		}
		pos++;
	}

	return ((pos==end) ? max : (pos-buf));
}

static void mem_rollback(uint8_t *buf, int max, int rb)
{
	memmove(buf, buf+rb, max-rb);
}

static void xyrect_from_center(struct _bk_eots_xy_point c, uint16_t w, uint16_t h, 
	struct _bk_eots_xy_rect *r)
{
	uint16_t x_offset;
	uint16_t y_offset;
	
	if (!r) {
		return;
	}

	x_offset = w/2;
	y_offset = h/2;

	r->tl.x = c.x - x_offset;
	r->tl.y = c.y - y_offset;
	r->br.x = c.x + x_offset;
	r->br.y = c.y + y_offset;
}

err_type bk_eots_hd_ecmd_process(uint8_t *buf, int length)
{
	int len;
	int remain = length;
	int rollback_length;

	do {

		len = ecmd_process(buf, remain);
		bk_trace("processd len:%d", len);
		if (len) {
			rollback_length = len;
		} else {
			rollback_length = scan_next(buf, remain, 0x0000, 0x8101);
			bk_trace("scan len:%d", rollback_length);
		}

		if (rollback_length) {
			bk_debug("rollback length:%d", rollback_length);
			mem_rollback(buf, remain, rollback_length);
			remain -= rollback_length;
			awoke_hexdump_trace(buf, remain);
		}
		
	} while (remain > 0);

	return et_ok;
}

static bool syscon_command_ready(int length)
{
	return (!(length < 6));
}

#define ICMD_SIG1       ((uint8_t)0xa5)
#define ICMD_SIG2       ((uint8_t)0x50)

static int icmd_process(uint8_t *buf, int length)
{
	if ((buf[0] == ICMD_SIG1) && (buf[1] == ICMD_SIG2)) {
		bk_info("find icmd");
		return 6;
	}
	return 0;
}

static int syscon_one(uint8_t *buf, int length)
{
	int len;
	int rollback_length;
	uint8_t *pos = buf;
	int remain = length;
	uint16_t icmdhdr = (ICMD_SIG1<<8)|ICMD_SIG2;

	bk_trace("remain:%d", remain);

	len = icmd_process(pos, remain);
	if (len) {
		rollback_length = len;
	} else {
		len = ecmd_process(pos, remain);
		if (len) {
			rollback_length = len;
		} else {
			rollback_length = scan_next(pos, remain, icmdhdr, 0x8101);
		}
	}

	if (rollback_length) {
		bk_trace("rollback length:%d", rollback_length);
		mem_rollback(pos, remain, rollback_length);
		remain -= rollback_length;
		bk_trace("remain:%d", remain);
	}

	return remain;
}

err_type bk_eots_hd_syscon(uint8_t *buf, int length)
{
	int remain = length;

	bk_trace("length:%d", length);

	if (!syscon_command_ready(remain))
		return et_ok;

	do {
		awoke_hexdump_trace(buf, remain);
		remain = syscon_one(buf, remain);
		bk_debug("remain:%d", remain);

	} while (remain > 0);

	return et_ok;
}
