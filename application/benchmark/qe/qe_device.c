
#include "qe_device.h"



LIST_HEAD_INIT(qe_device_list);


err_type qe_device_register(struct qe_device *dev,
									const char *	  name,
									uint16_t          flags)
{
	qe_assert(dev == NULL);

	if (qe_device_find(name) != NULL) {
		bk_err("device:%s exist");
		return err_exist;
	}

	qe_strncpy(dev->name, name, QE_NAME_MAX);

	list_append(&dev->list, &qe_device_list);

	dev->flag = flags;
	dev->reference = 0;
	dev->open_flag = 0;

	return et_ok;
}

err_type qe_device_unregister(struct qe_device *dev)
{
    qe_assert(dev != NULL);

    list_unlink(&(dev->list));

    return et_ok;
}

struct qe_device *qe_device_find(const char *name)
{
	struct qe_device *dev;

	list_foreach(dev, &(qe_device_list), list) {
		if (qe_strncmp(dev->name, name, QE_NAME_MAX) == 0) {
			return dev;
		}
	}
	
	return NULL;
}

