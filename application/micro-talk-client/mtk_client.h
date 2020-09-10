
#ifndef __MTK_CLIENT_H__
#define __MTK_CLIENT_H__




#include "micro_talk.h"



err_type mtk_nc_connect(struct _mtk_context *ctx, struct _mtk_work *work);
err_type mtk_dc_connect(struct _mtk_context *ctx, struct _mtk_work *work);

#endif /* __MTK_CLIENT_H__ */
