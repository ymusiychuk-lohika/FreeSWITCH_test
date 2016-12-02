#ifndef BJN_GDI_VIDEORENDERER_H_
#define BJN_GDI_VIDEORENDERER_H_

#include <winsock2.h>
#include <windows.h>
#include "../bjnrenderer.h"
#include "critical_section_wrapper.h"

class WinBjnRenderBuffer;

class GdiVideoRenderer
{
public:
    GdiVideoRenderer(HWND win);
    ~GdiVideoRenderer();

    int DeliverFrame(webrtc::BASEVideoFrame *videoFrame);
    void setHandle(HWND wnd) { wnd_ = wnd; }
    void DrawIntoDC(HDC hDC, int x, int y, int cx, int cy);
    void Redraw();

private:
    HWND wnd_;
    WinBjnRenderBuffer* _renderFrame;
};

#endif  // BJN_GDI_VIDEORENDERER_H_
