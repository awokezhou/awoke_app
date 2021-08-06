
#include "qe_core.h"
#include "qe_spi.h"
#include "benchmark.h"


struct xxx_spi_hw {
	qe_uint32_t regbase;
};

static struct xxx_spi_hw spi0_hw = {
	.regbase = 0x41000000
};

static qe_err_t xxx_spi_configure(struct qe_spi_device *spi, struct qe_spi_configuration *cfg)
{
	return qe_ok;
}

static qe_uint32_t xxx_spi_xfer(struct qe_spi_device *spi, struct qe_spi_message *message)
{
	//qe_uint32_t cr;
	qe_uint8_t rxdata = 0xff;
	qe_uint8_t txdata = 0xff;
	qe_uint8_t *txbuff = (qe_uint8_t *)message->send_buf;
	qe_uint8_t *rxbuff = (qe_uint8_t *)message->recv_buf;
	qe_uint32_t txbytes = message->length;
	qe_uint32_t rxbytes = message->length;
	qe_uint32_t rlen = message->length;
	//struct xxx_spi_hw *spihw = (struct xxx_spi_hw *)spi->user_data;

	if (message->cs_take) {
		//bk_debug("cs take");
	}

	//bk_debug("inhibit transmit, reset fifo");

	//bk_debug("start transmit");

	while (txbytes > 0 || rxbytes > 0) {

		while (txbytes > 0) {
			if (txbuff != QE_NULL) {
				txdata = *txbuff++;
				(void)txdata;
			}
			//bk_trace("txfifo:0x%x", txdata);
			txbytes--;
		}
		
		while (rxbytes > 0) {
			rxbytes--;
			//bk_trace("rxfifo:0x%x", rxdata);
			if (rxbuff != QE_NULL) {
				*rxbuff++ = rxdata;
			}
		}
	}

	if (txbytes!=0 || rxbytes!=0) {
		rlen = 0;
		goto __exit;
	}

__exit:
	//bk_debug("inhibit transmit, reset fifo");

	if (message->cs_release) {
		//bk_debug("cs release");
	}

	return rlen;
}

static struct qe_spi_bus spi0_bus;
static struct qe_spi_device spi0_device0;

static struct qe_spi_ops spi0_ops = {
	.configure = xxx_spi_configure,
	.xfer      = xxx_spi_xfer
};

static void xxx_spi_hw_init(struct xxx_spi_hw *hw)
{
	bk_debug("spi hw init:0x%x", hw->regbase);

	bk_debug("disable interrupt");

	bk_debug("reset fifo, master mode");
}

static int xxx_spi_bus_init(void)
{
	xxx_spi_hw_init(&spi0_hw);
	qe_spi_bus_register(&spi0_bus, "spi0", &spi0_ops);
	qe_spi_bus_attach_device(&spi0_device0, "spi0.0", "spi0", &spi0_hw);
	return 0;
}
QE_BOARD_EXPORT(xxx_spi_bus_init);