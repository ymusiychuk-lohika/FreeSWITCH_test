#ifndef __WEBRTC_VOE_DEV_H__
#define __WEBRTC_VOE_DEV_H__

#include <pjmedia-audiodev/audiodev_imp.h>
#include <pj/os.h>

using namespace bjn_sky;
/*
 * Init webrtc voice engine driver.
 */
pjmedia_aud_dev_factory*
pjmedia_webrtc_voe_audio_factory(pj_pool_factory *pf,
                                 skinnySipManager* sipManager);

#endif  //__WEBRTC_VOE_DEV_H__