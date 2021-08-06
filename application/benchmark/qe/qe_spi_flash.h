
#ifndef __QE_SPI_FLASH_H__
#define __QE_SPI_FLASH_H__


#include "qe_core.h"


/**
 * status register bits
 */
enum {
    QE_SPIFLASH_SR_BUSY = (1 << 0),                  /**< busing */
    QE_SPIFLASH_SR_WEL = (1 << 1),                   /**< write enable latch */
    QE_SPIFLASH_SR_SRP = (1 << 7),                   /**< status register protect */
};

/**
 * flash program(write) data mode
 */
enum flash_write_mode {
    FLASH_WM_PAGE_256B = 1 << 0,                            /**< write 1 to 256 bytes per page */
    FLASH_WM_BYTE = 1 << 1,                                 /**< byte write */
    FLASH_WM_AAI = 1 << 2,                                  /**< auto address increment */
    FLASH_WM_DUAL_BUFFER = 1 << 3,                          /**< dual-buffer write, like AT45DB series */
};


#define QE_SPIFLASH_CMD_PAGE_PROGRAM					0x02
#define QE_SPIFLASH_CMD_READ_DATA						0x03
#define QE_SPIFLASH_CMD_WRITE_DISABLE					0x04
#define QE_SPIFLASH_CMD_READ_SR							0x05
#define QE_SPIFLASH_CMD_WRITE_ENABLE					0x06
#define QE_SPIFLASH_CMD_ENABLE_RESET					0x66
#define QE_SPIFLASH_CMD_RESET							0x99
#define QE_SPIFLASH_CMD_WORD_PROGRAM					0xAD
#define QE_SPIFLASH_CMD_ENTER_4B_ADDRESS_MODE			0xB7
#define QE_SPIFLASH_CMD_ERASE_CHIP						0xC7
#define QE_SPIFLASH_CMD_EXIT_4B_ADDRESS_MODE			0xE9
#define QE_SPIFLASH_CMD_JEDEC_ID						0x9F
							

/* Support manufacturer JEDEC ID */
#define FLASH_MF_ID_CYPRESS                             0x01
#define FLASH_MF_ID_FUJITSU                             0x04
#define FLASH_MF_ID_EON                                 0x1C
#define FLASH_MF_ID_ATMEL                               0x1F
#define FLASH_MF_ID_MICRON                              0x20
#define FLASH_MF_ID_AMIC                                0x37
#define FLASH_MF_ID_SANYO                               0x62
#define FLASH_MF_ID_INTEL                               0x89
#define FLASH_MF_ID_ESMT                                0x8C
#define FLASH_MF_ID_FUDAN                               0xA1
#define FLASH_MF_ID_HYUNDAI                             0xAD
#define FLASH_MF_ID_SST                                 0xBF
#define FLASH_MF_ID_MACRONIX                            0xC2
#define FLASH_MF_ID_GIGADEVICE                          0xC8
#define FLASH_MF_ID_ISSI                                0xD5
#define FLASH_MF_ID_WINBOND                             0xEF


/* manufacturer information */
struct spiflash_mf {
    char *name;
    qe_uint8_t id;
};

struct qe_spiflash_chip {
	char *name;
	qe_uint8_t mf_id;
	qe_uint8_t mt_id;
	qe_uint8_t capacity_id;
	qe_uint8_t erase_gran_cmd;
	qe_uint32_t capacity;
	qe_uint16_t write_mode;
	qe_uint16_t reserve;
	qe_uint32_t erase_gran;
};

struct qe_spiflash_device {

	struct qe_device 		 parent;
	struct qe_spi_device    *spi_device;
	struct qe_spiflash_chip  chip;

	qe_uint32_t 			 nr_sectors;				/* sector number */
	qe_uint32_t				 sector_size;				/* bytes per sector */
	qe_uint32_t     	     block_size;				/* bytes per erase block */

	qe_uint32_t 			 address_4byte:1;
	qe_uint32_t 			 reserve;
};
typedef struct qe_spiflash_device *qe_spiflash_t;




qe_size_t qe_spiflash_read(qe_spiflash_t  flash,
								   qe_uint32_t    addr,
								   qe_size_t      size,
								   void          *buffer);

qe_size_t qe_spiflash_write(qe_spiflash_t  flash,
								   qe_uint32_t    addr,
								   qe_size_t      size,
								   const void    *buffer);

qe_err_t qe_spiflash_erase(qe_spiflash_t flash,
								   qe_uint32_t   addr,
								   qe_size_t     size);
qe_err_t qe_spiflash_chip_erase(qe_spiflash_t flash);

qe_spiflash_t qe_spiflash_find(const char *name);
qe_spiflash_t qe_spiflash_probe(const char *flash_dev_name, const char *spi_dev_name);

#endif /* qe_spiflash_t */

