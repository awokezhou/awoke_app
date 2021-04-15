
#ifndef __BK_EOTS_HD_H__
#define __BK_EOTS_HD_H__



#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_macros.h"


#define PACKED_ALIGN_BYTE __attribute__ ((packed,aligned(1)))


/*
 * external command entry {
 */
typedef struct _ecmd_entry {
	uint8_t code;
	uint8_t cmdlen;
	int (*handler)(struct _ecmd_entry *e, uint8_t *p);
} ecmd_entry;
/*
 * } external command entry
 */

typedef struct _ecmd_item {
	uint16_t prefix;
	uint8_t code;
	int length;
} ecmd_item;

/*
 * External Command(ecmd) ack
 */
typedef struct _ecmd_ack {

	uint16_t header;
	
	uint16_t focus_hi;
	uint16_t focus_lo;
	
	uint16_t view_hi;
	uint16_t view_lo;
	
	uint16_t cross_x;
	uint16_t cross_y;
	
	uint16_t expo;
	
	uint8_t gain;

	/* algorithm control { */
	uint8_t algctl_dfg:1;
	uint8_t algctl_gray:1;
	uint8_t algctl_ehc:1;
	uint8_t algctl_aec:1;
	uint8_t algctl_zm2:1;		/* zoom 2 */
	uint8_t algctl_zm4:1;		/* zoom 4 */
	uint8_t algctl_hdr:1;
	uint8_t algctl_rsv:1;		/* bit7 reserve */
	/* } algorithm control */

	/* algorithm status { */
	uint8_t algsta_aea:2;		/* aec area */
	uint8_t algsta_fps:2;
	uint8_t algsta_mir:2;		/* mirror */
	uint8_t algsta_res:1;		/* resolution */
	uint8_t algsta_rsv:1;		/* bit7 reserve */
	/* } algorithm status */

	/* self inspection { */
	uint8_t seinsp_img:1;		/* image output enable */
	uint8_t seinsp_opf:1;		/* optical filter */
	uint8_t seinsp_ope:1;		/* operture */
	uint8_t seinsp_zmr:1;		/* zoom motor */
	uint8_t seinsp_fmr:1;		/* focus motor */
	uint8_t seinsp_rsv:3;
	/* } self inspection */

	uint8_t checkbyte;
} PACKED_ALIGN_BYTE ecmd_ack;


typedef struct _bk_eots_xy_point {
	uint16_t x;
	uint16_t y;
} bk_eots_xy_point;

typedef struct _bk_eots_xy_rect {
	struct _bk_eots_xy_point tl;
	struct _bk_eots_xy_point br;
} bk_eots_xy_rect;

#define xypoint_get(x, y)	{x, y}

#define XYPOINT_2D_CENTER	{800,800}


err_type bk_eots_hd_syscon(uint8_t *buf, int length);


#endif /* __BK_EOTS_HD_H__ */
