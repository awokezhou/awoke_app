
#ifndef __BK_FBKFLOW_H__
#define __BK_FBKFLOW_H__



#include "awoke_type.h"
#include "awoke_error.h"


typedef enum {
	FBKFLOW_S_INIT = 0,
	FBKFLOW_S_IDLE,
	FBKFLOW_S_SET,
	FBKFLOW_S_EXPO,
	FBKFLOW_S_FRAME,
} fbkflow_state;

#define FBKFLOW_F_ENABLE	0x0001

#define FBKFLOW_E_SET		0x00000001
#define FBKFLOW_E_EXPO		0x00000002
#define FBKFLOW_E_FRAME		0x00000004

typedef struct _bk_feedback_flow {
	
	uint16_t state;

	uint16_t flags;

	uint32_t event;

	bool (*wait)(struct _bk_feedback_flow *);

	err_type (*update)(struct _bk_feedback_flow *);

	void *data;
	
} bk_fbkflow;



void bk_fbkflow_init(struct _bk_feedback_flow *flow, bool enable);
void bk_fbkflow_notify(struct _bk_feedback_flow *flow, uint32_t event);
void bk_fbkflow_enable(struct _bk_feedback_flow *flow, bool enable);
err_type bk_fbkflow_run(struct _bk_feedback_flow *flow);
err_type benchmark_fbkflow_test(int argc, char *argv[]);


#endif /* __BK_FBK_RING_H__ */

