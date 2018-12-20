#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
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


#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_memory.h"
#include "awoke_list.h"
#include "awoke_macros.h"
#include "awoke_log.h"
#include "awoke_vector.h"
#include "awoke_string.h"

#include "quaternion.h"


static inline err_type _qua_up_ary(quaternion *q)
{
	q->val[0] = q->r;
	q->val[1] = q->i;
	q->val[2] = q->j;
	q->val[3] = q->k;
	return et_ok;
}

static inline err_type _qua_up_cpx(quaternion *q)
{
	q->r = q->val[0];
	q->i = q->val[1];
	q->j = q->val[2];
	q->k = q->val[3];
	return et_ok;
}

err_type _qua_update(quaternion *q, uint16_t flag)
{
	if (flag == QUA_UP_F_C) {
		return _qua_up_cpx(q);
	} else if (flag == QUA_UP_F_A) {
		return _qua_up_ary(q);
	}

	return et_param;
}

err_type quaternion_init_complex(quaternion *q, double r, double i, double j, double k)
{
	q->r = r;
	q->i = i;
	q->j = j;
	q->k = k;
	return _qua_update(q, QUA_UP_F_A);
}

err_type quaternion_init_array(quaternion *q, double *array)
{
	q->val[0] = array[0];
	q->val[1] = array[1];
	q->val[2] = array[2];
	q->val[3] = array[3];
	return _qua_update(q, QUA_UP_F_C);
}

err_type quaternion_init_vector(quaternion *q, vector *v)
{
	q->r = 0;
	q->i = v->x;
	q->j = v->y;
	q->k = v->z;
	return _qua_update(q, QUA_UP_F_A);
}

double inline quaternion_mod(quaternion *q)
{
	double mod;
	
	mod = sqrt(pow(q->r,2) + pow(q->i,2) + pow(q->j,2) + pow(q->k,2));

	return mod;
}

err_type quaternion_normalize(quaternion *q, quaternion *qn)
{
	double mod;

	mod = quaternion_mod(q);

	qn->r = q->r/mod;
	qn->i = q->i/mod;
	qn->j = q->j/mod;
	qn->k = q->k/mod;
	_qua_update(q, QUA_UP_F_A);
	return et_ok;
}

static inline err_type mat_transpose(double arr_src[][4], double arr_tsp[][3])
{
	int i, j;

	for (i=0; i<3; i++) {
		for (j=0; j<4; j++) {
			arr_tsp[j][i] = arr_src[i][j];
		}
	}

	return et_ok;
}

static inline err_type mat_dot(double j[][3], double *d, double *g)
{
	g[0] = j[0][0]*d[0] + j[0][1]*d[1] + j[0][2]*d[2]; 
	g[1] = j[1][0]*d[0] + j[1][1]*d[1] + j[1][2]*d[2]; 
	g[2] = j[2][0]*d[0] + j[2][1]*d[1] + j[2][2]*d[2]; 
	g[3] = j[3][0]*d[0] + j[3][1]*d[1] + j[3][2]*d[2]; 
	return et_ok;
}

static inline bool qua_sgd_need_over(quaternion *q, double accuracy)
{
	int i;
	double delta = 0.0;
		
	for (i=0; i<4; i++) {
		delta += q->val[i];
	} 

	if (delta < 0)
		delta = -delta;
	
	if (delta < accuracy)
		return TRUE;
	else
		return FALSE;
}


static err_type qua_sgd_jacob(quaternion *q_r, quaternion *q_n, quaternion *q_g)
{
	double delta_f[3] = {0x0};
	double gradt_f[4] = {0.0};
	double jacob_f[3][4] = {0x0};
	double jacob_f_transpose[4][3];

	delta_f[0] = 2 * (0.5 - pow(q_r->j, 2) - pow(q_r->k, 2)) - q_n->i;
	delta_f[1] = 2 * (q_r->i*q_r->j - q_r->r*q_r->k) - q_n->j;
	delta_f[2] = 2 * (q_r->r*q_r->j + q_r->i*q_r->k) - q_n->k;

	jacob_f[0][0] = 0;
    jacob_f[0][1] = 0;
    jacob_f[0][2] = -4*q_r->j;
    jacob_f[0][3] = -4*q_r->k;
    jacob_f[1][0] = -2*q_r->k;
    jacob_f[1][1] =  2*q_r->j;
    jacob_f[1][2] =  2*q_r->i;
    jacob_f[1][3] = -2*q_r->r;
    jacob_f[2][0] =  2*q_r->j;
    jacob_f[2][1] =  2*q_r->k;
    jacob_f[2][2] =  2*q_r->r;
    jacob_f[2][3] =  2*q_r->i;
	
	mat_transpose(jacob_f, jacob_f_transpose);
	
	mat_dot(jacob_f_transpose, delta_f, gradt_f);

	quaternion_init_array(q_g, gradt_f);

	return et_ok;
}

static err_type qua_sgd_update(quaternion *q_r, quaternion *q_g, double eta)
{
	int i;

	for (i=0; i<4; i++) {
		q_r->val[i] = q_r->val[i] - q_g->val[i]*eta;
	}
	_qua_update(q_r, QUA_UP_F_C);
	return et_ok;
}

/** stochastic gradient desce to calculate the angle 
*   between sensor and vehicle coordinate system
*
* @param epochs		sgd iteration times.
* @param eta		sgd iterative step size.
* @param accuracy   sgd iterative, use to judge when to over iteration
* @param q_r		the rotate quaternion to be calculate
* @param q_s		sensor quaternion
*/
err_type quaternion_sgd(int epochs, double eta, double accuracy,
							  quaternion *q_r, quaternion *q_s)
{
	int i;
	quaternion q_g;
	quaternion q_n;
	quaternion_normalize(q_s, &q_n);
	
	for (i=0; i<epochs; i++) {
		qua_sgd_jacob(q_r, &q_n, &q_g);
		if (qua_sgd_need_over(&q_g, accuracy)) {
			log_debug("sgd approach");
			break;
		}
		qua_sgd_update(q_r, &q_g, eta);
	}
	
	return et_ok;
}

/** quaternion rotate
*
* @param q_ro	rotate quaternion.
* @param q_sr	the quaternion to be rotate.
* @param q_re   rotate result
*/
err_type quaternion_rotate(quaternion *q_ro, quaternion *q_sr, quaternion *q_re)
{
	double mat_re[4] = {0x0};
  	double mat_ro[3][3] = {0x0};
  
  	mat_ro[0][0] = 1 - 2*pow(q_ro->j,2) - 2*pow(q_ro->k,2);
  	mat_ro[0][1] = 2*q_ro->i*q_ro->j - 2*q_ro->r*q_ro->k;
  	mat_ro[0][2] = 2*q_ro->r*q_ro->j + 2*q_ro->i*q_ro->k;
  	mat_ro[1][0] = 2*q_ro->i*q_ro->j + 2*q_ro->r*q_ro->k;
  	mat_ro[1][1] = 1 - 2*pow(q_ro->i,2) - 2*pow(q_ro->k,2);
  	mat_ro[1][2] = 2*q_ro->j*q_ro->k - 2*q_ro->r*q_ro->i;
  	mat_ro[2][0] = 2*q_ro->i*q_ro->k - 2*q_ro->r*q_ro->j;
  	mat_ro[2][1] = 2*q_ro->r*q_ro->i + 2*q_ro->j*q_ro->k;
  	mat_ro[2][2] = 1 - 2*pow(q_ro->i,2) - 2*pow(q_ro->j,2);
  
  	mat_re[1] = mat_ro[0][0]*q_sr->i + mat_ro[0][1]*q_sr->j + mat_ro[0][2]*q_sr->k;
  	mat_re[2] = mat_ro[1][0]*q_sr->i + mat_ro[1][1]*q_sr->j + mat_ro[1][2]*q_sr->k;
  	mat_re[3] = mat_ro[2][0]*q_sr->i + mat_ro[2][1]*q_sr->j + mat_ro[2][2]*q_sr->k;
  
  	quaternion_init_array(q_re, mat_re);
  	return et_ok;
}

/** quaternion's conjugate
 *
 * @param q 	source quaternion.
 * @param qc 	the conjugate to be caculate.
 */
err_type quaternion_conjugate(quaternion *q, quaternion *qc)
{
	qc->r = q->r;
	qc->i = -q->i;
	qc->j = -q->j;
	qc->k = -q->k;
	_qua_update(qc, QUA_UP_F_A);
	return et_ok;
}



void config_load()
{	
	FILE *fp;
	char *path = "config.dat";

	log_level(LOG_DBG);
	log_mode(LOG_TEST);

	fp = fopen(path, "r");
	if (!fp) {
		log_err("open %s error", path);
		fopen(path, "a+");
		return et_file_open;
	}

	log_debug("open %s ok", path);

	while (!feof(fp)) {
		char key[32];
		char _k[128];
		double x, y, z;
		char *_v1;
		char *_v2;
		char *_v3;
		char line[1024];

		fgets(line, 1024, fp);
		log_debug("\n\nline %s", line);
		
		if (line[0] != '[')
			continue;

		sscanf(line, "[%[^]]", key);
		log_debug("key %s", key);	

		sscanf(line, "%*[^:]:%s", _k);
		//log_debug("_k %s", _k);
		
		if (!strcmp(key, "hz-adjust")) {
			vector val;
			vector *v = &val;
			sscanf(_k, "%lf,%lf,%lf", &v->x, &v->y, &v->z);
			log_debug("val %lf %lf %lf", vector_print(val));
		} else if (!strcmp(key, "vh-rotate")) {
			double _val[4];
			quaternion val;
			sscanf(_k, "%lf,%lf,%lf,%lf", &_val[0], &_val[1], &_val[2], &_val[3]);
			quaternion_init_array(&val, _val);
			log_debug("val %lf %lf %lf", print_qua_c(val));
		}
	}
}

int main(int argc, char **argv)
{
	
	quaternion q_rotate;
	quaternion q_rotate_conj;
	quaternion q_direct; 
	quaternion q_sensor;
	quaternion q_sensor_nmz;
	quaternion q_s2v, q_v2s, q_s2r;
	err_type ret = et_ok;

	log_level(LOG_DBG);
	log_mode(LOG_TEST);

	double q_r_init[4] = {0.5, 0.5, 0.5, 0.5};
	double q_d_init[4] = {0.0, 1.0, 0.0, 0.0};
	double q_s_init[4] = {0.0, -2342326.23, 3223455, 7634};

	quaternion_init_array(&q_rotate, q_r_init);
	quaternion_init_array(&q_direct, q_d_init);
	quaternion_init_array(&q_sensor, q_s_init);

	quaternion_normalize(&q_sensor, &q_sensor_nmz);
	ret = quaternion_sgd(5000, 0.02, 0.0001, &q_rotate, &q_sensor);
	log_debug("q_sensor");
	log_debug(QUA_FORMAT_C, print_qua_c(q_sensor_nmz));
	log_debug("q_rotate");
	log_debug(QUA_FORMAT_C, print_qua_c(q_rotate));
	quaternion_conjugate(&q_rotate, &q_rotate_conj);
	log_debug("q_rotate_conj");
	log_debug(QUA_FORMAT_C, print_qua_c(q_rotate_conj));

	quaternion_rotate(&q_rotate, &q_sensor_nmz, &q_s2v);
	quaternion_rotate(&q_rotate_conj, &q_direct, &q_v2s);
	log_debug("sensor to vehicle");
	log_debug(QUA_FORMAT_C, print_qua_c(q_s2v));
	log_debug("vehicle to sensor");
	log_debug(QUA_FORMAT_C, print_qua_c(q_v2s));

	quaternion_rotate(&q_rotate, &q_sensor, &q_s2r);
	log_debug("sensor rotate");
	log_debug(QUA_FORMAT_C, print_qua_c(q_s2r));

	//config_load();
	return 0;
	
}
