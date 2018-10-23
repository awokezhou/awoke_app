#ifndef __DRV_BHV_CALCULATE_H__
#define __DRV_BHV_CALCULATE_H__


#include "drv_bhv_core.h"


typedef struct _bhv_calc {
	char *name;
	uint32_t mask;
#define CALC_NULL			0x0000
#define CALC_HEAD_ACC		0x0001
#define CALC_GYR_CORNER		0x0002
#define CALC_ACC_ANGLE		0x0004
#define CALC_GYR_ANGLE		0x0008
#define CALC_GPS_ACC		0x0010
#define CALC_GPS_CORNER		0x0020
	uint32_t dtct_flag;
	err_type (*calc_handler) (void *, struct _bhv_calc *, struct _bhv_manager *);
	awoke_list _head;
}bhv_calc;


typedef struct _calc_gps_acc {
	bhv_calc calc;
	err_type (*private_init) (struct _calc_gps_acc *);
}calc_gps_acc;

typedef struct _calc_gps_corner {
	bhv_calc calc;
	err_type (*private_init) (struct _calc_gps_corner *);
}calc_gps_corner;

typedef struct _calc_gyr_angle {
	bhv_calc calc;
	int vec_gyr_num;
	vector vec_gyr_angle;
	awoke_list vec_gyr_list;
	err_type (*private_init) (struct _calc_gyr_angle *);
}calc_gyr_angle;

typedef struct _calc_acc_angle {
	bhv_calc calc;
	vector vec_acc_angle;
	vector vec_cmp_angle;
	err_type (*private_init) (struct _calc_acc_angle *);
}calc_acc_angle;

typedef struct _calc_gyr_corner {
	bhv_calc calc;
	double corner;
	int vec_gyr_num;
	awoke_list vec_gyr_list;
	err_type (*private_init) (struct _calc_gyr_corner *);
}calc_gyr_corner;

typedef struct _calc_head_acc {
	bhv_calc calc;

	vector vec_acc;
	vector vec_gravitv;
	vector vec_vehicle;
	vector vec_vehicle_sum;
	vector vec_direction;

	int vec_gravitv_num;
	awoke_list vec_gravitv_list;
	awoke_list vec_vehicle_list;

	double mod;
	bool if_gravity_calculated;
	int gravity_calc_count;

	err_type (*private_init) (struct _calc_head_acc *);
}calc_head_acc;

#define calc_to_calc_head_acc(p) container_of(p, calc_head_acc, calc)
#define calc_to_calc_gyr_corner(p) container_of(p, calc_gyr_corner, calc)
#define calc_to_calc_acc_angle(p) container_of(p, calc_acc_angle, calc)
#define calc_to_calc_gyr_angle(p) container_of(p, calc_gyr_angle, calc)
#define calc_to_calc_gps_corner(p) container_of(p, calc_gps_corner, calc)
#define calc_to_calc_gps_acc(p) container_of(p, calc_gps_acc, calc)

#define calculate_register(calc, father_obj, manager) do {\
		father_obj *p = calc_to_##father_obj(calc);\
		if (p->private_init != NULL)\ 
			p->private_init(p);\
		_calculate_register(calc, manager);\
	} while(0)





err_type bhv_calculate_init(struct _bhv_manager *manager);

#endif /* __DRV_BHV_CALCULATE_H__ */
