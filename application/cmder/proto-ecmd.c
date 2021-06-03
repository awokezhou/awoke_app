
#include "proto-ecmd.h"
#include "awoke_log.h"
#include "awoke_package.h"


static err_type ecmd_match(struct cmder_protocol *proto, void *buf, int len, int *rlen)
{
    uint8_t *head = (uint8_t *)buf;
    uint8_t *pos = head;
    uint16_t prefix;

    if (len < 2) {
        return err_match;
    }

    pkg_pull_word(prefix, pos);
    prefix = awoke_htons(prefix);
    lib_debug("prefix:0x%x", prefix);

    if (prefix != 0x8101) {
        return err_match;
    }

    if (len < 6) {
        return err_incomplete;
    }

    if (head[5] != 0xff) {
        *rlen = 2;
        return err_param;
    }

    *rlen = 6;

    return et_ok;
}

static err_type ecmd_scan(struct cmder_protocol *proto, void *buf, int len, int *rlen)
{
    uint8_t *head = (uint8_t *)buf;
    uint8_t *pos = head, *p;
    uint16_t prefix;
    bool find = FALSE;

    while (pos != (head+len)) {
        p = pos;
        pkg_pull_word(prefix, p);
        prefix = awoke_htons(prefix);
        if (prefix == 0x8101) {
            find = TRUE;
            break;
        }
        pos++;
    }

    if (!find) {
        return err_match;
    } else {
        *rlen = (pos - head);
        return et_ok;
    }
}

static err_type ecmd_read(struct cmder_protocol *proto, void *buf, int len)
{
    lib_debug("ecmd read");
    return et_ok;
}

struct cmder_protocol ecmd_protocol = {
	.name = "ecmd",
	.read = ecmd_read,
	.scan = ecmd_scan,
	.match = ecmd_match,
};

static err_type ecmd2_match(struct cmder_protocol *proto, void *buf, int len, int *rlen)
{
    uint8_t *head = (uint8_t *)buf;
    uint8_t *pos = head;
    uint16_t prefix;

    if (len < 2) {
        return err_match;
    }

    pkg_pull_word(prefix, pos);
    prefix = awoke_htons(prefix);
    lib_debug("prefix:0x%x", prefix);

    if (prefix != 0x8201) {
        return err_match;
    }

    if (len < 6) {
        return err_incomplete;
    }

    if (head[5] != 0xff) {
        return err_match;
    }

    *rlen = 6;

    return et_ok;
}

static err_type ecmd2_scan(struct cmder_protocol *proto, void *buf, int len, int *rlen)
{
    uint8_t *head = (uint8_t *)buf;
    uint8_t *pos = head, *p;
    uint16_t prefix;
    bool find = FALSE;

    while (pos != (head+len)) {
        p = pos;
        pkg_pull_word(prefix, p);
        prefix = awoke_htons(prefix);
        if (prefix == 0x8201) {
            find = TRUE;
            break;
        }
        pos++;
    }

    if (!find) {
        return err_match;
    } else {
        *rlen = (pos - head);
        return et_ok;
    }
}

static err_type ecmd2_read(struct cmder_protocol *proto, void *buf, int len)
{
    lib_debug("ecmd2 read");
    return et_ok;
}

struct cmder_protocol ecmd2_protocol = {
	.name = "ecmd2",
	.read = ecmd2_read,
	.scan = ecmd2_scan,
	.match = ecmd2_match,
};