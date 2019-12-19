
#include "vin_parser.h"
#include "vin_manufac.h"



static char g_chars[] = "ABCDEFGHJKLMNPRSTUVWXYZ1234567890";
static char g_year_chars[] = "ABCDEFGHJKLMNPRSTVWXY123456789";

static vininfo_section g_country_table[] = {

	vin_unknown_section,
	{"1", 	vincoun_us,		"USA"},
	{"2",	vincoun_ca,		"Canada"},
	{"3",	vincoun_mx,		"Mexico"},
	{"4",	vincoun_us,		"USA"},
	{"6",	vincoun_au,		"Australia"},
	{"9",	vincoun_br,		"Brazil"},
	{"J",	vincoun_jp,		"Japan"},
	{"K",	vincoun_kr,		"Korea"},
	{"L",	vincoun_cn,		"China"},
	{"R",	vincoun_tw,		"Taiwan"},
	{"S",	vincoun_gb,		"UK"},
	{"T",	vincoun_ch,		"Switzerland"},
	{"V",	vincoun_fr,		"France"},
	{"W",	vincoun_de,		"Germany"},
	{"Y",	vincoun_se,		"Sweden"},
	{"Z",	vincoun_it,		"Italy"},
};

static vininfo_section *vin_country_table()
{
	return g_country_table;
}

static bool in_chars(char x, int f, int e, char *chars)
{
	int i;

	for (i=0; i<strlen(chars); i++) {
		if (chars[i] == x)
			break;
	}

	if ((f <= i) && (i <= e))
		return TRUE;

	return FALSE;
}

static bool chars_between(char x, char f, char e)
{
	int i;
	char *table = g_chars;
	char *find_x, *find_f, *find_e;
	
	find_x = strchr(table, x);
	if (!find_x) 
		return FALSE;
	
	find_f = strchr(table, f);
	if (!find_f) 
		return FALSE;

	find_e = strchr(table, e);
	if (!find_e) 
		return FALSE;

	if ((find_f <= find_x) && (find_x <= find_e)) 
		return TRUE;

	return FALSE;
}

static void vp_init(vin_parser *vp, const char *vin)
{
	memset(vp, 0x0, sizeof(vin_parser));
	vp->vin = vin;
	vp->pos = vp->vin;
	vp->tb_country = vin_country_table();
	vp->tb_country_size = vincoun_offset;
	vp->tb_manufac = vin_manufac_table();
	vp->tb_manufac_size = vinmanu_offset;
	vp->map_manu2vds = vin_manu2vds_map();
	vp->map_manu2vds_size = vinmanu_offset;
}

static vininfo_section unknown_section()
{
	vininfo_section section = {"?", 0, "Unknown"};
	return section;
}

static void section_set(vininfo_section *dst, vininfo_section *src)
{
	memcpy(dst, src, sizeof(vininfo_section));
}

static vininfo_section *section_find(vininfo_section *head, int size, char *pos)
{
	vininfo_section *p;

	log_debug("table size %d", size);

	array_foreach(head, size, p) {
		log_debug("pos:%.*s code:%.*s", strlen(p->code), pos, strlen(p->code), p->code);
		if (!strncmp(pos, p->code, strlen(p->code))) {
			return p;
		}
	}

	return NULL;
}

static void vininfo_init(vininfo *vi, vin_parser *vp)
{
	vi->vin = mem_mk_ptr(vp->vin);
	vi->country = unknown_section();
	vi->manufac = unknown_section();
	vi->modelin = unknown_section();
	vi->bodytyp = unknown_section();
}

void vininfo_dump(vininfo *vi)
{
	vininfo_section *vic, *vim, *vid, *vib;

	vic = &vi->country;
	vim = &vi->manufac;
	vid = &vi->modelin;
	vib = &vi->bodytyp;
		
	log_info("");
	log_info(">>> vininfo dump:");
	log_info("------------------------------");
	log_info("VIN: %.*s", vi->vin.len, vi->vin.p);
	log_info("Country: %s %s", vic->code, vic->string);
	log_info("Manufacturer: %s %s", vim->code, vim->string);
	log_info("Continent: %.*s", vi->continent.len, vi->continent.p);
	log_info("Model line: %s %s", vid->code, vid->string);
	log_info("Body type: %s %s", vib->code, vib->string);
	log_info("Year: %d", vi->year);
	log_info("vds: %.*s", vi->vds.len, vi->vds.p);
	log_info("vis: %.*s", vi->vis.len, vi->vis.p);
	log_info("------------------------------");
	log_info("");
}

static err_type vp_manufac(vin_parser *vp, vininfo *vi)
{
	vininfo_section *s;

	if (!vp || !vp->tb_manufac)
		return et_fail;
	
	s = section_find(vp->tb_manufac, vp->tb_manufac_size, vp->pos);
	if (!s) {
		return et_exist;
	}

	section_set(&vi->manufac, s);
	vp->pos += 3;
	
	return et_ok;
}

static err_type vp_continent(vin_parser *vp, vininfo *vi)
{
	if (chars_between(vp->pos[0], 'A', 'H')) {
		vi->continent = mem_mk_ptr("Africa");
	} else if (chars_between(vp->pos[0], 'J', 'R')) {
		vi->continent = mem_mk_ptr("Asia");
	} else if (chars_between(vp->pos[0], 'S', 'Z')) {
		vi->continent = mem_mk_ptr("Europe");
	} else if (chars_between(vp->pos[0], '1', '5')) {
		vi->continent = mem_mk_ptr("North America");
	} else if (chars_between(vp->pos[0], '6', '7')) {
		vi->continent = mem_mk_ptr("Oceania");
	} else if (chars_between(vp->pos[0], '8', '9')) {
		vi->continent = mem_mk_ptr("South America");
	}

	return et_ok;
}

static err_type vp_country(vin_parser *vp, vininfo *vi)
{
	vininfo_section *s;

	if (!vp || !vp->tb_country)
		return et_fail;
	
	s = section_find(vp->tb_country, vp->tb_country_size, vp->pos);
	if (!s) {
		return et_exist;
	}

	section_set(&vi->country, s);

	return et_ok;
}

static err_type _ptr_set(char **pos, mem_ptr_t *p, int len)
{
	p->p = *pos;
	p->len = len;
	*pos += len;
	return et_ok;
}

static vinmanu2vds *vp_maun2vds_map(vin_parser *vp, vininfo *vi)
{
	int size;
	vinmanu2vds *p, *head;

	head = vp->map_manu2vds;
	size = vp->map_manu2vds_size;

	log_debug("manuid %d", vi->manufac.id);

	array_foreach(head, size, p) {
		log_debug("map id %d", p->manuid);
		if (p->manuid == vi->manufac.id) {
			return p;
		}
	}

	return NULL;	
}

static err_type vp_mode(vin_parser *vp, vininfo *vi)
{
	vinmanu2vds *map;
	vininfo_section *s;
	
	map = vp_maun2vds_map(vp, vi);
	if (!map) {
		return et_exist;
	}

	if (map->tb_vds != NULL) {
		log_debug("table vds exist");
		s = section_find(map->tb_vds, map->tb_vds_size, vi->vds.p);
		if (s) {
			log_debug("find section %d %s %s", s->id, s->code, s->string);
			section_set(&vi->modelin, s);
		}
	}
	
	if (map->tb_bt != NULL) {
		s = section_find(map->tb_bt, map->tb_bt_size, vi->vds.p);
		if (s) {
			section_set(&vi->bodytyp, s);
		}
	}

	return et_ok;
}

static err_type vp_vds(vin_parser *vp, vininfo *vi)
{
	_ptr_set(&vp->pos, &vi->vds, 6);
	vp_mode(vp, vi);
}

static void year_range_set(vin_year_range *range, int min, int max)
{
	range->min = min;
	range->max = max;
}

static err_type vp_year(vin_parser *vp, vininfo *vi)
{
	int i;
	int year;
	time_t now;
	struct tm *date;
	vin_year_range range;
	
	if (!strncmp(vi->continent.p, "North America", vi->continent.len)) {
		if (in_chars(vi->vin.p[6], 0, 22, g_year_chars)) {
			year_range_set(&range, 2010, 2040);
		} else {
			year_range_set(&range, 1980, 2010);
		}
	} else {
		year_range_set(&range, 2010, 2040);
	}

	time(&now);
	date = localtime(&now);
	year = date->tm_year+1900;
		
	for (i=0; i<strlen(g_year_chars); i++) {
		
		if (g_year_chars[i] == vi->vin.p[9]) {
			
			if (range.min+i > year) {
				vi->year = range.min + i - 30;
			} else {
				vi->year = range.min + i;
			}
		}
	}

	return et_ok;
}

static err_type vp_vis(vin_parser *vp, vininfo *vi)
{
	_ptr_set(&vp->pos, &vi->vis, 8);
	vp_year(vp, vi);
}

err_type vin_parse(const char *vin, vininfo *vi)
{
	vin_parser vp;

	vp_init(&vp, vin);
	vininfo_init(vi, &vp);
	
	vp_continent(&vp, vi);
	vp_country(&vp, vi);
	vp_manufac(&vp, vi);

	vp_vds(&vp, vi);
	vp_vis(&vp, vi);
}
