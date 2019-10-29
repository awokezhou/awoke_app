
#include <getopt.h>
#include <stdio.h>
#include "step_counter.h"



static FILE *g_fp = NULL;
static step_unit g_unit;
static step_peaker g_peaker;
static step_monitor g_monitor;


static void usage(int ex)
{	
	printf("usage : step_counter [option]\n\n");

    printf("    -f,    --file\t\tread data from file\n");
    printf("    -i,    --input\t\tread data from input\n");

	EXIT(ex);
}

static err_type get_input_data(vector *vec)
{
	char str[512] = "\0";
	printf("please input data:\n");

	while (fgets(str, 512, stdin) < 0) {
		/**/
	}	

	sscanf(str, "%lf,%lf,%lf", &vec->x, &vec->y, &vec->z);
	log_debug("input data [%lf,%lf,%lf]", vector_printp(vec));

	return et_ok;
}

static err_type step_file_open(char *file)
{
	FILE *fp;

	fp = fopen(file, "r");
	if (!fp) 
		return et_file_open;

	g_fp = fp;
	return et_ok;
}

static void step_file_close()
{
	FILE *fp = g_fp;
	fclose(fp);
}

static err_type get_step_file_one(vector_tm *tv)
{
	FILE *fp = g_fp;
	char str[1024] = "\0";
	char str_time[128] = "\0";
	char str_data[128] = "\0";
	vector *vec = &tv->vec;
	struct tm now;

	if (feof(fp)) {
		step_file_close();
		return et_fail;
	}

	if (!fgets(str, 1024, fp)) {
		step_file_close();
		return et_fail;
	}

	if (str[0] != '[') {
		step_file_close();
		return et_fail;
	}

	sscanf(str, "[%[^\]]][%[^\]]", str_time, str_data);
	strptime(str_time, "%Y-%m-%d %H:%M:%S", &now);
	tv->tm = mktime(&now);
	sscanf(str_data, "%lf,%lf,%lf", &vec->x, &vec->y, &vec->z);
	//log_debug("input data [%lf,%lf,%lf]", vector_printp(vec));
	return et_ok;
}

static step_wave step_peaker_detect(double mod, vector_tm *tv)
{
	int rise_count;
	step_wave wave;
	step_direct now_direct;
	
	if (mod >= g_peaker.mod) {
		now_direct = STEP_DIRECT_RISE;
		g_peaker.rise_count++;
	} else {
		now_direct = STEP_DIRECT_DOWN;
		if (g_peaker.rise_count) {
			rise_count = g_peaker.rise_count;
			g_peaker.rise_count = 0;
		}
	}

	if (now_direct != g_peaker.direct) {
		// peak
		if ((now_direct == STEP_DIRECT_DOWN) && 
			(rise_count>=g_monitor.rise_count ||
			(g_peaker.mod>=g_monitor.mod_min && g_peaker.mod<=g_monitor.mod_max)) &&
			//((tv->tm - g_peaker.time)<=1)) {
			TRUE){
			wave = STEP_WAVE_PEAK;
			g_peaker.time = tv->tm;
		} else if (now_direct == STEP_DIRECT_RISE) {
			wave = STEP_WAVE_VALLEY;
		} else {
			wave = STEP_WAVE_NONE;
		}
	} else {
		wave = STEP_WAVE_NONE;
	}

	g_peaker.direct = now_direct;
	return wave;
}

static bool step_peak_detect(double mod)
{
	g_unit.prev_status = g_unit.is_rise;

	if (mod >= g_unit.prev_mod) {
		//log_debug("rise");
		g_unit.is_rise = TRUE;
		g_unit.rise_count++;
	} else {
		//log_debug("fall");
		g_unit.prev_rise_count = g_unit.rise_count;
		g_unit.is_rise = FALSE;
		g_unit.rise_count = 0;
	}

	if (!g_unit.is_rise && g_unit.prev_status) {
		if ((g_unit.prev_rise_count >= g_monitor.rise_count) || 
			((g_unit.prev_mod <= g_monitor.mod_max) && (g_unit.prev_mod >= g_monitor.mod_min))) {
			g_unit.peak_value = g_unit.prev_mod;
			//log_info("peak detect!!!, value:%lf", g_unit.peak_value);
			return TRUE;
		} 
	} else if (!g_unit.prev_status && g_unit.is_rise) {
		g_unit.trough_value = g_unit.prev_mod;
		//log_info("trough detect!!! value:%lf", g_unit.trough_value);
		return FALSE;
	}

	return FALSE;
}

static void step_detect(vector_tm *tv)
{
	double mod;
	step_wave wave;
	bool is_peak = FALSE;

	mod = vector_mod(&tv->vec);
	//log_debug("mod:%lf", mod);

	if (g_peaker.mod != 0) {
		//is_peak = step_peak_detect(mod);
		wave = step_peaker_detect(mod, tv);
		if (wave != STEP_WAVE_NONE) {
			if (wave == STEP_WAVE_PEAK) {
				g_monitor.peak_count++;
				char timestr[32];
				strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(& g_peaker.time));
				//log_info("peak value %lf, timestr %s", g_peaker.mod, timestr);
			} else {
				//log_debug("valley value %lf", g_peaker.mod);
			}
		}
	}

	//g_unit.prev_mod = mod;
	g_peaker.mod = mod;
	return;
}

static err_type get_step_data(vector_tm *vec, step_mode mode)
{
	if (mode == M_FILE)
		return get_step_file_one(vec);
	else
		return get_input_data(&vec->vec);
}

static void step_init(step_monitor *m)
{
	m->mod_max = 50;
	m->mod_min = 20;
	m->rise_count = 2;
}

int main(int argc, char *argv[])
{
	int opt;
	err_type ret;
	char *data_file = NULL;
	bool data_input = FALSE;
	step_mode mode = M_NONE;
		
	log_mode(LOG_TEST);
	log_level(LOG_DBG);
	memset(&g_unit, 0x0, sizeof(step_unit));
	memset(&g_monitor, 0x0, sizeof(step_monitor));

	step_init(&g_monitor);

	log_debug("step counter start");

	
	static const struct option long_opts[] = {
        {"file",	required_argument,	NULL,   'f'},
        {"input",  	no_argument,  		NULL,   'i'},
        {NULL, 0, NULL, 0}
    };

	while ((opt = getopt_long(argc, argv, "f:i?h", long_opts, NULL)) != -1)
    {
        switch (opt)
        {
            case 'f':
                data_file = optarg;
                break;
                
            case 'i':
                data_input = TRUE;
                break;

            case '?':
            case 'h':
            default:
                usage(AWOKE_EXIT_SUCCESS);
        }
    }

	log_debug("data_file:%s", data_file);

	if (data_file != NULL) {
		mode = M_FILE;
	} else if (data_input) {
		mode = M_INPUT;
	} else {
		mode = M_FILE;
		data_file = DEFAULT_FILE;
	}

	if (mode == M_FILE) {
		ret = step_file_open(data_file);
		if (ret != et_ok) {
			log_err("file %s open error", data_file);
			EXIT(AWOKE_EXIT_FAILURE);
		}
	}

	while (1) {
		vector_tm tv;
		ret = get_step_data(&tv, mode);
		if (ret != et_ok) {
			log_err("file end");
			log_debug("peak_count %d", g_monitor.peak_count);
			break;
		}
		step_detect(&tv);
		usleep(200);
	}

	return 0;
}
