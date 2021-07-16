#include <unistd.h>
#include <stdint.h>
#include <stdio.h>


typedef void (*init_call)(void);



init_call _init_start;
init_call _init_end;


#define _init __attribute__((unused, section(".myinit.1")))
#define DECLARE_INIT(func) init_call _fn_##func _init = func

static void A_init(void)
{
    printf("A_init\n");
}
DECLARE_INIT(A_init);

static void B_init(void)
{
    printf("B_init\n");
}

DECLARE_INIT(B_init);


void do_initcalls(void)
{
    init_call *init_ptr = &_init_start;

    for (; init_ptr < &_init_end; init_ptr++) {

        printf("init address: %p\n", init_ptr);

        (*init_ptr)();
    }
}

int main(void)
{
    do_initcalls();

    return 0;
}


