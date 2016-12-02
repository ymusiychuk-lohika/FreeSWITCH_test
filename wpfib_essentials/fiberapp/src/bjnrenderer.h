#ifndef H_BJNRENDERER_H
#define H_BJNRENDERER_H

#include "common_video/interface/i420_video_frame.h"
#include "common_video/interface/rgb_video_frame.h"
#include "talk/base/logging_libjingle.h"
#include "talk/base/timeutils.h"

#define DEFAULT_FPS 30

class UIWidget;

class BJNExternalRenderer {
 public:
  // This method will be called when the stream to be rendered changes in
  // resolution or number of streams mixed in the image.
  virtual int FrameSizeChange(unsigned int width,
                              unsigned int height,
                              unsigned int number_of_streams,
                              webrtc::RawVideoType videoType) = 0;

  // This method is called when a new frame should be rendered.
  virtual int DeliverFrame(webrtc::BASEVideoFrame *videoFrame ) = 0;

 protected:
  virtual ~BJNExternalRenderer() {}
};

static bool _softwareRenderer = false;
class BjnRenderer: BJNExternalRenderer
{
public:
    BjnRenderer()
        : _uiwidget(NULL)
        , _skipTimeMS(1000/DEFAULT_FPS)
        , _prevRenderTimeMS(talk_base::Time())
    {
    }
    virtual ~BjnRenderer() {
    }

    virtual bool setUpLayer(
#ifdef FB_WIN
		bool d3d9Renderer = true
#endif
	) {
        LOG(LS_WARNING) << "Renderer is not setup for current platform";
        return false;
    }

    virtual int FrameSizeChange(uint32_t width, uint32_t height, uint32_t numberOfStreams, webrtc::RawVideoType videoType) {
        LOG(LS_WARNING) << "Renderer is not setup for current platform";
        return 0;
    }

    virtual int DeliverFrame(webrtc::BASEVideoFrame *videoFrame) {
        LOG(LS_WARNING) << "Renderer is not setup for current platform";
        return 0;
    }

    virtual void windowChangeDimensions(long width, long height) {
        return;
    }

    virtual void Redraw() {
        return;
    }

    virtual void RenderIntoContext(void* context, int x, int y, int cx, int cy) {
        return;
    }
	
    virtual void Reset() {
        return;
    };

    virtual void SetUIWidget(UIWidget* widget) {
        _uiwidget = widget;
    }

    static bool softwareRenderer() {
        return _softwareRenderer;
    }

    virtual void EnableHWColorConv() {}

    bool SkipFrame() {
        uint32_t currentTimeMS = talk_base::Time();
        int diff = currentTimeMS - _prevRenderTimeMS;
        if(diff < _skipTimeMS) {
            return true;
        }
        _prevRenderTimeMS = currentTimeMS;
        return false;
    }

    void SetFPS(int fps) {
        _skipTimeMS = (1000/fps);
    }

protected:
    UIWidget* _uiwidget;
    uint32_t _skipTimeMS;
    uint32_t _prevRenderTimeMS;
};

#endif

