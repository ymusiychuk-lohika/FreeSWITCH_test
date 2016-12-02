#pragma once

#include <string>
#include <map>

// PNG helper functions
HBITMAP LoadPNGToBitmap(HINSTANCE hInstance, LPCTSTR fileName);
bool LoadPNGToMemory(HINSTANCE hInstance, LPCTSTR fileName, RECT* rect, BYTE* buffer, UINT bufferSize);
const int kBlinkTimerFrequency = 100;

// timer callback for use in code that doesn't have a window but wants
// the callback to be called on the threads message queue thread
typedef void (CALLBACK*MsgQueueCallbackFunc)(void* param, UINT id);

class MsgQueueTimerCallback
{
public:
    typedef struct
    {
        MsgQueueCallbackFunc    func;
        void*                   param;
    } MsgQueueParam;

    typedef std::map<UINT, MsgQueueParam> msgQueueMap;

    MsgQueueTimerCallback();
    ~MsgQueueTimerCallback();
    UINT Add(UINT interval, MsgQueueCallbackFunc callback, void* param);
    void Remove(UINT id);

private:
    static VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

private:
    DWORD                   _thread;
    static msgQueueMap      _timerMap; // static object as we need to access it from TimerProc
};

// global object definition
extern MsgQueueTimerCallback globalTimerCallbacks;



class WindowProcBase
{
public:
    WindowProcBase();
    virtual ~WindowProcBase();

    void Destroy();
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
    void RegisterWndClass(WNDCLASSEX* wndClass, HINSTANCE hInstance = NULL);
    bool WindowProcBase::Create(DWORD dwExStyle,
        LPCWSTR lpWindowName,
        DWORD dwStyle,
        int X,
        int Y,
        int nWidth,
        int nHeight,
        HWND hWndParent,
        HMENU hMenu);

private:
    virtual bool OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result);

protected:
    HWND            _hwnd;
    HWND            _hwndParent;
    HINSTANCE       _hInstance;

private:
    std::wstring    _className;
};
