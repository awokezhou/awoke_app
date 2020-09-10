
#ifndef __MTK_COMMON_PROTOCOL_H__
#define __MTK_COMMON_PROTOCOL_H__



#include "micro_talk.h"



err_type mtk_pkg_request_header(struct _mtk_request_msg *msg, mtk_method_e method, 
	int dlen, mtk_lbl_e label);


#endif /* __MTK_COMMON_PROTOCOL_H__ */

