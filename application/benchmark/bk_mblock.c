
#include "bk_mblock.h"


static err_type xilspi_probe(struct bk_spi_device *dev);
static err_type spansion_probe(struct bk_flash_device *dev);
static err_type bk_iostream_msg_init(struct bk_iostream *io, char *buf, int nr, int size);
static err_type bk_iostream_msg_sync(struct bk_iostream *io);
static err_type bk_iostream_msg_add(struct bk_iostream *io, struct bk_iostream_msg *msg);



static awoke_list flash_driver_list;

static struct bk_flash_device flash_devices[] = {
	{.dev={.id=BK_DEV_ID_MICRON_FLASH},},
	{.dev={.id=BK_DEV_ID_SPANSION_FLASH},},
};

static struct bk_mbdev_match spansion_flash_matchs[] = {
	{"spansion", BK_DEV_ID_SPANSION_FLASH,},
};

static struct bk_flash_driver spansion_flash_driver = {
	.match_table = spansion_flash_matchs,
	.match_table_size = array_size(spansion_flash_matchs),
	.probe = spansion_probe,
};

static struct bk_flash_controller flash_controllers[2];

static awoke_list spi_driver_list;

static struct bk_mbdev_match spi_matchs[] = {
	{"xilspi", BK_DEV_ID_XILSPI,},
};

static struct bk_spi_device spi_devices[] = {
	{.dev={.id=BK_DEV_ID_XILSPI},},
};

static struct bk_spi_driver xilspi_driver = {
	.match_table = spi_matchs,
	.match_table_size = array_size(spi_matchs),
	.probe = xilspi_probe,
};

static struct bk_spi_master spi_masters[2];

static int spansion_readcmd(struct bk_flash_controller *c, uint32_t addr)
{
	return 3;
}

static int spansion_writecmd(struct bk_flash_controller *c, uint32_t addr)
{
	return 3;
}

static int spansion_erasecmd(struct bk_flash_controller *c, uint32_t addr)
{
	return 3;
}

static err_type spansion_probe(struct bk_flash_device *dev)
{
	int size;
	bool find = FALSE;
	struct bk_flash_controller *head, *p, *controller;

	head = flash_controllers;
	size = array_size(flash_controllers);

	array_foreach_start(head, size, p) {
		if (!mask_exst(p->flags, BK_DEV_F_INSTANCE)) {
			find = TRUE;
			break;
		}
	} array_foreach_end();

	if (!find) {
		bk_err("there have no uninstanced controller");
		return et_notfind;
	}

	controller = p;
	controller->build_readcmd = spansion_readcmd;
	controller->build_writecmd = spansion_writecmd;
	controller->build_erasecmd = spansion_erasecmd;

	controller->dev = dev;
	mask_push(controller->flags, BK_DEV_F_INSTANCE);

	return et_ok;
}

static err_type xilspi_setup(struct bk_spi_device *dev)
{
	bk_trace("xil spi fifo reset");
	return et_ok;
}

static err_type xilspi_cs_control(struct bk_spi_device *dev, bool val)
{
	bk_trace("xilspi cs:%d", val);
	return et_ok;
}

static err_type xilspi_io_prepare(struct bk_iostream *io, void *priv)
{
	struct bk_spi_device *dev = priv;
	xilspi_setup(dev);
	xilspi_cs_control(dev, TRUE);
	return et_ok;
}

static err_type xilspi_io_sync(struct bk_iostream *io, void *priv)
{
	struct bk_iostream_msg msg;
	
	bk_trace("xilspi sync");

	while (!awoke_fifo_empty(&io->message)) {
		awoke_fifo_dequeue(&io->message, &msg);
		bk_trace("msg deq");
		sleep(1);
	}
	
	return et_ok;
}

static err_type xilspi_io_finish(struct bk_iostream *io, void *priv)
{
	struct bk_spi_device *dev = priv;
	xilspi_cs_control(dev, FALSE);
	return et_ok;
}

static err_type xilspi_probe(struct bk_spi_device *dev)
{
	int size;
	bool find = FALSE;
	struct bk_spi_master *head, *p, *master;
	struct bk_iostream_ch *ch;

	head = spi_masters;
	size = array_size(spi_masters);

	array_foreach_start(head, size, p) {
		if (!mask_exst(p->flags, BK_DEV_F_INSTANCE)) {
			find = TRUE;
			break;
		}
	} array_foreach_end();

	if (!find) {
		bk_err("there have no uninstanced master");
		return et_notfind;
	}

	master = p;
	master->setup = xilspi_setup;

	ch = &master->io_channel;
	ch->prepare = xilspi_io_prepare;
	ch->sync = xilspi_io_sync;
	ch->finish = xilspi_io_finish;
	ch->priv = master;

	master->dev = dev;
	mask_push(master->flags, BK_DEV_F_INSTANCE);

	return et_ok;
}

static struct bk_mbdev_match *bk_mbdev_matchdev(struct bk_mbdev_match *head, int size, 
	struct bk_mb_device *dev)
{
	struct bk_mbdev_match *p;

	array_foreach_start(head, size, p) {

		if (p->id == dev->id) {
			bk_trace("match device:%s:%d", p->name, p->id);
			return p;
		}

	} array_foreach_end();

	return NULL;
}

static struct bk_spi_device *bk_mbdev_spi_devfind(uint8_t devid)
{
	int size;
	struct bk_spi_device *head, *p;

	head = spi_devices;
	size = array_size(spi_devices);

	array_foreach_start(head, size, p) {

		if (p->dev.id == devid) {
			bk_trace("find device:%d", devid);
			return p;
		}

	} array_foreach_end();

	return NULL;
}

static err_type bk_spidev_register(uint8_t devid)
{
	struct bk_mbdev_match *match;
	struct bk_spi_device *dev;
	struct bk_spi_driver *drv;

	/* find device */
	dev = bk_mbdev_spi_devfind(devid);
	if (!dev) {
		bk_err("dev:%d not find", devid);
		return et_notfind;
	}

	/* match driver */
	list_for_each_entry(drv, &spi_driver_list, _head) {

		match = bk_mbdev_matchdev(drv->match_table, drv->match_table_size, &dev->dev);
		if (match) {
			return drv->probe(dev);
		}
	}

	return et_notfind;	
}

static struct bk_flash_device *bk_mbdev_flash_devfind(uint8_t devid)
{
	int size;
	struct bk_flash_device *head, *p;

	head = flash_devices;
	size = array_size(flash_devices);

	array_foreach_start(head, size, p) {

		if (p->dev.id == devid) {
			bk_trace("find device:%d", devid);
			return p;
		}

	} array_foreach_end();

	return NULL;	
}

static err_type bk_flashdev_register(uint8_t devid)
{
	struct bk_mbdev_match *match;
	struct bk_flash_device *dev;
	struct bk_flash_driver *drv;

	/* find device */
	dev = bk_mbdev_flash_devfind(devid);
	if (!dev) {
		bk_err("dev:%d not find", devid);
		return et_notfind;
	}

	/* match driver */
	list_for_each_entry(drv, &flash_driver_list, _head) {

		match = bk_mbdev_matchdev(drv->match_table, drv->match_table_size, &dev->dev);
		if (match) {
			return drv->probe(dev);
		}
	}

	return et_notfind;
}

err_type bk_flash_read(struct bk_flash_controller *c, uint32_t addr, uint8_t *buf, int len)
{
	err_type ret;
	struct bk_iostream_msg msg[2];
	char msgbuf[64] = {0x0};
	
	bk_iostream_msg_init(&c->io, msgbuf, 2, 64);
	bk_trace("msg init");
	
	c->build_readcmd(c, addr);
	msg[0].tx_buf = c->command;
	msg[0].len = len;

	bk_iostream_msg_add(&c->io, &msg[0]);
	bk_trace("msg[0] add");

	msg[1].rx_buf = c->command;
	msg[1].len = len;
	bk_iostream_msg_add(&c->io, &msg[1]);
	bk_trace("msg[1] add");

	ret = bk_iostream_msg_sync(&c->io);
	if (ret != et_ok) {
		bk_err("iostream sync error:%d", ret);
		return ret;
	}

	return ret;
}

static err_type bk_flash_instance(struct bk_flash_controller **c, uint8_t id)
{
	int size;
	struct bk_flash_controller *head, *p;

	head = flash_controllers;
	size = array_size(flash_controllers);

	array_foreach_start(head, size, p) {
		if (id == p->dev->dev.id) {
			bk_debug("find flash controller:%d", id);
			*c = p;
			return et_ok;
		}
	} array_foreach_end();

	bk_err("flash controller:%d not find", id);
	return et_notfind;
}

static err_type bk_spi_instance(struct bk_spi_master **m, uint8_t id)
{
	int size;
	struct bk_spi_master *head, *p;

	head = spi_masters;
	size = array_size(spi_masters);

	array_foreach_start(head, size, p) {
		if (id == p->dev->dev.id) {
			bk_debug("find spi master:%d", id);
			*m = p;
			return et_ok;
		}
	} array_foreach_end();

	bk_err("spi master:%d not find", id);
	return et_notfind;
}

static err_type bk_spi_init(void)
{
	int size;
	struct bk_spi_master *head, *p;

	head = spi_masters;
	size = array_size(spi_masters);

	array_foreach_start(head, size, p) {
		memset(p, 0x0, sizeof(struct bk_spi_master));
	} array_foreach_end();

	list_init(&spi_driver_list);

	list_append(&xilspi_driver._head, &spi_driver_list);

	return et_ok;
}

static err_type bk_flash_init(void)
{
	int size;
	struct bk_flash_controller *head, *p;

	head = flash_controllers;
	size = array_size(flash_controllers);

	array_foreach_start(head, size, p) {
		memset(p, 0x0, sizeof(struct bk_flash_controller));
	} array_foreach_end();

	list_init(&flash_driver_list);

	list_append(&spansion_flash_driver._head, &flash_driver_list);

	return et_ok;
}

static err_type bk_iostream_msg_init(struct bk_iostream *io, char *buf, int nr, int size)
{
	awoke_afifo_create(buf, nr, size, &io->message);
	return et_ok;
}

static err_type bk_iostream_msg_sync(struct bk_iostream *io)
{
	struct bk_iostream_ch *ch = io->ch;

	ch->prepare(io, ch->priv);

	ch->sync(io, ch->priv);

	ch->finish(io, ch->priv);

	return et_ok;
}

static err_type bk_iostream_msg_add(struct bk_iostream *io, struct bk_iostream_msg *msg)
{
	return awoke_afifo_enqueue(&io->message, msg);
}

static void bk_iostream_connect(struct bk_iostream *io, struct bk_iostream_ch *ch)
{
	io->ch = ch;
}

static err_type bk_mbdev_init(void)
{
	bk_flash_init();
	bk_spi_init();
	return et_ok;
}

err_type benchmark_mblock_test(int argc, char *argv[])
{
	struct bk_spi_master *master = NULL;
	struct bk_flash_controller *flash = NULL;
	uint8_t buffer[32] = {0x0};

	bk_mbdev_init();
	
	bk_spidev_register(BK_DEV_ID_XILSPI);
	//bk_flashdev_register(BK_DEV_ID_MICRON_FLASH);
	bk_flashdev_register(BK_DEV_ID_SPANSION_FLASH);

	
	bk_flash_instance(&flash, BK_DEV_ID_SPANSION_FLASH);
	bk_spi_instance(&master, BK_DEV_ID_XILSPI);
	
	bk_iostream_connect(&flash->io, &master->io_channel);

	bk_flash_read(flash, 0x0, buffer, 32);
	
	return et_ok;
}

