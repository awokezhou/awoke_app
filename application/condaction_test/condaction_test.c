
#include "condition_action.h"


#define CONDITION_C1	0x01
#define ACTION_A1		0x0001

condition cond_c1 = {
	.name = "c1",
	.type = CONDITION_C1,
};

action action_a1 = {
	.name = "a1",
	.type = ACTION_A1,
};

static err_type test()
{
	typedef struct _test {
		condition_action test_ca;
	} test_t;

	test_t ts;

	condition_init(&ts.test_ca);

	condition_register(&ts.test_ca, &cond_c1);
	action_register(&ts.test_ca, &action_a1);

	condition_action_add(&ts.test_ca, CONDITION_C1, ACTION_A1);

	condition_tigger(&ts.test_ca, CONDITION_C1, NULL);
}

int main(int argc, char *argv[])
{
	log_mode(LOG_TEST);
	log_level(LOG_DBG);

	test();

	return 0;
}
