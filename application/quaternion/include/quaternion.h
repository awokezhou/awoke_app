#ifndef __QUATERNION_H__
#define __QUATERNION_H__


#define QUA_FORMAT_A 			"%lf %lf %lf %lf"
#define QUA_FORMAT_C 			"%lf + %lfi + %lfj + %lfk"

#define print_qua_a(q) 			q.val[0], q.val[1], q.val[2], q.val[3]
#define print_qua_ap(q) 		q->val[0], q->val[1], q->val[2], q->val[3]
#define print_qua_c(q) 			q.r, q.i, q.j, q.k
#define print_qua_cp(q)			q->r, q->i, q->j, q->k

#define QUA_UP_F_C				0x0001
#define QUA_UP_F_A				0x0002

typedef struct _qua{
	double r;	/* real part */
	double i;	
	double j;	
	double k;	
	double val[4];	/* array format */
} quaternion;


err_type quaternion_init_array(quaternion *, double *);
err_type quaternion_init_complex(quaternion *, double, double, double, double);
err_type quaternion_init_vector(quaternion *, vector *);
double inline quaternion_mod(quaternion *);
err_type quaternion_normalize(quaternion *, quaternion *);
err_type quaternion_sgd(int, double, double, quaternion *, quaternion *);
err_type quaternion_rotate(quaternion *, quaternion *, quaternion *);
err_type quaternion_conjugate(quaternion *, quaternion *);

#endif /* __QUATERNION_H__ */