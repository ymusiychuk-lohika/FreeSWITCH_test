#ifndef BJN_VID_RENDER_DEV_H_
#define BJN_VID_RENDER_DEV_H_

#include <pjmedia-videodev/videodev_imp.h>

PJ_BEGIN_DECL
PJ_DECL(pjmedia_vid_dev_factory*) pjmedia_bjn_vid_render_factory(pj_pool_factory *pf,
                                                         bjn_sky::skinnySipManager* sipManager);
PJ_END_DECL

#endif /* BJN_VID_RENDER_DEV_H_ */
