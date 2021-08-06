
#include "bk_quickemb.h"

#include "qe_spi.h"



void qe_spi_bus_setlock(struct qe_spi_bus *bus,
							      void              *lock,
								  qe_err_t (*bus_lock)(struct qe_spi_bus *bus, qe_int32_t ms),
								  void     (*bus_unlock)(struct qe_spi_bus *bus))
{
	bus->lock = lock;
	bus->bus_lock = bus_lock;
	bus->bus_unlock = bus_unlock;
	bus->multi_access = 1;
}

qe_err_t qe_spi_bus_lock(struct qe_spi_bus *bus, qe_int32_t ms)
{
	if (bus->multi_access == 1) {
		return bus->bus_lock(bus, ms);
	}

	return qe_ok;
}

void qe_spi_bus_unlock(struct qe_spi_bus *bus)
{
	if (bus->multi_access == 1) {
		bus->bus_unlock(bus);
	}
}

qe_err_t qe_spi_bus_attach_device(struct qe_spi_device *spi,
											   const char           *name,
											   const char           *bus_name,
											   void                 *user_data)
{
	qe_err_t ret;
	qe_device_t bus;

	bus = qe_device_find(bus_name);
	if ((bus != QE_NULL) && (bus->class == QE_DevClass_SPIBUS)) {
		spi->bus = (struct qe_spi_bus *)bus;
		ret = qe_spidev_register(spi, name);
		if (ret != qe_ok) {
			return ret;
		}
		qe_memset(&spi->config, 0, sizeof(spi->config));
		spi->parent.private = user_data;
		bk_debug("%s attach to bus:%s", name, bus_name);
		return qe_ok;
	}

	return qe_notexist;
}

qe_size_t qe_spi_transfer(struct qe_spi_device *dev,
								 const void 		  *send_buf,
								 void                 *recv_buf,
								 qe_size_t             length)
{
	qe_size_t n;
	qe_err_t ret;
	struct qe_spi_message message;

	qe_assert(dev != QE_NULL);
	qe_assert(dev->bus != QE_NULL);

	n = length;

	ret = qe_spi_bus_lock(dev->bus, QE_WAIT_FOREVER);
	if (ret == qe_ok) {

		if (dev->bus->owner != dev) {
			/* not the same owner as current, re-configure SPI bus */
			ret = dev->bus->ops->configure(dev, &dev->config);
			if (ret == qe_ok) {
				/* set SPI bus owner */
				dev->bus->owner = dev;
			} else {
				goto __exit;
			}
		}

		/* initial message */
		message.send_buf   = send_buf;
		message.recv_buf   = recv_buf;
		message.length     = length;
		message.cs_take    = 1;
		message.cs_release = 1;
		message.next       = QE_NULL;

		/* transfer message */
		n = dev->bus->ops->xfer(dev, &message);
		if (n == 0) {
			goto __exit;
		}
	} else {
		return ret;	
	}

__exit:
	qe_spi_bus_unlock(dev->bus);

	return n;
}

qe_err_t qe_spi_send_recv(struct qe_spi_device *spi,
								   const void           *send_buf,
								   qe_size_t             send_length,
								   void                 *recv_buf,
								   qe_size_t             recv_length)
{
	qe_err_t ret;
	qe_uint32_t n;
	struct qe_spi_message message;

	qe_assert(spi != QE_NULL);
	qe_assert(spi->bus != QE_NULL);

	ret = qe_spi_bus_lock(spi->bus, QE_WAIT_FOREVER);
	if (ret == qe_ok) {

		if (spi->bus->owner != spi) {
			/* not the same owner as current, re-configure SPI bus */
			ret = spi->bus->ops->configure(spi, &spi->config);
			if (ret == qe_ok) {
				spi->bus->owner = spi;
			} else {
				/* configure SPI bus failed */
				goto __exit;
			}
		}

		/* send data */
		message.send_buf   = send_buf;
		message.recv_buf   = recv_buf;
		message.length     = send_length;
		message.cs_take    = 1;
		message.cs_release = 0;
		message.next       = QE_NULL;

		n = spi->bus->ops->xfer(spi, &message);
		if (n == 0) {
			ret = qe_esend;
			goto __exit;
		}

		/* recv data */
		message.send_buf   = QE_NULL;
		message.recv_buf   = recv_buf;
		message.length     = recv_length;
		message.cs_take    = 0;
		message.cs_release = 1;
		message.next       = QE_NULL;

		n = spi->bus->ops->xfer(spi, &message);
		if (n == 0) {
			ret = qe_erecv;
			goto __exit;
		}

		ret = qe_ok;
	} else {
		return ret;
	}

__exit:
	qe_spi_bus_unlock(spi->bus);

	return ret;
}

struct qe_spi_message *qe_spi_transfer_message(struct qe_spi_device *spi,
															struct qe_spi_message *message)
{
	qe_err_t ret;
	qe_uint32_t n;
	struct qe_spi_message *index;

	qe_assert(spi != QE_NULL);

	/* get first message */
	index = message;
	if (index == QE_NULL)
		return index;

	ret = qe_spi_bus_lock(spi->bus, QE_WAIT_FOREVER);
	if (ret != qe_ok) {
		qe_set_errno(qe_ebusy);
		return index;
	}

	/* reset errno */
	qe_set_errno(qe_ok);

	if (spi->bus->owner != spi) {
		ret = spi->bus->ops->configure(spi, &spi->config);
		if (ret == qe_ok) {
			spi->bus->owner = spi;
		} else {
			qe_set_errno(ret);
			goto __exit;
		}
	}

	/* transmit each message */
	while (index != QE_NULL) {
		n = spi->bus->ops->xfer(spi, index);
		if (n == 0) {
			qe_set_errno(qe_esend);
			break;
		}

		index = index->next;
	}

__exit:
	/* release bus lock */
	qe_spi_bus_unlock(spi->bus);

	return index;
}

