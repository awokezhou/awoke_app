
#include <getopt.h>

#include "benchmark.h"


static void usage(int ex)
{	
	printf("usage : benchamrk [option]\n\n");

    printf("    -waitev-test,    --file\t\tread data from file\n");
    printf("    -condaction-test,    --input\t\tread data from input\n");

	EXIT(ex);
}

static err_type wait_test1_fn(awoke_wait_ev *ev)
{
	static int a = 0;

	a++;

	log_debug("a %d", a);

	if (a >= 5) {
		log_debug("a > 5, finish");
		return et_waitev_finish;
	}

	return et_ok;
}

err_type awoke_wait_test()
{
	log_debug("awoke_wait_test");
	awoke_wait_ev *ev = awoke_waitev_create("wait-test", 5, WAIT_EV_F_SLEEP, wait_test1_fn, NULL);
	if (!ev) {
		log_err("ev create error");
		return et_waitev_create;
	}

	return awoke_waitev(ev);
}

int main(int argc, char *argv[])
{

	int opt;
	bencmark_func bmfn = NULL;

	log_debug("bencmark");

	static const struct option long_opts[] = {
        {"waitev-test",			no_argument,	NULL,   arg_waitev_test},
        {"condaction-test",  	no_argument,  	NULL,   arg_condaction_test},
        {NULL, 0, NULL, 0}
    };	

	while ((opt = getopt_long(argc, argv, "?h-", long_opts, NULL)) != -1)
    {
        switch (opt)
        {
            case arg_waitev_test:
                return awoke_wait_test();
                
            case arg_condaction_test:
                break;

            case '?':
            case 'h':
			case '-':
            default:
                usage(AWOKE_EXIT_SUCCESS);
        }
    }

	return 0;
}
