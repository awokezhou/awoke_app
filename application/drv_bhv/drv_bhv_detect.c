

#include "drv_bhv_core.h"
#include "drv_bhv_calculate.h"
#include "drv_bhv_detect.h"

err_type bhv_detect(bhv_manager *manager)
{
	bhv_calc *calc;
	bhv_dtct *dtct;

	list_for_each_entry(calc, &manager->calc_list, _head) {
		list_for_each_entry(dtct, &manager->dtct_list, _head) {
			if (calc->dtct_flag & dtct->mask) {
				log_err("%s will run......", dtct->name);
				dtct->dtct_handler(calc, dtct, manager);
			}
		}
	}

	return et_ok;
}

err_type bhv_detect_register(const char *name, uint32_t mask,
		err_type (*handler) (struct _bhv_calc *, struct _bhv_dtct *, struct _bhv_manager *),
		bhv_manager *manager)
{
	bhv_dtct *dtct;
	bhv_dtct *new_dtct;

	list_for_each_entry(dtct, &manager->dtct_list, _head) {
		if (!strcmp(dtct->name, name))
			return et_exist;
	}

	new_dtct = mem_alloc_z(sizeof(bhv_dtct));
	if (!new_dtct)
		return et_nomem;

	new_dtct->mask = mask;
	new_dtct->name = awoke_string_dup(name);
	new_dtct->dtct_handler = handler;
	list_append(&new_dtct->_head, &manager->dtct_list);
	return et_ok;
}

err_type detect_crash(bhv_calc *calc, bhv_dtct *dtct, bhv_manager *manager)
{
	bhv_detection *detection = &manager->detection;
	
	if (calc->mask == CALC_HEAD_ACC) {
		calc_head_acc *ha = calc_to_calc_head_acc(calc);
		if (ha->mod > 1400) {
			detection->if_crash = TRUE;
			log_info("%s crash", dtct->name);
		}
	} else if (calc->mask == CALC_GPS_ACC) {
		/* gps acc */
	}

	return et_ok;
}

err_type detect_rapid_acc(bhv_calc *calc, bhv_dtct *dtct, bhv_manager *manager)
{
	bhv_detection *detection = &manager->detection;

	if (calc->mask == CALC_HEAD_ACC) {
		calc_head_acc *ha = calc_to_calc_head_acc(calc);
		if (ha->mod > 250) {
			detection->if_rp_acc = TRUE;
			log_info("%s rapid acc", dtct->name);
		} else if (calc->mask == CALC_GPS_ACC) {
			/* gps acc */
		}
	}

	return et_ok;
}

err_type detect_rapid_dec(bhv_calc *calc, bhv_dtct *dtct, bhv_manager *manager)
{
	bhv_detection *detection = &manager->detection;

	if (calc->mask == CALC_HEAD_ACC) {
		calc_head_acc *ha = calc_to_calc_head_acc(calc);
		if (ha->mod < -350) {
			detection->if_rp_dec = TRUE;
			log_info("%s rapid dec", dtct->name);
		} else if (calc->mask == CALC_GPS_ACC) {
			/* gps acc */
		}
	}

	return et_ok;	
}

err_type detect_brakes(bhv_calc *calc, bhv_dtct *dtct, bhv_manager *manager)
{
	bhv_detection *detection = &manager->detection;

	if (calc->mask == CALC_HEAD_ACC) {
		calc_head_acc *ha = calc_to_calc_head_acc(calc);
		if (ha->mod < -500) {
			detection->if_brakes = TRUE;
			log_info("%s brakes", dtct->name);
		} else if (calc->mask == CALC_GPS_ACC) {
			/* gps acc */
		}
	}

	return et_ok;		
}

err_type detect_sudden_turn(bhv_calc *calc, bhv_dtct *dtct, bhv_manager *manager)
{
	bhv_detection *detection = &manager->detection;

	if (calc->mask == CALC_GYR_CORNER) {
		calc_gyr_corner *gc = calc_to_calc_gyr_corner(calc);
		if (gc->corner > 60) {
			detection->if_sudden_turn = TRUE;
			log_info("%s sudden turn", dtct->name);
		}
	} else if (calc->mask == CALC_GPS_CORNER) {
		/* gps corner */
	}

	return et_ok;
}

err_type detect_roll_over(bhv_calc *calc, bhv_dtct *dtct, bhv_manager *manager)
{
	bhv_detection *detection = &manager->detection;

	if (calc->mask == CALC_ACC_ANGLE) {
		calc_acc_angle *aa = calc_to_calc_acc_angle(calc);
		if ((aa->vec_cmp_angle.y >= 46) || (aa->vec_cmp_angle.y <= -46)) {
			detection->if_roll_over = TRUE;
			log_info("%s roll over", dtct->name);
		} 
	}

	return et_ok;
}

err_type bhv_detect_init(bhv_manager *manager)
{
	bhv_detect_register("[detect-crash]", DTCT_CRASH, 
		detect_crash, manager);
	bhv_detect_register("[detect-rapid-acc]", DTCT_RP_ACC, 
		detect_crash, manager);
	bhv_detect_register("[detect-rapid-dec]", DTCT_RP_DEC, 
		detect_crash, manager);
	bhv_detect_register("[detect-brakes]", DTCT_BRAKE, 
		detect_crash, manager);
	bhv_detect_register("[detect-sudden-turn]", DTCT_SUDDEN_TURN, 
		detect_crash, manager);
	bhv_detect_register("[detect-roll-over]", DTCT_ROLL_OVER, 
		detect_crash, manager);
	return et_ok;
}
