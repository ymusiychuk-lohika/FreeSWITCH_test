#ifndef COMMON_VIDEO_INTERFACE_BASE_VIDEO_FRAME_H
#define COMMON_VIDEO_INTERFACE_BASE_VIDEO_FRAME_H

// BASEVideoFrame class
//
// Provides interfaces for handling video frames.

#include "webrtc/common_video/plane.h"
#include "webrtc/typedefs.h"
#include "webrtc/common_types.h"

/*
 *  BASEVideoFrame includes support for a reference counted impl.
 */

namespace webrtc {

class BASEVideoFrame {
 public:
  BASEVideoFrame() : width_(0), height_(0), timestamp_(0), render_time_ms_(0),
                      frame_type_(kVideoI420)
  {

  }
  virtual ~BASEVideoFrame()
  {

  }
  // Infrastructure for refCount implementation.
  // Implements dummy functions for reference counting so that non reference
  // counted instantiation can be done. These functions should not be called
  // when creating the frame with new I420VideoFrame().
  // Note: do not pass a I420VideoFrame created with new I420VideoFrame() or
  // equivalent to a scoped_refptr or memory leak will occur.
  virtual int32_t AddRef() {assert(false); return -1;}
  virtual int32_t Release() {assert(false); return -1;}

  // Set frame width.
  virtual int set_width(int width) {width_ = width; return 0;};

  // Set frame height.
  virtual int set_height(int height) {height_ = height; return 0;};

  // Get frame width.
  int width() const {return width_;}

  // Get frame height.
  int height() const {return height_;}

  // Set frame timestamp (90kHz).
  void set_timestamp(uint32_t timestamp) {timestamp_ = timestamp;}

  // Get frame timestamp (90kHz).
  uint32_t timestamp() const {return timestamp_;}

  // Set render time in miliseconds.
  void set_render_time_ms(int64_t render_time_ms) {render_time_ms_ =
                                                   render_time_ms;}

  // Get render time in miliseconds.
  int64_t render_time_ms() const {return render_time_ms_;}

  // Return true if underlying plane buffers are of zero size, false if not.
  virtual bool IsZeroSize() const {return false;}

  // Reset underlying plane buffers sizes to 0. This function doesn't
  // clear memory.
  virtual void ResetSize() {};

  virtual void set_frame_type(RawVideoType type) { frame_type_ = type; }

  virtual RawVideoType get_frame_type(void) const { return frame_type_; }

  bool is_RGB_Frame (void)
  {
    return (frame_type_ == kVideoARGB || frame_type_ == kVideoRGB24);
  }
    
  RawVideoType get_video_type(void) const { return  frame_type_;}

  int width_;
  int height_;
  uint32_t timestamp_;
  int64_t render_time_ms_;
  RawVideoType frame_type_;
};  // BASEVideoFrame

}  // namespace webrtc

#endif  // COMMON_VIDEO_INTERFACE_BASE_VIDEO_FRAME_H
