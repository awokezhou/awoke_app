#include <time.h>

#include "drv_bhv_core.h"
#include "drv_bhv_calculate.h"
#include "drv_bhv_detect.h"

err_type calc_gyr_angle_init(calc_gyr_angle *ga)
{
	ga->vec_gyr_num = 0;
	list_init(&ga->vec_gyr_list);
	vector_init(&ga->vec_gyr_angle);
	return et_ok;
}

err_type calc_acc_angle_init(calc_acc_angle *aa)
{
	vector_init(&aa->vec_acc_angle);
	vector_init(&aa->vec_cmp_angle);
	return et_ok;
}

err_type calc_head_acc_init(calc_head_acc *ha)
{
	ha->mod = 0;
	ha->vec_gravitv_num = 0;
	vector_init(&ha->vec_acc);
	vector_init(&ha->vec_gravitv);
	vector_init(&ha->vec_vehicle);
	vector_init(&ha->vec_vehicle_sum);
	vector_init(&ha->vec_direction);
	list_init(&ha->vec_gravitv_list);
	list_init(&ha->vec_vehicle_list);
	return et_ok;
}

err_type calc_gyr_corner_init(calc_gyr_corner *gc)
{
	gc->corner = 0.0;
	gc->vec_gyr_num = 0;
	list_init(&gc->vec_gyr_list);
	return et_ok;
}

err_type head_acc_gravity(vector *vs, calc_head_acc *ha)
{
	vector *vn;

	vector_clone(&vn, vs);
	list_append(&vn->_head, &ha->vec_gravitv_list);
	ha->vec_gravitv_num++;

	log_debug("gravity calculate... %d", ha->vec_gravitv_num);

	if (ha->vec_gravitv_num >= ha->gravity_calc_count) {
		vector sum;
		vector_init(&sum);
		vector_sum(&sum, &ha->vec_gravitv_list);
		sum.x = sum.x/ha->vec_gravitv_num;
		sum.y = sum.y/ha->vec_gravitv_num;
		sum.z = sum.z/ha->vec_gravitv_num;
		vector_copy(&ha->vec_gravitv, &sum);
		ha->if_gravity_calculated = TRUE;
		log_info("gravity calculated, vec_gravitv(%lf, %lf, %lf)", 
			vector_print(ha->vec_gravitv));
	}
	return et_ok;
}

err_type head_acc_direction(vector *vs, calc_head_acc *ha)
{
	vector *vg = &ha->vec_gravitv;
	vector *vv = &ha->vec_vehicle;
	vector *v_sum = &ha->vec_vehicle_sum;
	vector *vd = &ha->vec_direction;
	
	vector_subtract(vs, vg, vv);
	vector_add_up(v_sum, vv);
	vector_unit(vd, v_sum);
	ha->mod = vector_projection_mod(vd, vv);
	log_info("vehicle(%lf, %lf, %lf)", vector_print(ha->vec_vehicle));
	log_info("vehicle sum(%lf, %lf, %lf)", vector_print(ha->vec_vehicle_sum));
	log_info("direction(%lf, %lf, %lf)", vector_print(ha->vec_direction));
	log_info("mod %lf", ha->mod);
	return et_ok;
}

err_type head_acc_calculate(void *data, bhv_calc *calc, bhv_manager *manager)
{
	vector vec_sensor;
	calc_head_acc *ha = calc_to_calc_head_acc(calc);

	vector_copy(&vec_sensor, (vector *)data);

	if (!ha->if_gravity_calculated) {
		return head_acc_gravity(&vec_sensor, ha);
	} else {
		return head_acc_direction(&vec_sensor, ha);
	}
}

err_type gyr_corner_calculate(void *data, bhv_calc *calc, bhv_manager *manager)
{		
	double intv;
	uint64_t interval;
	double angle_z = 0;
	vector_tm *vs, *vec;
	calc_gyr_corner *gc = calc_to_calc_gps_corner(calc);

	vector_tm_clone_from_vec(&vs, (vector *)data);
	list_prepend(&vs->_head, &gc->vec_gyr_list);
	gc->vec_gyr_num++;

	interval = vector_tm_interval(&gc->vec_gyr_list, 1, gc->vec_gyr_num);
	if (interval > 4) {
		vector_tm_rollback(&gc->vec_gyr_list, VEC_ROLLBACK_H);
		gc->vec_gyr_num--;
	}

	intv = 4000/gc->vec_gyr_num;

	list_for_each_entry(vec, &gc->vec_gyr_list, _head) {
		angle_z += vec->vec.z*intv/1000;
	}
	gc->corner = angle_z*180/PAI;

	log_info("corner %lf", gc->corner);
	
	return et_ok;
}

err_type gyr_angle_calculate(void *data, bhv_calc *calc, bhv_manager *manager)
{	
	double intv;
	vector_tm *vs, *vec;
	uint64_t interval;
	
	calc_gyr_angle *ga = calc_to_calc_gyr_angle(calc);
	vector *v_angle = &ga->vec_gyr_angle;

	vector_tm_clone_from_vec(&vs, (vector *)data);
	list_prepend(&vs->_head, &ga->vec_gyr_list);
	ga->vec_gyr_num++;

	interval = vector_tm_interval(&ga->vec_gyr_list, 1, ga->vec_gyr_num);
	if (interval > 4) {
		vector_tm_rollback(&ga->vec_gyr_list, VEC_ROLLBACK_H);
		ga->vec_gyr_num--;
	}

	intv = 4000/ga->vec_gyr_num;

	list_for_each_entry(vec, &ga->vec_gyr_list, _head) {
		v_angle->x += vec->vec.x*intv/1000;
		v_angle->y += vec->vec.y*intv/1000;
		v_angle->z += vec->vec.z*intv/1000;
	}

	v_angle->x = v_angle->x*180/(PAI*1000000);
	v_angle->y = v_angle->y*180/(PAI*1000000);
	v_angle->z = v_angle->z*180/(PAI*1000000);

	return et_ok;
}

err_type acc_angle_calculate(void *data, bhv_calc *calc, bhv_manager *manager)
{
	vector vs;
	bhv_calc *calc_gangle;
	calc_acc_angle *aa = calc_to_calc_acc_angle(calc);
	vector *v_acc_angle = &aa->vec_acc_angle;
	vector *v_cmp_angle = &aa->vec_cmp_angle;

	vector_copy(&vs, (vector *)data);
	vector_euler_angle(v_acc_angle, &vs);

	list_for_each_entry(calc_gangle, &manager->calc_list, _head) {
		if (calc_gangle->mask == CALC_GYR_ANGLE)
			break;
	}
	calc_gyr_angle *ga = calc_to_calc_gyr_angle(calc_gangle);
	vector *v_gyr_angle = &ga->vec_gyr_angle;

	vector_comp_filter(v_cmp_angle, v_acc_angle, v_gyr_angle, 0.01);
	
	return et_ok;
}

err_type _calculate_register(bhv_calc *new_calc, bhv_manager *manager)
{
	bhv_calc *calc;

	list_for_each_entry(calc, &manager->calc_list, _head) {
		if (!strcmp(calc->name, new_calc->name)) {
			return et_exist;
		}
	}

	list_append(&new_calc->_head, &manager->calc_list);
	return et_ok;
}

struct _calc_gps_acc gacc_calc = {
	.calc = {
		.name = "[calc-gps-acc]",
		.mask = CALC_GPS_ACC,	
		.dtct_flag = DTCT_CRASH|DTCT_BRAKE|DTCT_RP_ACC|DTCT_RP_DEC,
	},
	.private_init = NULL,
};

struct _calc_gps_corner gpsconr_calc = {
	.calc = {
		.name = "[calc-gps-corner]",
		.mask = CALC_GPS_CORNER,
		.dtct_flag = DTCT_SUDDEN_TURN,
	},
	.private_init = NULL,
};

struct _calc_gyr_angle gangle_calc = {
	.calc = {
		.name = "[calc-gyr-angle]",
		.mask = CALC_GYR_ANGLE,
		.dtct_flag = DTCT_NULL,
		.calc_handler = gyr_angle_calculate,
	},
	.private_init = calc_gyr_angle_init,
};

struct _calc_acc_angle aangle_calc = {
	.calc = {
		.name = "[calc-acc-angle]",
		.mask = CALC_ACC_ANGLE,
		.dtct_flag = DTCT_ROLL_OVER,
		.calc_handler = acc_angle_calculate,
	},
	.private_init = calc_acc_angle_init,
};

struct _calc_gyr_corner gyrconr_calc = {
	.calc = {
		.name = "[calc-gyr-corner]",
		.mask = CALC_GYR_CORNER,
		.dtct_flag = DTCT_SUDDEN_TURN,
		.calc_handler = gyr_corner_calculate,
	},
	.private_init = calc_gyr_corner_init,
};

struct _calc_head_acc hacc_calc = {
	.if_gravity_calculated = FALSE,
	.gravity_calc_count = 10,
	.calc = {
		.name = "[calc-head-acc]",
		.mask = CALC_HEAD_ACC,
		.dtct_flag = DTCT_CRASH|DTCT_BRAKE|DTCT_RP_ACC|DTCT_RP_DEC,
		.calc_handler = head_acc_calculate,
	},
	.private_init = calc_head_acc_init,
};


err_type bhv_calculate_init(bhv_manager *manager)
{
	/* thread sync, lock init */

	calculate_register(&hacc_calc.calc, calc_head_acc, manager);
	
	calculate_register(&gyrconr_calc.calc, calc_gyr_corner, manager);

	calculate_register(&aangle_calc.calc, calc_acc_angle, manager);

	calculate_register(&gangle_calc.calc, calc_gyr_angle, manager);

	calculate_register(&gacc_calc.calc, calc_gps_acc, manager);

	calculate_register(&gpsconr_calc.calc, calc_gps_corner, manager);
	
	return et_ok;
}


