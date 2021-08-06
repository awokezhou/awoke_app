/*
 * qe_spi_dev.c
 *
 *  Created on: 2021Äê7ÔÂ21ÈÕ
 *      Author: z04200
 */

#include "qe_spi.h"
#include "bk_quickemb.h"

qe_size_t spibus_device_read(qe_device_t  dev,
									   qe_off_t     pos,
									   void        *buffer,
									   qe_size_t    size)
{
	struct qe_spi_bus *bus;

	bus = (struct qe_spi_bus *)dev;
	qe_assert(bus != QE_NULL);
	qe_assert(bus->owner != QE_NULL);

	return qe_spi_transfer(bus->owner, QE_NULL, buffer, size);
}

qe_size_t spibus_device_write(qe_device_t  dev,
									   qe_off_t     pos,
									   const void  *buffer,
									   qe_size_t    size)
{
	struct qe_spi_bus *bus;

	bus = (struct qe_spi_bus *)dev;
	qe_assert(bus != QE_NULL);
	qe_assert(bus->owner != QE_NULL);

	return qe_spi_transfer(bus->owner, buffer, QE_NULL, size);
}

const static struct qe_device_ops spi_bus_ops = {
	QE_NULL,
	QE_NULL,
	QE_NULL,
	spibus_device_read,
	spibus_device_write,
	QE_NULL
};

qe_err_t qe_spi_bus_register(struct qe_spi_bus       *bus,
									  const char              *name,
									  const struct qe_spi_ops *ops)
{
	qe_err_t ret;
	struct qe_device *dev;
	
	qe_assert(bus != QE_NULL);

	dev = &bus->parent;

	dev->class = QE_DevClass_SPIBUS;
	dev->ops = &spi_bus_ops;
	
	ret = qe_device_register(dev, name, QE_DEV_F_RDWR);
	if (ret != qe_ok) {
		return ret;
	}
	bk_info("register spibus:%s", name);
	bus->ops = ops;
	bus->owner = QE_NULL;
	bus->mode = QE_SPI_BUS_MODE_SPI;

	return qe_ok;
}

static qe_size_t spidev_device_read(qe_device_t  dev,
										      qe_off_t     pos,
											  void        *buffer,
											  qe_size_t    size)
{
	struct qe_spi_device *spi;

	spi = (struct qe_spi_device *)dev;
	qe_assert(spi != QE_NULL);
	qe_assert(spi->bus != QE_NULL);

	return qe_spi_transfer(spi, QE_NULL, buffer, size);
}

static qe_size_t spidev_device_write(qe_device_t  dev,
											   qe_off_t     pos,
											   const void  *buffer,
											   qe_size_t    size)
{
	struct qe_spi_device *spi;

	spi = (struct qe_spi_device *)dev;
	qe_assert(spi != QE_NULL);
	qe_assert(spi->bus != QE_NULL);

	return qe_spi_transfer(spi, buffer, QE_NULL, size);
}

const static struct qe_device_ops spi_device_ops = {
	QE_NULL,
	QE_NULL,
	QE_NULL,
	spidev_device_read,
	spidev_device_write,
	QE_NULL
};

qe_err_t qe_spidev_register(struct qe_spi_device *spidev, const char *name)
{
	struct qe_device *dev;

	qe_assert(spidev != QE_NULL);

	dev = &(spidev->parent);

	/* set device class */
	dev->class = QE_DevClass_SPIDevice;

	/* set ops */
	dev->ops = &spi_device_ops;

	return qe_device_register(dev, name, QE_DEV_F_RDWR);
}

