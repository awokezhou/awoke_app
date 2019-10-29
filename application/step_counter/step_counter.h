
#ifndef __STEP_COUNTER_H__
#define __STEP_COUNTER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <sys/wait.h>  
#include <semaphore.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/mman.h>
#include <sys/sem.h>

#include "awoke_log.h"
#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_macros.h"
#include "awoke_vector.h"



#define DEFAULT_FILE	"default.log"


typedef enum {
	M_NONE = 0,
	M_FILE,
	M_INPUT,
} step_mode;

typedef struct _step_unit {
	bool is_rise;
	int rise_count;
	int prev_rise_count;
	bool prev_status;
	double prev_mod;
	double peak_value;
	double trough_value;
	time_t prev_peak_time;
} step_unit;

typedef struct _step_monitor {
	char timestr[128];
	int peak_count;
	double mod_max;
	double mod_min;
	int rise_count;
} step_monitor;

typedef enum _step_direct {
	STEP_DIRECT_FLAT = 0,
	STEP_DIRECT_DOWN,
	STEP_DIRECT_RISE,
} step_direct;

typedef enum _step_wave {
	STEP_WAVE_NONE = 0,
	STEP_WAVE_PEAK,
	STEP_WAVE_VALLEY,
} step_wave;

typedef struct _step_peaker {
	step_direct direct;
	int rise_count;
	double mod;
	time_t time;
} step_peaker;

#endif /* __STEP_COUNTER_H__ */
