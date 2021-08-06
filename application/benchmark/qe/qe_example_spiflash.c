
#include "qe_spi_flash.h"
#include "bk_quickemb.h"

static int xxx_spiflash_init(void)
{
	qe_spiflash_t dev;

	dev = qe_spiflash_probe("W25qxx", "spi0.0");
	if (!dev) {
		bk_err("probe error");
		return qe_notsupport;
	}

	return qe_ok;
}
QE_DEVICE_EXPORT(xxx_spiflash_init);

