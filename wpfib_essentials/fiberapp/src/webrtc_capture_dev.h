#ifndef __WEB_RTC_CAPTURE_DEV_H_
#define __WEB_RTC_CAPTURE_DEV_H_

#include <pjmedia-videodev/videodev_imp.h>
#include <pj/os.h>

/*
 * Init webrtc_cap_ video driver.
 */

pjmedia_vid_dev_factory* pjmedia_webrtc_vid_capture_factory(
                            pj_pool_factory *pf,
                            bjn_sky::skinnySipManager* sipManager);

typedef void (*func_ptr)(void *strm);
void run_func_on_main_thread(void *strm, func_ptr func);

#endif  //__WEB_RTC_CAPTURE_DEV_H_
