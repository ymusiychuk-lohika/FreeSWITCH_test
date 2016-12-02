#ifndef __SCREEN_CAP_BORDER_H_
#define __SCREEN_CAP_BORDER_H_

#include "../screen_capturer.h"
#include "video_capture_defines.h"
#include <string>

namespace BJN
{
    // Class to create the infobar, manage its state
    // and respond to messages posted by windows and
    // other modules in the application.
    class GreenBorderInfoBar
    {
    public:
        GreenBorderInfoBar(void);
        ~GreenBorderInfoBar(void);
        void Init(HINSTANCE hInstance, HMONITOR hMonitor, COLORREF rgbColor);
        void Close();
        void Reposition(const RECT& rect);

    private:
        // GreenBorderBroadcastMessage structure will be passed to
        // EnumWindowCallback procedures. It contains information about
        // the message to be posted and a pointer to the object which is
        // posting the message.
        struct GreenBorderBroadcastMessage {
            GreenBorderInfoBar* pInfoBar;
            UINT                message;
            WPARAM              wParam;
            LPARAM              lParam;
        };

    private:
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        bool OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);
        void NotifyInit();
        void NotifyStopButtonPressed();
        bool IsPointInStopArea(POINT pt, bool isScreenCoordinate);
        void BroadcastMessageToSlave(const GreenBorderBroadcastMessage& msg) const;
        static BOOL CALLBACK EnumThreadWindowsProc(HWND hwnd, LPARAM lParam);
        static BOOL CALLBACK EnumChildWindowsProc(HWND hwnd, LPARAM lParam);

    private:
        HWND                    _hwnd;
        HBRUSH                  _hGreenBrush;
        HBRUSH                  _hRedBrush;
        std::wstring            _stopText;
        std::wstring            _titleText;
        HMONITOR                _hMonitor;
        RECT                    _rcMon;
        HCURSOR                 _hMoveCursor;
        HCURSOR                 _hStopCursor;
        DWORD                   _dwProcId;
        UINT                    _borderToSlaveMessageId;
        HBRUSH                  _hWhiteBrush;
        BOOL                    _bIsWindows8OrGreater;
        POINT                   _bDragStartPos;
        BOOL                    _bAllowDrag;
    };

    static const int kBorderThickness = 5;
    static const int kNumberBorderWindows = 4;

    class BorderWin
    {
    public:
        BorderWin(void);
        ~BorderWin(void);
        /*
        * rgbColor rgb color value in hexadecimal
        * default color is green = 0x5ff500
        * r = 0x5f
        * g = 0xf5
        * b = 0x00
        */
        void Create(HMONITOR hMon, COLORREF rgbColor, bool enableInfoBar = false);
        void Destroy();
        void GetWinHandles(HWND* winHandles, UINT count);

    private:
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        bool OnMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);
        void Reposition();
        void CalcRects();

    private:
        HWND                    _hwnd[kNumberBorderWindows];
        RECT                    _rect[kNumberBorderWindows];
        HBRUSH                  _hBrush;
        HINSTANCE               _hInstance;
        BYTE                    _borderAlpha;
        HMONITOR                _hMonitor;
        GreenBorderInfoBar      _infoBar;
        bool                    _enableInfoBar;
    };
}

#endif // __SCREEN_CAP_BORDER_H_

