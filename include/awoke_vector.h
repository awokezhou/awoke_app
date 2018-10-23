#ifndef __AWOKE_VECTOR_H__
#define __AWOKE_VECTOR_H__

#include <math.h>
#include "awoke_list.h"
#include "awoke_log.h"


#define VEC_ROLLBACK_H	0x01
#define VEC_ROLLBACK_E	0X02

#define PAI 3.14

#define vector_print(vec) \
		vec.x, vec.y, vec.z

typedef struct _vector {
	double x;
	double y;
	double z;
	struct _awoke_list _head;
}vector;

typedef struct _vector_tm {
	vector vec;
	time_t tm;
	struct _awoke_list _head;
}vector_tm;


static inline void vector_init(vector *pvec)
{
	pvec->x = 0.0;
	pvec->y = 0.0;
	pvec->z = 0.0;
}

static inline void vector_copy(vector *vd, vector *vs)
{
	vd->x = vs->x;
	vd->y = vs->y;
	vd->z = vs->z;
}

static inline vector *vector_clone(vector **vn, vector *vs)
{
	vector *p;
	p = mem_alloc_z(sizeof(vector));
	vector_copy(p, vs);
	*vn = p;
	return p;
}

static inline vector *vector_sum(vector *vs, awoke_list *list)
{
	vector *vec;

	list_for_each_entry(vec, list, _head) {
		vs->x += vec->x;
		vs->y += vec->y;
		vs->z += vec->z;
	}

	return vs;
}

static inline vector *vector_add_up(vector *vs, vector *vec)
{
	vs->x += vec->x;
	vs->y += vec->y;
	vs->z += vec->z;
	return vs;
}

static inline vector *vector_subtract(vector *vm, vector *vs, vector *vr)
{
	vr->x = vm->x - vs->x;
	vr->y = vm->y - vs->y;
	vr->z = vm->z - vs->z;
	
	return vr;
}

static inline double vector_mod(vector *vec)
{
	return (sqrt(pow(vec->x, 2) + pow(vec->y, 2) + pow(vec->z, 2)));
}

static inline vector *vector_unit(vector *vu, vector *vec)
{
	double mod;

	mod = vector_mod(vec);
	vu->x = vec->x/mod;
	vu->y = vec->y/mod;
	vu->z = vec->z/mod;
	return vu;
}

static inline double vector_projection_mod(vector *v1, vector *v2)
{
	return (v1->x*v2->x + v1->y*v2->y + v1->z*v2->z);
}

static inline vector_tm *vector_tm_copy_from_vec(vector_tm *vd, vector *vs)
{
	vd->tm = time(NULL);
	vd->vec.x = vs->x;
	vd->vec.y = vs->y;
	vd->vec.z = vs->z;
	return vd;
}

static inline vector_tm *vector_tm_clone_from_vec(vector_tm **vn, vector *vs)
{
	vector_tm *p;
	p = mem_alloc_z(sizeof(vector_tm));
	p->tm = time(NULL);
	vector_copy(&p->vec, vs);
	*vn = p;
	return p;
}

static inline uint64_t vector_tm_interval(awoke_list *list, int idx_s, int idx_e)
{
	int i = 0;
	vector_tm *vec, *vs, *ve;
	uint64_t intv;

	if (list_empty(list))
		return 0;

	list_for_each_entry(vec, list, _head) {
		i++;		
		if (i == idx_s)
			vs = vec;
		if (i == idx_e)
			ve = vec;
	}
	
	intv = ve->tm - vs->tm;
	return intv;
}

static inline void vector_tm_rollback(awoke_list *list, int flag)
{
	int i = 0;
	vector_tm *vec, *tmp, *vh, *ve, *vd;

	if (list_empty(list))
		return;
	
	list_for_each_entry_safe(vec, tmp, list, _head) {
		i++;
		if (i == 1)
			vh = vec;
	}
	ve = vec;
	if (flag == VEC_ROLLBACK_H)
		vd = vh;
	else
		vd = ve;
	list_unlink(&vd->_head);
	mem_free(vd);
}

static inline vector *vector_euler_angle(vector *v_angle, vector *vec)
{
	v_angle->x = atan2(vec->x, sqrt(pow(vec->y, 2) + pow(vec->z, 2)))*180/PAI;
	v_angle->y = atan2(vec->y, sqrt(pow(vec->x, 2) + pow(vec->z, 2)))*180/PAI;
	v_angle->z = atan2(vec->z, sqrt(pow(vec->x, 2) + pow(vec->y, 2)))*180/PAI;
	return v_angle;
}

static inline vector *vector_comp_filter(vector *vc, vector *va, vector *vg, double weight)
{
	vc->x = weight*vg->x + (1-weight)*va->x;
	vc->y = weight*vg->y + (1-weight)*va->y;
	vc->z = weight*vg->z + (1-weight)*va->z;
	return vc;
}


#endif /* __AWOKE_VECTOR_H__ */
