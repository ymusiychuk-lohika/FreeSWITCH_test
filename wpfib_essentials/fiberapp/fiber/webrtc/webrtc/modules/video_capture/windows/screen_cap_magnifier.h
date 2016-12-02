#pragma once

#include "../screen_capturer.h"
#include "video_capture_defines.h"
#include <Magnification.h>
#include <map>
#include <vector>


namespace BJN
{
    class BufferWrapper;

    class MagScaleCallbackMap
    {
    public:
        MagScaleCallbackMap() : _isValidDataAvailable(false), _magnifierOp(CaptureInProgress)
        {
            _cs = webrtc::CriticalSectionWrapper::CreateCriticalSection();
        }
        ~MagScaleCallbackMap()
        {
            delete _cs;
        }
        void Add(HWND hwnd, BufferWrapper* dib)
        {
            webrtc::CriticalSectionScoped lock(_cs);
            _list[hwnd] = dib;
        }
        void Remove(HWND hwnd)
        {
            webrtc::CriticalSectionScoped lock(_cs);
            std::map<HWND, BufferWrapper*>::iterator it;
            it = _list.find(hwnd);
            if (it != _list.end())
            {
                // remove it from the list
                _list.erase(it);
            }
        }
        void Lock()
        {
            _cs->Enter();
        }
        void Unlock()
        {
            _cs->Leave();
        }
        BufferWrapper* GetBufferWrapper(HWND hwnd)
        {
            // this MUST be called with the lock!
            BufferWrapper* dib = NULL;
            std::map<HWND, BufferWrapper*>::const_iterator it = _list.find(hwnd);
            if (it != _list.end())
            {
                dib = it->second;
            }
            return dib;
        }

        // Magnifier Capture operations
        typedef enum _MagnifierOp{
            CaptureInProgress,
            CaptureSuccess,
            CaptureFailed,
        }MagnifierOp;

        // Start the capture process
        void startMagnifierOp() { _magnifierOp = CaptureInProgress; }

        // Complete the capture process
        void completeMagnifierOp(MagnifierOp op)
        {
            if (CaptureSuccess != op && CaptureFailed != op) {
                // Only success and failure is allowed to be set here!
                return;
            }
            _magnifierOp = op;
            // If once succeeded then valid data is available in buffer
            if (CaptureSuccess == _magnifierOp && false == _isValidDataAvailable) {
                _isValidDataAvailable = true;
            }
        }

        // Retrive the current magnifier operation status
        MagnifierOp getMagnifierOp() {
            return _magnifierOp;
        }

        // Is valid data available in buffer
        bool isValidDataAvailable() const {
            return _isValidDataAvailable;
        }


    private:
        webrtc::CriticalSectionWrapper* _cs;
        std::map<HWND, BufferWrapper*>  _list;
        bool                            _isValidDataAvailable;
        MagnifierOp                     _magnifierOp;
    };


    class BufferWrapper
    {
    public:
        BufferWrapper(int width, int height)
            : _pBitmapData(NULL)
            , _width(0)
            , _height(0)
        {
            _cs = webrtc::CriticalSectionWrapper::CreateCriticalSection();
            Resize(width, height);
        }
        ~BufferWrapper()
        {
            _cs->Enter();
            if (_pBitmapData)
            {
                delete [] _pBitmapData;
                _pBitmapData = NULL;
            }
            _cs->Leave();
            delete _cs;
        }

        void Resize(int width, int height)
        {
            webrtc::CriticalSectionScoped lock(_cs);
            if ((_width != width) || (_height != height))
            {
                // we need to resize the buffer
                _width = width;
                _height = height;
                if (_pBitmapData)
                {
                    delete [] _pBitmapData;
                    _pBitmapData = NULL;
                }
                _pBitmapData = new BYTE[width * height * 4];
                if (_pBitmapData)
                {
                    // zero out the buffer as it may get
                    // returned before any data is copied to it
                    memset(_pBitmapData, 0, width * height * 4);
                }
            }
        }

        BYTE* Lock(int& width, int& height)
        {
            width = _width;
            height = _height;
            if (_pBitmapData)
            {
                // we only lock if we have a valid buffer
                _cs->Enter();
            }
            return _pBitmapData;
        }

        void Unlock()
        {
            _cs->Leave();
        }

    private:
        webrtc::CriticalSectionWrapper* _cs;
        BYTE*                           _pBitmapData;
        int                             _width;
        int                             _height;
    };


    class MagCapture
    {
    public:
        MagCapture();
        ~MagCapture();

        bool Create(HMONITOR hMon, HWND hwndParent);
        void Destroy();
        void AddWindowToFilterList(HWND hwnd);
        void RemoveWindowFromFilterList(HWND hwnd);
        bool CopyData(BYTE* buffer, UINT size, int width, int height, bool& useFallbackCapture);

    private:
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        bool OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);
        void CheckWidgetWindow();
        void UpdateExcludeHWNDList();

    private:
        HINSTANCE           _hInstance;
        HWND                _hwnd;
        HWND                _hwndMag;
        HWND                _hwndWidget;
        HMONITOR            _hMonitor;
        BufferWrapper*      _widgetDIB;
        std::vector<HWND>   _capExcludeList;
        WCHAR               _widgetTitle[32];
        bool                _notSupportedOnThisMonitor;
    };
}
