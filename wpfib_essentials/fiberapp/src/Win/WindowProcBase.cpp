#include <Windows.h>
#include <Wincodec.h>
#include <tchar.h>
#include "WindowProcBase.h"
#include <string>
#include <map>
#include <crtdbg.h>

#pragma comment(lib, "Windowscodecs.lib")


// global timer helper class objects. The globalTimerCallbacks object
// provides a simple way to get the equivalent of a WM_TIMER message
// for code that doesn't have a HWND. This object is not thread safe
// so it must be only used on the main UI thread, asserts in the code
// will fire if its used on the wrong thread.
MsgQueueTimerCallback globalTimerCallbacks;
MsgQueueTimerCallback::msgQueueMap  MsgQueueTimerCallback::_timerMap;

MsgQueueTimerCallback::MsgQueueTimerCallback()
{
    _thread = GetCurrentThreadId();
}
MsgQueueTimerCallback::~MsgQueueTimerCallback()
{
    // as this class is used as a global instance and _timerMap is
    // also a global object, we can't guarantee the order of
    // these 2 objects destruction, so we can't access _timerMap
    // here to clean up as it may have already been deleted.
    // But the process is exiting at this point so even if we
    // haven't cleanup up correct it's not an issue
}
UINT MsgQueueTimerCallback::Add(UINT interval, MsgQueueCallbackFunc callback, void* param)
{
    _ASSERT(_thread == GetCurrentThreadId());
    UINT id = SetTimer(NULL, 0, interval, TimerProc);
    msgQueueMap::const_iterator it = _timerMap.find(id);
    if (it == _timerMap.end())
    {
        // add the timer to our list
        MsgQueueParam data;
        data.func = callback;
        data.param = param;
        _timerMap[id] = data;
    }
    return id;
}
void MsgQueueTimerCallback::Remove(UINT id)
{
    _ASSERT(_thread == GetCurrentThreadId());
    msgQueueMap::iterator it = _timerMap.find(id);
    if (it != _timerMap.end())
    {
        KillTimer(NULL, id);
        // remove the timer from our list
        _timerMap.erase(it);
    }
}

VOID CALLBACK MsgQueueTimerCallback::TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    // find the id in our map
    msgQueueMap::const_iterator it = _timerMap.find(idEvent);
    if (it != _timerMap.end())
    {
        // found it, now call the callback
        MsgQueueParam data = it->second;
        if (data.func)
        {
            (data.func)(data.param, idEvent);
        }
    }
}


// global map to store the class name and it's registered ATOM
static std::map<std::wstring, ATOM> globalAtomList;

WindowProcBase::WindowProcBase()
    : _hwnd(NULL)
    , _hwndParent(NULL)
    , _hInstance(NULL)
{
}

WindowProcBase::~WindowProcBase()
{
    Destroy();
}

void WindowProcBase::RegisterWndClass(WNDCLASSEX* wndClass, HINSTANCE hInstance)
{
    // get the instance handle of this code if one wasn't provided
    if (hInstance == NULL)
    {
        GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&WindowProcBase::WndProc),
            &_hInstance);
    }
    else
    {
        _hInstance = hInstance;
    }

    // save the class name
    _className = wndClass->lpszClassName;

    // has the class been registered
    std::map<std::wstring, ATOM>::const_iterator it;
    it = globalAtomList.find(_className);
    if (it == globalAtomList.end())
    {
        // use our WndProc and instance
        wndClass->lpfnWndProc = WindowProcBase::WndProc;
        wndClass->hInstance = _hInstance;

        ATOM atom = RegisterClassEx(wndClass);
        // add it to our global map
        globalAtomList[_className] = atom;
    }
}

bool WindowProcBase::Create(DWORD dwExStyle,
    LPCWSTR lpWindowName,
    DWORD dwStyle,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HMENU hMenu)
{
    _hwnd = CreateWindowEx(dwExStyle, _className.c_str(), lpWindowName, dwStyle,
        X, Y, nWidth, nHeight, hWndParent, hMenu, _hInstance, (LPVOID) this);

    _hwndParent = hWndParent;

    return (_hwnd != NULL);
}

void WindowProcBase::Destroy()
{
    if (_hwnd)
    {
        DestroyWindow(_hwnd);
        _hwnd = NULL;
    }
}

LRESULT CALLBACK WindowProcBase::WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    bool handled = false;
    LRESULT lRet = 0;
    WindowProcBase* that = reinterpret_cast<WindowProcBase*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (WM_CREATE == uMsg)
    {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        that = static_cast<WindowProcBase*>(cs->lpCreateParams);
        ::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(that));
    }

    if (that)
    {
        handled = that->OnMessage(uMsg, wParam, lParam, lRet);
    }

    if (WM_NCDESTROY == uMsg)
    {
        ::SetWindowLongPtr(hwnd, GWLP_USERDATA, NULL);
    }

    if (!handled)
    {
        lRet =  ::DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return lRet;
}

bool WindowProcBase::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    return false;
}


// Utility functions for dealing with transparent PNGs
IStream* CreateStreamOnResource(HINSTANCE hInstance, LPCTSTR lpName, LPCTSTR lpType)
{
    IStream * ipStream = NULL;
 
    // find the resource
    HRSRC hrsrc = FindResource(hInstance, lpName, lpType);
    if (hrsrc)
    {
        // load the resource
        DWORD dwResourceSize = SizeofResource(hInstance, hrsrc);
        HGLOBAL hglbImage = LoadResource(hInstance, hrsrc);
        if (hglbImage)
        {
            // lock the resource, getting a pointer to its data
            LPVOID pvSourceResourceData = LockResource(hglbImage);
            if (pvSourceResourceData)
            {
                // allocate memory to hold the resource data
                HGLOBAL hgblResourceData = GlobalAlloc(GMEM_MOVEABLE, dwResourceSize);
                if (hgblResourceData)
                {
                    // get a pointer to the allocated memory
                    LPVOID pvResourceData = GlobalLock(hgblResourceData);
                    if (pvResourceData)
                    {
                        // copy the data from the resource to the new memory block
                        CopyMemory(pvResourceData, pvSourceResourceData, dwResourceSize);
                        GlobalUnlock(hgblResourceData);
 
                        // create a stream on the HGLOBAL containing the data
                        if (SUCCEEDED(CreateStreamOnHGlobal(hgblResourceData, TRUE, &ipStream)))
                        {
                        }
                        else
                        {
                            // couldn't create stream; free the memory
                            GlobalFree(hgblResourceData);
                        }
                    }
                }
            }
        }
    }
 
    // no need to unlock or free the resource
    return ipStream;
}

IWICBitmapSource* LoadBitmapFromStream(IStream * ipImageStream)
{
    IWICBitmapSource * ipBitmap = NULL;
 
    // load WIC's PNG decoder
    IWICBitmapDecoder * ipDecoder = NULL;
    if (SUCCEEDED(CoCreateInstance(CLSID_WICPngDecoder, NULL, CLSCTX_INPROC_SERVER,
        __uuidof(ipDecoder), reinterpret_cast<void**>(&ipDecoder))))
    {
        // load the PNG
        if (SUCCEEDED(ipDecoder->Initialize(ipImageStream, WICDecodeMetadataCacheOnLoad)))
        {
            // check for the presence of the first frame in the bitmap
            UINT nFrameCount = 0;
            if (SUCCEEDED(ipDecoder->GetFrameCount(&nFrameCount)) || nFrameCount != 1)
            {
                // load the first frame (i.e., the image)
                IWICBitmapFrameDecode * ipFrame = NULL;
                if (SUCCEEDED(ipDecoder->GetFrame(0, &ipFrame)))
                {
                    // convert the image to 32bpp BGRA format with pre-multiplied alpha
                    //   (it may not be stored in that format natively in the PNG resource,
                    //   but we need this format to create the DIB to use on-screen)
                    WICConvertBitmapSource(GUID_WICPixelFormat32bppPBGRA, ipFrame, &ipBitmap);
                    ipFrame->Release();
                }
            }
        }
        ipDecoder->Release();
    }
 
    return ipBitmap;
}

HBITMAP CreateHBITMAP(IWICBitmapSource * ipBitmap)
{
    HBITMAP hbmp = NULL;
    // get image attributes and check for valid image
    UINT width = 0;
    UINT height = 0;
    if (SUCCEEDED(ipBitmap->GetSize(&width, &height)) || width == 0 || height == 0)
    {
        // prepare structure giving bitmap information (negative height indicates a top-down DIB)
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
        hbmp = CreateDIBSection(hdcScreen, &bminfo, DIB_RGB_COLORS, &pvImageBits, NULL, 0);
        ReleaseDC(NULL, hdcScreen);
        if (hbmp)
        {
            // extract the image into the HBITMAP
            const UINT cbStride = width * 4;
            const UINT cbImage = cbStride * height;
            if (SUCCEEDED(ipBitmap->CopyPixels(NULL, cbStride, cbImage, static_cast<BYTE *>(pvImageBits))))
            {
            }
            else
            {
                // couldn't extract image; delete HBITMAP
                DeleteObject(hbmp);
                hbmp = NULL;
            }
        }
    }
 
    return hbmp;
}

HBITMAP LoadPNGToBitmap(HINSTANCE hInstance, LPCTSTR fileName)
{
    HBITMAP hbmp = NULL;

    // init COM, should return S_OK or S_FALSE
    HRESULT hrCom = CoInitialize(NULL);

    // load the PNG image data into a stream
    IStream* ipImageStream = CreateStreamOnResource(hInstance, fileName, _T("PNG"));
    if (ipImageStream)
    {
        // load the bitmap with WIC
        IWICBitmapSource* ipBitmap = LoadBitmapFromStream(ipImageStream);
        if (ipBitmap)
        {
            // create a HBITMAP containing the image
            hbmp = CreateHBITMAP(ipBitmap);

            ipBitmap->Release();
        }
        ipImageStream->Release();
    }

    // un-init COM if needed to match init
    if (SUCCEEDED(hrCom))
    {
        CoUninitialize();
    }

    return hbmp;
}

bool LoadPNGToMemory(HINSTANCE hInstance, LPCTSTR fileName, RECT* rect, BYTE* buffer, UINT bufferSize)
{
    bool ok = false;

    // init COM, should return S_OK or S_FALSE
    HRESULT hrCom = CoInitialize(NULL);

    // load the PNG image data into a stream
    IStream* ipImageStream = CreateStreamOnResource(hInstance, fileName, _T("PNG"));
    if (ipImageStream)
    {
        // load the bitmap with WIC
        IWICBitmapSource* ipBitmap = LoadBitmapFromStream(ipImageStream);
        if (ipBitmap)
        {
            // get image attributes and check for valid image
            UINT width = 0;
            UINT height = 0;
            if (SUCCEEDED(ipBitmap->GetSize(&width, &height)) || width == 0 || height == 0)
            {
                // extract the image into the supplied buffer
                const WICRect wicRect = { rect->left, rect->top, (rect->right - rect->left), (rect->bottom - rect->top) };
                const UINT cbStride = wicRect.Width * 4;
                const UINT cbImage = cbStride * wicRect.Height;
                if (bufferSize >= cbImage)
                {
                    if (SUCCEEDED(ipBitmap->CopyPixels(&wicRect, cbStride, cbImage, buffer)))
                    {
                        ok = true;
                    }
                }
            }

            ipBitmap->Release();
        }
        ipImageStream->Release();
    }

    // un-init COM if needed to match init
    if (SUCCEEDED(hrCom))
    {
        CoUninitialize();
    }

    return ok;
}