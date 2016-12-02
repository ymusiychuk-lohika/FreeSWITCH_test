/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef _BJN_D3D_RENDER_H_
#define _BJN_D3D_RENDER_H_

#include <d3d9.h>
#include "video_render_defines.h"
#include "critical_section_wrapper.h"

#pragma comment(lib, "d3d9.lib")       // located in DirectX SDK

class WinBjnRenderBuffer;

class D3DVideoRenderer {
public:
    D3DVideoRenderer(HWND win, bool enableHwColorConv);
    ~D3DVideoRenderer();

    bool Init(bool allowGDIFallback, bool forceGDI);
    int DeliverFrame(webrtc::BASEVideoFrame* videoFrame);
    void setHandle(HWND wnd) { _wnd = wnd; }
    void DrawIntoDC(HDC hDC, int x, int y, int cx, int cy);
    void Redraw();
    void RenderBlackness(bool black) { _blackness = black; }
    void SetRecordingIndicatorData(BYTE* data, int width, int height);
    void ShowRecordingIndicator(bool show);
    bool IsHwColorConv() { return _isHwColorConvEnabled; };

private:
    void ReleaseAllD3DObjects();
    HRESULT ResetDevice(int width, int height);
    HRESULT ResetOffScreenSurface(int width, int height);
    HRESULT UpdateOffScreenSurface();
    HRESULT UpdateRenderSurface(int width, int height);
    HRESULT UpdateVerticeBuffer(int width, int height,
	    float startWidth, float startHeight, float stopWidth, float stopHeight);
    bool ShouldRenderRecordingIndicator();
    static void CALLBACK StaticIntervalTimer(void* param, UINT id);
    void IntervalTimer(UINT id);

private:
    HWND                    _wnd;
    WinBjnRenderBuffer*       _renderFrame;  //frame to be rendered
    // d3d members
    LPDIRECT3D9             _pD3D;
    LPDIRECT3DDEVICE9       _pd3dDevice;
    LPDIRECT3DTEXTURE9      _pTexture;
    LPDIRECT3DTEXTURE9      _pRecordingTexture;
    LPDIRECT3DVERTEXBUFFER9 _pVB;
    LPDIRECT3DSURFACE9      _pTextureSurface;  //Surface pointer of _pTexture
    LPDIRECT3DSURFACE9      _pOffSurface;     // Off screen surface to hold the data to be rendered (YV12)
    D3DPRESENT_PARAMETERS   _d3dpp;
    HMONITOR                _deviceMonitor;
    int                     _deviceWidth;
    int                     _deviceHeight;
    int                     _surfaceWidth;
    int                     _surfaceHeight;
    int                     _d3dFailureCount;
    BYTE*                   _recIndicatorData;
    HBITMAP                 _recIndicatorBitmap;
    int                     _recIndicatorWidth;
    int                     _recIndicatorHeight;
    DWORD                   _recIndicatorLastToggle;
    bool                    _recIndicatorOn;
    UINT                    _recIndicatorTimerId;
    DWORD                   _lastRedraw;
    bool                    _showRecordingIndicator;
    bool                    _canAlphaBlendTexture;
    // control members
    bool                    _resetTexturePending;
    bool                    _updateTexturePending;
    bool                    _gdiFallbackMode;
    bool                    _loggedGDISwitch;
    bool                    _blackness;
    bool                    _isFrameCopied;//flag to prevent rendering the backbuffer data if there is no frame received for rendering
    bool                    _renderYUV;// flag to indicate the direct YV12 rendering
    bool                    _isHwColorConvEnabled; //feature enablement flag for HW Color conversion
};

#endif // _BJN_D3D_RENDER_H_
