
#include "qe_device.h"



static qe_size_t imgsensor_read(struct qe_device *dev,
										qe_off_t          pos,
										void       	     *buffer,
										qe_size_t         count)
{
	return 0;
}

static qe_size_t imgsensor_write(struct qe_device *dev,
								       	 qe_off_t 		   pos,
									     const void *      buffer,
									     qe_size_t         size)
{
	return 0;
}

static qe_err_t imgsensor_control(struct qe_device *dev,
										   int 			     cmd,
										   void       	    *args)
{
	qe_assert(dev != QE_NULL);

	switch (cmd) {

	case QE_IMGSENSOR_CTRL_REG_READ:
		break;

	case QE_IMGSENSOR_CTRL_REG_WRITE:
		break;

	default:
		break;
	}

	return qe_ok;
}

static struct qe_device_ops imgsensor_ops = {
	QE_NULL,
	QE_NULL,
	QE_NULL,
	imgsensor_read,
	imgsensor_write,
	imgsensor_control
};

qe_err_t qe_imgsensor_register(struct qe_imgsensor_device *sensor,
										  const char                 *name,
										  void 				         *data)
{
	struct qe_device *dev;

	dev = &(sensor->parent);

	dev->class = QE_DevClass_ImgSensor;

	dev->ops = &imgsensor_ops;

	dev->private = data;

	return qe_device_register(dev, name, QE_DEV_F_RDWR);
}
