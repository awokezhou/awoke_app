
#include "qe_core.h"
#include "qe_spi.h"
#include "bk_quickemb.h"
#include "qe_spi_flash.h"

static int xxx_app_flash_init(void)
{
	struct qe_spiflash_device *w25q;
	qe_uint8_t buffer[8192];
	
	w25q = (struct qe_spiflash_device *)qe_device_find("W25qxx");
	if (!w25q) {
		qelog_err("app", "no device spi.0");
		return -1;
	}

	qelog_info("app", "find device:%s", w25q->parent.name);

	qe_spiflash_erase(w25q, 0x0, 8192);

	qe_spiflash_read(w25q, 0x0, 8192, buffer);
	awoke_hexdump_info(buffer, 32);
	
	qe_spiflash_write(w25q, 0x0, 8192, buffer);
	
	return 0;
}
QE_APP_EXPORT(xxx_app_flash_init);

