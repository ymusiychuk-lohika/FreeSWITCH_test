/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// Own include file
#include "talk/base/logging_libjingle.h"
#include "d3d_render.h"
#include "winbjnrenderer.h"
#include "WindowProcBase.h"
#include "uiwidget.h"

// System include files
#include <windows.h>

// WebRtc include files
#include "critical_section_wrapper.h"
#include "event_wrapper.h"
#include "trace.h"
#include "thread_wrapper.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "bjnhelpers.h"
#include "talk/base/timeutils.h"

// A structure for our custom vertex type
struct CUSTOMVERTEX
{
    FLOAT x, y, z;
    DWORD color; // The vertex color
    FLOAT u, v;
};

// Our custom FVF, which describes our custom vertex structure
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)

D3DVideoRenderer::D3DVideoRenderer(HWND win,bool enableHwColorConv)
    : _wnd(win)
    , _renderFrame(NULL)
    , _pD3D(NULL)
    , _pd3dDevice(NULL)
    , _pTexture(NULL)
    , _pRecordingTexture(NULL)
    , _pVB(NULL)
    , _deviceMonitor(NULL)
    , _deviceWidth(0)
    , _deviceHeight(0)
    , _surfaceWidth(0)
    , _surfaceHeight(0)
    , _resetTexturePending(false)
    , _updateTexturePending(false)
    , _d3dFailureCount(0)
    , _recIndicatorData(NULL)
    , _recIndicatorBitmap(NULL)
    , _recIndicatorWidth(0)
    , _recIndicatorHeight(0)
    , _recIndicatorLastToggle(0)
    , _recIndicatorOn(false)
    , _recIndicatorTimerId(0)
    , _showRecordingIndicator(false)
    , _canAlphaBlendTexture(false)
    , _lastRedraw(0)
    , _gdiFallbackMode(false)
    , _loggedGDISwitch(false)
    , _blackness(false)
    , _pTextureSurface(NULL)
    , _pOffSurface(NULL)
    , _renderYUV(false)
    , _isFrameCopied(false)
    , _isHwColorConvEnabled(enableHwColorConv)
{
    memset(&_d3dpp, 0, sizeof(_d3dpp));
}

D3DVideoRenderer::~D3DVideoRenderer()
{
    ReleaseAllD3DObjects();
    globalTimerCallbacks.Remove(_recIndicatorTimerId);
    if (_recIndicatorData)
    {
        delete [] _recIndicatorData;
        _recIndicatorData = NULL;
    }
    if (_recIndicatorBitmap)
    {
        DeleteObject(_recIndicatorBitmap);
        _recIndicatorBitmap = NULL;
    }
    if(_renderFrame)
    {
        delete _renderFrame;
        _renderFrame = NULL;
    }
}

bool D3DVideoRenderer::Init(bool allowGDIFallback, bool forceGDI)
{
    bool ok = false;

    ZeroMemory(&_d3dpp, sizeof(_d3dpp));

    if (!forceGDI)
    {
        _pD3D = Direct3DCreate9(D3D_SDK_VERSION);
        if (_pD3D)
        {
            HRESULT hr = S_OK;

            // check if we the hardware supports the options we need
            D3DCAPS9 caps;
            DWORD dwVertexProcessing = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
            if (SUCCEEDED(_pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps)))
            {
                if ((caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == D3DDEVCAPS_HWTRANSFORMANDLIGHT)
                {
                    dwVertexProcessing = D3DCREATE_HARDWARE_VERTEXPROCESSING;
                }

                // check the caps for alpha blending support
                LOG(LS_INFO) << "D3D BlendCaps - src: " << caps.SrcBlendCaps << ", dest: " << caps.DestBlendCaps;

                if ((caps.SrcBlendCaps & D3DPBLENDCAPS_SRCALPHA) && (caps.DestBlendCaps & D3DPBLENDCAPS_INVSRCALPHA))
                {
                    // we can alpha blend textures on top of the video
                    _canAlphaBlendTexture = true;
                }
                else 
                {
                    LOG(LS_INFO) << "D3D device doesn't support Alpha Blending";
                    _canAlphaBlendTexture = false;
                }
            }

            if (dwVertexProcessing == D3DCREATE_HARDWARE_VERTEXPROCESSING)
            {
                _d3dpp.BackBufferHeight = 0;
                _d3dpp.BackBufferWidth = 0;
                _d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
                _d3dpp.BackBufferCount = 1;
                _d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
                _d3dpp.MultiSampleQuality = 0;
                _d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
                _d3dpp.hDeviceWindow = _wnd;
                _d3dpp.Windowed = TRUE;
                _d3dpp.EnableAutoDepthStencil = FALSE;
                _d3dpp.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
                _d3dpp.Flags = 0;
                _d3dpp.FullScreen_RefreshRateInHz = 0;
                _d3dpp.PresentationInterval = 0;

                // get the display mode
                D3DDISPLAYMODE d3ddm;
                _pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
                _d3dpp.BackBufferFormat = d3ddm.Format;

                RECT rcWin;
                GetClientRect(_wnd, &rcWin);
                int width = rcWin.right - rcWin.left;
                int height = rcWin.bottom - rcWin.top;

                // we must have a valid width and height to create
                // the d3d device, we need to create it here so we
                // can fail and switch to the GDI renderer if the
                // hardware doesn't support the features we need
                // so we use a dummy width and height if the plugin
                // was created with a 0 x 0 size.
                if (width == 0)
                {
                    width = 16;
                }
                if (height == 0)
                {
                    height = 16;
                }
                hr = ResetDevice(width, height);
                if (SUCCEEDED(hr))
                {
                    if(_isHwColorConvEnabled)
                    {
                        hr = _pD3D ->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3ddm.Format, 0,  D3DRTYPE_SURFACE, (D3DFORMAT)MAKEFOURCC('Y','V','1','2'));
                        if(SUCCEEDED(hr))
                        {
                            hr = _pD3D ->CheckDeviceFormatConversion(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, (D3DFORMAT)MAKEFOURCC('Y','V','1','2'), _d3dpp.BackBufferFormat);  // Checking direct YV12 rendering capability
                            if(SUCCEEDED(hr))
                            {
                                _renderYUV = true;
                                LOG(LS_INFO) << "D3D HW Color Conversion capability check complete YU12 render is enabled";
                            }
                            else {
                                LOG(LS_INFO) << "D3D HW Color Conversion capability check failed from YV12 to back buffer format:" << _d3dpp.BackBufferFormat << " hr:" << hr;
                            }
                        }
                        else {
                            LOG(LS_INFO) << "D3D HW Color Conversion capability check failed from "<< d3ddm.Format << " to YV12 hr:" << hr;
                        }
                    }
                    else {
                        LOG(LS_INFO) << "D3D HW Color Conversion capability is not checked";
                    }
                    ok = true;
                }
            }
            else
            {
                LOG(LS_INFO) << "D3D device doesn't support hardware vertex processing";
            }
        }
    }
    else
    {
        // forcing GDI due to config.xml setting
        LOG(LS_INFO) << "D3D device forcing GDI due to config.xml setting";
    }

    // re-drawing of the video is done only when we receive a new video
    // frame, but we may also have to 'blink' a recording indicator which
    // is drawn when the video frame is drawn, this works when we are
    // receiving video at normal frame rates, but there can be situations
    // when the no video frames are received but we still need to 'blink'
    // the recording indicator. The solution is the have a timer running
    // and it checks if there have been any updated video frames during the
    // timer interval, if not it will force a repaint, the _lastRedraw
    // variable is use to track the last paint time.
    _lastRedraw = GetTickCount();

    _recIndicatorTimerId = globalTimerCallbacks.Add(kBlinkTimerFrequency, StaticIntervalTimer, (void*)this);

    if (!ok)
    {
        ReleaseAllD3DObjects();

        if (allowGDIFallback)
        {
            // the caller is OK with using GDI when D3D isn't
            // available so we set a flag and return true.
            // The renderer will now be a GDI renderer it will
            // not use D3D.
            _gdiFallbackMode = true;
            ok = true;

            LOG(LS_INFO) << "D3D can't be initialised, allowing GDI rendering fallback";
        }
    }

    if(_renderYUV)   // YV12 render
    {
        LOG(LS_INFO) << "D3D HW color conversion is initiated";
        if(NULL == _renderFrame)
            _renderFrame = new WinBjnRenderBuffer(kVideoI420);
        else
            _renderFrame->Reset(kVideoI420);
    }
    else            // RGB render
    {
        LOG(LS_INFO) << "D3D RGB render is initiated";
        if(NULL == _renderFrame)
            _renderFrame = new WinBjnRenderBuffer(kVideoARGB);
        else
            _renderFrame->Reset(kVideoARGB);
    }

    return ok;
}

void D3DVideoRenderer::ReleaseAllD3DObjects()
{
    if (_pVB)
    {
        _pVB->Release();
        _pVB = NULL;
    }
    if (_pRecordingTexture)
    {
        _pRecordingTexture->Release();
        _pRecordingTexture = NULL;
    }
    if (_pTexture)
    {
        _pTexture->Release();
        _pTexture = NULL;
    }
    if(_pOffSurface)
    {
        _pOffSurface->Release();
        _pOffSurface = NULL;
    }
    if(_pTextureSurface)
    {
        _pTextureSurface->Release();
        _pTextureSurface = NULL;
    }
    if (_pd3dDevice)
    {
        _pd3dDevice->Release();
        _pd3dDevice = NULL;
    }
    if (_pD3D)
    {
        _pD3D->Release();
        _pD3D = NULL;
    }
}

HRESULT D3DVideoRenderer::ResetDevice(int width, int height)
{
    // reset all D3D interfaces that depend on the window size
    if (_pVB)
    {
        _pVB->Release();
        _pVB = NULL;
    }
    if(_pTexture)
    {
        _pTexture->Release();
        _pTexture = NULL;
    }
    if(_pTextureSurface)
    {
        _pTextureSurface->Release();
        _pTextureSurface = NULL;
    }
    if(_pOffSurface) // needed to be released before device release as surface is allocated with D3DPOOL_DEFAULT
    {
        _pOffSurface->Release();
        _pOffSurface = NULL;
    }
    if (_pd3dDevice)
    {
        _pd3dDevice->Release();
        _pd3dDevice = NULL;
    }

    // now recreate the device
    _d3dpp.BackBufferWidth = width;
    _d3dpp.BackBufferHeight = height;

    HRESULT hr = _pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, _wnd,
        D3DCREATE_HARDWARE_VERTEXPROCESSING, &_d3dpp, &_pd3dDevice);
    if (SUCCEEDED(hr))
    {
        _resetTexturePending = true;

        // save the window size
        _deviceWidth = width;
        _deviceHeight = height;

        // save the monitor that the device was created on
        _deviceMonitor = MonitorFromWindow(_wnd, MONITOR_DEFAULTTOPRIMARY);

        // Turn off D3D lighting, since we are providing our own vertex colors
        _pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

		// bilinear filtering
        _pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        _pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        _pd3dDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);

        // alpha blending, used when drawing the recording indicator
        if (_canAlphaBlendTexture)
        {
            _pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            _pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        }

        // Create the vertex buffer. We create a buffer for 2 textures (8 vertices)
        hr = _pd3dDevice->CreateVertexBuffer(sizeof(CUSTOMVERTEX) * 8, D3DUSAGE_WRITEONLY,
            D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &_pVB, NULL);

    }

    return hr;
}

HRESULT D3DVideoRenderer::ResetOffScreenSurface(int width, int height)
{
    HRESULT hr = E_FAIL;

    if(_pTextureSurface)
    {
        _pTextureSurface->Release();
        _pTextureSurface = NULL;
    }
    if (_pRecordingTexture)
    {
        _pRecordingTexture->Release();
        _pRecordingTexture = NULL;
    }
    if (_pTexture)
    {
        _pTexture->Release();
        _pTexture = NULL;
    }
    if(_pOffSurface)
    {
        _pOffSurface->Release();
        _pOffSurface = NULL;
    }
    if (_pd3dDevice)
    {
        if(_renderYUV)
        {
            hr = _pd3dDevice->CreateOffscreenPlainSurface(width, height, (D3DFORMAT)MAKEFOURCC('Y','V','1','2'), D3DPOOL_DEFAULT, &_pOffSurface, NULL);
            hr = _pd3dDevice->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, _d3dpp.BackBufferFormat, D3DPOOL_DEFAULT, &_pTexture, NULL); // stretchrect requires D3DUSAGE_RENDERTARGET for dest surface.
            if(SUCCEEDED(hr))
            {
                hr = _pTexture->GetSurfaceLevel(0, &_pTextureSurface);
            }
        }
        else
            hr = _pd3dDevice->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &_pTexture, NULL);
         if (SUCCEEDED(hr))
        {
            _resetTexturePending = false;

            _surfaceWidth = width;
            _surfaceHeight = height;

            // set the flag so the render surface will be updated
            _updateTexturePending = true;
            _isFrameCopied = false;

            if(_pOffSurface)
            {
                D3DLOCKED_RECT lr = {0};
                hr = _pOffSurface->LockRect(&lr,0,0);
                if(SUCCEEDED(hr))
                {
                    // reallocating the I420 render buffer if the dx surface pitch is more than the width
                    if(_renderFrame && _renderFrame->UpdateStride(lr.Pitch,width,height))
                        _updateTexturePending = false;  //buffer is re-allocated, so don't update the texture now.
                    _pOffSurface->UnlockRect();
                }
            }
        }

        if (_recIndicatorData)
        {
            hr = _pd3dDevice->CreateTexture(_recIndicatorWidth, _recIndicatorHeight, 1, 0,
                D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &_pRecordingTexture, NULL);
            if (SUCCEEDED(hr))
            {
                // copy the image to our texture
                D3DLOCKED_RECT lr;
                hr = _pRecordingTexture->LockRect(0, &lr, NULL, 0);
                if (SUCCEEDED(hr))
                {
                    UCHAR* pRect = (UCHAR*) lr.pBits;
                    memcpy(pRect, _recIndicatorData, _recIndicatorWidth * _recIndicatorHeight * 4);
                    hr = _pRecordingTexture->UnlockRect(0);
                }
            }
        }

        // setup the vertex buffer(s)
        float startWidth, startHeight, stopWidth, stopHeight;
		startWidth = 0.0;
		startHeight = 0.0;
        stopWidth = (float) _recIndicatorWidth / (float) _deviceWidth;
        stopHeight = (float) _recIndicatorHeight / (float) _deviceHeight;
        UpdateVerticeBuffer(_deviceWidth, _deviceHeight,
            startWidth, startHeight, stopWidth, stopHeight);
    }
    else
    {
        // if we don't have a device this isn't an error
        hr = S_OK;
    }

    return hr;
}

HRESULT D3DVideoRenderer::UpdateOffScreenSurface()
{
    HRESULT hr = E_FAIL;

    if(_renderFrame)
    {
        int frameWidth = 0, frameHeight = 0;
        if(_renderYUV)
        {
            I420VideoFrame* renderI420Buffer = dynamic_cast<I420VideoFrame*>(_renderFrame->LockBuffer(&frameWidth, &frameHeight, NULL));
            if(renderI420Buffer && _pOffSurface)
            {
                D3DLOCKED_RECT lr;
                hr = _pOffSurface->LockRect(&lr,NULL,0);
                if (SUCCEEDED(hr))
                {
                    int y_stride = renderI420Buffer->stride(kYPlane);
                    int u_stride = renderI420Buffer->stride(kUPlane);
                    int v_stride = renderI420Buffer->stride(kVPlane);

                    if(lr.Pitch == y_stride)
                    {
                        UCHAR* pRect = (UCHAR*) lr.pBits;

                        memcpy(pRect, renderI420Buffer->buffer(kYPlane),  y_stride*frameHeight);  // y-plane
                        pRect += y_stride * frameHeight;

                        memcpy(pRect,renderI420Buffer->buffer(kVPlane), v_stride*(frameHeight>>1));  //v-plane
                        pRect += v_stride * (frameHeight>>1);

                        memcpy(pRect,renderI420Buffer->buffer(kUPlane), u_stride*(frameHeight>>1));  //u-plane
                        _isFrameCopied = true;
                    }
                    _pOffSurface->UnlockRect();
                }
            }
        }
        else
        {
            RGBVideoFrame* renderI420Buffer = dynamic_cast<RGBVideoFrame*>(_renderFrame->LockBuffer(NULL,NULL,NULL));
            if(renderI420Buffer && _pTexture)
            {
                D3DLOCKED_RECT lr;
                hr = _pTexture->LockRect(0, &lr, NULL, 0);
                if (SUCCEEDED(hr))
                {
                    UCHAR* pRect = (UCHAR*) lr.pBits;
                    memcpy(pRect, renderI420Buffer->buffer(), renderI420Buffer->width() * renderI420Buffer->height() * 4);
                    _isFrameCopied = true;
                    hr = _pTexture->UnlockRect(0);
                }
            }
        }
        _renderFrame->UnlockBuffer();
    }

    _updateTexturePending = false;

    return hr;
}

HRESULT D3DVideoRenderer::UpdateVerticeBuffer(int width, int height,
	float startWidth, float startHeight, float stopWidth, float stopHeight)
{
    HRESULT hr = E_FAIL;

    // at the moment the video texture is alway the full window size
    float left, right, top, bottom;
    // 0,1 => -1,1
    left =  0.0f * 2.0f - 1.0f - (1.0f/(float)width);
    right = 1.0f * 2.0f - 1.0f - (1.0f/(float)height);
    //0,1 => 1,-1
    top =    1.0f - 0.0f * 2.0f + (1.0f/(float)width);
    bottom = 1.0f - 1.0f * 2.0f + (1.0f/(float)height);

    // recording indicator texture, scaled according to passed in values
    float left2, right2, top2, bottom2;
    // 0,1 => -1,1
    left2 = startWidth * 2.0f - 1.0f - (1.0f/(float)width);
    right2 = stopWidth * 2.0f - 1.0f - (1.0f/(float)height);
    //0,1 => 1,-1
    top2 = 1.0f - startHeight * 2.0f + (1.0f/(float)width);
    bottom2 = 1.0f - stopHeight * 2.0f + (1.0f/(float)height);

    CUSTOMVERTEX newVertices[] = {
        // video texture
        { left,  bottom, 0.0f, 0xffffffff, 0, 1 },
        { left,  top,    0.0f, 0xffffffff, 0, 0 },
        { right, bottom, 0.0f, 0xffffffff, 1, 1 },
        { right, top,    0.0f, 0xffffffff, 1, 0 },
        // rec indicator texture
        { left2,  bottom2, 0.0f, 0xffffffff, 0, 1 },
        { left2,  top2,    0.0f, 0xffffffff, 0, 0 },
        { right2, bottom2, 0.0f, 0xffffffff, 1, 1 },
        { right2, top2,    0.0f, 0xffffffff, 1, 0 },
    };

    if (_pVB)
    {
        // Now we fill the vertex buffer.
        void* pVertices = NULL;
        hr = _pVB->Lock(0, sizeof(newVertices), (void**) &pVertices, 0);
        {
	        memcpy(pVertices, newVertices, sizeof(newVertices));
		    _pVB->Unlock();
	    }
    }

    return hr;
}

HRESULT D3DVideoRenderer::UpdateRenderSurface(int width, int height)
{
    HRESULT hr = E_FAIL;

    if (_pd3dDevice)
    {
        // Clear the backbuffer to the plugin window color
        hr = _pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET,
             D3DCOLOR_XRGB(0,0,0), 1.0f, 0);

        // if we don't have any data to display exit here
        if ( ((_pOffSurface == NULL) && (_pTexture == NULL)) || _blackness)
        {
            // this will present black from the Clear() call above
            _pd3dDevice->Present(NULL, NULL, NULL, NULL);
            return hr;
        }

        if(_isFrameCopied)  // Presenting the backbuffer data only if there is a video frame to be rendered.
        {
            if(_renderYUV)  // Surface rendering for YV12 data
            {
                hr = _pd3dDevice->StretchRect(_pOffSurface, NULL, _pTextureSurface, NULL, D3DTEXF_LINEAR);
            }
            // Begin the scene
            if (SUCCEEDED(_pd3dDevice->BeginScene()))
            {
                // We don't change the relative size/position of the textures in
                // the window unless the window size changes so the vertex buffers
                // are setup when we create the textures, this saves updating them
                // here as they don't change unless the window size change at which
                // point we reset the textures anyway.
                _pd3dDevice->SetStreamSource(0, _pVB, 0, sizeof(CUSTOMVERTEX));
                _pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);

                _pd3dDevice->SetTexture(0, _pTexture);
                _pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);

                // draw the recording indicator over the video, if needed
                if (ShouldRenderRecordingIndicator() && _pRecordingTexture)
                {
                    _pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
                    _pd3dDevice->SetTexture(0, _pRecordingTexture);
                    _pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 4, 2);
                    _pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
                }

                // End the scene
                _pd3dDevice->EndScene();
            }
        }
        // Present the backbuffer contents to the display
        hr = _pd3dDevice->Present(NULL, NULL, NULL, NULL);
    }

    return hr;
}

void CALLBACK D3DVideoRenderer::StaticIntervalTimer(void* param, UINT id)
{
    D3DVideoRenderer* obj = (D3DVideoRenderer*)param;
    if (obj)
    {
        obj->IntervalTimer(id);
    }
}

void D3DVideoRenderer::IntervalTimer(UINT id)
{
    // currently we only care about frequent redraws if we are
    // displaying the recording indicator, check for that here
    if (_recIndicatorData && (_recIndicatorTimerId == id))
    {
        DWORD now = GetTickCount();
        if (now > (_lastRedraw + 200))
        {
            // we haven't redrawn for 200 ms, force it to
            // happen now so we update the recording indicator
            // TODO: what if we don't have a HWND?
            InvalidateRect(_wnd, NULL, FALSE);
        }
    }
}

bool D3DVideoRenderer::ShouldRenderRecordingIndicator()
{
    bool render = false;

    if (_showRecordingIndicator)
    {
        // check to see if we should display the indicator, we show it for
        // approx 0.7 seconds and hide it for approx 0.3 seconds
        DWORD now = GetTickCount();
        if (_recIndicatorOn)
        {
            // has it been on for more than 0.7 seconds
            if (now > (_recIndicatorLastToggle + UIWidget::kUIWidgetRecIndicatorOnTime))
            {
                _recIndicatorOn = false;
                _recIndicatorLastToggle = now;
            }
        }
        else
        {
            // has it been off for more than 0.3 seconds
            if (now > (_recIndicatorLastToggle + UIWidget::kUIWidgetRecIndicatorOffTime))
            {
                _recIndicatorOn = true;
                _recIndicatorLastToggle = now;
            }
        }

        render = _recIndicatorOn;
    }

    return render;
}

void D3DVideoRenderer::SetRecordingIndicatorData(BYTE* data, int width, int height)
{
    if (_recIndicatorData)
    {
        delete [] _recIndicatorData;
        _recIndicatorData = NULL;
    }
    if (_recIndicatorBitmap)
    {
        DeleteObject(_recIndicatorBitmap);
        _recIndicatorBitmap = NULL;
    }

    _recIndicatorData = new BYTE[width * height * 4];
    if (_recIndicatorData)
    {
        _recIndicatorWidth = width;
        _recIndicatorHeight = height;
        memcpy(_recIndicatorData, data, width * height * 4);

        // we also create a bitmap from the image data
        BITMAPINFO bminfo;
        ZeroMemory(&bminfo, sizeof(bminfo));
        bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bminfo.bmiHeader.biWidth = width;
        bminfo.bmiHeader.biHeight = -((LONG) height);
        bminfo.bmiHeader.biPlanes = 1;
        bminfo.bmiHeader.biBitCount = 32;
        bminfo.bmiHeader.biCompression = BI_RGB;
 
        // create a DIB section that can hold the image
        void * pvImageBits = NULL;
        HDC hdcScreen = GetDC(NULL);
        _recIndicatorBitmap = CreateDIBSection(hdcScreen, &bminfo, DIB_RGB_COLORS, &pvImageBits, NULL, 0);
        ReleaseDC(NULL, hdcScreen);
        if (_recIndicatorBitmap)
        {
            // copy the data to the bitmap
            memcpy(pvImageBits, data, width * height * 4);
        }

        // TODO: update the texture....
    }
}

void D3DVideoRenderer::ShowRecordingIndicator(bool show)
{
    bool oldValue = _showRecordingIndicator;
    _showRecordingIndicator = show;
    if (oldValue != _showRecordingIndicator)
    {
        // status has changed refresh UI
    }
}

int D3DVideoRenderer::DeliverFrame(webrtc::BASEVideoFrame *videoFrame)
{
    // flag that the texture needs to be
    // updated on the main d3d thread
    if(_renderFrame)
    {
        _renderFrame->CopyFrame(videoFrame);   // Copy the videoFrame to the render frame buffer to share with the render thread.
        _updateTexturePending = true;
    }
    // the calling function will force a window repaint
    return 0;
}

void D3DVideoRenderer::Redraw()
{
    // this is called from WM_PAINT
    _lastRedraw = GetTickCount();

    bool renderWithGDI = false;
    RECT rcClient;
    GetClientRect(_wnd, &rcClient);
    int width = rcClient.right - rcClient.left;
    int height = rcClient.bottom - rcClient.top;

    // if we have had too many D3D renderer failures, switch to GDI
    // or we may be in GDI fallback mode where we always use GDI
    if (_d3dFailureCount > 200)
    {
        renderWithGDI = true;

        if (!_loggedGDISwitch)
        {
            LOG(LS_INFO) << "Switching D3D rendering to GDI as we have had too many D3D failures";
            _loggedGDISwitch = true;
        }
    }

    // Are we in GDI fallback mode where we always use GDI
    if (_gdiFallbackMode)
    {
        renderWithGDI = true;
    }

    if (!renderWithGDI)
    {
        // check if we are on the same monitor as the
        // current device, if not reset the device
        bool resetDevice = false;
        HMONITOR currentMonitor = MonitorFromWindow(_wnd, MONITOR_DEFAULTTOPRIMARY);
        if (currentMonitor != _deviceMonitor)
        {
            resetDevice = true;
        }

        const int maxDeviceResetAttempts = 3;
        HRESULT hr = S_OK;
        int retryCount = 0;
        do
        {
            // if we havent recieved any video data yet (buffer will be NULL)
            // then don't bother with d3d setup as we don't have a frame to render
            int frameWidth, frameHeight;
            if(_renderFrame)
            {
                BASEVideoFrame* buffer = _renderFrame->LockBuffer(&frameWidth, &frameHeight, NULL);
                if (buffer)
                {
                    if ((_pd3dDevice == NULL) || resetDevice
                        || (width != _deviceWidth) || (height != _deviceHeight))
                    {
                        ResetDevice(width, height);
                        ResetOffScreenSurface(frameWidth, frameHeight);
                    }
                    else if (_resetTexturePending
                        || (frameWidth != _surfaceWidth)
                        || (frameHeight != _surfaceHeight))
                    {
                        ResetOffScreenSurface(frameWidth, frameHeight);
                    }
                    if (_updateTexturePending)
                    {
                       UpdateOffScreenSurface();
                    }
                    _renderFrame->UnlockBuffer();
                }
            }
            hr = UpdateRenderSurface(width, height);
            if (SUCCEEDED(hr))
            {
                // quit the retry loop as rendering worked
                retryCount = maxDeviceResetAttempts + 1;
            }
            else
            {
                // loop will reset the device and try again...
                resetDevice = true;
            }
            retryCount++;
        } while (retryCount <= maxDeviceResetAttempts);

        if (SUCCEEDED(hr))
        {
            // mark the window as painted
            ValidateRect(_wnd, NULL);
        }
        else
        {
            // increment the failure count, so that if something
            // had gone bad with d3d we switch to GDI for the
            // remainder of the lifetime of this object
            _d3dFailureCount++;
        }
    }
    else
    {
        if(_renderFrame)
        {
            if(_renderFrame->GetFrameType() == kVideoI420)  // For GDI fallback scenario, swtiching back to RGB rendering.
            {
                _renderFrame->Reset(kVideoARGB);
            }
            // render with GDI
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(_wnd, &ps);
            DrawIntoDC(hdc, 0, 0, rcClient.right, rcClient.bottom);
            EndPaint(_wnd, &ps);
        }
    }
}

void D3DVideoRenderer::DrawIntoDC(HDC hDC, int x, int y, int cx, int cy)
{
    BITMAPINFO bmi;
    RGBVideoFrame* renderRGBbuffer = dynamic_cast<RGBVideoFrame*>(_renderFrame->LockBuffer(NULL, NULL, &bmi));
    if (renderRGBbuffer)
    {
        int oldMode = SetStretchBltMode(hDC, HALFTONE);
        StretchDIBits(hDC, x, y, cx, cy,
            0, 0, bmi.bmiHeader.biWidth, -bmi.bmiHeader.biHeight,
            renderRGBbuffer->buffer(), &bmi, DIB_RGB_COLORS, SRCCOPY);
        SetStretchBltMode(hDC, oldMode);
        _renderFrame->UnlockBuffer();
    }
    else
    {
        RECT rc = { x, y, x + cx, y + cy };
        FillRect(hDC, &rc, (HBRUSH) GetStockObject(BLACK_BRUSH));
    }

    if (ShouldRenderRecordingIndicator())
    {
        // draw the record indicator with alpha
        HDC hdcAlpha = CreateCompatibleDC(hDC);
        HGDIOBJ hOldBitmap = SelectObject(hdcAlpha, _recIndicatorBitmap);
        BLENDFUNCTION blend;
        blend.BlendOp = AC_SRC_OVER;
        blend.BlendFlags = 0;
        blend.SourceConstantAlpha = 255;
        blend.AlphaFormat = AC_SRC_ALPHA;

        int srcX = 0;
        int dstX = 0;
        int dstY = 0;
        int dstCX = _recIndicatorWidth;
        int dstCY = _recIndicatorHeight;
        AlphaBlend(hDC, dstX, dstY, dstCX, dstCX,
            hdcAlpha, srcX, 0, dstCX, dstCY, blend);

        SelectObject(hdcAlpha, hOldBitmap);
        DeleteDC(hdcAlpha);
    }
}
