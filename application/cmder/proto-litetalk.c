
#include "cmder.h"
#include "cmder_protocol.h"
#include "proto-litetalk.h"
#include "awoke_package.h"


static bool litetalk_match(struct cmder_protocol *proto, void *buf, int len)
{
	uint8_t *pos = buf;
	uint32_t marker;

	pkg_pull_dwrd(marker, pos);
	marker = htonl(marker);
	cmder_trace("marker:0x%x", marker);
	
	if (marker != 0x13356789) {
		cmder_trace("match");
		return FALSE;
	}

	cmder_debug("matched");

	return TRUE;
}

static err_type litetalk_stream_download(struct cmder_protocol *proto, 
	uint8_t flag, uint8_t *buf, uint32_t length)
{
	uint8_t *pos = (uint8_t *)buf;
	struct litetalk_streaminfo sinfo;

	pkg_pull_byte(sinfo.media, pos);

	pkg_pull_dwrd(sinfo.totalsize, pos);
	sinfo.totalsize = htonl(sinfo.totalsize);

	pkg_pull_byte(sinfo.streamid, pos);

	pkg_pull_byte(sinfo.index, pos);

	sinfo.length = length - 8;

	cmder_debug("media:%d", sinfo.media);
	cmder_debug("streamid:%d", sinfo.streamid);
	cmder_debug("index:%d", sinfo.index);
	cmder_debug("totalsize:%d", sinfo.totalsize);
	cmder_debug("length:%d", sinfo.length);

	return proto->callback(proto, LITETALK_CALLBACK_STREAM_DOWNLOAD, 
		&sinfo, pos, sinfo.length);
}

static err_type litetalk_stream_read(struct cmder_protocol *proto, 
	uint8_t flag, uint8_t *buf, uint32_t length)
{
	uint8_t subtype;
	uint8_t *pos = (uint8_t *)buf;
	
	pkg_pull_byte(subtype, pos);

	if (subtype == 0x01) {
		litetalk_stream_download(proto, flag, pos, length);
	} else if (subtype == 0x02) {
		//litetalk_stream_ack(proto, flag, buf, length);
	}
	
	return et_ok;
}

static err_type litetalk_command_read(struct cmder_protocol *proto, 
	uint8_t flag, uint8_t *buf, uint32_t length)
{
	uint8_t subflag;
	uint8_t *pos = (uint8_t *)buf;
	struct litetalk_cmdinfo cinfo;

	pkg_pull_byte(cinfo.cmdtype, pos);
	pkg_pull_byte(cinfo.flag, pos);
	
	proto->callback(proto, LITETALK_CALLBACK_COMMAND, &cinfo, pos, length-2);

	return et_ok;
}

static err_type litetalk_read(struct cmder_protocol *proto, void *buf, int len)
{
	uint8_t flag;
	uint8_t category;
	uint32_t content_length;
	uint8_t *pos = (uint8_t *)buf;
	
	pos += 4;
	pkg_pull_byte(category, pos);
	pkg_pull_byte(flag, pos);
	pkg_pull_dwrd(content_length, pos);
	content_length = htonl(content_length);

	cmder_trace("category:0x%x", category);
	cmder_trace("flag:0x%x", flag);
	cmder_trace("contentlength:%d 0x%x", content_length, content_length);
	
	switch (category) {

	case LITETALK_CATEGORY_COMMAND:
		litetalk_command_read(proto, flag, pos, content_length);
		break;

	case LITETALK_CATEGORY_STREAM:
		litetalk_stream_read(proto, flag, pos, content_length);
		break;

	default:
		cmder_err("unknown category %d", category);
		return err_param;
	}

	return et_ok;
}

struct cmder_protocol litetalk_protocol = {
	.name = "LiteTalk",
	.match = litetalk_match,
	.read = litetalk_read,
};

awoke_buffchunk *litetalk_filedown_ack(uint8_t code)
{
	uint16_t mark;
	uint8_t type;
	uint8_t *pos, *head;
	awoke_buffchunk *p = awoke_buffchunk_create(8);

	pos = p->p;
	head = p->p;

	mark = 0x5656;
	type = 0x02;

	pkg_push_word(mark, pos);
	pkg_push_byte(type, pos);
	pkg_push_byte(code, pos);

	p->length = pos-head;
	return p;
}

err_type litetalk_pack_header(uint8_t *buf, uint8_t category, int length)
{
	uint8_t *pos = buf;
	uint8_t flag = 0x0;
	uint32_t marker = 0x13356789;

	marker = htonl(marker);
	pkg_push_dwrd(marker, pos);

	pkg_push_byte(category, pos);

	pkg_push_byte(flag, pos);

	length = htonl(length);
	pkg_push_dwrd(length, pos);

	return et_ok;
}

int litetalk_build_cmderr(void *buf, struct litetalk_cmdinfo *info, 
	uint8_t code)
{
	uint8_t *head = (uint8_t *)buf;
	uint8_t *pos = head + LITETALK_HEADERLEN;
	uint32_t value = 0x0;

	pkg_push_byte(info->cmdtype, pos);
	pkg_push_byte(code, pos);
	pkg_push_byte(info->flag, pos);
	pkg_push_dwrd(value, pos);

	litetalk_pack_header(head, LITETALK_CATEGORY_COMMAND, 7);

	return pos-head;
}

err_type litetalk_build_stream_ack(awoke_buffchunk *p, 
	uint8_t streamid, uint8_t index, uint8_t code)
{
	uint8_t *head = (uint8_t *)p->p + LITETALK_HEADERLEN;
	uint8_t *pos = head;
	uint8_t subtype = 0x02;

	pkg_push_byte(subtype, pos);
	pkg_push_byte(streamid, pos);
	pkg_push_byte(index, pos);
	pkg_push_byte(code, pos);

	litetalk_pack_header(p->p, LITETALK_CATEGORY_STREAM, pos-head);

	p->length = pos - head + LITETALK_HEADERLEN;
	
	return et_ok;
}