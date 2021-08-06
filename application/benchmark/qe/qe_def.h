
#ifndef __QE_DEF_H__
#define __QE_DEF_H__


#include "qe_config.h"
#include <stdarg.h>


/**
 * @Space Basic @{
 */

/* Quick Embedded Software Framework version information */
#define QE_VERSION						1L					/**< major version number */
#define QE_SUBVERSION					0L					/**< minor version number */
#define QE_REVISION						0L					/**< revise version number */

#define QESF_VERSION                	((QE_VERSION * 10000) + \
                                         (QE_SUBVERSION * 100) + QE_REVISION)

#define QE_NULL                         (0)

typedef int								qe_bool_t;	   	/**< boolean type */

typedef signed   char                   qe_int8_t;      	/**<  8bit integer type */
typedef signed   short                  qe_int16_t;     	/**< 16bit integer type */
typedef signed   int                    qe_int32_t;     	/**< 32bit integer type */
typedef unsigned char                   qe_uint8_t;     	/**<  8bit unsigned integer type */
typedef unsigned short                  qe_uint16_t;    	/**< 16bit unsigned integer type */
typedef unsigned int                    qe_uint32_t;    	/**< 32bit unsigned integer type */

typedef signed long long                qe_int64_t;     	/**< 64bit integer type */
typedef unsigned long long              qe_uint64_t;    	/**< 64bit unsigned integer type */

typedef unsigned long                   qe_ubase_t;     	/**< Nbit unsigned CPU related data type */
typedef long                            qe_base_t;      	/**< Nbit CPU related date type */

typedef qe_ubase_t                      qe_size_t;      	/**< Type for size number */
typedef qe_base_t                       qe_off_t;       	/**< Type for offset */

#define QE_TRUE							(1)
#define QE_FALSE						(0)

#define QE_WAIT_FOREVER					(-1)


/* Stage Initialization Export */
typedef int (*init_fn_t)(void);

#define QE_SECTION(x)					__attribute__((section(x)))
#define QE_EXPORT(fn, level) \
	__attribute__((used)) const init_fn_t __qe_init_##fn QE_SECTION(".qesi_fn."level) = fn

/* board init routines will be called in board_init() function */
#define QE_BOARD_EXPORT(fn)          	QE_EXPORT(fn, "1")

/* components pre-initialization (pure software initilization) */
#define QE_PREV_EXPORT(fn)            	QE_EXPORT(fn, "2")

/* device initialization */
#define QE_DEVICE_EXPORT(fn)          	QE_EXPORT(fn, "3")

/* components initialization (dfs, lwip, ...) */
#define QE_COMPONENT_EXPORT(fn)       	QE_EXPORT(fn, "4")

/* environment initialization (mount disk, ...) */
#define QE_ENV_EXPORT(fn)             	QE_EXPORT(fn, "5")

/* appliation initialization (application etc ...) */
#define QE_APP_EXPORT(fn)				QE_EXPORT(fn, "6")


/**
 * Double List structure
 */
struct qe_list_node {
    struct qe_list_node *next;                          /**< point to next node. */
    struct qe_list_node *prev;                          /**< point to prev node. */
};
typedef struct qe_list_node qe_list_t;               /**< Type for lists. */

/**
 * Single List structure
 */
struct qe_slist_node {
    struct qe_slist_node *next;                         /**< point to next node. */
};
typedef struct qe_slist_node qe_slist_t;             /**< Type for single list. */


/**
 * String Builder
 */
struct qe_str_builder {
	char *p;
	char *head;
	int   max;
	int   len;
};
typedef struct qe_str_builder qe_strb_t;


/* error code definitions @{ */
typedef enum {

	qe_ok 			= 0,
		
	qe_ecomm  		= 10,
	qe_ebusy,											/**< Resource busy. */
	qe_nosys,											
	qe_eparam,
	qe_eexist,
	qe_notexist,
	qe_notsupport,
	qe_esend,
	qe_erecv,
	qe_ewrite,
	qe_erange,
} qe_err_t;

/*}@*/



/**
 * @Space Device @{
 */

/**
 * device (I/O) class type
 */
enum qe_devclass_type {
	QE_DevClass_Char = 0,							/**< character device */
	QE_DevClass_Block,								/**< block device */
	QE_DevClass_NetIf,								/**< net interface */
	QE_DevClass_MTD,								/**< memory device */
	QE_DevClass_CAN,								/**< CAN device */
	QE_DevClass_RTC,								/**< RTC device */
	QE_DevClass_Sound,								/**< Sound device */
	QE_DevClass_Graphic,							/**< Graphic device */
	QE_DevClass_I2CBUS, 							/**< I2C bus device */
	QE_DevClass_USBDevice,							/**< USB slave device */
	QE_DevClass_USBHost,							/**< USB host bus */
	QE_DevClass_SPIBUS, 							/**< SPI bus device */
	QE_DevClass_SPIDevice,							/**< SPI device */
	QE_DevClass_SDIO,								/**< SDIO bus device */
	QE_DevClass_PM, 								/**< PM pseudo device */
	QE_DevClass_Pipe,								/**< Pipe device */
	QE_DevClass_Portal, 							/**< Portal device */
	QE_DevClass_Timer,								/**< Timer device */
	QE_DevClass_Miscellaneous,						/**< Miscellaneous device */
	QE_DevClass_Sensor, 							/**< Sensor device */
	QE_DevClass_ImgSensor,							/**< Image Sensor device */
	QE_DevClass_Touch,								/**< Touch device */
	QE_DevClass_PHY,								/**< PHY device */
	QE_DevClass_Unknown 							/**< unknown device */
};

/**
 * device flags defitions
 */
#define QE_DEV_F_DEACTIVATE       0x000           /**< device is not not initialized */

#define QE_DEV_F_RDONLY           0x001           /**< read only */
#define QE_DEV_F_WRONLY           0x002           /**< write only */
#define QE_DEV_F_RDWR             0x003           /**< read and write */

#define QE_DEV_F_REMOVABLE        0x004           /**< removable device */
#define QE_DEV_F_STANDALONE       0x008           /**< standalone device */
#define QE_DEV_F_ACTIVATED        0x010           /**< device is activated */
#define QE_DEV_F_SUSPENDED        0x020           /**< device is suspended */
#define QE_DEV_F_STREAM           0x040           /**< stream mode */

#define QE_DEV_F_INT_RX           0x100           /**< INT mode on Rx */
#define QE_DEV_F_DMA_RX           0x200           /**< DMA mode on Rx */
#define QE_DEV_F_INT_TX           0x400           /**< INT mode on Tx */
#define QE_DEV_F_DMA_TX           0x800           /**< DMA mode on Tx */

#define QE_DEV_OF_CLOSE           0x000           /**< device is closed */
#define QE_DEV_OF_OPEN            0x008           /**< device is opened */
#define QE_DEV_OF_MASK            0xf0f           /**< mask of open flag */

#define QE_DEV_CTRL_RESUME        0x01            /**< resume device */
#define QE_DEV_CTRL_SUSPEND       0x02            /**< suspend device */
#define QE_DEV_CTRL_CONFIG        0x03            /**< configure device */
#define QE_DEV_CTRL_CLOSE         0x04            /**< close device */
#define QE_DEV_CTRL_GET_NAME	  0x05
#define QE_DEV_CTRL_SET_NAME	  0X06

#define QE_DEV_CTRL_SET_INT       0x10            /**< set interrupt */
#define QE_DEV_CTRL_CLR_INT       0x11            /**< clear interrupt */
#define QE_DEV_CTRL_GET_INT       0x12            /**< get interrupt status */


typedef struct qe_device * qe_device_t;

/**
 * operations set for device object
 */
struct qe_device_ops {
    /* common device interface */
    qe_err_t   (*init)   (qe_device_t dev);
    qe_err_t   (*open)   (qe_device_t dev, qe_uint16_t oflag);
    qe_err_t   (*close)  (qe_device_t dev);
    qe_size_t  (*read)   (qe_device_t dev, qe_off_t pos, void *buffer, qe_size_t size);
    qe_size_t  (*write)  (qe_device_t dev, qe_off_t pos, const void *buffer, qe_size_t size);
    qe_err_t   (*control)(qe_device_t dev, int cmd, void *args);
};

/**
 * Device structure
 */
struct qe_device {

	enum qe_devclass_type class;

	char name[QE_NAME_MAX];

	qe_uint16_t flag;
	qe_uint16_t open_flag;

	qe_uint16_t reference;

	qe_uint16_t device_id;

	/* device callback */
	qe_err_t (*rx_indicate)(qe_device_t dev, qe_size_t size);
	qe_err_t (*tx_complete)(qe_device_t dev, void *buffer);

	const struct qe_device_ops *ops;

	/* private data */
	void *private;

	/* register to device list */
	qe_list_t list;
};

#endif /* __QE_DEF_H__ */


