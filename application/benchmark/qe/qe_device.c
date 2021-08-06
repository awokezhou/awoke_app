
/*
 * Copyright (c) 2021-2021, Luster SCBU Team
 *
 * License: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-07-13     Weizhou      the first version
 */




#include "qe_core.h"
#include "bk_quickemb.h"


qe_list_t qe_device_list = QE_LIST_OBJECT_INIT(qe_device_list);

/**
 * This function registers a device driver with specified name.
 *
 * @param dev: 	 the pointer of device driver structure
 * @param name:  the device driver's name
 * @param flags: the capabilities flag of device
 * 
 * @return the error code, successfully with qe_ok.
 */
qe_err_t qe_device_register(struct qe_device *dev,
                                          const char *name,
                                          qe_uint16_t flags)
{
	if (dev == QE_NULL) {
		return qe_eparam;
	}

	if (qe_device_find(name) != QE_NULL) {
		return qe_eexist;
	}

	qe_strncpy(dev->name, name, QE_NAME_MAX);

	qe_list_append(&dev->list, &qe_device_list);
	
	dev->flag = flags;
	dev->reference = 0;
	dev->open_flag = 0;

	return qe_ok;
}

/**
 * This function removes a previously registered device driver
 *
 * @param dev the pointer of device driver structure
 *
 * @return the error code, successfully with qe_ok.
 */
qe_err_t qe_device_unregister(struct qe_device *dev)
{
    //qe_assert(dev != QE_NULL);

    qe_list_unlink(&(dev->list));

    return qe_ok;
}

struct qe_device *qe_device_find(const char *name)
{
	struct qe_device *dev;
	struct qe_list_node *node = QE_NULL;


	qe_list_foreach(node, &(qe_device_list)) {
		dev = qe_list_entry(node, struct qe_device, list);
		if (qe_strncmp(dev->name, name, QE_NAME_MAX) == 0) {
			return dev;
		}
	}

	return QE_NULL;
}

qe_err_t qe_device_open(struct qe_device *dev, qe_uint16_t oflag)
{
	qe_err_t ret = qe_ok;

	qe_assert(dev != QE_NULL);

	/* if device is not initialized, initialize it. */
	if (!(dev->flag & QE_DEV_F_ACTIVATED)) {
		if (dev->ops->init != QE_NULL) {
			ret = dev->ops->init(dev);
			if (ret != qe_ok) {
				bk_err("init dev:%s error:%d", ret);
				return ret;
			}
		}

		dev->flag |= QE_DEV_F_ACTIVATED;
	}

	/* device is a stand alone device and opened */
	if ((dev->flag & QE_DEV_F_STANDALONE) &&
		(dev->open_flag & QE_DEV_OF_OPEN)) {
		return qe_ebusy;
	}

	/* call device open interface */
	if (dev->ops->open != QE_NULL) {
		ret = dev->ops->open(dev, oflag);
	} else {
		/* set open flag */
		dev->open_flag = (oflag & QE_DEV_OF_MASK);
	}

	/* set open flag */
	if (ret == qe_ok || ret == qe_ebusy) {
		dev->open_flag |= QE_DEV_OF_OPEN;
		dev->reference++;
	}

	return ret;
}

/**
 * This function will close a device
 *
 * @param dev the pointer of device driver structure
 *
 * @return the result
 */
qe_err_t qe_device_close(struct qe_device *dev)
{
	qe_err_t ret = qe_ok;

	//qe_assert(dev != QE_NULL);

	if (dev->reference == 0) 
		return qe_ecomm;

	dev->reference--;

	if (dev->reference != 0)
		return qe_ok;

	/* call device close interface */
	if (dev->ops->close != QE_NULL) {
		ret = dev->ops->close(dev);
	}

	/* set open flag */
	if (ret == qe_ok || ret == qe_nosys)
		dev->open_flag = QE_DEV_OF_CLOSE;

	return ret;
}


/**
 * This function will read some data from a device.
 *
 * @param dev the pointer of device driver structure
 * @param pos the position of reading
 * @param buffer the data buffer to save read data
 * @param size the size of buffer
 *
 * @return the actually read size on successful, otherwise negative returned.
 *
 */
qe_size_t qe_device_read(struct qe_device *dev,
								 qe_off_t		   pos,
								 void             *buffer,
								 qe_size_t   size)
{
	qe_assert(dev != QE_NULL);

	if (dev->reference == 0) {
		return 0;
	}

	/* call device read interface */
	if (dev->ops->read != QE_NULL) {
		return dev->ops->read(dev, pos, buffer, size);
	}

	return 0;
}

/**
 * This function will write some data to a device.
 *
 * @param dev the pointer of device driver structure
 * @param pos the position of written
 * @param buffer the data buffer to be written to device
 * @param size the size of buffer
 *
 * @return the actually written size on successful, otherwise negative returned.
 *
 */
qe_size_t qe_device_write(struct qe_device *dev,
								 qe_off_t		   pos,
  								 const void       *buffer,
  								 qe_size_t         size)
{
	//qe_assert(dev != QE_NULL);

	if (dev->reference == 0) {
		return 0;
	}

	/* call device_write interface */
	if (dev->ops->write!= QE_NULL) {
		return dev->ops->write(dev, pos, buffer, size);
	}

	return 0;
}

/**
 * This function will perform a variety of control functions on devices.
 *
 * @param dev the pointer of device driver structure
 * @param cmd the command sent to device
 * @param arg the argument of command
 *
 * @return the result
 */
qe_err_t qe_device_control(qe_device_t dev, int cmd, void *arg)
{
    //qe_assert(dev != QE_NULL);

    /* call device_write interface */
    if (dev->ops->control != QE_NULL) {
        return dev->ops->control(dev, cmd, arg);
    }

    return qe_nosys;
}

/**
 * This function will set the reception indication callback function. This callback function
 * is invoked when this device receives data.
 *
 * @param dev the pointer of device driver structure
 * @param rx_ind the indication callback function
 *
 * @return qe_ok
 */
qe_err_t 
qe_device_set_rx_indicate(struct qe_device *dev,
                          qe_err_t (*rx_ind)(struct qe_device *, qe_size_t))
{
    //qe_assert(dev != QE_NULL);

    dev->rx_indicate = rx_ind;

    return qe_ok;
}


/**
 * This function will set the indication callback function when device has
 * written data to physical hardware.
 *
 * @param dev the pointer of device driver structure
 * @param tx_done the indication callback function
 *
 * @return qe_ok
 */
qe_err_t
qe_device_set_tx_complete(struct qe_device *dev,
                          qe_err_t (*tx_done)(struct qe_device *, void *))
{
    //qe_assert(dev != QE_NULL);

    //dev->tx_complete = tx_done;

    return qe_ok;
}


