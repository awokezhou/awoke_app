/*
 * qe_serial.h
 *
 *  Created on: 2021Äê7ÔÂ19ÈÕ
 *      Author: z04200
 */

#ifndef __QE_SERIAL_H__
#define __QE_SERIAL_H__



#include "qe_core.h"


#define BAUD_RATE_2400                  2400
#define BAUD_RATE_4800                  4800
#define BAUD_RATE_9600                  9600
#define BAUD_RATE_19200                 19200
#define BAUD_RATE_38400                 38400
#define BAUD_RATE_57600                 57600
#define BAUD_RATE_115200                115200
#define BAUD_RATE_230400                230400
#define BAUD_RATE_460800                460800
#define BAUD_RATE_921600                921600
#define BAUD_RATE_2000000               2000000
#define BAUD_RATE_3000000               3000000

#define DATA_BITS_5                     5
#define DATA_BITS_6                     6
#define DATA_BITS_7                     7
#define DATA_BITS_8                     8
#define DATA_BITS_9                     9

#define STOP_BITS_1                     0
#define STOP_BITS_2                     1
#define STOP_BITS_3                     2
#define STOP_BITS_4                     3

#define PARITY_NONE                     0
#define PARITY_ODD                      1
#define PARITY_EVEN                     2

#define BIT_ORDER_LSB                   0
#define BIT_ORDER_MSB                   1

#define NRZ_NORMAL                      0       /* Non Return to Zero : normal mode */
#define NRZ_INVERTED                    1       /* Non Return to Zero : inverted mode */

#ifndef QE_SERIAL_RBSZ
#define QE_SERIAL_RBSZ					64
#endif

struct qe_serial_configure {
    qe_uint32_t baud_rate;

    qe_uint32_t data_bits               :4;
    qe_uint32_t stop_bits               :2;
    qe_uint32_t parity                  :2;
    qe_uint32_t bit_order               :1;
    qe_uint32_t invert                  :1;
    qe_uint32_t bufsz                   :16;
    qe_uint32_t reserved                :6;
};

struct qe_serial_rxfifo {
	qe_uint8_t *buffer;
	qe_uint16_t put_index;
	qe_uint16_t get_index;
	qe_bool_t	is_full;
};

struct qe_serial_rxdma {
    qe_bool_t activated;
};

struct qe_serial_device {
    struct qe_device          	parent;

    const struct qe_uart_ops 	*ops;
    struct qe_serial_configure   config;

    void *rx;
    void *tx;
};
typedef struct qe_serial_device qe_serial_t;

/**
 * uart operators
 */
struct qe_uart_ops {
    qe_err_t (*configure)(struct qe_serial_device *serial, struct qe_serial_configure *cfg);
    qe_err_t (*control)(struct qe_serial_device *serial, int cmd, void *arg);

    int (*putc)(struct qe_serial_device *serial, char c);
    int (*getc)(struct qe_serial_device *serial);

    qe_size_t (*dma_transmit)(struct qe_serial_device *serial, qe_uint8_t *buf, qe_size_t size, int direction);
};



qe_err_t qe_serial_register(qe_serial_t              *serial,
								   const char 			   *name,
								   qe_uint32_t 			    flag,
								   void 				    *data);

#endif /* __QE_SERIAL_H__ */


