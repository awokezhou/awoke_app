/*
 * qe_spi.h
 *
 *  Created on: 2021Äê7ÔÂ20ÈÕ
 *      Author: z04200
 */

#ifndef __QE_SPI_H__
#define __QE_SPI_H__



#include "qe_core.h"



#define QE_SPI_BUS_MODE_SPI         (1<<0)
#define QE_SPI_BUS_MODE_QSPI        (1<<1)


struct qe_spi_message
{
    const void *send_buf;
    void *recv_buf;
    qe_size_t length;
    struct qe_spi_message *next;

    unsigned cs_take    : 1;
    unsigned cs_release : 1;
	unsigned reserve    : 30;
};

struct qe_spi_configuration
{
    qe_uint8_t mode;
    qe_uint8_t data_width;
    qe_uint16_t reserved;

    qe_uint32_t max_hz;
};

struct qe_spi_ops;
struct qe_spi_bus
{
    struct qe_device parent;
	
    qe_uint32_t mode 			:8;
	qe_uint32_t multi_access 	:1;
	qe_uint32_t reserve			:13;
	
    const struct qe_spi_ops *ops;

    void *lock;
	qe_err_t (*bus_lock)(struct qe_spi_bus *bus, qe_int32_t ms);
	void     (*bus_unlock)(struct qe_spi_bus *bus);
	
    struct qe_spi_device *owner;
};

struct qe_spi_ops
{
    qe_err_t (*configure)(struct qe_spi_device *device, struct qe_spi_configuration *configuration);
    qe_uint32_t (*xfer)(struct qe_spi_device *device, struct qe_spi_message *message);
};

struct qe_spi_device
{
    struct qe_device parent;
    struct qe_spi_bus *bus;

    struct qe_spi_configuration config;
    void   *user_data;
};



qe_size_t qe_spi_transfer(struct qe_spi_device *dev,
						  	     const void 		  *send_buf,
						         void                 *recv_buf,
						         qe_size_t             length);

qe_err_t qe_spi_send_recv(struct qe_spi_device *spi,
								   const void           *send_buf,
								   qe_size_t             send_length,
								   void                 *recv_buf,
								   qe_size_t             recv_length);
struct qe_spi_message *qe_spi_transfer_message(struct qe_spi_device *spi,
															struct qe_spi_message *message);

qe_err_t qe_spidev_register(struct qe_spi_device *spidev, const char *name);

qe_err_t qe_spi_bus_attach_device(struct qe_spi_device *spi,
											   const char           *name,
											   const char           *bus_name,
											   void                 *user_data);

qe_err_t qe_spi_bus_register(struct qe_spi_bus       *bus,
									  const char              *name,
									  const struct qe_spi_ops *ops);

#endif /* __QE_SPI_H__ */


