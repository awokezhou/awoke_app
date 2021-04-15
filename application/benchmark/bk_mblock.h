
#ifndef __BK_MBLOCK_H__
#define __BK_MBLOCK_H__



#include "benchmark.h"



typedef enum {
	BK_DEV_ID_NONE = 0,
	BK_DEV_ID_XILSPI,
	BK_DEV_ID_MICRON_FLASH,
	BK_DEV_ID_SPANSION_FLASH,
} bk_dev_ids;

#define BK_DEV_F_INSTANCE		0x00000001
#define BK_DEV_F_IOSTREAM_CH	0x00000100



struct bk_mb_device {
	int id;
};

struct bk_mbdev_match {
	const char *name;
	bk_dev_ids id;
};

struct bk_mbdev {
	const char *name;
	struct bk_mb_device *dev;
};

typedef enum {
	BK_MBDEV_SPI = 0,
	BK_MBDEV_FLASH,
	BK_MBDEV_SIZEOF,
} bk_mbdev_type;

typedef struct _bk_mbdev_obj {
	uint8_t type;
	
} bk_mbdev_obj;


typedef enum {
	BK_MBLOCK_FLASH = 1,
} bk_mblock_type;

typedef struct _bk_mblock_info {

	uint8_t type;

	const char *name;

	uint32_t flags;

	uint32_t size;

	uint32_t erasesize;

	uint32_t writesize;

	/* operations */

	awoke_list _head;
} bk_mblock_info;

typedef struct bk_iostream_msg {
	const void	*tx_buf;
	void		*rx_buf;
	unsigned	len;
} iostream_msg;

struct bk_iostream;

typedef struct bk_iostream_ch {
	err_type (*sync)(struct bk_iostream *io, void *);
	err_type (*prepare)(struct bk_iostream *io, void *);
	err_type (*finish)(struct bk_iostream *io, void *);
	void *priv;
} iostream_ch;

struct bk_iostream {
	struct bk_iostream_ch *ch;
	awoke_fifo message;
} iostream;

struct bk_flash_info {
	const char *name;
	uint8_t id;
	uint32_t flags;
	err_type (*probe)(struct bk_flash_info *);
} flash_info;

struct bk_flash_device {
	struct bk_mb_device dev;
	struct bk_flash_controller *controller;
};

struct bk_flash_driver {
	struct bk_mbdev_match *match_table;
	int match_table_size;
	err_type (*probe)(struct bk_flash_device *);
	awoke_list _head;
};

struct bk_flash_controller {
	uint32_t flags;
	struct bk_flash_device *dev;
	struct bk_iostream io;
	uint8_t command[4];

	int (*build_readcmd)(struct bk_flash_controller *, uint32_t addr);
	int (*build_writecmd)(struct bk_flash_controller *, uint32_t addr);
	int (*build_erasecmd)(struct bk_flash_controller *, uint32_t addr);
};

struct bk_spi_match {
	struct bk_mbdev_match match;
	uint16_t max_speed_hz;
	uint8_t mode;
};

struct bk_spi_device {
	struct bk_mb_device dev;
	struct bk_spi_master *master;
	uint16_t max_speed_hz;
	uint32_t flags;
};

struct bk_spi_driver {
	struct bk_mbdev_match *match_table;
	int match_table_size;
	err_type (*probe)(struct bk_spi_device *);
	awoke_list _head;
};

struct bk_spi_master {
	uint32_t flags;
	struct bk_spi_device *dev;
	struct bk_iostream_ch io_channel;
	err_type (*setup)(struct bk_spi_device *);
};


err_type benchmark_mblock_test(int argc, char *argv[]);


#endif /* __BK_MBLOCK_H__ */