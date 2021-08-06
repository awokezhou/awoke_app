
#include "qe_device.h"



static qe_size_t i2c_bus_read(struct qe_device *dev,
								    qe_off_t          pos,
									void             *buffer,
									qe_size_t         count)
{
	qe_uint16_t addr;
	qe_uint16_t flags;
	struct qe_i2c_bus *bus = (struct qe_i2c_bus *)dev->private;

	qe_assert(dev != QE_NULL);
	qe_assert(buffer != QE_NULL);

	addr = pos & 0xffff;
	flags = (pos >> 16) & 0xffff;

	return qe_i2c_master_recv(bus, addr, flags, (qe_uint8_t *)buffer, count);
}

static qe_size_t i2c_bus_write(struct qe_device *dev,
									 qe_off_t          pos,
									 const void       *buffer,
									 qe_size_t         count)
{
	qe_uint16_t addr;
	qe_uint16_t flags;
	struct qe_i2c_bus *bus = (struct qe_i2c_bus *)dev->private;

	qe_assert(dev != QE_NULL);
	qe_assert(buffer != QE_NULL);

	qelog_debug("sys", "i2c bus %s write %d bytes", bus->parent.name, count);

	addr = pos & 0xffff;
	flags = (pos >> 16) & 0xffff;

	return qe_i2c_master_send(bus, addr, flags, (const qe_uint8_t *)buffer, count);
}

static qe_err_t i2c_bus_control(struct qe_device *dev,
									   int               cmd,
									   void             *args)
{
	qe_err_t ret;
	struct qe_i2c_priv_data *priv_data;
	struct qe_i2c_bus *bus = (struct qe_i2c_bus *)dev->private;

	qe_assert(bus != QE_NULL);

	switch (cmd) {

	case QE_I2C_DEV_CTRL_10BIT:
		bus->flags |= QE_I2C_ADDR_10BIT;
		break;

	case QE_I2C_DEV_CTRL_ADDR:
		bus->addr = *(qe_uint16_t *)args;
		break;

	case QE_I2C_DEV_CTRL_TIMEOUT:
		bus->timeout = *(qe_uint32_t *)args;
		break;

	case QE_I2C_DEV_CTRL_RW:
		priv_data = (struct qe_i2c_priv_data *)args;
		ret = qe_i2c_transfer(bus, priv_data->msgs, priv_data->number);
		if (ret < 0) {
			return qe_esend;
		}
		break;

	case QE_I2C_DEV_CTRL_CLK:
		break;

	default:
		break;
	}

	return qe_ok;
}

static struct qe_device_ops i2c_ops = {
	QE_NULL,
	QE_NULL,
	QE_NULL,
	i2c_bus_read,
	i2c_bus_write,
	i2c_bus_control
};

void qe_i2c_bus_setlock(struct qe_i2c_bus *bus,
							      void              *lock,
								  qe_err_t (*bus_lock)(struct qe_i2c_bus *bus, qe_int32_t ms),
								  void     (*bus_unlock)(struct qe_i2c_bus *bus))
{
	bus->lock = lock;
	bus->bus_lock = bus_lock;
	bus->bus_unlock = bus_unlock;
	bus->multi_access = 1;
}

static qe_err_t i2c_bus_take(struct qe_i2c_bus *bus, qe_int32_t ms)
{
	if (bus->multi_access == 1) {
		return bus->bus_lock(bus, ms);
	}

	return qe_ok;
}

static void i2c_bus_release(struct qe_spi_bus *bus)
{
	if (bus->multi_access == 1) {
		bus->bus_unlock(bus);
	}
}
qe_err_t qe_i2c_bus_register(struct qe_i2c_bus *bus,
									  const char        *name)
{
	struct qe_device *dev;

	qe_assert(bus != QE_NULL);
	qe_assert(name != QE_NULL);

	if (bus->timeout == 0) bus->timeout = 100;

	dev = &(bus->parent);
	dev->private = bus;

	dev->class = QE_DevClass_I2CBUS;
	dev->ops = &i2c_ops;

	return qe_device_register(dev, name, QE_DEV_F_RDWR);
}

struct qe_i2c_bus *qe_i2c_bus_find(const char *name)
{
	struct qe_device *dev;
	struct qe_i2c_bus *bus;

	qe_assert(name);

	dev = qe_device_find(name);
	if (!dev || (dev->class != QE_DevClass_I2CBUS)) {
		qelog_err("sys", "i2c bus %s not exist", name);
		return QE_NULL;
	}

	bus = (struct qe_i2c_bus *)dev->private;

	return bus;
}

qe_size_t qe_i2c_transfer(struct qe_i2c_bus     *bus,
                          		struct qe_i2c_message  msgs[],
                          		qe_uint32_t            num)
{
	qe_size_t n;

	if (bus->ops->master_xfer) {
		i2c_bus_take(bus->lock, QE_WAIT_FOREVER);
		n = bus->ops->master_xfer(bus, msgs, num);
		i2c_bus_release(bus->lock);
		return n;
	} else {
		qelog_err("sys", "i2c bus operation not supported");
		return 0;
	}
}

qe_err_t qe_i2c_control(struct qe_i2c_bus *bus,
                        	  qe_uint32_t        cmd,
                        	  qe_uint32_t        arg)
{
	if (bus->ops->control) {
		return bus->ops->control(bus, cmd, arg);
	} else {
		qelog_err("sys", "i2c bus operation not supported");
		return 0;
	}
}

qe_size_t qe_i2c_master_send(struct qe_i2c_bus *bus,
									   qe_uint16_t 		  addr,
									   qe_uint16_t  	  flags,
									   const qe_uint8_t  *buf,
									   qe_uint32_t        count)
{
	struct qe_i2c_message message;

	message.addr  = addr;
	message.flags = flags;
	message.len   = count;
	message.buf   = (qe_uint8_t *)buf;

	return qe_i2c_transfer(bus, &message, 1);
}

qe_size_t qe_i2c_master_recv(struct qe_i2c_bus *bus,
                             		  qe_uint16_t         addr,
                             		  qe_uint16_t         flags,
                             		  qe_uint8_t         *buf,
                             		  qe_uint32_t         count)
{
    struct qe_i2c_message message;
	
    qe_assert(bus != QE_NULL);

    message.addr   = addr;
    message.flags  = flags | QE_I2C_RD;
    message.len    = count;
    message.buf    = buf;

    return qe_i2c_transfer(bus, &message, 1);
}