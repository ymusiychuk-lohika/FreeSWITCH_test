#include "webrtc/common_video/interface/rgb_video_frame.h"

#include <algorithm>  // swap
#include <string.h>

namespace webrtc {
RGBVideoFrame::RGBVideoFrame()
: buffer_(NULL), prev_size_(0)
{
    frame_type_ = kVideoRGB24;
}

RGBVideoFrame::~RGBVideoFrame()
{
    if (buffer_)
        delete [] buffer_;
}

int RGBVideoFrame::CreateEmptyFrame(int width, int height,
                                    RawVideoType type) {
    if (CheckDimensions(width, height) < 0)
        return -1;
    // Creating empty frame - reset all values.
    timestamp_ = 0;
    render_time_ms_ = 0;
    uint32_t size = 0;
    
    width_ = width;
    height_ = height;
    
    if(type == kVideoRGB24)
        size = width_ * height_ * 3;
    else if(type == kVideoARGB)
        size = width_ * height_ * 4;
    else
        return -1;
    
    if(size > prev_size_)
    {
        if(NULL != buffer_)
            delete [] buffer_;
        
        buffer_ = new uint8_t [size];
        prev_size_ = size;
    }
    
    frame_type_ = type;
    
    return 0;
}

int RGBVideoFrame::CreateFrame(uint8_t* src_buffer, int width, int height, RawVideoType type) {
    int ret = CreateEmptyFrame(width, height, type);
    if (ret < 0)
        return ret;
    
    int size = 0;
    if(type == kVideoRGB24)
        size = width * height * 3;
    else
        size = width * height * 4;
    
    memcpy(buffer_,src_buffer,size);
    
    return 0;
}

void RGBVideoFrame::SwapFrame(RGBVideoFrame* videoFrame) {

  CreateEmptyFrame(videoFrame->width(),videoFrame->height(),videoFrame->get_frame_type());

  memcpy(buffer_,videoFrame->buffer(),(videoFrame->height() * videoFrame->stride()));

  std::swap(width_, videoFrame->width_);
  std::swap(height_, videoFrame->height_);
  std::swap(timestamp_, videoFrame->timestamp_);
  std::swap(render_time_ms_, videoFrame->render_time_ms_);
}

uint8_t* RGBVideoFrame::buffer() {
    if (buffer_)
        return buffer_;
    return NULL;
}

const uint8_t* RGBVideoFrame::buffer() const {
    if (buffer_)
        return buffer_;
    return NULL;
}

int RGBVideoFrame::stride() const {

    if(frame_type_ == kVideoRGB24)
        return width_ * 3;
    else
        return width_ * 4;
}

int RGBVideoFrame::set_width(int width) {
    if (CheckDimensions(width, height_) < 0)
        return -1;
    width_ = width;
    return 0;
}

int RGBVideoFrame::set_height(int height) {
    if (CheckDimensions(width_, height) < 0)
        return -1;
    height_ = height;
    return 0;
}

int RGBVideoFrame::CheckDimensions(int width, int height) {
    if (width < 1 || height < 1 )
        return -1;
    return 0;
}

unsigned int RGBVideoFrame::frame_size() {
    int32_t size = 0;
    
    if(frame_type_ == kVideoRGB24)
        size = width_ * height_ * 3;
    else
        size = width_ * height_ * 4;
    
    return size;
}


bool RGBVideoFrame::IsZeroSize() const
{
    return (!width_ || !height_);
}


void RGBVideoFrame::ResetSize()
{
    //Nothing is done here
}

}  // namespace webrtc




