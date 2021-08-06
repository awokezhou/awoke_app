
#ifndef __QE_I2C_H__
#define __QE_I2C_H__

#include "qe_core.h"



#define QE_I2C_WR                0x0000
#define QE_I2C_RD               (1u << 0)
#define QE_I2C_ADDR_10BIT       (1u << 2)  /* this is a ten bit chip address */
#define QE_I2C_NO_START         (1u << 4)
#define QE_I2C_IGNORE_NACK      (1u << 5)
#define QE_I2C_NO_READ_ACK      (1u << 6)  /* when I2C reading, we do not ACK */
#define QE_I2C_NO_STOP          (1u << 7)

#define QE_I2C_DEV_CTRL_10BIT        0x20
#define QE_I2C_DEV_CTRL_ADDR         0x21
#define QE_I2C_DEV_CTRL_TIMEOUT      0x22
#define QE_I2C_DEV_CTRL_RW           0x23
#define QE_I2C_DEV_CTRL_CLK          0x24

struct qe_i2c_message {
    qe_uint16_t addr;
    qe_uint16_t flags;
    qe_uint16_t len;
    qe_uint8_t  *buf;
};

struct qe_i2c_bus;

struct qe_i2c_bus_ops {
    qe_size_t (*master_xfer)(struct qe_i2c_bus *bus,
                             struct qe_i2c_message msgs[],
                             qe_uint32_t num);
    qe_size_t (*slave_xfer)(struct qe_i2c_bus *bus,
                            struct qe_i2c_message msgs[],
                            qe_uint32_t num);
    qe_err_t (*control)(struct qe_i2c_bus *bus,
                                qe_uint32_t,
                                qe_uint32_t);
};

struct qe_i2c_bus {
    struct qe_device parent;
    const struct qe_i2c_bus_ops *ops;
    qe_uint16_t  flags;
    qe_uint16_t  addr;
    qe_uint32_t  timeout;
    qe_uint32_t  retries;
	void *lock;
    void *priv;

	qe_uint32_t multi_access:1;
	qe_uint32_t reserve:31;
	
	qe_err_t (*bus_lock)(struct qe_i2c_bus *bus, qe_int32_t ms);
	void     (*bus_unlock)(struct qe_i2c_bus *bus);
};

struct qe_i2c_priv_data {
    struct qe_i2c_message *msgs;
    qe_size_t  number;
};



qe_err_t qe_i2c_bus_register(struct qe_i2c_bus *bus,
									  const char        *name);

struct qe_i2c_bus *qe_i2c_bus_find(const char *name);

qe_size_t qe_i2c_transfer(struct qe_i2c_bus     *bus,
                          		struct qe_i2c_message  msgs[],
                          		qe_uint32_t            num);

qe_err_t qe_i2c_control(struct qe_i2c_bus *bus,
                        	  qe_uint32_t        cmd,
                        	  qe_uint32_t        arg);

qe_size_t qe_i2c_master_send(struct qe_i2c_bus *bus,
									   qe_uint16_t 		  addr,
									   qe_uint16_t  	  flags,
									   const qe_uint8_t  *buf,
									   qe_uint32_t        count);

qe_size_t qe_i2c_master_recv(struct qe_i2c_bus *bus,
                             		  qe_uint16_t         addr,
                             		  qe_uint16_t         flags,
                             		  qe_uint8_t         *buf,
                             		  qe_uint32_t         count);
#endif /* __QE_I2C_H__ */
