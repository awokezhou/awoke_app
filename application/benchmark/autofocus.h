
#ifndef _BK_AUTOFOCUS_H__
#define _BK_AUTOFOCUS_H__



#include "benchmark.h"
#include "bk_fbkflow.h"
#include "awoke_macros.h"


typedef enum {
	AFOCUS_S_NONE = 0,
	AFOCUS_S_INIT_POS,
	AFOCUS_S_STEP,
	AFOCUS_S_FIN_POS,
} afocus_state_e;

typedef struct _bk_autofocus_struct {
	int window;
	int gmax;
	int fall;
	uint32_t pos;
	uint32_t step;
	uint8_t state;
	bk_fbkflow flow;

#define AFOCUS_MOTOR_CMD_POS		0x01
#define AFOCUS_MOTOR_CMD_STEP		0x02

#define AFOCUS_MOTOR_CMD_POS_RSP	0x10
#define AFOCUS_MOTOR_CMD_STEP_RSP	0x20
	uint8_t motor_cmd;

#define AFOCUS_E_MOTOR_POS			0x01
#define AFOCUS_E_MOTOR_STEP			0x02
#define AFOCUS_E_EXPO_START			0x04
#define AFOCUS_E_NEW_FRAME			0x08
	uint8_t intr_events;
} bk_autofocus_struct;



err_type benchmark_autofocus_test(int argc, char *argv[]);



#endif /* _BK_AUTOFOCUS_H__ */
