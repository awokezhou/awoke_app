
#include "proto-xfer.h"
#include "cmder.h"
#include "cmder_protocol.h"
#include "awoke_package.h"


static err_type xfer_scan(struct cmder_protocol *proto, void *buf, int len, int *rlen)
{
	uint32_t marker;
	uint8_t *p;
	uint8_t *head = (uint8_t *)buf;
	uint8_t *pos = head;
	uint8_t *end = buf+len-1;

	if (len < 1) {
		*rlen = 0;
		return err_notfind;
	}

	while (pos != end) {
		p = pos;
		pkg_pull_byte(marker, p);
		if (marker == XFER_MARK) {
			break;
		}
		pos++;
	}

	*rlen = ((pos==end) ? len : (pos-head));
	return et_ok;
}

static err_type xfer_match(struct cmder_protocol *proto, void *buf, int len, int *rlen)
{
	uint8_t *head = buf; 
	uint8_t *pos = head;
	uint8_t *p;
	uint8_t marker;
	uint8_t postfix;
	uint8_t command;
	bool find_postfix = FALSE;

	if (len < 4) {
		return et_unfinished;
	}

	pkg_pull_byte(marker, pos);
	pkg_pull_byte(command, pos);
	if (marker != XFER_MARK) {
		return err_match;
	}
	lib_debug("marker:0x%x", marker);

	/* find postfix */
	while (pos != (head+len)) {
		p = pos;
		pkg_pull_byte(postfix, p);
		lib_debug("postfix:0x%x", postfix);
		if (postfix == XFER_POSTFIX) {
			find_postfix = TRUE;
			lib_debug("find postfix");
			break;
		}
		pos++;
	}

	if (!find_postfix) {
		*rlen = 0;
		return err_match;
	}

	*rlen = pos-head;
	return et_ok;
}

static err_type xfer_read(struct cmder_protocol *proto, void *buf, int len) 
{
	uint8_t *head = buf; 
	uint8_t *pos = head;
	struct xferinfo info;

	/* skip mark */
	pos += 1;

	pkg_pull_byte(info.ctype, pos);
	pkg_pull_word(info.length, pos);
	pkg_pull_dwrd(info.address, pos);

	lib_debug("command:%d length:%d address:0x%x", 
		info.ctype, info.length, info.address);

	/*
	switch(space) {

	case XFER_SPACE_MB:
		break;

	case XFER_SPACE_CONFIG:
		break;

	case XFER_SPACE_EPC:
		break;

	case XFER_SPACE_FLASH:
		break;

	case XFER_SPACE_SENSOR:
		break;

	default:
		lib_err("unknown space:%d", space);
		break;
	}

	return xfer_action(proto, addr, len, opt);*/
	return et_ok;
}

struct cmder_protocol xfer_protocol = {
	.name = "xfer",
	.read  = xfer_read,
	.scan  = xfer_scan,
	.match = xfer_match,
};

