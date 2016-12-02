#include "gdivideorenderer.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "talk/base/logging_libjingle.h"
#include "winbjnrenderer.h"

#define THIS_FILE "gdivideorenderer"


GdiVideoRenderer::GdiVideoRenderer(HWND win)
    : wnd_(win)
    , _renderFrame(NULL)
{
}

GdiVideoRenderer::~GdiVideoRenderer()
{
    if(_renderFrame)
    {
        delete _renderFrame;
        _renderFrame = NULL;
    }
}

int GdiVideoRenderer::DeliverFrame(webrtc::BASEVideoFrame *videoFrame)
{
    // the calling function will force a window repaint
    if(NULL == _renderFrame)
        _renderFrame = new WinBjnRenderBuffer(kVideoARGB);
    if(_renderFrame)
        _renderFrame->CopyFrame(videoFrame);

    return 0;
}

void GdiVideoRenderer::Redraw()
{
    RECT rcClient;
    GetClientRect(wnd_, &rcClient);
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(wnd_, &ps);
    DrawIntoDC(hdc, 0, 0, rcClient.right, rcClient.bottom);
    EndPaint(wnd_, &ps);
}

void GdiVideoRenderer::DrawIntoDC(HDC hDC, int x, int y, int cx, int cy)
{
    BITMAPINFO bmi;
    bool isFrameUpdated = false;
    if(_renderFrame)
    {
        RGBVideoFrame* renderRGBbuffer = dynamic_cast<RGBVideoFrame*>(_renderFrame->LockBuffer(NULL, NULL, &bmi));
        if (renderRGBbuffer)
        {
            int oldMode = SetStretchBltMode(hDC, HALFTONE);
            StretchDIBits(hDC, x, y, cx, cy,
                0, 0, bmi.bmiHeader.biWidth, -bmi.bmiHeader.biHeight,
                renderRGBbuffer->buffer(), &bmi, DIB_RGB_COLORS, SRCCOPY);
            SetStretchBltMode(hDC, oldMode);
            _renderFrame->UnlockBuffer();
            isFrameUpdated = true;
        }
    }
    if(!isFrameUpdated)
    {
        RECT rc = { x, y, x + cx, y + cy };
        FillRect(hDC, &rc, (HBRUSH) GetStockObject(BLACK_BRUSH));
    }
}
