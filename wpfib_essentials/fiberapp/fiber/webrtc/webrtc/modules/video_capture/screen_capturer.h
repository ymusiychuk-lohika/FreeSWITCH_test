#ifndef __SCREEN_CAPTURER_H_
#define __SCREEN_CAPTURER_H_

#include "video_capture_defines.h"
#include "common_types.h"
#include "tick_util.h"
#include "critical_section_wrapper.h"
#include "event_wrapper.h"
#include "module_common_types.h"
#include "thread_wrapper.h"
#include "screen_capturer_defines.h"
#include <iostream>


class FramePacemaker {
 public:
  explicit FramePacemaker(uint32_t max_fps)
      : time_per_frame_ms_(1000 / max_fps) {
    frame_start_ = webrtc::TickTime::MillisecondTimestamp();
  }

  void SleepIfNecessary(webrtc::EventWrapper* sleeper) {
    uint64_t now = webrtc::TickTime::MillisecondTimestamp();
    if (now - frame_start_ < time_per_frame_ms_) {
      sleeper->Wait(time_per_frame_ms_ - (now - frame_start_));
    }
  }

 private:
  uint64_t frame_start_;
  uint64_t time_per_frame_ms_;
};

namespace BJN
{
    class ScreenCapturer
    {
        public:
            enum Status
            {
                ST_Stopped,
                ST_Running
            };
            enum
            {
                MAX_SCREEN_CAPTURE_COUNT = 16,
            };
            ScreenCapturer(int32_t id, webrtc::VideoCaptureExternal& captureObserver);
            virtual ~ScreenCapturer();
            virtual void init() = 0;
            virtual void destroy() = 0;
            virtual void captureConfigComplete() = 0;
            virtual void startCapture();
            virtual void stopCapture();
            int32_t ConfigureCaptureParams(webrtc::ScreenCaptureConfig& config);
            virtual Status status() { return mStatus; }

            static std::string friendlyName() { return cFriendlyName; }
            static std::string uniqueName() { return cUniqueName; }
            static int32_t getCaptureScreenCount();
            static webrtc::VideoCaptureCapability getCaptureCapability(int screenIndex);
            static bool stCaptureThread(void*);
            static bool isScreenSharingDevice(const std::string& id);
            static int32_t getScreenSharingDeviceIndex(const std::string& id);
            static uint32_t getScreenSharingFrameRate();
        protected:
            void captureThread();
            virtual void captureFrame() = 0;
            int32_t mId;
            webrtc::VideoCaptureExternal& mCaptureObserver;
            webrtc::ThreadWrapper* mCaptureThread;
            Status mStatus;
            uint32_t mCapturePid;
            bool mCaptureConfigComplete;
            static const uint32_t kInvalidCapturePid = 0xffffffff;

            // Let's cache the entire config params
            webrtc::ScreenCaptureConfig mScreenCaptureConfig;
        private:
            static std::string cFriendlyName;
            static std::string cUniqueName;
    };


}   // namespace BJN

#endif  //__SCREEN_CAPTURER_H_

