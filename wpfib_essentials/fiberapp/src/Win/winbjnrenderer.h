/**********************************************************\
Original Author: Utkarsh Khokhar

Created:    May 21, 2012
\**********************************************************/

#ifndef H_WIN_BJNRENDERER
#define H_WIN_BJNRENDERER

#include <winsock2.h>
#include "bjnrenderer.h"
#include "talk/base/timeutils.h"
#include "d3d_render.h"
#include "gdivideorenderer.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "boost/thread/thread.hpp"

using namespace webrtc;
class bjnpluginslaveAPI;

// A base class that wraps a video buffer (RGBA, YV12) so it can be used as input buffers for the various renderers.
// All access to the buffer is protected by a critical section as the writing and reading the buffer are done in separate threads

class WinBjnRenderBuffer
{
    BASEVideoFrame*                 _renderFrame;
    int                             _width;
    int                             _height;
    webrtc::CriticalSectionWrapper* _cs;
    RawVideoType                    _frameType;  // frame type of the render buffer
    BITMAPINFO                      _bmi;

public:
    WinBjnRenderBuffer(RawVideoType type): _renderFrame(NULL)
        , _width(0)
        , _height(0)
        , _frameType(type)
    {
        _cs = webrtc::CriticalSectionWrapper::CreateCriticalSection();

        memset(&_bmi.bmiHeader, 0, sizeof(_bmi.bmiHeader));
        _bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        _bmi.bmiHeader.biPlanes = 1;
        _bmi.bmiHeader.biBitCount = 32;
        _bmi.bmiHeader.biCompression = BI_RGB;
    }

    ~WinBjnRenderBuffer()
    {
        Reset(_frameType);
        delete _cs;
    }

    int Width()
    {
        return _width;
    }

    int Height()
    {
        return _height;
    }

    RawVideoType GetFrameType()
    {
        return _frameType;
    }

    void Reset(RawVideoType type)  // used to reset frame buffers in order to switch between argb and yv12.
    {
        webrtc::CriticalSectionScoped lock(_cs);
        if (_renderFrame)
        {
            delete _renderFrame;
            _renderFrame = NULL;
        }
        _width = 0;
        _height = 0;
        _frameType = type;
    }

    BASEVideoFrame* LockBuffer(int* width, int* height, BITMAPINFO* bmi)
    {
        _cs->Enter();
        if (_renderFrame)
        {
            // return optional values if requested
            if (width)
                *width = _width;
            if (height)
                *height = _height;
            if (bmi)
                *bmi = _bmi;
        }
        else
        {
            // as we don't have a buffer we exit locked
            _cs->Leave();
        }
        return _renderFrame;
    }

    void UnlockBuffer()
    {
        _cs->Leave();
    }

    bool UpdateStride(int dxStride, int width, int height)
    {
        webrtc::CriticalSectionScoped lock(_cs);

        if(_frameType != kVideoI420 || dxStride == width)
        {
            return false;
        }
        if (_renderFrame)
        {
            delete _renderFrame;
            _renderFrame = NULL;
        }

        I420VideoFrame *pI420RenderFrame = new (std::nothrow) I420VideoFrame();
        if(pI420RenderFrame)
        {
            int ret = pI420RenderFrame->CreateEmptyFrame(width,height,dxStride,dxStride>>1,dxStride>>1);
            if (!ret)
            {
                _width = width;
                _height = height;
                _renderFrame = dynamic_cast<BASEVideoFrame*>(pI420RenderFrame);
                return true;
            }
        }
        return false;
    }

    void CopyFrame(BASEVideoFrame *videoFrame)  // copies the BaseVideoFrame data into the image buffer.
    {
        webrtc::CriticalSectionScoped lock(_cs);

        // check that the buffer is the correct size
        if ((videoFrame->width() != _width) || (videoFrame->height() != _height))
        {
            // it's not delete and create one the correct size
            if (_renderFrame)
            {
                delete _renderFrame;
                _renderFrame = NULL;
            }

            if(_frameType == kVideoARGB)  // 32 bit RGBA buffer
            {
                RGBVideoFrame *pRGBRenderFrame = new (std::nothrow) RGBVideoFrame();
                if(pRGBRenderFrame)
                {
                    int ret = pRGBRenderFrame->CreateEmptyFrame(videoFrame->width(),videoFrame->height(),kVideoARGB);
                    if (!ret )
                    {
                        _width = videoFrame->width();
                        _height = videoFrame->height();
                        _bmi.bmiHeader.biWidth = _width;
                        _bmi.bmiHeader.biHeight = -_height;
                        _bmi.bmiHeader.biSizeImage = _width * _height * 4;
                    }
                }
                _renderFrame = dynamic_cast<BASEVideoFrame*>(pRGBRenderFrame);
            }
            else if (_frameType == kVideoI420) //I420 Buffer
            {
                I420VideoFrame *pI420VideoFrame = dynamic_cast<I420VideoFrame* >(videoFrame);
                I420VideoFrame *pI420RenderFrame = new (std::nothrow) I420VideoFrame();
				if(pI420RenderFrame)
                {
                    int ret = pI420RenderFrame->CreateEmptyFrame(pI420VideoFrame->width(),pI420VideoFrame->height(),pI420VideoFrame->stride(webrtc::kYPlane),pI420VideoFrame->stride(webrtc::kUPlane),pI420VideoFrame->stride(webrtc::kVPlane));
                    if (!ret)
                    {
                        _width = pI420VideoFrame->width();
                        _height = pI420VideoFrame->height();
                    }
                    _renderFrame = dynamic_cast<BASEVideoFrame*>(pI420RenderFrame);
                }
            }
        }

        if (_renderFrame)
        {
            bool isRgb = videoFrame->is_RGB_Frame();
            if(_frameType == kVideoARGB)
            {
                webrtc::RGBVideoFrame *pRGBRenderFrame = dynamic_cast<webrtc::RGBVideoFrame* >(_renderFrame);
                if(isRgb)
                {
                    webrtc::RGBVideoFrame *pRGBVideoFrame = NULL;
                    pRGBVideoFrame = dynamic_cast<webrtc::RGBVideoFrame* >(videoFrame);	
                    memcpy(pRGBRenderFrame->buffer(),pRGBVideoFrame->buffer(),pRGBVideoFrame->frame_size());
                }
                else
                {
                    webrtc::I420VideoFrame *pI420VideoFrame = NULL;
                    pI420VideoFrame = dynamic_cast<webrtc::I420VideoFrame* >(videoFrame);
                    // convert the I420 frame into the rgba buffer
                    webrtc::ConvertFromI420(pI420VideoFrame, webrtc::kARGB, 0, pRGBRenderFrame->buffer());
                }
            }
            else if (_frameType == kVideoI420 && !isRgb)
            {
                webrtc::I420VideoFrame *pI420VideoFrame = NULL;
                pI420VideoFrame = dynamic_cast<webrtc::I420VideoFrame* >(videoFrame);
                webrtc::I420VideoFrame *pI420RenderFrame = dynamic_cast<I420VideoFrame* >(_renderFrame);
                // copy the I420 frame into the render buffer
                webrtc::CopyI420(pI420VideoFrame,pI420RenderFrame);
            }
        }
    }
};

class WinBjnRenderer : public BjnRenderer
{
public:
    WinBjnRenderer(bool windowless);
    virtual ~WinBjnRenderer();

    bool setUpLayer(HWND wnd, bool d3d9Renderer);
    int FrameSizeChange(uint32_t width, uint32_t height, uint32_t numberOfStreams);
    int FrameSizeChange(uint32_t width, uint32_t height, uint32_t numberOfStreams, webrtc::RawVideoType videoType);
    int DeliverFrame(BASEVideoFrame *videoFrame);
    virtual void Redraw();
    virtual void RenderIntoContext(void* context, int x, int y, int cx, int cy);
    virtual void Reset();
    bool IsD3DEnabled() { return _d3dEnabled; }
    void EnableHWColorConv() { _hwColorConv = true;}
    bool IsHWColorConv() {return _hwColorConv;}

private:
    bool setUpWinD3DLayer(HWND wnd);
    bool InFFSingleProcessMode();

    D3DVideoRenderer* _ptrD3DRenderer;
    GdiVideoRenderer* _ptrGdiRenderer;
    uint32 _lastFrameTimeMS;
    int _count;
    bool _windowless;
    bool _d3dEnabled;
    bool _hwColorConv;
};

#endif
