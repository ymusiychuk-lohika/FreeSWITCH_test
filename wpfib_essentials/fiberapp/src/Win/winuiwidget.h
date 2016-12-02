#ifndef __WIN_UI_WIDGET_H
#define __WIN_UI_WIDGET_H

#include <Windows.h>
#include "critical_section_wrapper.h"
#include "uiwidget.h"
#include "WindowProcBase.h"
#include "d3d_render.h"
#include <string>
#include <Windowsx.h>

class WinBasicLabel : public WindowProcBase
{
public:
    WinBasicLabel();
    virtual ~WinBasicLabel();

    bool Create(HWND hwndParent, int x, int y, int cx, int cy);
    void Destroy();
    void Show(bool show);
    void Resize(int width, int height);
    void SetText(LPCWSTR text);
    void ShowRecordingIndicator(bool show);
    void SetButtonTooltipText(int button, int state, const WCHAR* text);

private:
    bool OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);
    void OnPaint();

private:
    std::wstring    _text;
    HBITMAP         _hBitmapSprites;
    bool            _showRecordingIndicator;
    RECT            _recIndicatorRect;
};


class WinUIWidget : public UIWidgetBase, public WindowProcBase
{
public:
    WinUIWidget(void* window);
    virtual ~WinUIWidget();

    bool Show(bool show, int posX, int posY, int width, int height, unsigned int flags);
    bool GetPosition(int& x, int& y, int& cx, int& cy);
    void SetButtonState(int button, int state);
    void SetButtonVisible(int button, bool visible);
    void SetButtonTooltipText(int button, int state, const std::string& text);
    void UpdateVideo(webrtc::BASEVideoFrame* videoFrame);
    void ShowRecordingIndicator(bool show);
	LRESULT HandleWindowMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	HWND GetWindowHandle(void);

private:
    bool Init();
    bool Close();
    bool OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);
    void OnPaint();
    void SetWidgetPosition(int x, int y, int cx, int cy, bool checkBounds);
    void CheckMouseTimer();
    void ShowToolbar(bool show);
    void UpdateWindowRegion();
    bool IsWidgetLocationOnScreen(int x, int y, int cx, int cy);
    bool IsWidgetShowingVideo();
    void UpdateToolbarRecordingIndicatorVisibility();
    int  ResizeBorderHitTest(POINT pt, int defaultHitTest);
    void FixAspectRatio(LPRECT rc, WPARAM edge);
    bool Minimized() { return ((_flags & kUIWidgetFlagsMinimize) == kUIWidgetFlagsMinimize); }

    enum { kPeriodicTimerId = 22 };

    webrtc::CriticalSectionWrapper* _cs;
    bool                    _visible;
    WinBasicLabel           _noVideoLabel;
    std::wstring            _recIndicatorTooltipText;
    HBITMAP                 _hBitmapSprites;
    bool                    _mouseOver;
    DWORD                   _lastMouseEventTime;
    LPARAM                  _lastMousePosition;
    int                     _flags;
    D3DVideoRenderer*       _d3dRenderer;
    int                     _srcWidth;
    int                     _srcHeight;
    bool                    _showRecordingIndicator;
    bool                    _inSizeMove;
    WPARAM                  _dragCorner;
    WPARAM                  _anchorCorner;
    int                     _resizeBorderThickness;
    int                     _normalHeight;
    int                     _lastSizingWidth;
    int                     _lastSizingHeight;
};

#endif // __WIN_UI_WIDGET_H
