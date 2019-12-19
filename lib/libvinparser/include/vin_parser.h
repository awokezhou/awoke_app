
#ifndef __VIN_PARSER_H__
#define __VIN_PARSER_H__



#include <time.h> 

#include "awoke_log.h"
#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_memory.h"

#define VIN_LEN			17
#define VIN_WMI_FILE	"wmi.csv"

typedef struct _vininfo_section {
	const char *code;
#define vin_unknown	0
	uint16_t id;
	const char *string;
} vininfo_section;

#define vin_unknown_section		{"?", vin_unknown, "Unknown"}

typedef struct _vinmanu2vds {
	uint16_t manuid;
	vininfo_section *tb_vds;
	int tb_vds_size;
	vininfo_section *tb_bt;
	int *tb_bt_size;
} vinmanu2vds;

typedef struct _vininfo {
	int year;
	mem_ptr_t vin;
	mem_ptr_t vds;
	mem_ptr_t vis;
	mem_ptr_t continent;
	vininfo_section country;
	vininfo_section manufac;
	vininfo_section modelin;
	vininfo_section bodytyp;
} vininfo;

typedef struct _vin_year_range {
	int min;
	int max;
} vin_year_range;

typedef struct _vin_parser {
	char *vin;
	char *pos;
	int tb_manufac_size;
	int tb_country_size;
	int map_manu2vds_size;
	vinmanu2vds *map_manu2vds;
	vininfo_section *tb_country;
	vininfo_section *tb_manufac;
} vin_parser;

typedef enum {
	vincoun_unknown = 0,
	vincoun_us,				/* United States of America */
	vincoun_ca,				/* Canada */
	vincoun_mx,				/* Mexico */
	vincoun_au,				/* Australia */
	vincoun_br,				/* Brazil */			
	vincoun_jp,				/* Japan */
	vincoun_gb,				/* United Kiongdom */
	vincoun_kr,				/* Korea */
	vincoun_ch,				/* Switzerland */
	vincoun_cn,				/* China */
	vincoun_fr,				/* France */
	vincoun_tw,				/* Taiwan */
	vincoun_de,				/* Germany */
	vincoun_se,				/* Sweden */
	vincoun_it,				/* Italy */
	vincoun_offset,
} vincoun_e;		/* vehicle country */


typedef enum {
	vinmanu_unknown = 0,			
	vinmanu_lsg,
	vinmanu_ldc,
	vinmanu_pna,
	vinmanu_offset,
} vinmanu_e;		/* Vehicle manufac */


typedef enum {
	vin_peugeot_ml_unknown = 0,
	vin_peugeot_206,
	vin_peugeot_406508,
	vin_peugeot_407,
	vin_peugeot_308,
	vin_peugeot_expert,
	vin_peugeot_208,
	vin_peugeot_301,
	vin_peugeot_307,
	vin_peugeot_partner,
	vin_peugeot_ml_offset,
} vinmode_peugeot_e;		/* Peugeot vehicle model line */

typedef enum {
	vin_peugeot_bt_unknown = 0,
	vin_peugeot_3ds,
	vin_peugeot_cab,
	vin_peugeot_5ds,
	vin_peugeot_4ds,
	vin_peugeot_est,
	vin_peugeot_e7s,
	vin_peugeot_hat,
	vin_peugeot_cou,
	vin_peugeot_3dss,
	vin_peugeot_5dss,
	vin_peugeot_3dc,
	vin_peugeot_5dc,
	vin_peugeot_bt_offset,
} vinbody_peugeot_e;		/* Peugeot vehicle body type */

typedef enum {
	vin_dpca_ml_unknown = 0,
	vin_dpca_n68,
	vin_dpca_t11,
	vin_dpca_t21,
	vin_dpca_t31,
	vin_dpca_t33,
	vin_dpca_r31,
	vin_dpca_r33,
	vin_dpca_b51,
	vin_dpca_b53,
	vin_dpca_bx3,
	vin_dpca_b73,
	vin_dpca_tx3,
	vin_dpca_t73,
	vin_dpca_t61,
	vin_dpca_t63,
	vin_dpca_t88,
	vin_dpca_g25,
	vin_dpca_x7,
	vin_dpca_w23,
	vin_dpca_m33,
	vin_dpca_m43,
	vin_dpca_a94,
	vin_dpca_m44,
	vin_dpca_t91,
	vin_dpca_t93,
	vin_dpca_ml_offset,
} vinmode_dpca_e;		/* DPCA vehicle model line */


err_type vin_parse(const char *vin, vininfo *vi);
void vininfo_dump(vininfo *vi);

#endif /* __VIN_PARSER_H__ */
