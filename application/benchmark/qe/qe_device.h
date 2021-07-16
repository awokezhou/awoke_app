
#ifndef __QE_DEVICE_H__
#define __QE_DEVICE_H__



enum qe_device_class {
	Qe_DevClass_Char = 0,
	Qe_DevClass_Block,
	Qe_DevClass_NetIf,
	Qe_DevClass_MTD,
	Qe_DevClass_I2CBus,
	Qe_DevClass_SpiBus,
	Qe_DevClass_PHY,
	Qe_DevClass_ImgSensor,
	Qe_DevClass_Unknown
};

struct qe_device {

	enum qe_device_class class;

	char name[QE_NAME_MAX];

	uint16_t flag;
	uint16_t open_flag;

	uint16_t reference;

	uint16_t device_id;

	err_type (*rx_indicate)(struct qe_device *dev, uint32_t size);
	err_type (*tx_complete)(struct qe_device *dev, void *buffer);

	//const struct qe_device_ops *ops;

	/* common device interface */
	err_type (*init)(struct qe_device *dev);
	err_type (*open)(struct qe_device *dev, uint16_t flag);
	err_type (*close)(struct qe_device *dev);
	err_type (*read)(struct qe_device *dev, long pos, void *buffer, uint32_t size);
	err_type (*write)(struct qe_device *dev, long pos, const void *buffer, uint32_t size);
	err_type (*control)(struct qe_device *dev, int cmd, void *args);

	void *private;

	awoke_list  list;
};

#endif /* __QE_DEVICE_H__ */
