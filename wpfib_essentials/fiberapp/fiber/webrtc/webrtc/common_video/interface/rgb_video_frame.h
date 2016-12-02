#ifndef COMMON_VIDEO_INTERFACE_RGB_VIDEO_FRAME_H
#define COMMON_VIDEO_INTERFACE_RGB_VIDEO_FRAME_H

// RGB Frame class
//
// Storing and handling of RGB video frames.

#include "webrtc/common_video/plane.h"
#include "webrtc/typedefs.h"
#include "webrtc/common_video/interface/base_video_frame.h"
/*
 *  RGBVideoFrame includes support for a reference counted impl.
 */

namespace webrtc {

class RGBVideoFrame : public BASEVideoFrame {
 public:
  RGBVideoFrame();
  virtual ~RGBVideoFrame();
  // Infrastructure for refCount implementation.
  // Implements dummy functions for reference counting so that non reference
  // counted instantiation can be done. These functions should not be called
  // when creating the frame with new RGBVideoFrame().
  // Note: do not pass a RGBVideoFrame created with new RGBVideoFrame() or
  // equivalent to a scoped_refptr or memory leak will occur.
  virtual int32_t AddRef() {assert(false); return -1;}
  virtual int32_t Release() {assert(false); return -1;}

  // CreateEmptyFrame: Sets frame dimensions and allocates buffers based
  // on set dimensions - height and plane stride.
  // If required size is bigger than the allocated one, new buffers of adequate
  // size will be allocated.
  // Return value: 0 on success ,-1 on error.
  int CreateEmptyFrame(int width, int height,
                       RawVideoType type);


  // CreateFrame: Sets the frame's members and buffers. If required size is
  // bigger than allocated one, new buffers of adequate size will be allocated.
  // Return value: 0 on success ,-1 on error.
  int CreateFrame(uint8_t* src_buffer, int width, int height, RawVideoType type);

  void SwapFrame(RGBVideoFrame* videoFrame);

  // Get pointer to buffer per plane.
  uint8_t* buffer();

  const uint8_t* buffer() const;

  //Provided the size of frame
  unsigned int frame_size();

  // Get allocated stride per plane.
  int stride() const;

  // Set frame width.
  int set_width(int width);

  // Set frame height.
  int set_height(int height);

  // Return true if underlying plane buffers are of zero size, false if not.
  bool IsZeroSize() const;

  // Reset underlying plane buffers sizes to 0. This function doesn't
  // clear memory.
  void ResetSize();

  RawVideoType get_frame_type(void) const { return frame_type_; }

  bool is_RGB_Frame (void) { return true; }

 private:
  // Verifies legality of parameters.
  // Return value: 0 on success ,-1 on error.
  int CheckDimensions(int width, int height);

  uint8_t* buffer_;
  uint32_t prev_size_;
};  // RGBVideoFrame

}  // namespace webrtc

#endif  // COMMON_VIDEO_INTERFACE_RGB_VIDEO_FRAME_H
