
#include "cmder.h"
#include "cmder_protocol.h"
#include "proto-litetalk.h"
#include "awoke_package.h"


static err_type litetalk_match(struct cmder_protocol *proto, void *buf, int len)
{
	uint8_t *pos = buf;
	uint32_t marker;
	uint32_t content_length;

	pkg_pull_dwrd(marker, pos);
	marker = awoke_htonl(marker);
	pos += 2;
	pkg_pull_dwrd(content_length, pos);
	content_length = awoke_htonl(content_length);

	//cmder_trace("mark:0x%x", marker);
	
	if (marker != LITETALK_MARK) {
		return err_match;
	}

	//cmder_trace("len:%d content length:%d", len, content_length);
	if (len == (content_length+LITETALK_HEADERLEN)) {
		return et_ok;
	} else if (len < (content_length+LITETALK_HEADERLEN)) {
		cmder_trace("rx not finish");
		return err_unfinished;
	}

	return et_ok;
}

static err_type litetalk_stream_download(struct cmder_protocol *proto, 
	uint8_t flag, uint8_t *buf, uint32_t length)
{
	uint8_t *pos = (uint8_t *)buf;
	struct litk_streaminfo sinfo;

	pkg_pull_byte(sinfo.media, pos);

	pkg_pull_dwrd(sinfo.totalsize, pos);
	sinfo.totalsize = awoke_htonl(sinfo.totalsize);

	pkg_pull_byte(sinfo.id, pos);

	pkg_pull_word(sinfo.index, pos);
	sinfo.index = awoke_htons(sinfo.index);

	sinfo.length = length - LITETALK_STREAM_HEADERLEN;

	cmder_debug("media:%d", sinfo.media);
	cmder_debug("streamid:%d", sinfo.id);
	cmder_debug("index:%d", sinfo.index);
	cmder_debug("totalsize:%d", sinfo.totalsize);
	cmder_debug("length:%d", sinfo.length);

	return proto->callback(proto, LITETALK_CALLBACK_STREAM_DOWNLOAD, 
		&sinfo, pos, sinfo.length);
}

#if (LITETALK_CONFIG_CMI)
static err_type litetalk_cmdi_read(struct cmder_protocol *proto, 
	uint8_t flag, uint8_t *buf, uint32_t length)
{
	struct litetalk_cmdireq creq;
	uint8_t *pos = (uint8_t *)buf;

	pkg_pull_byte(creq.subtype, pos);
	pkg_pull_byte(creq.requestid, pos);
	
	pkg_pull_dwrd(creq.address[0], pos);
	pkg_pull_dwrd(creq.address[1], pos);
	creq.address[0] = awoke_htonl(creq.address[0]);
	creq.address[1] = awoke_htonl(creq.address[1]);
	
	if (creq.subtype == LITETALK_CMDI_SET_REQUEST) {
		pkg_pull_byte(creq.dtype, pos);
		pkg_pull_word(creq.datalen, pos);
		creq.datalen = awoke_htons(creq.datalen);
	}

	return proto->callback(proto, LITETALK_CALLBACK_CMDI_REQUEST, 
		&creq, pos, creq.datalen);
}
#endif

static err_type litetalk_stream_read(struct cmder_protocol *proto, 
	uint8_t flag, uint8_t *buf, uint32_t length)
{
	uint8_t subtype;
	uint8_t *pos = (uint8_t *)buf;
	
	pkg_pull_byte(subtype, pos);

	if (subtype == LITETALK_STREAM_ST_DOWNLOAD) {
		litetalk_stream_download(proto, flag, pos, length);
	} else if (subtype == LITETALK_STREAM_ST_ACK) {
		//litetalk_stream_ack(proto, flag, buf, length);
	}
	
	return et_ok;
}

static err_type litetalk_command_read(struct cmder_protocol *proto, 
	uint8_t flag, uint8_t *buf, uint32_t length)
{
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
	content_length = awoke_htonl(content_length);

	//cmder_trace("category:0x%x", category);
	//cmder_trace("flag:0x%x", flag);
	//cmder_trace("contentlength:%d 0x%x", content_length, content_length);
	
	switch (category) {

	case LITETALK_CATEGORY_COMMAND:
		litetalk_command_read(proto, flag, pos, content_length);
		break;

	case LITETALK_CATEGORY_STREAM:
		litetalk_stream_read(proto, flag, pos, content_length);
		break;

#if (LITETLAK_CONFIG_CMI)
	case LITETALK_CATEGORY_CMDI:
		litetalk_cmdi_read(proto, flag, pos, content_length);
		break;
#endif

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

err_type litetalk_pack_header(uint8_t *buf, uint8_t category, int length)
{
	uint8_t *pos = buf;
	uint8_t flag = 0x0;
	uint32_t marker = LITETALK_MARK;

	marker = awoke_htonl(marker);
	pkg_push_dwrd(marker, pos);

	pkg_push_byte(category, pos);

	pkg_push_byte(flag, pos);

	length = awoke_htonl(length);
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
	uint8_t streamid, uint16_t index, uint8_t code)
{
	uint8_t *head = (uint8_t *)p->p + LITETALK_HEADERLEN;
	uint8_t *pos = head;
	uint8_t subtype = LITETALK_STREAM_ST_ACK;

	pkg_push_byte(subtype, pos);
	pkg_push_byte(streamid, pos);
	index = awoke_htons(index);
	pkg_push_word(index, pos);
	pkg_push_byte(code, pos);

	litetalk_pack_header((uint8_t *)p->p, LITETALK_CATEGORY_STREAM, pos-head);

	p->length = pos - head + LITETALK_HEADERLEN;
	
	return et_ok;
}