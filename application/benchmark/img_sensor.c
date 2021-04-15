
#include "img_sensor.h"
#include "benchmark.h"


static err_type sony265_write_reg(uint8_t addr, uint8_t data);
static err_type sony265_config_write(struct bk_imgsensor *s, uint8_t *buf, int length);
static err_type flash_config_read(struct bk_imgsensor_configure *c, uint8_t *buf, int length);
static err_type flash_config_write(struct bk_imgsensor_configure *c, uint8_t *buf, int length);
static err_type memory_config_read(struct bk_imgsensor_configure *c, uint8_t *buf, int length);

static struct bk_imgsensor imgsensors[] = {
	{"SONY-IMX265LQR-C", IMG_SENSOR_ID_SONY265, 32, NULL, sony265_write_reg, NULL, sony265_config_write},
	//{"GMAX3265", IMG_SENSOR_ID_GMAX3265, NULL, gmax3265_train, gmax3265_write_reg, gmax3265_read_reg},
};

static struct bk_imgsensor_configure flash_configure = {
	.read = flash_config_read,
	.write = flash_config_write,
};

static struct bk_imgsensor_configure memory_configure = {
	.read = memory_config_read,
};

static err_type sony265_write_reg(uint8_t addr, uint8_t data)
{
	bk_debug("sony265 write reg addr:0x%x data:0x%x", addr, data);

	//mb_i2c_write();
	
	return et_ok;
}

static err_type sony265_config_write(struct bk_imgsensor *s, uint8_t *buf, int length)
{
	int i;
	
	for (i=0; i<length; i++) {
		s->write_reg(i, buf[i]);
	}

	return et_ok;
}

static err_type flash_config_read(struct bk_imgsensor_configure *c, uint8_t *buf, int length)
{
	struct bk_imgsensor_flash_configure *flashcfg = config_to_flashcfg(c);

	bk_debug("will read %d bytes from addr:0x%x", length, flashcfg->addr);

	int i;

	for (i=0; i<length; i++) {
		buf[i] = i;
	}

	return et_ok;
}

static err_type flash_config_write(struct bk_imgsensor_configure *c, uint8_t *buf, int length)
{
	struct bk_imgsensor_flash_configure *flashcfg = config_to_flashcfg(c);

	bk_debug("will read %d bytes from addr:0x%x", length, flashcfg->addr);

	int i;

	for (i=0; i<length; i++) {
		buf[i] = i;
	}

	return et_ok;	
}

static err_type memory_config_read(struct bk_imgsensor_configure *c, uint8_t *buf, int length)
{
	struct bk_imgsensor_flash_configure *flashcfg = config_to_flashcfg(c);

	bk_debug("will read %d bytes from addr:0x%x", length, flashcfg->addr);

	int i;

	for (i=0; i<length; i++) {
		buf[i] = 0xaa;
	}

	return et_ok;
}

struct bk_imgsensor *bk_imgsensor_get(uint8_t id)
{
	return &imgsensors[0];
}

err_type bk_imgsensor_config(struct bk_imgsensor *s, struct bk_imgsensor_configure *cfg)
{
	bk_debug("imgsensor(%s:%d) config...", s->name, s->id);
	
	cfg->read(cfg, cfg->buffer, s->config_bytes);
	
	s->config_write(s, cfg->buffer, s->config_bytes);

	return et_ok;
}

err_type bk_imgsensor_flash_config_generator(struct bk_imgsensor_flash_configure *flash, uint32_t addr)
{
	memcpy(&flash->base, &flash_configure, sizeof(struct bk_imgsensor_configure));
	flash->addr = addr;
	memset(flash->base.buffer, 0x0, IMG_SENSOR_CONFIG_MAXSIZE);

	bk_debug("generate flash config addr:0x%x", addr);
	
	return et_ok;
}

err_type bk_imgsensor_memory_config_generator(struct bk_imgsensor_memory_configure *mem, uint8_t *addr)
{
	memcpy(&mem->base, &memory_configure, sizeof(struct bk_imgsensor_configure));
	mem->addr = addr;
	memset(&mem->base.buffer, 0x0, IMG_SENSOR_CONFIG_MAXSIZE);

	bk_debug("generate memory config addr:0x%x", addr);
	
	return et_ok;
}

err_type benchmark_imgsensor_test(int argc, char *argv[])
{
	int i;
	struct bk_imgsensor_flash_configure fconfig;
	struct bk_imgsensor_memory_configure mconfig;
	struct bk_imgsensor *sensor = bk_imgsensor_get(IMG_SENSOR_ID_SONY265);
	if (!sensor) {
		bk_err("get sensor(%d) error", IMG_SENSOR_ID_SONY265);
		return et_param;
	}

	uint8_t buffer[32];

	for (i=0; i<32; i++) {
		buffer[i] = 0x55;
	}

	bk_imgsensor_flash_config_generator(&fconfig, 0x00000020);
	bk_imgsensor_memory_config_generator(&mconfig, &buffer);
	
	bk_imgsensor_config(sensor, &mconfig.base);
	bk_debug("config finish");
	return et_ok;
}