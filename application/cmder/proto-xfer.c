
#include "proto-xfer.h"
#include "cmder.h"
<<<<<<< HEAD
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
=======
#include "awoke_package.h"


static uint8_t sg_txbuf[512] = {0x0};
static uint8_t sg_flashbuff[4096] = {0x0};

static err_type xfer_match(struct cmder_protocol *proto, void *buf, int len, int *rlen)
{
	int length;
	struct xfer_head *head = (struct xfer_head *)buf;
	uint8_t *pos = (uint8_t *)buf;

	if ((len < (sizeof(struct xfer_head) + sizeof(struct xfer_tail)))) {
		return err_unfinished;
	}

	if (head->mark != XFER_PREFIX) {
		*rlen = 1;
		return err_match;
	}

	length = sizeof(struct xfer_head) + sizeof(struct xfer_tail);
	if (head->cmd == XFER_CMD_WR) {
		length += head->len;
	}
	lib_debug("length:%d", length);
	
	if (len < length) {
		return err_unfinished;
	}
	
	if (pos[length-1] != XFER_POSFIX) {
		*rlen = 1;
		return err_match;
	}

	*rlen = length;

	return et_ok;
}

static err_type xfer_scan(struct cmder_protocol *proto, void *buf, int len, int *rlen)
{
	int i;
	err_type ret;
	bool find = FALSE;
	uint8_t *head = (uint8_t *)buf;

	for (i=*rlen; i<len; i++) {
		if (head[i] == XFER_PREFIX) {
			find = TRUE;
			break;
		}
	}

	if (find) {
		*rlen = i;
		ret = et_ok;
	} else {
		*rlen = len;
		ret = err_match;
	}

	lib_debug("rlen:%d", *rlen);

	return ret;
}

static err_type xfer_pack_header(awoke_buffchunk *rsp, struct xfer_head *head, 
    uint8_t code, int len)
{
    uint8_t *pos;
    uint8_t postfix;
    struct xfer_head rsphdr;

    memcpy(&rsphdr, head, sizeof(struct xfer_head));
    if (head->cmd != XFER_CMD_RD) {
        rsphdr.len = 0;
    }

    memcpy(rsp->p, &rsphdr, sizeof(struct xfer_head));

    pos = (uint8_t *)rsp->p + sizeof(struct xfer_head) + len;
    postfix = XFER_POSFIX;
    pkg_push_byte(code, pos);
    pkg_push_byte(postfix, pos);
    rsp->length = sizeof(struct xfer_head) + len + sizeof(struct xfer_tail);
	return et_ok;
}

static err_type xfer_pack_mcudata(struct cmder_protocol *proto, uint8_t *addr, uint16_t len)
{
    uint8_t *pos;

    pos = (uint8_t *)&sg_txbuf + sizeof(struct xfer_head);

    memcpy(pos, addr, len);
    
	return et_ok;
}

static err_type xfer_pack_sensordata(struct cmder_protocol *proto, uint32_t addr, uint16_t len)
{
	int i;
	uint8_t value;
	uint8_t raddr[2];
    struct mbp_context *ctx = (struct mbp_context *)proto->context;

	lib_info("len:%d", len);

	for (i=0; i<len; i++) {
		raddr[0] = (addr>>8)&0xff;
		raddr[1] = addr & 0xff;
		//imgsensor_read_reg(ctx->isensor, raddr, &value);
		lib_info("addr:0x%x 0x%x val:0x%x", raddr[0], raddr[1], value);
		sg_txbuf[i] = value;
		addr++;
	}

	return uartcmder_senddata(sg_txbuf, i, proto->tid);
}

static err_type xfer_pack_flashdata(struct cmder_protocol *proto, uint32_t addr, uint16_t len)
{
	//nvm_block_read(addr, sg_txbuf, len);
    int i;
    int offset = sizeof(struct xfer_head);

    for (i=0; i<256; i++) {
        sg_txbuf[i+offset] = sg_flashbuff[addr+i];
    }

	return et_ok;
}

static err_type xfer_send_tail(struct cmder_protocol *proto, uint8_t code)
{
	sg_txbuf[0] = code;
	sg_txbuf[1] = XFER_POSFIX;

	return uartcmder_senddata(sg_txbuf, 2, proto->tid);
}

static err_type xfer_muc_cmd_proc(struct mbp_context *ctx, uint8_t cmd, bool factory)
{
	if (cmd == XFER_CMD_WR) {
		//mbp_config_save();
	} else if (XFER_CMD_RD) {
		//mbp_config_load(ctx, factory);
	}

	return et_ok;
}

static uint32_t xfer_vam_offset(struct mbp_context *ctx, uint32_t addr)
{
    uint32_t base, offset;
    
    if (addr >= XFER_VAM_OFFSET_SPC) {
        base = addr - XFER_VAM_OFFSET_SPC;
    } else if (addr >= XFER_VAM_OFFSET_NUC) {
        //base = addr - XFER_VAM_OFFSET_NUC + ctx->nuc_block.p;
    } else if (addr >= XFER_VAM_OFFSET_BPC) {
        //base = addr - XFER_VAM_OFFSET_BPC + ctx->dpc_block.p;
    }

    return base;
}

static err_type xfer_pack_tail_send(struct cmder_protocol *proto, uint8_t code, struct xfer_head *head)
{
    int offset = sizeof(struct xfer_head);

    if (head->cmd == XFER_CMD_RD) {
        offset += head->len;
    }
    sg_txbuf[offset] = code;
	sg_txbuf[offset+1] = XFER_POSFIX;
    return uartcmder_senddata(sg_txbuf, offset+2, proto->tid);
}

static err_type xfer_response_build(awoke_buffchunk *rsp, struct xfer_head *head, 
    uint8_t code, uint8_t *buf, int len)
{
    int padding = 0;
	int datalen = 0;
    uint8_t *phead = (uint8_t *)rsp->p + sizeof(struct xfer_head);
    uint8_t *pos = phead;

    if (head->cmd == XFER_CMD_RD) {
        datalen = (head->len <= len) ? head->len : len;
        if (head->len > len) {
            padding = head->len-len;
        }
        lib_debug("datalen:%d padding:%d", datalen, padding);
        memcpy(pos, buf, datalen);
        pos += datalen;
        if (padding) {
            memset(pos, 0x0, padding);
            pos += padding;
        }
    }

    lib_debug("datalen:%d", pos-phead);

    return xfer_pack_header(rsp, head, code, pos-phead);
}

char pdtmodel[16] = "Cobra2000";
uint32_t cmdid = 1234567;

uint8_t xfer_mcu_range(uint32_t addr)
{
    if (addr >= XFER_MCU_ALGORITHM_BASE) {  
        return XFER_ADDR_RANGE_ALGORITHM;
    } else if (addr >= XFER_MCU_PARAMS_BASE) {
        return XFER_ADDR_RANGE_PARAMS;
    } else {
        return XFER_ADDR_RANGE_BOOTINFO;
    }
}

static err_type xfer_bitwidth_get(struct uartcmder *ctx, struct cmder_protocol *proto, 
    struct xfer_head *head, awoke_buffchunk *rsp)
{
    uint32_t data;
    data = 10;
    lib_info("bitwidth:%d", data);
    return xfer_response_build(rsp, head, 0, (uint8_t *)&data, 4);
}

static err_type xfer_fps_set(struct uartcmder *ctx, struct cmder_protocol *proto, 
    uint8_t *in, int length)
{
    uint32_t data;
    pkg_pull_dwrd(data, in);
    lib_info("fps:%d", data);
    return et_ok;
}

static err_type xfer_fps_get(struct uartcmder *ctx, struct cmder_protocol *proto, 
    struct xfer_head *head, awoke_buffchunk *rsp)
{
    uint32_t data;
    data = 100;
    lib_info("fps:%d", data);
    return xfer_response_build(rsp, head, 0, (uint8_t *)&data, 4);
}

static err_type xfer_fpsmin_set(struct uartcmder *ctx, struct cmder_protocol *proto, 
    uint8_t *in, int length)
{
    uint32_t data;
    pkg_pull_dwrd(data, in);
    lib_info("fps min:%d", data);
    return et_ok;
}

static err_type xfer_fpsmin_get(struct uartcmder *ctx, struct cmder_protocol *proto, 
    struct xfer_head *head, awoke_buffchunk *rsp)
{
    uint32_t data;
    data = 1;
    lib_info("fps min:%d", data);
    return xfer_response_build(rsp, head, 0, (uint8_t *)&data, 4);
}

static err_type xfer_fpsmax_set(struct uartcmder *ctx, struct cmder_protocol *proto, 
    uint8_t *in, int length)
{
    uint32_t data;
    pkg_pull_dwrd(data, in);
    lib_info("fps max:%d", data);
    return et_ok;
}

static err_type xfer_fpsmax_get(struct uartcmder *ctx, struct cmder_protocol *proto, 
    struct xfer_head *head, awoke_buffchunk *rsp)
{
    uint32_t data;
    data = 1;
    lib_info("fps max:%d", data);
    return xfer_response_build(rsp, head, 0, (uint8_t *)&data, 4);
}

static err_type xfer_trigger_set(struct uartcmder *ctx, struct cmder_protocol *proto, 
    uint8_t *in, int length)
{
    uint32_t data;
    pkg_pull_dwrd(data, in);
    lib_info("trigger:%d", data);
    return et_ok;
}

static err_type xfer_trigger_get(struct uartcmder *ctx, struct cmder_protocol *proto, 
    struct xfer_head *head, awoke_buffchunk *rsp)
{
    uint32_t data;
    data = 0;
    lib_info("trigger:%d", data);
    return xfer_response_build(rsp, head, 0, (uint8_t *)&data, 4);
}

static err_type xfer_invs_en_set(struct uartcmder *ctx, struct cmder_protocol *proto, 
    uint8_t *in, int length)
{
    uint32_t data;
    pkg_pull_dwrd(data, in);
    lib_info("invs en:%d", data);
    return et_ok;
}

static err_type xfer_invs_en_get(struct uartcmder *ctx, struct cmder_protocol *proto, 
    struct xfer_head *head, awoke_buffchunk *rsp)
{
    uint32_t data;
    data = 0;
    lib_info("invs en:%d", data);
    return xfer_response_build(rsp, head, 0, (uint8_t *)&data, 4);
}

static err_type xfer_config_save(struct uartcmder *ctx, struct cmder_protocol *proto, 
    uint8_t *in, int length)
{
    lib_info("config save");
    return et_ok;
}

static err_type xfer_config_restore(struct uartcmder *ctx, struct cmder_protocol *proto, 
    uint8_t *in, int length)
{
    lib_info("config restore");
    return et_ok;
}

static struct xfer_mcu_req xfer_mcu_reqtab[] = {
    {XFER_ADDR_BITWIDTH,        NULL,                   xfer_bitwidth_get},
    {XFER_ADDR_FPS,             xfer_fps_set,           xfer_fps_get},
    {XFER_ADDR_FPS_MIN,         xfer_fpsmin_set,        xfer_fpsmin_get},
    {XFER_ADDR_FPS_MAX,         xfer_fpsmax_set,        xfer_fpsmax_get},
    {XFER_ADDR_TRIGGER,         xfer_trigger_set,       xfer_trigger_get},
    {XFER_ADDR_INVS_EN,         xfer_invs_en_set,       xfer_invs_en_get},
    {XFER_ADDR_CONFIG_SAVE,     xfer_config_save,       NULL},
    {XFER_ADDR_CONFIG_RESTORE,  xfer_config_restore,    NULL},
};

static err_type xfer_mcu_params_process(struct uartcmder *ctx, struct cmder_protocol *proto, 
    struct xfer_head *head, uint8_t *rpos)
{
    int tsize;
    err_type ret;
    struct xfer_mcu_req *thead, *p;
    awoke_buffchunk *rsp;

    thead = xfer_mcu_reqtab;
    tsize = array_size(xfer_mcu_reqtab);

    rsp = awoke_buffchunk_create(512);

    array_foreach(thead, tsize, p) {

        if (p->address == head->addr) {
            if (head->cmd == XFER_CMD_WR) {
                ret = p->set(ctx, proto, rpos, head->len);
                xfer_response_build(rsp, head, ret, NULL, 0);
            } else if (head->cmd == XFER_CMD_RD) {
                ret = p->get(ctx, proto, head, rsp);
                awoke_hexdump_info(rsp->p, rsp->length);
            } else {
                ret = err_notfind;
                xfer_response_build(rsp, head, ret, NULL, 0);
            }
            return ret;
        }
    };

    xfer_response_build(rsp, head, ret, err_notfind, 0);
    return err_notfind;
}

static err_type xfer_mcu_process(struct uartcmder *ctx, struct cmder_protocol *proto, 
    struct xfer_head *head, uint8_t *rpos)
{
    uint8_t range;

    range = xfer_mcu_range(head->addr);

    switch (range) {

    case XFER_MCU_ALGORITHM_BASE:
        lib_info("range:algorithm");
    case XFER_ADDR_RANGE_PARAMS:
        lib_info("range:params");
        xfer_mcu_params_process(ctx, proto, head, rpos);
        break;

    case XFER_ADDR_RANGE_BOOTINFO:
        lib_info("range:bootinfo");
        break;

    default:
        lib_err("unknown range:%d", range);
        break;
    }

    return et_ok;
}

static err_type xfer_read(struct cmder_protocol *proto, void *buf, int len)
{
    err_type ret;
	uint8_t *base = 0x0;
	uint8_t *data;
	uint8_t code = 0;
	bool flash_opt = FALSE;
	bool sensor_opt = FALSE;
	uint32_t flash_erase_size;
	struct xfer_head *head = (struct xfer_head *)buf;
    struct uartcmder *ctx = (struct uartcmder *)proto->context;

	/* response header */
	data = XFER_GET_DATA(head);

	lib_info("space:%d cmd:%d seq:%d data:0x%x offset:0x%x len:%d", 
        head->space, head->cmd, head->seq, data, head->addr, head->len);

	switch (head->space) {

	case XFER_SPACE_MCU:
		lib_debug("SPACE MCU");
		ret = xfer_mcu_process(ctx, proto, head, data);
		break;

	case XFER_SPACE_FPGA:
		lib_debug("SPACE FPGA");
		//base = (uint32_t)epc->data + head->addr;
		break;

	case XFER_SPACE_SENSOR:
		lib_debug("SPACE FPGA");
		//base = (uint32_t)head->addr;
		sensor_opt = TRUE;
		break;

	case XFER_SPACE_FLASH:
		lib_info("SPACE FLASH");
		flash_opt = TRUE;
		base = (uint8_t *)head->addr;
		break;

    case XFER_SPACE_VAM:
        lib_debug("SPACE VAM");
        //base = xfer_vam_offset(ctx, head->addr);
        lib_debug("base:0x%x", base);
        break;

	default:
		lib_err("unknown space:%d", head->space);
		break;
	}
    
    return ret;

	switch (head->cmd) {

	case XFER_CMD_RD:
		lib_debug("CMD RD");
		if (sensor_opt) {
			xfer_pack_sensordata(proto, base, head->len);
		} else if (flash_opt) {
			xfer_pack_flashdata(proto, base, head->len);
		}
		break;

	case XFER_CMD_WR:
		lib_debug("CMD WR");
		if (!flash_opt && !sensor_opt) {
			if (head->space == XFER_SPACE_FPGA) {
				for (int i=0; i<head->len; i+=4) {
					//*(unsigned int*)(base+i)=*(unsigned int*)(data+i);
				}
			} else {
				//memcpy((uint32_t *)base, data, head->len);
			}
		} else if (flash_opt) {
			lib_notice("base:0x%x len:%d", base, head->len);
			for (int i=0; i<head->len; i++) {
                sg_flashbuff[(uint32_t)base+i] = data[i];
            }
		} else if (sensor_opt) {
			for(int i=0; i<head->len; i++) {				 
				//imgsensor_write_reg(ctx->isensor, (uint8_t *)(base++), &data[i]);
			}
		}
		break;

	case XFER_CMD_CL:
		lib_debug("CMD CL");
		if (!flash_opt && !sensor_opt) {
			//memset((uint32_t *)base, 0x0, head->len);
		} else if (flash_opt) {
			if (head->len == XFER_FLASH_OUTRANGE) {
				flash_erase_size = XFER_FLASH_OUTRANGE_SIZE;
			} else {
				flash_erase_size = head->len;
			}
			//nvm_size_erase(base, flash_erase_size);
		}
		break;

	default:
		lib_err("unknown cmd:%d", head->cmd);
		break;
	}

    xfer_pack_tail_send(proto, code, head);

	//xfer_send_tail(proto, code);

>>>>>>> 9f093788c86a002f47df2a51c71270239926b9d2
	return et_ok;
}

struct cmder_protocol xfer_protocol = {
<<<<<<< HEAD
	.name = "xfer",
	.read  = xfer_read,
	.scan  = xfer_scan,
	.match = xfer_match,
=======
	.name = "Xfer",
	.read = xfer_read,
	.scan = xfer_scan,
	.match = xfer_match,
	.max_report = 10,
	.tid = 0,
>>>>>>> 9f093788c86a002f47df2a51c71270239926b9d2
};

