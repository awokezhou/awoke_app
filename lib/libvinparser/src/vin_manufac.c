
#include "vin_parser.h"
#include "vin_manufac.h"


/*
 * Find this table at wikibooks:Vehicle Identification Numbers (VIN codes)/Peugeot/VIN Codes
 * 		address:https://en.wikibooks.org/wiki/Vehicle_Identification_Numbers_(VIN_codes)/Peugeot/VIN_Codes
 */
static vininfo_section g_modelin_peugeot_table[] = {

	vin_unknown_section,
	{"2",	vin_peugeot_206, 		"206"},
	{"8",	vin_peugeot_406508, 	"406/508"},
	{"6",	vin_peugeot_407, 		"407"},
	{"9",	vin_peugeot_308, 		"308"},
	{"4",	vin_peugeot_308, 		"308"},
	{"B",	vin_peugeot_expert, 	"Expert"},
	{"C",	vin_peugeot_208, 		"208"},
	{"d",	vin_peugeot_301, 		"301"},
	{"3",	vin_peugeot_307, 		"307"},
	{"7",	vin_peugeot_partner, 	"Partner"},
};


/*
 * https://wenku.baidu.com/view/7e68863bc1c708a1284a44fe.html
 */
static vininfo_section g_modelin_dpca_table[] = {

	vin_unknown_section,
	{"838",	vin_dpca_n68, 	"N68-Picasso"},
	{"611",	vin_dpca_t11, 	"T11-206"},
	{"621",	vin_dpca_t21, 	"T21-C2"},
	{"631",	vin_dpca_t31, 	"T31-207 Hatchback"},
	{"613",	vin_dpca_t33, 	"T33-207 Sedan"},
	{"271",	vin_dpca_r31, 	"R31-Elysee Hatchback"},
	{"703",	vin_dpca_r33, 	"R33-Elysee Sedan"},
	{"C41",	vin_dpca_b51, 	"B51-Sega Hatchback"},
	{"C43",	vin_dpca_b53, 	"B53-Triumph"},
	{"C13",	vin_dpca_bx3, 	"BX3-Sega Sedan"},
	{"C23",	vin_dpca_b73, 	"B73-C4L"},
	{"953",	vin_dpca_tx3, 	"TX3-308"},
	{"943",	vin_dpca_t73, 	"T73-408"},
	{"931",	vin_dpca_t61, 	"T61-307 Hatchback"},
	{"913",	vin_dpca_t63, 	"T63-307 Sedan"},
	{"923",	vin_dpca_t63, 	"T63-307 Sedan"},
	{"933",	vin_dpca_t63, 	"T63-307 Sedan"},
	{"T81",	vin_dpca_t88, 	"T88-307 3008"},
	{"C33",	vin_dpca_g25, 	"G25-L60"},
	{"A13",	vin_dpca_x7, 	"X7-C5"},
	{"B13",	vin_dpca_w23, 	"W23-508"},
	{"633",	vin_dpca_m33, 	"M33-301"},
	{"643",	vin_dpca_m43, 	"M43-New Elysee"},
	{"673",	vin_dpca_a94, 	"A94-2008"},
	{"661",	vin_dpca_m44, 	"M44-C3-XR"},
	{"961",	vin_dpca_t91, 	"T91-308S"},
	{"973",	vin_dpca_t93, 	"T93-Brand new 408"},
};

/*
 * Find this table at youcanic:VEHICLE|PEUGEOT
 * 		address:https://www.youcanic.com/wiki/peugeot-vin-decoder
 */
static vininfo_section g_bodytype_peugeot_table[] = {

	vin_unknown_section,
	{"A",	vin_peugeot_3ds,	"3-door Saloon"},
	{"B",	vin_peugeot_cab, 	"Cabriolet"},
	{"C",	vin_peugeot_5ds,	"5-door Saloon"},
	{"D",	vin_peugeot_4ds,	"4-door Saloon"},
	{"E",	vin_peugeot_est,	"Estate"},
	{"F",	vin_peugeot_e7s, 	"Estate 7-seater"},
	{"G",	vin_peugeot_hat,	"Hatchback"},
	{"J",	vin_peugeot_cou,	"Coupe"},
	{"K",	vin_peugeot_3dss,	"3-door Sport Saloon"},
	{"L",	vin_peugeot_5dss,	"5-door Sport Saloon"},
	{"S",	vin_peugeot_3dc,	"3-door Commercial"},
	{"T",	vin_peugeot_5dc,	"5-door Commercial"},
};


/* https://en.wikipedia.org/wiki/Vehicle_identification_number */
static vininfo_section g_manufac_table[] = {

	vin_unknown_section,
	{"LSG",		vinmanu_lsg,	"Shanghai General Motors"},
	{"LDC",		vinmanu_ldc, 	"Dong Feng Peugeot Citroen (DPCA)"},
	{"PNA", 	vinmanu_pna,	"Peugeot"}
};

static vinmanu2vds g_manu2vds_map[] = {

	{vinmanu_unknown, NULL,	0, NULL, 0},
	{vinmanu_lsg, NULL,	0, NULL, 0},
	{vinmanu_ldc, g_modelin_dpca_table,	array_size(g_modelin_dpca_table), NULL, 0},
	{vinmanu_pna, g_modelin_peugeot_table, array_size(g_modelin_peugeot_table), 
	              g_bodytype_peugeot_table, vin_peugeot_bt_offset},
};

vininfo_section *vin_manufac_table()
{
	return g_manufac_table;
}

vinmanu2vds *vin_manu2vds_map()
{
	return g_manu2vds_map;
}

