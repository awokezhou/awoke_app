
#include "cmder.h"
#include "proto-swift.h"
#include "awoke_package.h"


static bool swift_match(struct cmder_protocol *proto, void *buf)
{
	uint8_t *pos = buf;
	uint32_t marker;

	pkg_pull_dwrd(marker, pos);
	cmder_debug("marker:0x%x", marker);

	if (marker != 0x12345678) {
		cmder_debug("not match");
		return FALSE;
	}

	return TRUE;
}

static err_type swift_connect_accept(struct cmder_protocol *proto, uint8_t *pos)
{
	struct swift_connect *conn = proto->private;
	conn->f_connected = 1;
	return proto->callback(proto, SWIFT_CALLBACK_CONNECTION_ESTABLISHED, NULL, NULL, 0);
}

static err_type swift_read(struct cmder_protocol *proto, void *buf)
{
	uint8_t *pos = (uint8_t *)buf;
	uint8_t byte;
	struct swift_connect *conn = proto->private;

	pos += 4;
	pkg_pull_byte(byte, pos);
	cmder_debug("byte:0x%x", byte);
	
	if (!conn->f_connected) {	
		if (byte != 0x0) {
			cmder_debug("not connect package");
			return et_sock_accept;
		}
		
		return swift_connect_accept(proto, buf);
	}

	switch (byte) {

	case 0x01:
		cmder_debug("commands");
		break;

	case 0x02:
		cmder_debug("filestream");
		break;

	case 0x03:
		cmder_debug("others");
		break;
	}

	return proto->callback(proto, SWIFT_CALLBACK_CONNECTION_RECEIVE, NULL, buf, 2);
}

struct cmder_protocol swift_protocol = {
	.name = "swift",
	.match = swift_match,
	.read = swift_read,
};