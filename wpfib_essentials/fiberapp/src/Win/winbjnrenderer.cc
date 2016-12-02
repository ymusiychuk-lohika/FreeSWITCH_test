#include "winbjnrenderer.h"
#include "winuiwidget.h"
#include "talk/base/logging_libjingle.h"

#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "winUtils.h"


WinBjnRenderer::WinBjnRenderer(bool windowless)
    : _ptrD3DRenderer(NULL)
    , _ptrGdiRenderer(NULL)
    , _lastFrameTimeMS(0)
    , _count(0)
    , _windowless(windowless)
    , _d3dEnabled(true)
    , _hwColorConv(false)
{
}

WinBjnRenderer::~WinBjnRenderer()
{
    Reset();
}

bool WinBjnRenderer::setUpLayer(HWND wnd, bool d3d9Renderer)
{
    // note: this is a static global variable
    _softwareRenderer = !d3d9Renderer;

    // save the preferred renderer in a member variable
    _d3dEnabled = d3d9Renderer;

    // disable D3D for windowless plugin
    if (_windowless)
    {
        d3d9Renderer = false;
    }

    if(d3d9Renderer)
    {
        if(_ptrGdiRenderer) {
            LOG(LS_INFO) << "Switching to D3D Renderer";
            _ptrGdiRenderer->setHandle(NULL);
            return setUpWinD3DLayer(wnd);
        }
        else if(!_ptrD3DRenderer)
        {
            LOG(LS_INFO) << "Setting up D3D Renderer";
            return setUpWinD3DLayer(wnd);
        }
        else
        {
            if(_hwColorConv != _ptrD3DRenderer->IsHwColorConv())
            {
                LOG(LS_INFO) << "Reset D3D Renderer as HW color feature is updated";
                delete _ptrD3DRenderer;
                _ptrD3DRenderer = NULL;
                return setUpWinD3DLayer(wnd);
            }
            else
                LOG(LS_INFO) << "Already on D3D Renderer";
            return true;
        }
    }
    else
    {
        if(_ptrD3DRenderer)
        {
            LOG(LS_INFO) << "Switching to GDI Renderer";
        }

        if(!_ptrGdiRenderer)
        {
              LOG(LS_INFO) << "Setting up GDI Renderer";
            _ptrGdiRenderer = new GdiVideoRenderer(wnd);
            if(_ptrGdiRenderer)
            {
                return true;
            }
            else
            {
                LOG(LS_INFO) << "Failed to set up GDI Renderer... Falling back to D3D9";
                return setUpWinD3DLayer(wnd);
            }
        }
        else
        {
            LOG(LS_INFO) << "Already on GDI Renderer";
            return true;
        }
    }
}

bool WinBjnRenderer::setUpWinD3DLayer(HWND hwin)
{
    //if(!wnd)
    //{
    //    LOG(LS_ERROR) << "NULL Window handle. Can't setup Renderer";
    //    return false;
    //}

    //HWND hwin = wnd->getHWND();

    // Setup Direct3D drawing.
    if (hwin) {
        _ptrD3DRenderer = new D3DVideoRenderer(hwin,_hwColorConv);
    }

    if (_ptrD3DRenderer) {
        if (!_ptrD3DRenderer->Init(false, false)) {
            LOG(LS_ERROR) << "D3D initialization failed. Falling back to GDI";
            delete _ptrD3DRenderer;
            _ptrD3DRenderer = NULL;

            _ptrGdiRenderer = new GdiVideoRenderer(hwin);
        }
        return true;
    }
    else {
        return false;
    }
}

void WinBjnRenderer::Reset()
{
    if (_ptrD3DRenderer) {
        delete _ptrD3DRenderer;
        _ptrD3DRenderer = NULL;
    }
    if (_ptrGdiRenderer) {
        delete _ptrGdiRenderer;
        _ptrGdiRenderer = NULL;
    }
 /*   if (_slaveJSPtr) {
        _slaveJSPtr = NULL;
    }*/
    if (_uiwidget) {
        _uiwidget = NULL;
    }
}

int WinBjnRenderer::FrameSizeChange(uint32_t width, uint32_t height, uint32_t numberOfStreams)
{
   /* if(_slaveJSPtr) {
        _slaveJSPtr->UpdatedWidthHeight(width, height);
    }*/
    return 0;
}

int WinBjnRenderer::FrameSizeChange(uint32_t width, uint32_t height, uint32_t numberOfStreams, webrtc::RawVideoType videoType)
{
    /*if(_slaveJSPtr) {
        _slaveJSPtr->UpdatedWidthHeight(width, height);
    }*/
	TCHAR buf[128];
	_stprintf(buf, _T("Dimension %d x %d \r\n\r\n"), width, height);
	::OutputDebugString(buf);
    return 0;
}

int WinBjnRenderer::DeliverFrame(BASEVideoFrame *videoFrame)
{
    // deliver the frame to our shared buffer

    if(_ptrD3DRenderer) {
        _ptrD3DRenderer->DeliverFrame(videoFrame);
    } else if(_ptrGdiRenderer) {
        _ptrGdiRenderer->DeliverFrame(videoFrame);
    }

    // this will redraw the plugin window
  /*  if (_slaveJSPtr) {
        _slaveJSPtr->UpdateVideo();
    }*/

    // this will redraw the widget window if it's visible
    if (_uiwidget) {
        // if the widget isn't visible this does nothing
        _uiwidget->UpdateVideo(videoFrame);
		//_uiwidget  = NULL;
    }
    return 0;
}

void WinBjnRenderer::Redraw()
{
    if (_ptrD3DRenderer) {
        _ptrD3DRenderer->Redraw();
    } else if (_ptrGdiRenderer) {
        _ptrGdiRenderer->Redraw();
    }
}

void WinBjnRenderer::RenderIntoContext(void* context, int x, int y, int cx, int cy)
{
    if (_ptrGdiRenderer)
    {
        _ptrGdiRenderer->DrawIntoDC((HDC) context, x, y, cx, cy);
    }
    else if (_ptrD3DRenderer)
    {
        _ptrD3DRenderer->DrawIntoDC((HDC) context, x, y, cx, cy);
    }
}


