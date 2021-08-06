/*
 * qe_serial.c
 *
 *  Created on: 2021Äê7ÔÂ19ÈÕ
 *      Author: z04200
 */

#include "qe_serial.h"
#include "qe_core.h"
#include "string.h"


static int serial_poll_rx(struct qe_serial_device *serial, qe_uint8_t *data, int length)
{
	int c;
	int size;

	//qe_assert(serial != QE_NULL);
	size = length;

	while (length) {
		c = serial->ops->getc(serial);
		if (c == -1) break;

		*data = c;
		data++; length--;

		if (serial->parent.open_flag & QE_DEV_F_STREAM) {
			if (c == '\n') break;
		}
	}

	return size - length;
}

/*
static int serial_int_rx(struct qe_serial_device *serial, qe_uint8_t *data, int length)
{
	int size;
	struct qe_serial_rxfifo* rxfifo;

	//qe_assert(serial != QE_NULL);
	size = length;

	rxfifo = (struct qe_serial_rxfifo *)serial->rx;
	//qe_assert(rxfifo != QE_NULL);

	while (length) {
		int c;
		qe_base_t level;

		
	}
}*/

static qe_err_t qe_serial_init(struct qe_device *dev)
{
	qe_err_t ret = qe_ok;
	struct qe_serial_device *serial;

	//qe_assert(dev != QE_NULL);

	serial = (struct qe_serial_device *)dev;

	serial->rx = QE_NULL;
	serial->tx = QE_NULL;

	if (serial->ops->configure) {
		ret = serial->ops->configure(serial, &serial->config);
	}

	return ret;
}

static struct qe_serial_rxfifo *serial_rxfifo_create(qe_size_t size)
{
	struct qe_serial_rxfifo *rxfifo;
	rxfifo = (struct qe_serial_rxfifo *)qe_malloc(sizeof(struct qe_serial_rxfifo) + size);
	//qe_assert(rxfifo != QE_NULL);
	rxfifo->buffer = (qe_uint8_t *)(rxfifo + 1);
	memset(rxfifo->buffer, 0, size);
	rxfifo->put_index = 0;
	rxfifo->get_index = 0;
	rxfifo->is_full = QE_FALSE;
	return rxfifo;
}

static qe_err_t qe_serial_open(struct qe_device *dev, qe_uint16_t oflag)
{
	qe_uint16_t stream_flag = 0x0;
	struct qe_serial_device *serial;

	serial = (struct qe_serial_device *)dev;

	/* check device flag with the open flag */
	if ((oflag & QE_DEV_F_DMA_RX) && !(dev->flag & QE_DEV_F_DMA_RX))
		return qe_notsupport;
	if ((oflag & QE_DEV_F_DMA_TX) && !(dev->flag & QE_DEV_F_DMA_TX))
		return qe_notsupport;
    if ((oflag & QE_DEV_F_INT_RX) && !(dev->flag & QE_DEV_F_INT_RX))
        return qe_notsupport;
	/*
    if ((oflag & QE_DEV_F_INT_TX) && !(dev->flag & QE_DEV_F_INT_TX))
        return qe_notsupport;*/

	/* keep steam flag */
	if ((oflag & QE_DEV_F_STREAM) || (dev->open_flag & QE_DEV_F_STREAM))
		stream_flag = QE_DEV_F_STREAM;

	/* get open flags */
	dev->open_flag = oflag & 0xff;

	/* initialize the Rx/Tx structure according to open flag */
	if (serial->rx == QE_NULL) {
		if (oflag & QE_DEV_F_INT_RX) {

			struct qe_serial_rxfifo *rxfifo;
			rxfifo = serial_rxfifo_create(serial->config.bufsz);
			rxfifo = (struct qe_serial_rxfifo *)qe_malloc(sizeof(struct qe_serial_rxfifo) +
				serial->config.bufsz);
			
			serial->rx = rxfifo;
			dev->open_flag |= QE_DEV_F_INT_RX;
			serial->ops->control(serial, QE_DEV_CTRL_SET_INT, (void *)QE_DEV_F_INT_RX);
		}
#ifdef QE_SERIAL_WITH_DMA
		else if (oflag & QE_DEV_F_DMA_RX) {
			if (serial->config.bufsz == 0) {

				struct qe_serial_rxdma *rxdma;
				rxdma = (struct qe_serial_rxdma *)qe_malloc(sizeof(struct qe_serial_rxdma));
				//qe_assert(rxdma != QE_NULL);
				rxdma->activated = QE_FALSE;
				serial->serial_rx = rxdma;
			} else {
			
				struct qe_serial_rxfifo *rxfifo;
				rxfifo = serial_rxfifo_create(serial->config.bufsz);
				rxfifo = (struct qe_serial_rxfifo *)qe_malloc(sizeof(struct qe_serial_rxfifo) +
					serial->config.bufsz);

                serial->serial_rx = rxfifo;
				serial->ops->control(serial, QE_DEV_CTRL_CONFIG, (void *)QE_DEV_F_DMA_RX);
			}
			dev->open_flag |= QE_DEV_F_DMA_RX;
		}
#endif
		else {
			serial->rx = QE_NULL;
		}
	} else {
		if (oflag & QE_DEV_F_INT_RX) {
			dev->open_flag |= QE_DEV_F_INT_RX;
#ifdef QE_SERIAL_WITH_DMA
		else if (oflag & QE_DEV_F_DMA_RX) {
			dev->open_flag |= QE_DEV_F_DMA_RX;
		}
#endif
		}
	}

	/* set stream flag */	
	dev->open_flag |= stream_flag;

	return qe_ok;
}

static qe_err_t qe_serial_close(struct qe_device *dev)
{
	struct qe_serial_device *serial;

	qe_assert(dev != QE_NULL);
	serial = (struct qe_serial_device *)dev;
	(void)serial;
	
	/* this device has more reference count */
	if (dev->reference> 1) return qe_ok;
	
	return qe_ok;
}

static qe_size_t qe_serial_read(struct qe_device *dev,
									  qe_off_t 		    pos,
									  void             *buffer,
									  qe_size_t         size)
{
	struct qe_serial_device *serial;

	qe_assert(dev != QE_NULL);
	if (size == 0) return 0;

	serial = (struct qe_serial_device *)dev;

	if (dev->open_flag & QE_DEV_F_INT_RX) {
		//return serial_int_rx(serial, (qe_uint8_t *)buffer, size);
	}
#ifdef QE_SERIAL_WITH_DMA
	else if (dev->open_flag & QE_DEV_F_DMA_RX) {
		//return serial_dma_rx(serial, (qe_uint8_t *)buffer, size);
	}
#endif

	return serial_poll_rx(serial, (qe_uint8_t *)buffer, size);
}

static qe_size_t qe_serial_write(struct qe_device *dev,
								       qe_off_t 		 pos,
									   const void *      buffer,
									   qe_size_t         size)
{
	struct qe_serial_device *serial;

	//qe_assert(dev != QE_NULL);
	if (size == 0) return 0;

	serial = (struct qe_serial_device *)dev;
	(void)serial;

	if (dev->open_flag & QE_DEV_F_INT_RX) {
		//return serial_int_tx(serial, (const qe_uint8_t *)buffer, size);
	}
#ifdef QE_SERIAL_WITH_DMA
	else if (dev->open_flag & QE_DEV_F_DMA_RX) {
		//return serial_dma_tx(serial, (const qe_uint8_t *)buffer, size);
	}
#endif
	else {
		//return serial_poll_tx(serial, (const qe_uint8_t *)buffer, size);
	}

	return 0;
}

static qe_err_t qe_serial_control(struct qe_device *dev,
										 int 			   cmd,
										 void       	  *args)
{
	qe_err_t ret = qe_ok;
	struct qe_serial_device *serial;

	qe_assert(dev != QE_NULL);
	serial = (struct qe_serial_device *)dev;

	switch (cmd) {

	case QE_DEV_CTRL_SUSPEND:
		dev->flag |= QE_DEV_F_SUSPENDED;
		break;

	case QE_DEV_CTRL_RESUME:
		dev->flag &= ~QE_DEV_F_SUSPENDED;
		break;

	case QE_DEV_CTRL_GET_NAME:
		qe_strncpy((char *)args, dev->name, QE_NAME_MAX);
		break;

	case QE_DEV_CTRL_SET_NAME:
		sys_debug("set name %s", (char *)args);
		break;

	case QE_DEV_CTRL_CONFIG:
		if (args) {
			struct qe_serial_configure *pcfg = (struct qe_serial_configure *)args;
			if ((pcfg->bufsz != serial->config.bufsz) && (serial->parent.reference)) {
				/*can not change buffer size*/
				return qe_ebusy;
			}
			/* set serial configure */
			serial->config = *pcfg;
			if (serial->parent.reference) {
				/* serial device has been opened, to configure it */
				serial->ops->configure(serial, (struct qe_serial_configure *) args);
			}
		}
		break;

	default:
		ret = serial->ops->control(serial, cmd, args);
		break;
	}

	return ret;
}

static const struct qe_device_ops serial_ops = {
	qe_serial_init,
	qe_serial_open,
	qe_serial_close,
	qe_serial_read,
	qe_serial_write,
	qe_serial_control
};

qe_err_t qe_serial_register(qe_serial_t              *serial,
								   const char 			   *name,
								   qe_uint32_t 			    flag,
								   void 				    *data)
{
	struct qe_device *dev;

	dev = &(serial->parent);

	dev->class = QE_DevClass_Char;
	dev->rx_indicate = QE_NULL;
	dev->tx_complete = QE_NULL;

	dev->ops = &serial_ops;

	dev->private = data;

	return qe_device_register(dev, name, flag);
}


