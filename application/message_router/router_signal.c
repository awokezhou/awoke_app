#include "router_main.h"


static struct _rt_manager *rt_manager_ctx;


static void signal_exit()
{
	/* ignore future signals to properly handle the cleanup */
    signal(SIGTERM, SIG_IGN);
    signal(SIGINT,  SIG_IGN);
    signal(SIGHUP,  SIG_IGN);

    router_manager_clean(&rt_manager_ctx);

   	log_info("Exiting...");
    EXIT(AWOKE_EXIT_SUCCESS);
}

static void signal_handler(int signo, siginfo_t *si, void *context)
{
    switch (signo) 
    {
        case SIGTERM:
        case SIGINT:
			log_info("SIGTERM SIGINT");
            signal_exit();
            break;
            
        case SIGHUP:
			log_info("SIGHUP");
            signal_exit();
            break;
            
        case SIGBUS:
        case SIGSEGV:
			log_info("SIGBUS SIGSEGV");
            log_err("%s (%d), code=%d, addr=%p",
               strsignal(signo), signo, si->si_code, si->si_addr);
            abort();
            
        default:
            /* let the kernel handle it */
            kill(getpid(), signo);
    }
}

static void signal_context(void *ctx)
{
    rt_manager_ctx = ctx;
}

void router_signal_init(void *context)
{
	struct sigaction act;
	memset(&act, 0x0, sizeof(act));

	/* allow signals to be handled concurrently */
	act.sa_flags = SA_SIGINFO | SA_NODEFER;
	act.sa_sigaction = &signal_handler;

	sigaction(SIGSEGV, &act, NULL);
	sigaction(SIGBUS,  &act, NULL);
    sigaction(SIGHUP,  &act, NULL);
    sigaction(SIGINT,  &act, NULL);
    sigaction(SIGTERM, &act, NULL);

	signal_context(context);
}

