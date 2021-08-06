
#include "qe_core.h"
#include "qe_spi.h"
#include "qe_spi_flash.h"
#include "bk_quickemb.h"


static struct qe_spiflash_chip flash_chip_table[] = {
	{"AT45DB161E", 	FLASH_MF_ID_ATMEL, 	 	0x26, 0x00, 0x81, 2L*1024L*1024L,  FLASH_WM_BYTE|FLASH_WM_DUAL_BUFFER, 0x0, 512},
	{"W25Q40BV", 	FLASH_MF_ID_WINBOND, 	0x40, 0x13, 0x20, 512L*1024L, 	   FLASH_WM_PAGE_256B, 			0x0, 4096},
	{"W25Q16BV", 	FLASH_MF_ID_WINBOND, 	0x40, 0x15, 0x20, 2L*1024L*1024L,  FLASH_WM_PAGE_256B, 			0x0, 4096},
	{"W25Q64CV", 	FLASH_MF_ID_WINBOND, 	0x40, 0x17, 0x20, 8L*1024L*1024L,  FLASH_WM_PAGE_256B, 			0x0, 4096},
	{"W25Q64DW", 	FLASH_MF_ID_WINBOND, 	0x60, 0x17, 0x20, 8L*1024L*1024L,  FLASH_WM_PAGE_256B, 			0x0, 4096},
	{"W25Q128BV", 	FLASH_MF_ID_WINBOND, 	0x40, 0x18, 0x20, 16L*1024L*1024L, FLASH_WM_PAGE_256B, 			0x0, 4096},
	{"W25Q256FV", 	FLASH_MF_ID_WINBOND, 	0x40, 0x19, 0x20, 32L*1024L*1024L, FLASH_WM_PAGE_256B, 	 		0x0, 4096},
	{"SST25VF016B", FLASH_MF_ID_SST, 	 	0x25, 0x41, 0x20, 2L*1024L*1024L,  FLASH_WM_BYTE|FLASH_WM_AAI,	0x0, 4096},
	{"M25P32", 		FLASH_MF_ID_MICRON,  	0x20, 0x16, 0xD8, 4L*1024L*1024L,  FLASH_WM_PAGE_256B, 			0x0, 64L*1024L},
	{"M25P80", 		FLASH_MF_ID_MICRON,  	0x20, 0x14, 0xD8, 1L*1024L*1024L,  FLASH_WM_PAGE_256B, 			0x0, 64L*1024L},
	{"M25P40", 		FLASH_MF_ID_MICRON,  	0x20, 0x13, 0xD8, 512L*1024L, 	   FLASH_WM_PAGE_256B, 			0x0, 64L*1024L},
	{"EN25Q32B", 	FLASH_MF_ID_EON,      	0x30, 0x16, 0x20, 4L*1024L*1024L,  FLASH_WM_PAGE_256B, 			0x0, 4096},
	{"GD25Q64B", 	FLASH_MF_ID_GIGADEVICE, 0x40, 0x17, 0x20, 8L*1024L*1024L,  FLASH_WM_PAGE_256B, 			0x0, 4096},
	{"GD25Q16B", 	FLASH_MF_ID_GIGADEVICE, 0x40, 0x15, 0x20, 2L*1024L*1024L,  FLASH_WM_PAGE_256B, 			0x0, 4096},
	{"S25FL216K", 	FLASH_MF_ID_CYPRESS, 	0x40, 0x15, 0x20, 2L*1024L*1024L,  FLASH_WM_PAGE_256B, 			0x0, 4096},
	{"S25FL032P", 	FLASH_MF_ID_CYPRESS, 	0x02, 0x15, 0x20, 4L*1024L*1024L,  FLASH_WM_PAGE_256B, 			0x0, 4096},
	{"A25L080", 	FLASH_MF_ID_AMIC, 		0x30, 0x14, 0x20, 1L*1024L*1024L,  FLASH_WM_PAGE_256B, 			0x0, 4096},
	{"F25L004", 	FLASH_MF_ID_ESMT, 		0x20, 0x13, 0x20, 512L*1024L,  	   FLASH_WM_BYTE|FLASH_WM_AAI,	0x0, 4096},
	{"PCT25VF016B", FLASH_MF_ID_SST, 		0x25, 0x41, 0x20, 2L*1024L*1024L,  FLASH_WM_BYTE|FLASH_WM_AAI,	0x0, 4096},
};

static struct spiflash_mf flash_mf_table[] = {
    {"Cypress",    FLASH_MF_ID_CYPRESS},
    {"Fujitsu",    FLASH_MF_ID_FUJITSU},
    {"EON",        FLASH_MF_ID_EON},
    {"Atmel",      FLASH_MF_ID_ATMEL},
    {"Micron",     FLASH_MF_ID_MICRON},
    {"AMIC",       FLASH_MF_ID_AMIC},
    {"Sanyo",      FLASH_MF_ID_SANYO},
    {"Intel",      FLASH_MF_ID_INTEL},
    {"ESMT",       FLASH_MF_ID_ESMT},
    {"Fudan",      FLASH_MF_ID_FUDAN},
    {"Hyundai",    FLASH_MF_ID_HYUNDAI},
    {"SST",        FLASH_MF_ID_SST},
    {"GigaDevice", FLASH_MF_ID_GIGADEVICE},
    {"ISSI",       FLASH_MF_ID_ISSI},
    {"Winbond",    FLASH_MF_ID_WINBOND},
    {"Macronix",   FLASH_MF_ID_MACRONIX},
};


static void make_address_byte_array(struct qe_spiflash_device *flash, 
											     qe_uint32_t 				addr, 
											     qe_uint8_t                *array) 
{
    uint8_t len, i;

    len = flash->address_4byte ? 4 : 3;

    for (i=0; i<len; i++) {
        array[i] = (addr >> ((len - (i + 1)) * 8)) & 0xFF;
    }
}

static qe_err_t spiflash_read_jedec_id(struct qe_spiflash_device *flash)
{
	qe_err_t ret;
	qe_uint8_t cmd[1], data[3];

	bk_debug("read jedec id");
	cmd[0] = QE_SPIFLASH_CMD_JEDEC_ID;
	ret = qe_spi_send_recv(flash->spi_device, cmd, 1, data, 3);
	if (ret != qe_ok) {
		bk_err("read JEDEC ID error:%d", ret);
		return ret;
	}

	flash->chip.mf_id = data[0];
	flash->chip.mt_id = data[1];
	flash->chip.capacity_id = data[2];

	/*<debug>*/
	flash->chip.mf_id = 0xEF;
	flash->chip.mt_id = 0x40;
	flash->chip.capacity_id = 0x18;
	bk_info("Manufacturer ID: 0x%2X", flash->chip.mf_id);
	bk_info("Memory type ID: 0x%2X",  flash->chip.mt_id);
	bk_info("Capacity ID: 0x%2X",     flash->chip.capacity_id);

	return ret;
}

static qe_err_t spiflash_read_status(struct qe_spiflash_device *flash, qe_uint8_t *status)
{
	qe_size_t n;
	qe_uint8_t cmd = QE_SPIFLASH_CMD_READ_SR;
	n = qe_spi_transfer(flash->spi_device, &cmd, status, 1);
	if (n <= 0) {
		/* <debug> */
		return qe_ok;
		return qe_esend;
	}

	/* <debug> */
	*status &= ~QE_SPIFLASH_SR_BUSY;
	return qe_ok;
}

static qe_err_t spiflash_wait_busy(struct qe_spiflash_device *flash)
{
	qe_err_t ret;
	qe_uint8_t status = 0x0;

	while (1) {
		ret = spiflash_read_status(flash, &status);
		if ((ret==qe_ok) && !(status & QE_SPIFLASH_SR_BUSY)) {
			break;
		}
	}

	if ((ret!=qe_ok) || (status & QE_SPIFLASH_SR_BUSY)) {
		bk_err("wait busy");
	}

	return ret;
}

static qe_err_t spiflash_write_enable(struct qe_spiflash_device *flash, qe_bool_t en)
{
	qe_size_t n;
	qe_err_t ret;
	qe_uint8_t cmd, sr;

	qe_assert(flash);

	if (en) {
		cmd = QE_SPIFLASH_CMD_WRITE_ENABLE;
	} else {
		cmd = QE_SPIFLASH_CMD_WRITE_DISABLE;
	}

	n = qe_spi_transfer(flash->spi_device, &cmd, QE_NULL, 1);
	if (n) {
		ret = spiflash_read_status(flash, &sr);
	}

	if (ret == qe_ok) {
		if (en && !(sr & QE_SPIFLASH_SR_WEL)) {
			bk_err("write enable status error");
			return qe_ewrite;
		} else if (!en && (sr & QE_SPIFLASH_SR_WEL)) {
			return qe_ewrite;
		}
	}

	return ret;
}

static qe_err_t spiflash_reset(struct qe_spiflash_device *flash)
{
	qe_size_t n;
	qe_uint8_t cmd[1];

	cmd[0] = QE_SPIFLASH_CMD_ENABLE_RESET;
	n = qe_spi_transfer(flash->spi_device, cmd, QE_NULL, 1);
	if (n <= 0) {
		bk_err("flash enable reset failed");
		return qe_esend;
	}

	spiflash_wait_busy(flash);

	cmd[0] = QE_SPIFLASH_CMD_RESET;
	n = qe_spi_transfer(flash->spi_device, cmd, QE_NULL, 1);
	if (n <= 0) {
		bk_err("flash reset failed");
		return qe_esend;
	}

	spiflash_wait_busy(flash);

	return qe_ok;
}

/**
 * This function will write flash data for write 1 to 256 bytes per page
 * 
 * @flash      : the flash device
 * @addr  	   : write start address
 * @size       : write size
 * @write_gran : write granularity bytes, ony 1 or 256
 *
 * @return write length
 */
qe_err_t spiflash_page256_or_1byte_write(struct qe_spiflash_device *flash,
														qe_uint32_t 			   addr,
														qe_size_t				   size,
														qe_uint16_t			       write_gran,
														const qe_uint8_t  		  *buffer)
{
	qe_size_t n;
	qe_err_t ret = qe_ok;
	static qe_uint8_t cmd[5 + 256];
	qe_uint8_t cmd_size;
	qe_size_t write_size;

	qe_assert(flash);
	qe_assert((write_gran==1) || (write_gran==256));

	/* check flash address bound */
	if ((addr+size) > flash->chip.capacity) {
		bk_err("flash address out of bound");
		return qe_erange;
	}

	/* loop write operate. write unit is write granularity */
	while (size) {

		/* write enable */		
		ret = spiflash_write_enable(flash, QE_TRUE);
		if (ret != qe_ok) {
			goto __exit;
		}

		cmd[0] = QE_SPIFLASH_CMD_PAGE_PROGRAM;
		make_address_byte_array(flash, addr, &cmd[1]);
		cmd_size = flash->address_4byte ? 5 : 4;

		/* make write align and calculate next write address */
		if (addr % write_gran != 0) {
			if (size > write_gran - (addr % write_gran)) {
				write_size = write_gran - (addr % write_gran);
			} else {
				write_size = size;
			}
		} else {
			if (size > write_gran) {
            	write_size = write_gran;
        	} else {
            	write_size = size;
        	}
		}

		size -= write_size;
        addr += write_size;

		qe_memcpy(&cmd[cmd_size], (void *)buffer, write_size);
		bk_info("write size:%d", write_size);
		n = qe_spi_transfer(flash->spi_device, cmd, QE_NULL, cmd_size+write_size);
		if (n == 0) {
			bk_err("write spi error");
			goto __exit;
		}

		/* wait busy */
		ret = spiflash_wait_busy(flash);
		if (ret != qe_ok) {
			bk_err("wait busy");
			goto __exit;
		}

		buffer += write_size;
	}

__exit:
	spiflash_write_enable(flash, QE_FALSE);
	return ret;
}

/**
 * This function will write flash data for auto address increment mode
 *
 * @note: If address is odd number, it will place on 0xFF before the start of data for protect
 *        the old data. If the latest remain size is 1, it will append one 0xFF at the end of 
 *        data for protect the old data.
 *
 * @flash  : the flash device
 * @addr   : write start address
 * @size   : write size
 * @data   : write data
 *
 * @return result
 */
static qe_err_t spiflash_aai_write(struct qe_spiflash_device *flash,
										  qe_uint32_t                addr,
										  qe_size_t                  size,
										  const qe_uint8_t          *buffer)
{
	qe_size_t n;
	qe_err_t ret;
	qe_uint8_t cmd[8], cmd_size;
	qe_bool_t first_write = QE_TRUE;

	qe_assert(flash);
	qe_assert(flash->spi_device);

	/* check flash address bound */
	if ((addr+size) > flash->chip.capacity) {
		bk_err("address out of bound");
		return qe_erange;
	}

	/**
	 * The address must be even for AAI write mode. So it must write 
     * one byte first when address is odd. 
     */
    if (addr % 2) {
		ret = spiflash_page256_or_1byte_write(flash, addr++, 1, 1, buffer++);
		if (ret != qe_ok) {
			goto __exit;
		}
		size--;
	}

	/* set the flash write enable */
	ret = spiflash_write_enable(flash, QE_TRUE);
	if (ret != qe_ok) {
		goto __exit;
	}

	/* loop write operate. */
	cmd[0] = QE_SPIFLASH_CMD_WORD_PROGRAM;
	while (size >= 2) {
		if (first_write) {
			make_address_byte_array(flash, addr, &cmd[1]);
			cmd_size = flash->address_4byte ? 5 : 4;
			cmd[cmd_size] = *buffer;
			cmd[cmd_size+1] = *(buffer+1);
			first_write = QE_FALSE;
		} else {
            cmd_size = 1;
            cmd[1] = *buffer;
            cmd[2] = *(buffer + 1);			
		}

		n = qe_spi_transfer(flash->spi_device, &cmd, QE_NULL,  cmd_size+2);
		if (!n) {
			bk_err("write spi xfer error");
			goto __exit;
		}

		ret = spiflash_wait_busy(flash);
		if (ret != qe_ok) {
			bk_err("wait busy err:%d", ret);
			goto __exit;
		}

		size -= 2;
        addr += 2;
        buffer += 2;
	}

	/* set the flash write disable for exit AAI mode */
	ret = spiflash_write_enable(flash, QE_FALSE);

	/* write last one byte data when origin write size is odd */
	if (ret == qe_ok && size == 1) {
		ret = spiflash_page256_or_1byte_write(flash, addr, 1, 1, buffer);
	}

__exit:
	if (ret != qe_ok) {
		spiflash_write_enable(flash, QE_FALSE);
	}

	return ret;
}


/**
 * This function enable or disable spiflash 4-Byte address mode
 *
 * @note : The 4-Byte address mode just support for the flash capacity which is large than 16MB(256Mb)
 *
 * @flash : the flash device
 * @en	  : enable if true, else disable
 *
 * @return result
 */
static qe_err_t spiflash_set_4byte_address_mode(struct qe_spiflash_device *flash,
																 qe_bool_t 					en)
{
	qe_size_t n;
	qe_err_t ret;
	qe_uint8_t cmd;

	qe_assert(flash);

	/* write enable */
	ret = spiflash_write_enable(flash, QE_TRUE);
	if (ret != qe_ok) {
		return ret;
	}

	if (en) {
		cmd = QE_SPIFLASH_CMD_ENTER_4B_ADDRESS_MODE;
	} else {
		cmd = QE_SPIFLASH_CMD_EXIT_4B_ADDRESS_MODE;
	}

	n = qe_spi_transfer(flash->spi_device, &cmd, QE_NULL, 1);
	if (n > 0) {
		flash->address_4byte = en ? QE_TRUE : QE_FALSE;
		bk_info("%s 4-Byte address mode", en?"enter":"exit");
	} else {
		ret = qe_esend;
		bk_err("%s 4-Byte address mode error", en?"enter":"exit");
	}

	return ret;
}


/**
 * This function will read data from spiflash.
 *
 * @flash  : the spiflash device
 * @addr   : read start address
 * @size   : read size
 * @buffer : read data buffer
 *
 * @return read length
 */
qe_size_t qe_spiflash_read(qe_spiflash_t  flash,
								   qe_uint32_t    addr,
								   qe_size_t      size,
								   void          *buffer)
{
	qe_size_t n;
	qe_err_t ret;
	qe_uint8_t cmd[5], cmd_size;

	qe_assert(flash != QE_NULL);
	qe_assert(flash->spi_device != QE_NULL);

	/* check flash address bound */
	if ((addr+size) > flash->chip.capacity) {
		bk_err("read address out of bound");
		return qe_erange;
	}

	ret = spiflash_wait_busy(flash);
	if (ret == qe_ok) {
		cmd[0] = QE_SPIFLASH_CMD_READ_DATA;
		make_address_byte_array(flash, addr, &cmd[1]);
		cmd_size = flash->address_4byte ? 5 : 4;
		n = qe_spi_send_recv(flash->spi_device, cmd, cmd_size, buffer, size);
	} else {
		return 0;
	}

	return n;
}


/**
 * This function will write data to spiflash.
 * 
 * @flash  : the spiflash device
 * @addr   : write start address
 * @size   : write size
 * @buffer : write data buffer
 * 
 * @return write length
 */
qe_size_t qe_spiflash_write(qe_spiflash_t  flash,
								   qe_uint32_t    addr,
								   qe_size_t      size,
								   const void    *buffer)
{
	qe_err_t ret;

	ret = qe_spiflash_erase(flash, addr, size);
	if (ret != qe_ok) {
		bk_err("erase error:%d", ret);
		return ret;
	}

	if (flash->chip.write_mode & FLASH_WM_PAGE_256B) {
		bk_info("page256");
		ret = spiflash_page256_or_1byte_write(flash, addr, size, 256, buffer);
	} else if (flash->chip.write_mode & FLASH_WM_AAI) {
		bk_info("aai write");
		ret = spiflash_aai_write(flash, addr, size, buffer);
	}

	return ret;
}

/*
 * This function will erase all flash data on chip
 *
 * @flash : the flash device
 *
 * @return result
 */
qe_err_t qe_spiflash_chip_erase(qe_spiflash_t flash)
{
	qe_size_t n;
	qe_err_t ret;
	qe_uint8_t cmd[4];

	qe_assert(flash);
	qe_assert(flash->spi_device);

	/* set the flash write enable */
	ret = spiflash_write_enable(flash, QE_TRUE);
	if (ret != qe_ok) {
		goto __exit;
	}

	cmd[0] = QE_SPIFLASH_CMD_ERASE_CHIP;
    /* dual-buffer write, like AT45DB series flash chip erase operate is different for other flash */
	if (flash->chip.write_mode & FLASH_WM_DUAL_BUFFER) {
		cmd[1] = 0x94;
        cmd[2] = 0x80;
        cmd[3] = 0x9A;
		n = qe_spi_transfer(flash->spi_device, &cmd, QE_NULL, 4);
	} else {
		n = qe_spi_transfer(flash->spi_device, &cmd, QE_NULL, 1);
	}

	if (!n) {
		bk_err("chip erase spi xfer err");
		goto __exit;
	}

	spiflash_wait_busy(flash);

__exit:
	spiflash_write_enable(flash, QE_FALSE);
	return ret;
}

/**
 * This function will erase align by erase granularity.
 * 
 * @flash : the spiflash device
 * @addr  : erase start address
 * @size  : erase size
 *
 * @return result
 */
qe_err_t qe_spiflash_erase(qe_spiflash_t flash,
								   qe_uint32_t   addr,
								   qe_size_t     size)
{
	qe_err_t ret = qe_ok;
	qe_uint8_t cmd[5], cmd_size;
	qe_size_t curr_erase_size, n;
	
	qe_assert(flash);
	qe_assert(flash->spi_device);

	/* check flash address bound */
	if ((addr+size) > flash->chip.capacity) {
		bk_err("read address out of bound");
		return qe_erange;
	}

	if ((addr==0x0) && (size==flash->chip.capacity)) {
		return qe_spiflash_chip_erase(flash);
	}

	curr_erase_size = flash->chip.erase_gran;
	bk_debug("curr_erase_size:%d", curr_erase_size);

	/* loop erase operate. erase unit is erase granularity */
	while (size) {

		/* write enable */
		ret = spiflash_write_enable(flash, QE_TRUE);
		if (ret != qe_ok) {
			bk_err("write enable error:%d", ret);
			goto __exit;
		}

		/* do gran erase */
		cmd[0] = flash->chip.erase_gran_cmd;
		make_address_byte_array(flash, addr, &cmd[1]);
		cmd_size = flash->address_4byte ? 5 : 4;
		n = qe_spi_transfer(flash->spi_device, cmd, QE_NULL, cmd_size);
		if (n == 0) {
			bk_err("erase spi error");
			goto __exit;
		}

		/* wait busy */
		ret = spiflash_wait_busy(flash);
		if (ret != qe_ok) {
			bk_err("wait busy");
			goto __exit;
		}

		/* make erase align and calculate next erase address */
		if (addr % curr_erase_size) {
			if (size > curr_erase_size - (addr % curr_erase_size)) {
				size -= curr_erase_size - (addr % curr_erase_size);
				addr += curr_erase_size - (addr % curr_erase_size);
			} else {
				goto __exit;
			}
		} else {
			if (size > curr_erase_size) {
				size -= curr_erase_size;
				addr += curr_erase_size;
			} else {
				goto __exit;
			}
		}
	}

__exit:
	spiflash_write_enable(flash, QE_FALSE);
	return ret;
}

static qe_size_t spiflash_read(qe_device_t  dev,
								    qe_off_t      pos,
								    void         *buffer,
									qe_size_t     size)
{
	qe_size_t phy_size;
	qe_uint32_t phy_addr;
	struct qe_spiflash_device *flash;

	flash = (struct qe_spiflash_device *)dev;

	qe_assert(dev);
	qe_assert(flash);
	qe_assert(flash->spi_device);

	/* change the block device's logic address to physical address */
	phy_addr = pos * flash->sector_size;
	phy_size = size * flash->sector_size;
	return qe_spiflash_read(flash, phy_addr, phy_size, buffer);
}

static qe_size_t spiflash_write(qe_device_t  dev,
									 qe_off_t      pos,
									 const void   *buffer,
									 qe_size_t     size)
{
	qe_size_t phy_size;
	qe_uint32_t phy_addr;
	struct qe_spiflash_device *flash;

	flash = (struct qe_spiflash_device *)dev;
	
	qe_assert(dev);
	qe_assert(flash);
	qe_assert(flash->spi_device);

	/* change the block device's logic address to physical address */
	phy_addr = pos * flash->sector_size;
	phy_size = size * flash->sector_size;
	return qe_spiflash_write(flash, phy_addr, phy_size, buffer);
}

const static struct qe_device_ops spiflash_ops = {
	QE_NULL,
	QE_NULL,
	QE_NULL,
	spiflash_read,
	spiflash_write,
	QE_NULL
};

qe_spiflash_t qe_spiflash_find(const char *name)
{
	struct qe_spiflash_device *flash;

	flash = (struct qe_spiflash_device *)qe_device_find(name);
	if (flash == QE_NULL || flash->parent.class != QE_DevClass_Block) {
		bk_err("flash device %s not found", name);
		return QE_NULL;
	}

	return flash;
}

qe_spiflash_t qe_spiflash_probe(const char *flash_dev_name, const char *spi_dev_name)
{
	int i;
	qe_err_t ret;
	qe_bool_t find = QE_FALSE;
	struct qe_spi_device *spi;
	struct qe_spiflash_chip *chip;
	struct qe_spiflash_device *flash;
	char *flash_mf_name = QE_NULL;
	
	qe_assert(flash_dev_name != QE_NULL);
	qe_assert(spi_dev_name != QE_NULL);

	spi = (struct qe_spi_device *)qe_device_find(spi_dev_name);
	if (!spi) {
		bk_err("spi device %s not find", spi_dev_name);
		return QE_NULL;
	}

	flash = qe_malloc(sizeof(struct qe_spiflash_device));
	if (flash == QE_NULL) {
		return QE_NULL;
	}

	qe_memset(flash, 0x0, sizeof(struct qe_spiflash_device));
	flash->spi_device = spi;

	ret = spiflash_read_jedec_id(flash);
	if (ret != qe_ok)
		goto __error;

	for (i=0; i<sizeof(flash_chip_table); i++) {
		chip = &flash_chip_table[i];
		if ((chip->mf_id == flash->chip.mf_id) &&
			(chip->mt_id == flash->chip.mt_id) &&
			(chip->capacity_id == flash->chip.capacity_id)) {
			flash->chip.name = chip->name;
			flash->chip.capacity = chip->capacity;
			flash->chip.write_mode = chip->write_mode;
			flash->chip.erase_gran = chip->erase_gran;
			flash->chip.erase_gran_cmd = chip->erase_gran_cmd;
			find = QE_TRUE;
			break;
		}
	}

	if (!find) {
		bk_warn("flash device %s not support", flash_dev_name);
		goto __error;
	}

	/* find the manufacturer information */
	for (i=0; i<sizeof(flash_mf_table); i++) {
		if (flash_mf_table[i].id == flash->chip.mf_id) {
			flash_mf_name = flash_mf_table[i].name;
			break;
		}
	}

	/* print manufacturer and flash chip name */
	if (flash_mf_name && flash->chip.name) {
		bk_info("flash %s %s size %ld bytes", flash_mf_name, flash->chip.name, flash->chip.capacity);
	}

	/* reset flash device */
	ret = spiflash_reset(flash);
	if (ret != qe_ok) {
		goto __error;
	}

	/* if the flash is large than 16MB(256Mb) then enter 4-Byte address mode */
	if (flash->chip.capacity > (1L << 24)) {
		ret = spiflash_set_4byte_address_mode(flash, QE_TRUE);
		if (ret != qe_ok) {
			goto __error;
		}
	} else {
		flash->address_4byte = QE_FALSE;
	}

	flash->nr_sectors  = flash->chip.capacity / flash->chip.erase_gran;
	flash->sector_size = flash->chip.erase_gran;
	flash->block_size  = flash->chip.erase_gran;

	flash->parent.class = QE_DevClass_Block;
	flash->parent.ops = &spiflash_ops;
	qe_device_register(&(flash->parent), flash_dev_name, QE_DEV_F_RDWR|QE_DEV_F_STANDALONE);
	bk_info("probe spiflash %s by %s success", flash_dev_name, spi_dev_name);
	return flash;

__error:
	if (flash)
		qe_free(flash);
	return QE_NULL;
}
