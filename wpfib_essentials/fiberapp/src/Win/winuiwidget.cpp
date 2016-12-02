#include "talk/base/logging_libjingle.h"
#include "talk/base/common.h"
#include "modules/video_capture/screen_capturer_defines.h"
#include "winuiwidget.h"
#include "bjnrenderer.h"
#include "winbjnrenderer.h"
#include "resource.h"
#include <stdio.h>
#include <TlHelp32.h>


WinBasicLabel::WinBasicLabel()
    : WindowProcBase()
    , _hBitmapSprites(NULL)
    , _showRecordingIndicator(false)
{
    _recIndicatorRect.left = 0;
    _recIndicatorRect.top = 0;
    _recIndicatorRect.right = UIWidget::kButtonWidth;
    _recIndicatorRect.bottom = UIWidget::kButtonHeight;
}

WinBasicLabel::~WinBasicLabel()
{
}

bool WinBasicLabel::Create(HWND hwndParent, int x, int y, int cx, int cy)
{
    WNDCLASSEX wc;

    wc.cbSize = sizeof(wc);
    wc.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;
    wc.lpfnWndProc = NULL;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = NULL;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = L"BJNBasicLabel";
    wc.hIconSm = NULL;

    RegisterWndClass(&wc);

    bool ok = WindowProcBase::Create(WS_EX_NOACTIVATE,
        L"", WS_CHILD,
        x, y, cx, cy, hwndParent, NULL);

    // default text that can be overridden from the API
    if (_text.empty())
    {
        _text = L"No Video";
    }

    _hBitmapSprites = LoadPNGToBitmap(_hInstance, MAKEINTRESOURCE(IDC_BUTTONSPRITES));
    //_recordingButton.Create(_hwnd, _hInstance, &_recIndicatorRect, 0, UIWidget::kUIWidgetButtonRecording);
    //_recordingButton.SetupButtonStates(_hBitmapSprites, 300, 25, 1, ToolbarButton::kToolbarButtonIndicator);

    //_showRecordingIndicator = false;
    //_recordingButton.Show(_showRecordingIndicator);

    return ok;
}

void WinBasicLabel::Destroy()
{
    //_recordingButton.Destroy();
    WindowProcBase::Destroy();
    if (_hBitmapSprites)
    {
        DeleteObject(_hBitmapSprites);
        _hBitmapSprites = NULL;
    }
}

void WinBasicLabel::Show(bool show)
{
    ShowWindow(_hwnd, show ? SW_SHOWNA : SW_HIDE);
}

void WinBasicLabel::ShowRecordingIndicator(bool show)
{
    bool oldValue = _showRecordingIndicator;
    _showRecordingIndicator = show;
    //if (oldValue != _showRecordingIndicator)
    //{
    //    // status has changed refresh UI
    //    _recordingButton.Show(_showRecordingIndicator);
    //}
}

void WinBasicLabel::SetButtonTooltipText(int button, int state, const WCHAR* text)
{
    /*if (button == UIWidget::kUIWidgetButtonRecording)
    {
        _recordingButton.SetToolTipTextForState(state, text);
    }*/
}

void WinBasicLabel::Resize(int width, int height)
{
    SetWindowPos(_hwnd, NULL,
        0, 0, width, height,
        SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
}

void WinBasicLabel::SetText(LPCWSTR text)
{
    _text = text;
    if (_hwnd && IsWindowVisible(_hwnd))
    {
        InvalidateRect(_hwnd, NULL, FALSE);
    }
}

bool WinBasicLabel::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    bool handled = false;

    switch (uMsg)
    {
        case WM_ERASEBKGND:
        {
            // make sure we don't do any painting here
            // it is all handled in OnPaint()
            result = 1;
            handled = true;
            break;
        }
        case WM_PAINT:
        {
            OnPaint();
            result = 0;
            handled = true;
            break;
        }
        case WM_NCHITTEST:
        {
            // we need to pass mouse messages through to the parent window
            // returning HTTRANSPARENT will pass the events to the parent window
            result = HTTRANSPARENT;
            // we exclude the recording indicator so it gets mouse messages for tooltips
            //if (_recordingButton.IsVisible())
            //{
            //    POINT pt, ptClient;
            //    pt.x = GET_X_LPARAM(lParam);
            //    pt.y = GET_Y_LPARAM(lParam);
            //    ptClient = pt;
            //    // convert to client co-ords
            //    ScreenToClient(_hwnd, &ptClient);
            //    if (_recordingButton.PointInRect(ptClient))
            //    {
            //        result = HTCLIENT;
            //    }
            //}
            handled = true;
            break;
        }
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MOUSEMOVE:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        {
            // is the mouse over the recording indicator?
            DWORD msgPos = GetMessagePos();
            POINT pt, ptClient;
            pt.x = GET_X_LPARAM(msgPos);
            pt.y = GET_Y_LPARAM(msgPos);
            ptClient = pt;
            // convert to client co-ords
            ScreenToClient(_hwnd, &ptClient);
            /*if (_recordingButton.PointInRect(ptClient))
            {
                MSG msg = { 0 };
                msg.hwnd = _hwnd;
                msg.message = uMsg;
                msg.wParam = wParam;
                msg.lParam = lParam;
                msg.pt = pt;
                msg.time = GetMessageTime();
                _recordingButton.RelayToolTipMessage(&msg);
            }*/
            break;
        }
    }

    return handled;
}

void WinBasicLabel::OnPaint()
{
	PAINTSTRUCT ps;
	HDC wdc = BeginPaint(_hwnd, &ps);
	if (wdc){
		RECT rect;
		GetClientRect(_hwnd, &rect);
		FillRect(wdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
		SetTextColor(wdc, 0x00000000);
		SetBkMode(wdc, TRANSPARENT);
		rect.left = 0;
		rect.top = 0;
		DrawText(wdc, _text.c_str(), -1, &rect, DT_SINGLELINE | DT_NOCLIP);
		DeleteDC(wdc);
	}
	EndPaint(_hwnd, &ps);

    //PAINTSTRUCT ps;
    //HDC hdc = BeginPaint(_hwnd, &ps);
    //if (hdc)
    //{
    //    RECT rc;
    //    GetClientRect(_hwnd, &rc);

    //    HDC hdcMem = CreateCompatibleDC(hdc);
    //    HBITMAP hbmpMem = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
    //    HGDIOBJ hbmpOld = SelectObject(hdcMem, (HGDIOBJ)hbmpMem);
    //    FillRect(hdcMem, &rc, (HBRUSH) GetStockObject(BLACK_BRUSH));

    //    // draw the text
    //    // always assume full widget size for the text position...
    //    RECT textRect;
    //    GetClientRect(GetParent(_hwnd), &textRect);
    //    int height = textRect.bottom;

    //    int oldBkMode = SetBkMode(hdcMem, TRANSPARENT);
    //    COLORREF oldColor = SetTextColor(hdcMem,
    //        RGB(UIWidget::kUIWidgetNoVideoTextRed,
    //            UIWidget::kUIWidgetNoVideoTextGreen,
    //            UIWidget::kUIWidgetNoVideoTextBlue));
    //    // calculate the height of the text bounding rect
    //    // First add a margin
    //    const int margin = UIWidget::kUIWidgetNoVideoTextMargin;
    //    InflateRect(&textRect, -margin, -margin);
    //    RECT rcCalc = textRect;
    //    DrawText(hdcMem, _text.c_str(), -1, &rcCalc, DT_CENTER | DT_WORDBREAK | DT_CALCRECT);
    //    // rcCalc.bottom has been adjusted to be the text bouinding
    //    // box, now we need to center it vertically
    //    int adjustment = ((height - (margin * 2)) - (rcCalc.bottom - rcCalc.top)) / 2;
    //    OffsetRect(&textRect, 0, adjustment);
    //    // now we can draw the text which will be vertically centered
    //    DrawText(hdcMem, _text.c_str(), -1, &textRect, DT_CENTER | DT_WORDBREAK);
    //    SetTextColor(hdcMem, oldColor);
    //    SetBkMode(hdcMem, oldBkMode);

    //    // draw the recording indicator if needed
    //    if (_showRecordingIndicator)
    //    {
    //        _recordingButton.DrawButton(hdcMem);
    //    }

    //    BitBlt(hdc, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
    //        hdcMem, 0, 0, SRCCOPY);

    //    SelectObject(hdcMem, hbmpOld);
    //    DeleteObject(hbmpMem);
    //    DeleteDC(hdcMem);

    //    EndPaint(_hwnd, &ps);
    //}
}


WinUIWidget::WinUIWidget(void* window)
    : WindowProcBase()
    , UIWidgetBase()
    //, _recIndicatorTooltip()
    , _visible(false)
    , _hBitmapSprites(NULL)
    , _mouseOver(true)
    , _lastMouseEventTime(0)
    , _lastMousePosition(0)
    , _flags(kUIWidgetFlagsNormal)
    , _d3dRenderer(NULL)
    , _srcWidth(0)
    , _srcHeight(0)
    , _showRecordingIndicator(false)
    , _inSizeMove(false)
    , _dragCorner(WMSZ_BOTTOMRIGHT)
    , _anchorCorner(WMSZ_TOPLEFT)
    , _resizeBorderThickness(0)
    , _normalHeight(kUIWidgetDefaultHeight)
    , _lastSizingWidth(0)
    , _lastSizingHeight(0)
{
    _cs = webrtc::CriticalSectionWrapper::CreateCriticalSection();
	_hwnd = (HWND)window;
}

WinUIWidget::~WinUIWidget()
{
    Close();

    delete _cs;
}

bool WinUIWidget::Init()
{
    WNDCLASSEX wc;

    wc.cbSize = sizeof(wc);
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = NULL;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = NULL;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject(GRAY_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = TEXT(SCREEN_SHARING_WIDGET_NAME);
    wc.hIconSm = NULL;

    RegisterWndClass(&wc);

    WCHAR widgetTitle[32];

    // the widget uses the PID as a text string for it's window title, this is so
    // code in the screen capturer can find the right widget if more than one
    // process is running at the same time.
    swprintf_s(widgetTitle, L"%u", GetCurrentProcessId());
	
	// Create the Window only if it did not exist earlier - i.e was not created by fiberapp and passed
	if (!_hwnd) {
		bool ok = WindowProcBase::Create(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
			widgetTitle, WS_POPUP | WS_CLIPCHILDREN,
			40, 30, kUIWidgetDefaultWidth, kUIWidgetDefaultHeight, NULL, NULL);
	}
    UpdateWindowRegion();

    // make our layered window a real layered window here, we set the alpha
    // to 1 less than fully opaque so that the window isn't captured with the screen
    SetLayeredWindowAttributes(_hwnd, RGB(0xff, 0xff, 0xff), 255, LWA_ALPHA);

    SetTimer(_hwnd, kPeriodicTimerId, 250, NULL);

    /*if(_slaveJSPtr->isJF3WidgetNeeded())
    {
        _toolbar.SetJF3State(true);
    }*/
   /* _toolbar.Create(_hwnd, 0, kUIWidgetDefaultHeight - kUIWidgetToolbarHeight,
        kUIWidgetDefaultWidth, kUIWidgetToolbarHeight);
    _toolbar.Show(true);*/

    _noVideoLabel.Create(_hwnd, 0, 0, kUIWidgetDefaultWidth, kUIWidgetDefaultHeight);

    // default to the recording indicator being hidden
    _showRecordingIndicator = false;
    //// create a tooltip for the recording indicator area
    //_recIndicatorTooltip.Create(_hwnd, 0, 0, UIWidget::kButtonWidth, UIWidget::kButtonHeight);
    //// set the text if it has been set already
    //_recIndicatorTooltip.SetText(_recIndicatorTooltipText.c_str());

    // protect access to the d3d object
    _cs->Enter();
    _srcWidth = 0;
    _srcHeight = 0;

    bool d3dHwColorConv = false;
    WinBjnRenderer* winRenderer = dynamic_cast<WinBjnRenderer*>(_renderer);
    if(winRenderer)
        d3dHwColorConv = winRenderer->IsHWColorConv();  // retrieve the feature enablement flag for HW Color conversion

    LOG(LS_INFO) << "Hardware Color Conversion for UI Widget:"<<d3dHwColorConv;

    _d3dRenderer = new D3DVideoRenderer(_hwnd,d3dHwColorConv);
    if (_d3dRenderer)
    {
        // load the recording indicator image into the renderer
        BYTE recIndicatorImage[kButtonWidth * kButtonHeight * 4];
        // the rect is the position of the indicator image in the sprite strip
        RECT rcRecIndicator =  { 300, 0, 325, kButtonHeight };
        if (LoadPNGToMemory(_hInstance, MAKEINTRESOURCE(IDC_BUTTONSPRITES), &rcRecIndicator,
            recIndicatorImage, sizeof(recIndicatorImage)))
        {
            _d3dRenderer->SetRecordingIndicatorData(recIndicatorImage, kButtonWidth, kButtonHeight);
        }

        // if D3D has been disabled in config.xml we need to respect that in the
        // widget. As we don't have easy access to the config object we can get
        // the preference from the renderer that we are associated with.
        bool forceGDI = false;
        if (winRenderer)
        {
            forceGDI = !winRenderer->IsD3DEnabled();
        }

        // On XP layered windows and d3d rendering don't work very well with
        // a lot of the d3d drivers so we disable d3d rendering in the widget
        // when we are running on XP
        OSVERSIONINFO os = { 0 };
        os.dwOSVersionInfoSize = sizeof(os);
        if (GetVersionEx(&os))
        {
            // major version less that 6 is XP and below
            if (os.dwMajorVersion < 6)
            {
                forceGDI = true;
            }
        }

        // we don't need to check the error as passing false for the
        // first parameter means that the rendere will use GDI if D3D
        // isn't avialable so this always succeeds.
        _d3dRenderer->Init(true, forceGDI);
    }
    _cs->Leave();

    return true;
}

bool WinUIWidget::Close()
{
    //_toolbar.Destroy();
    _noVideoLabel.Destroy();
    //_recIndicatorTooltip.Destroy();

    _cs->Enter();
    _srcWidth = 0;
    _srcHeight = 0;
    if (_d3dRenderer)
    {
        delete _d3dRenderer;
        _d3dRenderer = NULL;
    }
    _cs->Leave();

    WindowProcBase::Destroy();

    return true;
}

bool WinUIWidget::Show(bool show, int x, int y, int width, int height, unsigned int flags)
{
    // do nothing if we are already in the correct visible state
    if (show == _visible)
    {
        bool wasMinimized = Minimized();
        // if we are visible flags may be changing
        if (show && (flags != _flags))
        {
            // save the flags
            _flags = flags;

            RECT rc;
            GetWindowRect(_hwnd, &rc);
            if (wasMinimized)
            {
                // restore to previous height
                rc.bottom = rc.top + _normalHeight;
            }
            SetWidgetPosition(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, false);
        }
        return true;
    }

    if (show)
    {
        _flags = flags;
        Init();
        SetWidgetPosition(x, y, width, height, true);
        ShowWindow(_hwnd, SW_SHOWNOACTIVATE);
    }
    else
    {
        ShowWindow(_hwnd, SW_HIDE);
        Close();
    }

    _visible = show;
    return true;
}

void WinUIWidget::UpdateWindowRegion()
{
    // clear the current region
    SetWindowRgn(_hwnd, NULL, FALSE);
    BOOL redraw = FALSE;
    if (IsWindowVisible(_hwnd))
    {
        redraw = TRUE;
    }
    RECT rc;
    GetClientRect(_hwnd, &rc);
    HRGN clipRgn = CreateRoundRectRgn(rc.left, rc.top, rc.right, rc.bottom,
        (UIWidget::kUIWidgetCornerRadius * 2), (UIWidget::kUIWidgetCornerRadius * 2));
    if (clipRgn)
    {
        // the window own this rgn handle after this
        // call, so we don't need to delete it
        SetWindowRgn(_hwnd, clipRgn, redraw);
    }
}

bool WinUIWidget::GetPosition(int& x, int& y, int& cx, int& cy)
{
    bool valid = false;

    if (IsWindow(_hwnd))
    {
        RECT rc;
        GetWindowRect(_hwnd, &rc);
        x = rc.left;
        y = rc.top;
        cx = rc.right - rc.left;
        cy = Minimized() ? _normalHeight : (rc.bottom - rc.top);
        valid = true;
    }

    return valid;
}

void WinUIWidget::SetButtonState(int button, int state)
{
    //_toolbar.UpdateButtonState(button, state);
}

void WinUIWidget::SetButtonVisible(int button, bool visible)
{
    //_toolbar.UpdateButtonVisible(button, visible);
}

void WinUIWidget::SetButtonTooltipText(int button, int state, const std::string& text)
{
    // limit tooltip to 256 characters as this string come from jscript
    WCHAR szWideBuffer[256];
    // convert text from our utf8 input to WCHAR
    if (MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1,
        szWideBuffer, _countof(szWideBuffer)))
    {
        if (button == kUIWidgetNoVideo)
        {
            // the no video text
            _noVideoLabel.SetText(szWideBuffer);
        }
        else
        {
            //_toolbar.SetToolTipTextForState(button, state, szWideBuffer);
            if ((button == kUIWidgetButtonRecording) && (state == 0))
            {
                // we have 2 other recording indicators
                // that can show the same tooltip text
                //_recIndicatorTooltip.SetText(szWideBuffer);
                _noVideoLabel.SetButtonTooltipText(button, state, szWideBuffer);
                // save the text as we need it later
                _recIndicatorTooltipText = szWideBuffer;
            }
        }
    }
}

void WinUIWidget::ShowRecordingIndicator(bool show)
{
    bool oldValue = _showRecordingIndicator;
    _showRecordingIndicator = show;
    if (oldValue != _showRecordingIndicator)
    {
        // the setting changed, update the UI
        UpdateToolbarRecordingIndicatorVisibility();
        _noVideoLabel.ShowRecordingIndicator(_showRecordingIndicator);
        if (_d3dRenderer)
        {
            _d3dRenderer->ShowRecordingIndicator(_showRecordingIndicator);
        }
    }
}

void WinUIWidget::UpdateToolbarRecordingIndicatorVisibility()
{
    // we only show the recording indicator in the toolbar when the
    // widget is in it's minimized state, as it's shown in the top
    // left of the widget in normal view, so we toggle the state
    // here depening on the widgets minimized state
    if ((_showRecordingIndicator) && Minimized())
    {
        //_toolbar.ShowRecordingIndicator(true);
    }
    else
    {
        //_toolbar.ShowRecordingIndicator(false);
    }
}

void WinUIWidget::SetWidgetPosition(int posX, int posY, int width, int height, bool checkBounds)
{
    // position the window in the location specified, if checkBounds is
    // true then make sure that the position is on screen

    int x = 0;
    int y = 0;
    int cx = width;
    int cy = height;
    if (Minimized())
    {
        cy = kUIWidgetToolbarHeight;
        _normalHeight = height;
    }

    if (checkBounds)
    {
        // has the position been specified?
        if ((posX != kUiWidgetDefaultPosition) && (posY != kUiWidgetDefaultPosition))
        {
            // we need to ensure the requested position is visible on screen
            // if not we will ignore and use the default position
            if (!IsWidgetLocationOnScreen(posX, posY, width, height))
            {
                // offscreen probably due to monitor config changes
                // reset position to defaults
                posX = kUiWidgetDefaultPosition;
                posX = kUiWidgetDefaultPosition;
            }
        }
    }

    // one final check for default position
    if ((posX == kUiWidgetDefaultPosition) && (posX == kUiWidgetDefaultPosition))
    {
        // use the default top right position
        x = GetSystemMetrics(SM_CXSCREEN) - kUIWidgetDefaultOffsetX - kUIWidgetDefaultWidth;
        y = kUIWidgetDefaultOffsetY;
        cx = kUIWidgetDefaultWidth;
        cy = kUIWidgetDefaultHeight;
    }
    else
    {
        x = posX;
        y = posY;
    }

    SetWindowPos(_hwnd, NULL, x, y, cx, cy,
        SWP_NOZORDER | SWP_NOACTIVATE);

    // make sure the toolbar is alway at the bottom of the window
    //_toolbar.ParentWindowResized();

    bool showNoVideo = false;
    bool showToolbar = true;
    // make sure we always show the toolbar when minimized
    if (Minimized())
    {
        showToolbar = true;
    }
    else
    {
        // this will hide the toolbar if the mouse isn't over our window
        showToolbar = _mouseOver;

        if (_flags & kUIWidgetFlagsNoVideo)
        {
            showNoVideo = true;
        }
    }

    // make sure we show or hide the recording indicator correctly
    UpdateToolbarRecordingIndicatorVisibility();

    // show, hide, etc.
    if (_d3dRenderer)
    {
        _d3dRenderer->RenderBlackness(showNoVideo);
    }
    _noVideoLabel.Show(showNoVideo);
    ShowToolbar(showToolbar);
}

bool WinUIWidget::IsWidgetLocationOnScreen(int x, int y, int cx, int cy)
{
    bool onScreen = false;

    // we check to see if placing the widget in the given position would
    // put the widget completly on the screen, if not we return false.

    RECT rcTemp;
    rcTemp.left = x + 1;
    rcTemp.top = y + 1;
    rcTemp.right = rcTemp.left + cx - 1;
    rcTemp.bottom = rcTemp.top + cy - 1;

    POINT ptTopLeft = { rcTemp.left, rcTemp.top };
    POINT ptTopRight = { rcTemp.right, rcTemp.top };
    POINT ptBottomLeft = { rcTemp.left, rcTemp.bottom };
    POINT ptBottomRight = { rcTemp.right, rcTemp.bottom };
    HMONITOR hMonTopLeft = MonitorFromPoint(ptTopLeft, MONITOR_DEFAULTTONULL);
    HMONITOR hMonTopRight = MonitorFromPoint(ptTopRight, MONITOR_DEFAULTTONULL);
    HMONITOR hMonBottomLeft = MonitorFromPoint(ptBottomLeft, MONITOR_DEFAULTTONULL);
    HMONITOR hMonBottomRight = MonitorFromPoint(ptBottomRight, MONITOR_DEFAULTTONULL);
    // make sure we don't have any NULL values
    if ((hMonTopLeft != NULL) && (hMonTopRight != NULL)
        && (hMonBottomLeft != NULL) && (hMonBottomRight != NULL))
    {
        // all corners are within a monitor
        onScreen = true;
    }

    return onScreen;
}

bool WinUIWidget::IsWidgetShowingVideo()
{
    bool showing = false;

    if (_visible && _hwnd
        && !Minimized()
        && !(_flags & kUIWidgetFlagsNoVideo))
    {
        showing = true;
    }

    return showing;
}

void WinUIWidget::UpdateVideo(webrtc::BASEVideoFrame* videoFrame)
{
    // if we are not visible or not showing video we don't need to redraw
    if (IsWidgetShowingVideo())
    {
        // protect access to the d3d object
        _cs->Enter();
        if (_d3dRenderer)
        {
	    _d3dRenderer->DeliverFrame(videoFrame);
        }
        _cs->Leave();

        // we have a new video frame, redraw
        InvalidateRect(_hwnd, NULL, FALSE);
    }
}
LRESULT WinUIWidget::HandleWindowMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result;
	OnMessage(uMsg, wParam, lParam, result);
	return result;
}

HWND WinUIWidget::GetWindowHandle(void)
{
	return _hwnd;
}

bool WinUIWidget::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    bool handled = false;

    // handle tooltip forwards here
    switch (uMsg)
    {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MOUSEMOVE:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        {
            if (_showRecordingIndicator)
            {
                // is the mouse over the recording indicator?
                DWORD msgPos = GetMessagePos();
                POINT pt, ptClient;
                pt.x = GET_X_LPARAM(msgPos);
                pt.y = GET_Y_LPARAM(msgPos);
                ptClient = pt;
                // convert to client co-ords
                ScreenToClient(_hwnd, &ptClient);
                /*if (_recIndicatorTooltip.PointInRect(ptClient))
                {
                    MSG msg = { 0 };
                    msg.hwnd = _hwnd;
                    msg.message = uMsg;
                    msg.wParam = wParam;
                    msg.lParam = lParam;
                    msg.pt = pt;
                    msg.time = GetMessageTime();
                    _recIndicatorTooltip.RelayToolTipMessage(&msg);
                }*/
            }
            break;
        }
    }

    switch (uMsg)
    {
    case WM_PAINT:
    {
        OnPaint();
        result = 0;
        handled = true;
        break;
    }
    case WM_ERASEBKGND:
    {
        result = 1;
        handled = true;
        break;
    }
    case WM_SIZE:
    {
        RECT rcWin;
        GetWindowRect(_hwnd, &rcWin);
        // save the widget height if we are not minimized
        if (!Minimized())
        {
            _normalHeight = rcWin.bottom - rcWin.top;
        }
        UpdateWindowRegion();
        //_noVideoLabel.Resize(rcWin.right - rcWin.left, rcWin.bottom - rcWin.top);
        ShowToolbar(true);
        //_toolbar.ParentWindowResized();
        break;
    }
    case WM_SIZING:
    {
        RECT rc = *((LPRECT) lParam);
        FixAspectRatio((LPRECT) lParam, wParam);
        _lastSizingWidth = rc.right - rc.left;
        _lastSizingHeight = rc.bottom - rc.top;
        result = TRUE;
        handled = true;
        break;
    }
    case WM_ENTERSIZEMOVE:
    {
        // make sure the toolbar is shown while we resize
        ShowToolbar(true);
        // work out the closest corner to the cursor
        RECT rc;
        GetWindowRect(_hwnd, &rc);
        DWORD msgPos = GetMessagePos();
        POINT pt;
        pt.x = GET_X_LPARAM(msgPos);
        pt.y = GET_Y_LPARAM(msgPos);
        int x = pt.x - rc.left;
        int y = pt.y - rc.top;
        int width = rc.right - rc.left;
        int height = rc.bottom - rc.top;
        if ((x < (width / 2)) && (y < (height / 2)))
        {
            _dragCorner = WMSZ_TOPLEFT;
            _anchorCorner = WMSZ_BOTTOMRIGHT;
        }
        else if ((x < (width / 2)) && (y >= (height / 2)))
        {
            _dragCorner = WMSZ_BOTTOMLEFT;
            _anchorCorner = WMSZ_TOPRIGHT;
        }
        else if ((x >= (width / 2)) && (y < (height / 2)))
        {
            _dragCorner = WMSZ_TOPRIGHT;
            _anchorCorner = WMSZ_BOTTOMLEFT;
        }
        else
        {
            _dragCorner = WMSZ_BOTTOMRIGHT;
            _anchorCorner = WMSZ_TOPLEFT;
        }
        _lastSizingWidth = 0;
        _lastSizingHeight = 0;
        _inSizeMove = true;
        break;
    }
    case WM_EXITSIZEMOVE:
    {
        _inSizeMove = false;
        // we can let the toolbar hide now
        _lastMouseEventTime = GetTickCount();
        _mouseOver = true;
        break;
    }
    case WM_NCHITTEST:
    {
        result = DefWindowProc(_hwnd, uMsg, wParam, lParam);
        if (result == HTCLIENT)
        {
            // this allows the window to be dragged by the client area
            result = HTCAPTION;
            // return HTCLIENT if we are over the recording indicator
            // so we get mouse message otherwise the tooltip wont work
            if (_showRecordingIndicator)
            {
                // is the mouse over the recording indicator?
                POINT pt, ptClient;
                pt.x = GET_X_LPARAM(lParam);
                pt.y = GET_Y_LPARAM(lParam);
                ptClient = pt;
                // convert to client co-ords
                ScreenToClient(_hwnd, &ptClient);
                /*if (_recIndicatorTooltip.PointInRect(ptClient))
                {
                    result = HTCLIENT;
                }*/
            }

            // update the last mouse event time
            _lastMouseEventTime = GetTickCount();
            _lastMousePosition = lParam;
            if (!_mouseOver)
            {
                _mouseOver = true;
                ShowToolbar(_mouseOver);
            }

            if (!Minimized())
            {
                POINT ptScreen, ptClient;
                ptScreen.x = GET_X_LPARAM(lParam);
                ptScreen.y = GET_Y_LPARAM(lParam);
                ptClient = ptScreen;
                // convert to client co-ords
                ScreenToClient(_hwnd, &ptClient);
                // check if we are over the resizing border
                result = ResizeBorderHitTest(ptClient, result);
            }
        }
        handled = true;
        break;
    }
    case WM_MOUSELEAVE:
    {
        _mouseOver = false;
        ShowToolbar(_mouseOver);
        break;
    }
    case WM_TIMER:
    {
        CheckMouseTimer();
        handled = true;
        break;
    }
    case WM_MOUSEMOVE:
    case WM_MOVE:
    {
        if (!_mouseOver)
        {
            _mouseOver = true;
            ShowToolbar(_mouseOver);
        }
        _lastMouseEventTime = GetTickCount();
        break;
    }
    case WM_COMMAND:
    {
        if (HIWORD(wParam) == BN_CLICKED)
        {
            /*if (_slaveJSPtr)
            {
                _slaveJSPtr->triggerPresenterWidgetButtonCallback(LOWORD(wParam));
            }*/
        }
        handled = true;
        break;
    }
    case WM_DISPLAYCHANGE:
    {
        // make sure the widget isn't off screen
        RECT rc;
        GetWindowRect(_hwnd, &rc);
        SetWidgetPosition(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, true);
        handled = true;
        break;
    }

    default:
        handled = false;
        break;
    }

    return handled;
}

int WinUIWidget::ResizeBorderHitTest(POINT pt, int defaultHitTest)
{
    int hitTest = defaultHitTest;

    // to allow resizing we need to check if the cursor is over our
    // virtual border as we don't draw a border around the window
    RECT rcClient, rcBorder;
    GetClientRect(_hwnd, &rcClient);
    rcBorder = rcClient;
    InflateRect(&rcBorder, -_resizeBorderThickness, -_resizeBorderThickness);
    int width = rcClient.right - rcClient.left;
    int height = rcClient.bottom - rcClient.top;
    // we make the corners larger so the user can more easily drag by a corner
    int cornerThickness = _resizeBorderThickness * 2;
    if (!PtInRect(&rcBorder, pt))
    {
        if ((pt.x < cornerThickness) && (pt.y < cornerThickness))
        {
            hitTest = HTTOPLEFT;
        }
        else if ((pt.x >= (width - cornerThickness)) && (pt.y < cornerThickness))
        {
            hitTest = HTTOPRIGHT;
        }
        else if ((pt.x < cornerThickness) && (pt.y >= (height - cornerThickness)))
        {
            hitTest = HTBOTTOMLEFT;
        }
        else if ((pt.x >= (width - cornerThickness)) && (pt.y >= (height - cornerThickness)))
        {
            hitTest = HTBOTTOMRIGHT;
        }
        else if (pt.x < _resizeBorderThickness)
        {
            hitTest = HTLEFT;
        }
        else if (pt.x >= (width - _resizeBorderThickness))
        {
            hitTest = HTRIGHT;
        }
        else if (pt.y < _resizeBorderThickness)
        {
            hitTest = HTTOP;
        }
        else if (pt.y >= (height - _resizeBorderThickness))
        {
            hitTest = HTBOTTOM;
        }
    }

    return hitTest;
}

void WinUIWidget::FixAspectRatio(LPRECT rc, WPARAM edge)
{
    // we alway anchor the 'opposite' corner from where the user is dragging
    RECT newRect = *rc;
    // modify the size to maintain a 16:9 aspect ratio
    float dragAspect = (float) (newRect.right - newRect.left)
        / (float) (newRect.bottom - newRect.top);
    float requiredAspect = 16.0f / 9.0f;
    int newWidth = newRect.right - newRect.left;
    int newHeight = newRect.bottom - newRect.top;
    // we have to use a slightly different method when sizing by a side rather than a corner
    // as when resizing by a side the rect given buy Windows only changes in one value, so we
    // must check for that otherwize the rect won't be allowed to grow.
    bool aspectFixed = false;
    if ((edge == WMSZ_TOP) || (edge == WMSZ_RIGHT) || (edge == WMSZ_BOTTOM) || (edge == WMSZ_LEFT))
    {
        // set this so we know which corner to anchor when sizing
        edge = _dragCorner;

        // check if we are growing, if now we fix the aspect below
        if ((newWidth > _lastSizingWidth) || (newHeight > _lastSizingHeight))
        {
            // we are dragging from a side so we
            if (requiredAspect < dragAspect)
            {
                // adjust the height
                newHeight = (int) ((newWidth + 0.5) / requiredAspect);
                newRect.bottom = newRect.top + newHeight;
            }
            else
            {
                // adjust the width
                newWidth = (int) ((newHeight + 0.5) * requiredAspect);
                newRect.right = newRect.left + newWidth;
            }
            aspectFixed = true;
        }
    }

    if (!aspectFixed)
    {
        if (requiredAspect < dragAspect)
        {
            // adjust the width
            newWidth = (int) ((newHeight + 0.5) * requiredAspect);
            newRect.right = newRect.left + newWidth;
        }
        else
        {
            // adjust the height
            newHeight = (int) ((newWidth + 0.5) / requiredAspect);
            newRect.bottom = newRect.top + newHeight;
        }
    }

    RECT rc1 = newRect;

    // enforce minimum size
    if (((newRect.right - newRect.left) < UIWidget::kUIWidgetDefaultWidth)
        || ((newRect.bottom - newRect.top) < UIWidget::kUIWidgetDefaultHeight))
    {
        newRect.right = newRect.left + UIWidget::kUIWidgetDefaultWidth;
        newRect.bottom = newRect.top + UIWidget::kUIWidgetDefaultHeight;
    }

    RECT rc2 = newRect;

    switch (edge)
    {
    case WMSZ_TOPLEFT:
        OffsetRect(&newRect,
            (rc->right - rc->left) - (newRect.right - newRect.left),
            (rc->bottom - rc->top) - (newRect.bottom - newRect.top));
        break;
    case WMSZ_TOPRIGHT:
        OffsetRect(&newRect,
            0,
            (rc->bottom - rc->top) - (newRect.bottom - newRect.top));
        break;
    case WMSZ_BOTTOMLEFT:
        OffsetRect(&newRect,
            (rc->right - rc->left) - (newRect.right - newRect.left),
            0);
        break;
    case WMSZ_BOTTOMRIGHT:
        // nothing to do here, the default anchor is top left
        break;
    }
    *rc = newRect;
}

void WinUIWidget::OnPaint()
{
    // this is ALWAYS caled on the same thread that creates and destroys
    // the d3d object so we don't need to take a lock to access it here
    if (_d3dRenderer)
    {
        _d3dRenderer->Redraw();
    }
    else
    {
        RECT rc;
        PAINTSTRUCT ps;
        GetClientRect(_hwnd, &rc);
        HDC hDC = BeginPaint(_hwnd, &ps);

        if (_renderer && !(_flags & kUIWidgetFlagsNoVideo))
        {
            _renderer->RenderIntoContext((void*)hDC, rc.left, rc.top,
                rc.right - rc.left, rc.bottom - rc.top);
        }
        EndPaint(_hwnd, &ps);
    }
}

void WinUIWidget::CheckMouseTimer()
{
    // if the mouse is over the window always show to toolbar
    POINT pt;
    GetCursorPos(&pt);
    // convert to client co-ords
    ScreenToClient(_hwnd, &pt);
    RECT rc;
    GetClientRect(_hwnd, &rc);
    // is the mouse over our window?
    if (PtInRect(&rc, pt))
    {
        _lastMouseEventTime = GetTickCount();
        _mouseOver = true;
    }

    if (_mouseOver)
    {
        // if the last mouse event was more than 250 ms ago
        DWORD now = GetTickCount();
        if (now > (_lastMouseEventTime + 250))
        {
            // hide the mouseover UI if visible
            _mouseOver = false;
            ShowToolbar(_mouseOver);
        }
    }
}

void WinUIWidget::ShowToolbar(bool show)
{
    // we never hide the toolbar in minimize mode
    if (Minimized() || _inSizeMove)
    {
        show = true;
    }

    // resize the no video window to match the widget area minus the toolbar
    RECT rc;
    GetWindowRect(_hwnd, &rc);
    int height = rc.bottom - rc.top;
    if (show)
    {
        height -= kUIWidgetToolbarHeight;
    }
    _noVideoLabel.Resize(rc.right - rc.left, height);

    //_toolbar.Show(show);
}
