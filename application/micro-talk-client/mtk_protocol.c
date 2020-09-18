
#include "micro_talk.h"
#include "mtk_protocol.h"



err_type mtk_pkg_request_header(struct _mtk_request_msg *msg, mtk_method_e method, 
	int dlen, mtk_lbl_e label, const char *imei)
{
	int n, i;
	uint8_t checksum;
	uint8_t token[8];
	uint16_t length;

	if (!msg || (dlen<0) || !msg->payload)
		return et_param;

	uint8_t *pos = msg->payload;

	/* IMEI encode BCD8421 */
	n = awoke_string_str2bcdv2(token, imei, 16);
	if (n < 0) {
		log_err("imei encode error");
		return et_encode;
	}

	/* method */
	pkg_push_byte(method, pos);

	/* data length */
	length = htons(dlen);
	pkg_push_word(length, pos);

	/* label */
	pkg_push_byte(label, pos);

	/* token */
	pkg_push_bytes(token, pos, 8);

	/* checksum */
	pos += dlen;
	n = pos - msg->payload;
	checksum = 0;
	for (i=0; i<n; i++) {
		checksum += msg->payload[i];
	}
	pkg_push_byte(checksum, pos);

	msg->length = pos - msg->payload;

	return et_ok;
}