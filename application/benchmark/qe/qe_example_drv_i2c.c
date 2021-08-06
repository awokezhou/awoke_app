
#include "qe_device.h"



static qe_size_t example_i2c_master_xfer(struct qe_i2c_bus    *bus, 
													 struct qe_i2c_message msgs[],
													 qe_uint32_t           num)
{
	int i;
	qe_size_t n = 0;

	qe_assert(bus != QE_NULL);

	/* slaver address set */

	for (i=0; i<num; i++) {
		
	}

	return n;
}

static const struct qe_i2c_bus_ops example_i2c_ops =
{
    .master_xfer = example_i2c_master_xfer,
    .slave_xfer = QE_NULL,
    .control = QE_NULL,
};

static struct qe_i2c_bus i2c0_bus = {
	.ops = &example_i2c_ops,
};

struct example_i2c_hw {
	qe_uint32_t regbase;
};

static struct example_i2c_hw i2c0_hw = {
	.regbase = 0x00000000,
};

static void example_i2c_hw_init(struct example_i2c_hw *hw)
{
	
}

static int example_i2c_init(void)
{
	example_i2c_hw_init(&i2c0_hw);

	return qe_i2c_bus_register(&i2c0_bus, "i2c0");
}
QE_BOARD_EXPORT(example_i2c_init);
