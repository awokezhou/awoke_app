
#include "qe_core.h"
#include "qe_serial.h"


static struct qe_serial_device serial0;

static int qex_hw_uart_init(void)
{
	/*
	struct qe_serial_configure config;

	config.baud_rate = BAUD_RATE_115200;
	config.bit_order = BIT_ORDER_LSB;
	config.data_bits = DATA_BITS_8;
	config.parity    = PARITY_NONE;
	config.invert    = NRZ_NORMAL;
	config.bufsz     = QE_SERIAL_RBSZ;*/
	
	qe_serial_register(&serial0, "uart0", 
					   QE_DEV_F_RDWR | QE_DEV_F_INT_RX,
					   QE_NULL);
	return 0;
}
QE_BOARD_EXPORT(qex_hw_uart_init);
