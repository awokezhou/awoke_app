
#ifndef __IMG_SENSOR_H__
#define __IMG_SENSOR_H__



#include "awoke_type.h"
#include "awoke_error.h"
//#include "benchmark.h"



typedef enum {
	IMG_SENSOR_ID_SONY265,
	IMG_SENSOR_ID_GMAX3265,
	IMG_SENSOR_ID_SIZEOF,
}imgsensor_id_e;

#define IMG_SENSOR_CONFIG_MAXSIZE	512

struct bk_imgsensor_configure {

	uint8_t buffer[IMG_SENSOR_CONFIG_MAXSIZE];

	err_type (*read)(struct bk_imgsensor_configure *, uint8_t *, int );
	err_type (*write)(struct bk_imgsensor_configure *, uint8_t *, int );
};

struct bk_imgsensor_flash_configure {
	struct bk_imgsensor_configure base;
	uint32_t addr;
};

struct bk_imgsensor_memory_configure {
	struct bk_imgsensor_configure base;
	uint8_t *addr;
};

struct bk_imgsensor {

	const char *name;

	uint8_t id;

	int config_bytes;

	err_type (*train)(void);

	err_type (*write_reg)(uint8_t addr, uint8_t data);

	err_type (*read_reg)(uint8_t addr, uint8_t *data);

	err_type (*config_write)(struct bk_imgsensor *s, uint8_t *buf, int length);
};



#define config_to_flashcfg(c)	container_of(c, struct bk_imgsensor_flash_configure, base)


struct bk_imgsensor *bk_imgsensor_get(uint8_t id);
err_type benchmark_imgsensor_test(int argc, char *argv[]);


#endif /* __IMG_SENSOR_H__ */
