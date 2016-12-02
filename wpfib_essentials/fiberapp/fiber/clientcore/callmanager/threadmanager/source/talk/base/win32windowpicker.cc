// Copyright 2010 Google Inc. All Rights Reserved


#include "talk/base/win32windowpicker.h"
#include <shellapi.h>
#include <tchar.h>
#include <string>
#include <vector>
#include <map>
#include <TlHelp32.h>
#include <CommCtrl.h>
#include <commoncontrols.h>
#include <Psapi.h>
#include <ntverp.h>
#include <unordered_set>
#include <dwmapi.h>
#include "talk/base/common.h"
#include "talk/base/logging_libjingle.h"
#include "talk/base/win32processdetails.h"

#if VER_PRODUCTBUILD < 9600
#define DWMWA_CLOAKED 14    // Define this macro as the current Skinny is built with Windows 7 SDK.
#endif

// message to share within application, when the app sharing doesn't start or some error then webrtc notifies via this message
#define WM_MSG_APP_CAPTURED_FAILED (WM_USER + 100)

namespace talk_base {

namespace {

// Window class names that we want to filter out.
const char kProgramManagerClass[] = "Progman";
const char kButtonClass[] = "Button";

}  // namespace

BOOL CALLBACK Win32WindowPicker::EnumProc(HWND hwnd, LPARAM l_param) {
  WindowDescriptionList* descriptions =
      reinterpret_cast<WindowDescriptionList*>(l_param);

  // Skip windows that are invisible, minimized, have no title, or are owned,
  // unless they have the app window style set. Except for minimized windows,
  // this is what Alt-Tab does.
  // TODO: Figure out how to grab a thumbnail of a minimized window and
  // include them in the list.
  int len = GetWindowTextLength(hwnd);
  HWND owner = GetWindow(hwnd, GW_OWNER);
  LONG exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);
  if (len == 0 || IsIconic(hwnd) || !IsWindowVisible(hwnd) ||
      (owner && !(exstyle & WS_EX_APPWINDOW))) {
    // TODO: Investigate if windows without title still could be
    // interesting to share. We could use the name of the process as title:
    //
    // GetWindowThreadProcessId()
    // OpenProcess()
    // QueryFullProcessImageName()
    return TRUE;
  }

  // Skip the Program Manager window and the Start button.
  TCHAR class_name_w[500];
  ::GetClassName(hwnd, class_name_w, 500);
  std::string class_name = ToUtf8(class_name_w);
  if (class_name == kProgramManagerClass || class_name == kButtonClass) {
    // We don't want the Program Manager window nor the Start button.
    return TRUE;
  }

  TCHAR window_title[500];
  GetWindowText(hwnd, window_title, ARRAY_SIZE(window_title));
  std::string title = ToUtf8(window_title);

  WindowId id(hwnd);
  WindowDescription desc(id, title);
  descriptions->push_back(desc);
  return TRUE;
}

BOOL CALLBACK Win32WindowPicker::MonitorEnumProc(HMONITOR h_monitor,
                                                 HDC hdc_monitor,
                                                 LPRECT lprc_monitor,
                                                 LPARAM l_param) {
  DesktopDescriptionList* desktop_desc =
      reinterpret_cast<DesktopDescriptionList*>(l_param);

  DesktopId id(h_monitor, static_cast<int>(desktop_desc->size()));
  // TODO: Figure out an appropriate desktop title.
  DesktopDescription desc(id, "");

  // Determine whether it's the primary monitor.
  MONITORINFO monitor_info = {0};
  monitor_info.cbSize = sizeof(monitor_info);
  bool primary = (GetMonitorInfo(h_monitor, &monitor_info) &&
      (monitor_info.dwFlags & MONITORINFOF_PRIMARY) != 0);
  desc.set_primary(primary);

  desktop_desc->push_back(desc);
  return TRUE;
}

void Win32WindowPicker::LoadNamesFromResource(DWORD pid, std::string& description, std::string& name)
{
    TCHAR szPath[MAX_PATH] = { 0 };
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProc)
    {
        DWORD pathLen = _countof(szPath);
        if (queryProcImageFunc_)
        {
            queryProcImageFunc_(hProc, 0, szPath, &pathLen);
        }
        CloseHandle(hProc);
    }
    else
    {
        return;
    }
    DWORD handle = 0;
    DWORD dwSize = GetFileVersionInfoSize(szPath, &handle);
    if (dwSize)
    {
        BYTE* data = new BYTE[dwSize];
        if (GetFileVersionInfo(szPath, handle, dwSize, data))
        {
            // get the name and version strings
            LPVOID pvProductName = NULL;
            unsigned int iProductNameLen = 0;
            LPVOID pvFileDesc = NULL;
            unsigned int iFileDescLen = 0;

            struct LANGANDCODEPAGE {
                WORD wLanguage;
                WORD wCodePage;
            } *lpTranslate;

            UINT cbTranslate = 0;
            // Read the list of languages and code pages.
            BOOL retVal = VerQueryValue(data,
                TEXT("\\VarFileInfo\\Translation"),
                (LPVOID*)&lpTranslate,
                &cbTranslate);
            if(retVal)
            {
                // Read the file description for each language and code page.
                for(size_t i=0; i < (cbTranslate/sizeof(struct LANGANDCODEPAGE)); i++ )
                {
                    WCHAR SubBlock[50];
                    char ascii[256];
                    wsprintf(SubBlock,
                        TEXT("\\StringFileInfo\\%04x%04x\\ProductName"),
                        lpTranslate[i].wLanguage,
                        lpTranslate[i].wCodePage);
                    // replace "040904e4" with the language ID of your resources
                    if (VerQueryValue(data, SubBlock, &pvProductName, &iProductNameLen))
                    {
                        WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)pvProductName, -1, ascii, _countof(ascii), NULL, NULL);
                        name = ascii;
                    }
                    wsprintf(SubBlock,
                        TEXT("\\StringFileInfo\\%04x%04x\\FileDescription"),
                        lpTranslate[i].wLanguage,
                        lpTranslate[i].wCodePage);
                    if (VerQueryValue(&data[0], SubBlock, &pvFileDesc, &iFileDescLen))
                    {
                        WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)pvFileDesc, -1, ascii, _countof(ascii), NULL, NULL);
                        description = ascii;
                    }
                }
            }
        }
        delete [] data;
    }
}


struct AppEnumData
{
    ApplicationDescriptionList*     appList;
    DesktopDescriptionList          desktopList;
    std::unordered_set<std::string> bgApps; // List of invisible apps which exists in the background (Feature unique to Windows 10)
    bool                            b_isVistaOrAbove;

    AppEnumData();
    void UpdateInvisibleApps(HWND);
};

AppEnumData::AppEnumData():appList(NULL),b_isVistaOrAbove(false)
{
    OSVERSIONINFO osInfo = { 0 };
    osInfo.dwOSVersionInfoSize = sizeof(osInfo);
    GetVersionEx(&osInfo);
    if (osInfo.dwMajorVersion >= 6)
    {
        b_isVistaOrAbove = true; // DwmGetWindowAttribute call is not present on Windows XP.
    }
}

void AppEnumData::UpdateInvisibleApps(HWND hwnd)
{
    if(b_isVistaOrAbove)
    {
        int len = GetWindowTextLengthW(hwnd) + 1;
        if(len)
        {
            char *buf = new(std::nothrow) char[len];
            if(buf)
            {
                if(GetWindowTextA(hwnd,buf,len))
                {
                    int isCloaked = 0;
                    HRESULT hr = DwmGetWindowAttribute(hwnd,DWMWA_CLOAKED,&isCloaked,sizeof(isCloaked));
                    if(hr == S_OK && isCloaked > 0)
                        bgApps.insert(buf);
                }
                delete []buf;
            }
        }
    }
}

typedef struct _AppWindowInfo
{
    HWND        hwnd;
    RECT        rcWin;
    DWORD       pid;
} AppWindowInfo;

typedef struct _AppWindowList
{
    DWORD                       pid;
    std::vector<AppWindowInfo>  list;
} AppWindowList;

BOOL CALLBACK Win32WindowPicker::AppEnumProc(HWND hwnd, LPARAM l_param) {
  AppEnumData* enumData = reinterpret_cast<AppEnumData*>(l_param);
  ProcessDetails pd(hwnd);
  std::string name(ToUtf8(pd.GetProcessImage().c_str()));
  if(strstr(name.c_str(),"ApplicationFrameHost.exe"))
  {
      char lpClassName[MAX_CLASS_NAME];
      GetClassNameA(hwnd, lpClassName, MAX_CLASS_NAME);
      //For ApplicationFrameHost process, iterate through the child windows and add their process to the list.
      if(strcmp(lpClassName, "ApplicationFrameWindow") == 0)
      {
          enumData->UpdateInvisibleApps(hwnd);
          HWND childWindow = NULL;
          while (childWindow =  FindWindowExA(hwnd, childWindow, NULL, NULL))
          {
              AppEnumProc(childWindow, l_param);//calling AppEnumProc for the child windows.
          }
      }
      return TRUE;// return for ApplicationFrameHost Process.
  }
  ApplicationDescriptionList* descriptions = enumData->appList;
  DesktopDescriptionList  desktopList = enumData->desktopList;
  // Skip windows that are invisible, have no title, or are owned,
  // (SKY-3780: Removed[unless they have the app window style set]). Except for minimized windows,
  // this is what Alt-Tab does.
  // TODO: Figure out how to grab a thumbnail of a minimized window and
  // include them in the list.
  int len = GetWindowTextLength(hwnd);
  HWND owner = GetWindow(hwnd, GW_OWNER);
  LONG exstyle = GetWindowLong(hwnd, GWL_EXSTYLE);
  bool isMetroApp = false;
  HWND parentWindow = GetParent(hwnd);
  if(parentWindow)
  {
	  char parentClassName[MAX_CLASS_NAME];
	  GetClassNameA(parentWindow, parentClassName, MAX_CLASS_NAME);
	  if(strcmp(parentClassName, "ApplicationFrameWindow") == 0)
	  {
		  isMetroApp = true;
	  }
  }
  //Metro apps have WS_EX_LAYERED property. So avoid exStyle check for them 
  if (len == 0 || !IsWindowVisible(hwnd) || ((exstyle & WS_EX_LAYERED) && (!isMetroApp))) {
      return TRUE;
  }
  //some application like Data card monitor hide the application by positioning outside the view boundary. 
  HMONITOR mon = MonitorFromWindow(hwnd,MONITOR_DEFAULTTONULL);
  if(!isMetroApp && !mon)// Exclude this check for metro apps as some of them will have NULL monitor when they are minimized.
      return TRUE;
  // Skip the Program Manager window and the Start button.
  TCHAR class_name_w[500];
  ::GetClassName(hwnd, class_name_w, 500);
  std::string class_name = ToUtf8(class_name_w);
  if (class_name == kProgramManagerClass || class_name == kButtonClass) {
    // We don't want the Program Manager window nor the Start button.
    return TRUE;
  }

  TCHAR window_title[500];
  GetWindowText(hwnd, window_title, ARRAY_SIZE(window_title));
  std::string title = ToUtf8(window_title);

  //Remove extension
  size_t lastdot = name.find_last_of(".");
  if (lastdot != std::string::npos)
      name = name.substr(0, lastdot);
  ProcessId id(pd.GetProcessId());
  HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
  int i;
  for(i = 0 ; i < desktopList.size() ; i++)
  {
      DesktopDescription deskDesc = desktopList[i];
      if(monitor == deskDesc.id().id())
      {
          break;
      }
  }

  WindowDescription winId(hwnd, title, i);
  /* SKY - 3829
  Finding the window handle for the powerpoint slide show and setting the slideshow boolean to true 
  so that the rivet can start the app sharing with slideshow window
  */

  HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,id);
  if(hProcess)
  {
      WCHAR buf[MAX_PATH] = L"";
      GetProcessImageFileNameW(hProcess,buf,MAX_PATH);   
      std::wstring processName(buf);
      size_t index = processName.find_last_of(L"\\");
      std::wstring exeName(processName.substr(index+1)); 
      if(exeName.compare(L"POWERPNT.EXE") == 0 && class_name.compare("screenClass") == 0)  
      {
          DWORD style = GetWindowLong(hwnd,GWL_STYLE);
          if(!(style & WS_MAXIMIZEBOX))
          {
              winId.set_SlideShowWnd(true);
              LOG_GLE(INFO)<<"slideshow window found hwnd = "<<winId.id().id();
          }
      }
      CloseHandle(hProcess);
  }
  // are we are enumerating windows not process we may have already added
  // this process to our list, check first before adding
  // TODO: which window title should we use if there is more than 1 window?
  ApplicationDescriptionList::iterator it;
  for (it = descriptions->begin(); it != descriptions->end(); ++it)
  {
      if (it->id() == id)
      {
          // we already have the process, add the window
          it->add_window(winId);
          break;
      }
  }
  if (it == descriptions->end())
  {
    ApplicationDescription desc(id, name);
    desc.add_window(winId);
    desc.set_integrity_level(pd.GetProcessIntegrityLevel());
    descriptions->push_back(desc);
  }
  return TRUE;
}

Win32WindowPicker::Win32WindowPicker()
  : queryProcImageFunc_(NULL), prevActiveWindow_(NULL) {
  set_watcher(new Win32DesktopWatcher(this));
  // initialize the list of desktops
  GetDesktopList(&lastDesktopList_);
  queryProcImageFunc_ = (QUERYPROCIMAGE_FUNC) GetProcAddress(GetModuleHandle(L"kernel32.dll"),
      "QueryFullProcessImageNameW");
}

Win32WindowPicker::~Win32WindowPicker() {
  watcher()->Stop();
}

bool Win32WindowPicker::Init() {
  watcher()->Start();
  return true;
}
// TODO: Consider changing enumeration to clear() descriptions
// before append().
bool Win32WindowPicker::GetWindowList(WindowDescriptionList* descriptions) {
  LPARAM desc = reinterpret_cast<LPARAM>(descriptions);
  return EnumWindows(Win32WindowPicker::EnumProc, desc) != FALSE;
}

bool Win32WindowPicker::GetDesktopList(DesktopDescriptionList* descriptions) {
  // Create a fresh WindowDescriptionList so that we can use desktop_desc.size()
  // in MonitorEnumProc to compute the desktop index.
  DesktopDescriptionList desktop_desc;
  HDC hdc = GetDC(NULL);
  bool success = false;
  if (EnumDisplayMonitors(hdc, NULL, Win32WindowPicker::MonitorEnumProc,
      reinterpret_cast<LPARAM>(&desktop_desc)) != FALSE) {
    // Append the desktop descriptions to the end of the returned descriptions.
    descriptions->insert(descriptions->end(), desktop_desc.begin(),
                         desktop_desc.end());
    success = true;
  }
  ReleaseDC(NULL, hdc);
  return success;
}

bool Win32WindowPicker::GetApplicationList(ApplicationDescriptionList* descriptions) {
    // check if application sharing is supported on this os version
    if (!IsApplicationSharingSupported())
    {
        return false;
    }
    DesktopDescriptionList currentList;
    ApplicationDescriptionList appList;
    GetDesktopList(&currentList);
    AppEnumData enumData;
    enumData.appList = &appList;
    enumData.desktopList = currentList;
    LPARAM desc = reinterpret_cast<LPARAM>(&enumData);
    bool success = EnumWindows(Win32WindowPicker::AppEnumProc, desc) != FALSE;
    int numInvisibleApps = enumData.bgApps.size();
    descriptions->reserve(appList.size());
    for (size_t i = 0; i < appList.size(); i++)
    {
        bool isVisible = true;
        if(numInvisibleApps > 0) // Discard the invisible metro apps (Win10)
        {
            WindowDescriptionList winList = appList[i].get_windows();
            for(size_t i = 0; i < winList.size(); i++)
            {
                std::string title(winList[i].title());
                if(enumData.bgApps.find(title) != enumData.bgApps.end())
                {
                    isVisible = false;
                    break;
                }
            }
        }
        if(isVisible) // Send only the visible ones
        {
            ProcessId pid = appList[i].id();
            std::string filedesc;
            std::string prodname;
            LoadNamesFromResource(pid, filedesc, prodname);
            appList[i].set_file_description(filedesc);
            appList[i].set_product_name(prodname);
            descriptions->push_back(appList[i]);
        }

    }
    return success;
}

bool Win32WindowPicker::GetDesktopDimensions(const DesktopId& id,
                                             int* width,
                                             int* height) {
  MONITORINFOEX monitor_info;
  monitor_info.cbSize = sizeof(MONITORINFOEX);
  if (!GetMonitorInfo(id.id(), &monitor_info)) {
    return false;
  }
  *width = monitor_info.rcMonitor.right - monitor_info.rcMonitor.left;
  *height = monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top;
  return true;
}

uint8_t* Win32WindowPicker::GetDesktopThumbnail(const DesktopId& id,
                                              int width,
                                              int height) {
  uint32 length = width * height * 4;
  uint8_t* buffer = new uint8_t[length];
  if(NULL == buffer)
      return NULL;
  HBITMAP hBitmap = NULL;
  HDC hScreenDC = NULL;
  HDC hMemoryDC = NULL;
  bool bReturn = false;
  do {
    HMONITOR    hMon = id.id();

    // Bounds of the captured screen
    MONITORINFOEX mi;
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMon, &mi);
    RECT rcScreen = mi.rcMonitor;
    int srcWidth = rcScreen.right - rcScreen.left;
    int srcHeight = rcScreen.bottom - rcScreen.top;

    // adjust capture are so we clip the green border
    int srcX = kGreenBorderThickness;
    int srcY = kGreenBorderThickness;
    srcHeight -= (kGreenBorderThickness * 2);
    srcWidth -= (kGreenBorderThickness * 2);

    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    int bytes_per_row = width * 4;
    bmi.bmiHeader.biSizeImage = bytes_per_row * height;
    bmi.bmiHeader.biXPelsPerMeter = 0;
    bmi.bmiHeader.biYPelsPerMeter = 0;

    hScreenDC = CreateDC(mi.szDevice, mi.szDevice, NULL, NULL);
    if(NULL == hScreenDC)
        break;

    hMemoryDC = CreateCompatibleDC(hScreenDC);
    if(NULL == hMemoryDC)
        break;

    // improve the scaling
    SetStretchBltMode(hMemoryDC, HALFTONE);
    // Create the DIB, and store a pointer to its pixel buffer.
    BYTE* bitmapData = NULL;
    hBitmap = CreateDIBSection(hMemoryDC, &bmi, DIB_RGB_COLORS, (void**)&bitmapData, NULL, 0);
    // Fix for crash SKY-3900 source: https://msdn.microsoft.com/en-us/library/windows/desktop/dd183494%28v=vs.85%29.aspx
    if(NULL == hBitmap ||
        ERROR_INVALID_PARAMETER == (long)hBitmap ||
        ERROR_OUTOFMEMORY == (long)hBitmap ||
        NULL == bitmapData)
    {
        LOG_GLE(LS_ERROR) << "CreateDIBSection Failed!";
        break;
    }
    // select the object into the DC
    HGDIOBJ hOldBitmap = SelectObject(hMemoryDC, hBitmap);

    StretchBlt(hMemoryDC, 0, 0, width, height, hScreenDC,
        srcX, srcY, srcWidth, srcHeight, SRCCOPY);
    // copy into our frame buffer
    memcpy(buffer, bitmapData, length);

    // make sure the alpha value is set correctly, this is
    // always a 32bpp image that has a stride of width * 4
    uint8_t* alphaValue = buffer + 3;
    for (size_t i = 0; i < length; i += 4) {
      *alphaValue = 0xff;
      alphaValue += 4;
    }
    // Select the previous bitmap object
    SelectObject(hMemoryDC, hOldBitmap);

    bReturn = true;
  }while(0);

  if(hBitmap)
    DeleteObject(hBitmap);
  if(hMemoryDC)
    DeleteDC(hMemoryDC);
  if(hScreenDC)
    DeleteDC(hScreenDC);

  if(false == bReturn)
  {
      delete[] buffer;
      return 0;
  }

  return buffer;
}

uint8_t* Win32WindowPicker::GetWindowThumbnail(const WindowDescription& desc, int width, int height)
{
  uint32 length = width * height * 4;
  uint8_t* buffer = new uint8_t[length];
  if(NULL == buffer)
      return NULL;
  HDC hdcScreen = NULL;
  HDC hMemoryDC = NULL;
  HBITMAP hBitmap = NULL;
  bool bSuccess = false;

  do {
    HWND hwnd = desc.id().id();

    RECT rcWin;
    GetWindowRect(hwnd, &rcWin);
    int srcX = 0;
    int srcY = 0;
    int srcWidth = rcWin.right - rcWin.left;
    int srcHeight = rcWin.bottom - rcWin.top;

    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    int bytes_per_row = width * 4;
    bmi.bmiHeader.biSizeImage = bytes_per_row * height;
    bmi.bmiHeader.biXPelsPerMeter = 0;
    bmi.bmiHeader.biYPelsPerMeter = 0;

    hdcScreen = GetDC(NULL);
    if(NULL == hdcScreen)
        break;

    hMemoryDC = CreateCompatibleDC(hdcScreen);
    if(NULL == hMemoryDC)
        break;
    // improve the scaling
    SetStretchBltMode(hMemoryDC, HALFTONE);
    // Create the DIB, and store a pointer to its pixel buffer.
    BYTE* bitmapData = NULL;
    hBitmap = CreateDIBSection(hMemoryDC, &bmi, DIB_RGB_COLORS, (void**)&bitmapData, NULL, 0);
    // Fix for crash SKY-3900 source: https://msdn.microsoft.com/en-us/library/windows/desktop/dd183494%28v=vs.85%29.aspx
    if(NULL == hBitmap ||
        ERROR_INVALID_PARAMETER == (long)hBitmap ||
        ERROR_OUTOFMEMORY == (long)hBitmap ||
        NULL == bitmapData)
    {
        LOG_GLE(LS_ERROR) << "CreateDIBSection Failed!";
        break;
    }
    // select the object into the DC
    HGDIOBJ hOldBitmap = SelectObject(hMemoryDC, hBitmap);

    HDC hdcWin = GetDC(hwnd);
    StretchBlt(hMemoryDC, 0, 0, width, height, hdcWin,
        srcX, srcY, srcWidth, srcHeight, SRCCOPY);
    ReleaseDC(hwnd, hdcWin);

    // copy into our frame buffer
    memcpy(buffer, bitmapData, length);

    // make sure the alpha value is set correctly, this is
    // always a 32bpp image that has a stride of width * 4
    uint8_t* alphaValue = buffer + 3;
    for (size_t i = 0; i < length; i += 4) {
      *alphaValue = 0xff;
      alphaValue += 4;
    }

    // Select the previous bitmap object
    SelectObject(hMemoryDC, hOldBitmap);

    bSuccess = true;
  } while(0);

  if(hBitmap)
    DeleteObject(hBitmap);
  if(hMemoryDC)
    DeleteDC(hMemoryDC);
  if(hdcScreen)
    ReleaseDC(NULL, hdcScreen);

  if(false == bSuccess)
  {
    delete []buffer;
    return 0;
  }

  return buffer;
}

HICON GetHighResolutionIcon(LPTSTR pszPath, int width, int height)
{
    // Get the image list index of the icon
    SHFILEINFO sfi;
    HICON hIcon = 0;
    HRESULT hr = S_FALSE;
    typedef HRESULT (WINAPI *SHCIFPN)(PCWSTR pszPath, IBindCtx * pbc, REFIID riid, void ** ppv);
    HMODULE hLib = LoadLibrary(L"shell32.dll");
    if (hLib)
    {
        SHCIFPN pSHCIFPN = (SHCIFPN)GetProcAddress(hLib, "SHCreateItemFromParsingName");
        if (pSHCIFPN)
        {
            IBindCtx* bindContext = NULL;
            BIND_OPTS bo = { sizeof(bo), 0, STGM_CREATE, 0 };
            hr = CreateBindCtx(0, &bindContext);
            if (hr == S_OK)
            {
                bindContext->SetBindOptions(&bo);
            }
            IShellItem* psi = NULL;
            hr = pSHCIFPN(pszPath, bindContext, IID_IShellItem, (void**)&psi);
            if(bindContext)
                bindContext->Release();
            if (hr == S_OK)
            {
                IShellItemImageFactory* psiif = NULL;
                hr = psi->QueryInterface(IID_IShellItemImageFactory, (void**)&psiif);
                if (hr == S_OK)
                {
                    HBITMAP iconBitmap = NULL;
                    SIZE iconSize = {width, height};
                    hr = psiif->GetImage(iconSize, SIIGBF_RESIZETOFIT |SIIGBF_BIGGERSIZEOK | SIIGBF_ICONONLY, &iconBitmap);
                    if (hr == S_OK)
                    {
                        BITMAP iconBitmapInfo;
                        GetObject(iconBitmap, sizeof(BITMAP),(LPSTR) &iconBitmapInfo);
                        HDC hDesktopDC = ::GetDC(NULL);
                        HBITMAP iconBitmapMask = ::CreateCompatibleBitmap(hDesktopDC, iconBitmapInfo.bmWidth, iconBitmapInfo.bmHeight);
                        ICONINFO iconInfo = {0};
                        iconInfo.fIcon = TRUE;
                        iconInfo.hbmColor = iconBitmap;
                        iconInfo.hbmMask = iconBitmapMask;
                        hIcon = ::CreateIconIndirect(&iconInfo);
                        ReleaseDC(NULL, hDesktopDC);
                        DeleteObject(iconBitmapMask);
                        DeleteObject(iconBitmap);
                        psiif->Release();
                        psi->Release();
                        FreeLibrary(hLib);
                        return hIcon;
                    }
                    psiif->Release();
                }
                psi->Release();
            }
        }
        FreeLibrary(hLib);
    }

    if(hr != S_OK)
    {
        if (!SHGetFileInfo(pszPath, 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX)) return NULL;
        // Get the jumbo image list
        IImageList *piml;
        if (FAILED(SHGetImageList(SHIL_EXTRALARGE, IID_PPV_ARGS(&piml)))) return NULL;

        // Extract an icon
        HICON hico;
        piml->GetIcon(sfi.iIcon, ILD_TRANSPARENT, &hico);

        // Clean up
        piml->Release();
        // Return the icon
        return hico;
    }
    return 0;
}

uint8_t* Win32WindowPicker::GetApplicationThumbnail(const ApplicationDescription& desc, int width, int height)
{
    bool success = false;
    HDC hdcScreen = 0;
    HDC hMemoryDC = 0;
    BYTE* bitmapData = 0;
    HBITMAP hBitmap = 0;
    HICON processIcon = 0;
    uint32 length = width * height * 4;
    uint8_t* buffer = new uint8_t[length];
    if(!buffer)
        return 0;
    do{
        BITMAPINFO bmi;
        memset(&bmi, 0, sizeof(bmi));
        bmi.bmiHeader.biHeight = -height;
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
        int bytes_per_row = width * 4;
        bmi.bmiHeader.biSizeImage = bytes_per_row * height;
        bmi.bmiHeader.biXPelsPerMeter = 0;
        bmi.bmiHeader.biYPelsPerMeter = 0;
        hdcScreen = GetDC(NULL);
        if(!hdcScreen) break;
        hMemoryDC = CreateCompatibleDC(hdcScreen);
        if(!hMemoryDC) break;
        // improve the scaling
        SetStretchBltMode(hMemoryDC, HALFTONE);
        // Create the DIB, and store a pointer to its pixel buffer.
        hBitmap = CreateDIBSection(hMemoryDC, &bmi, DIB_RGB_COLORS, (void**)&bitmapData, NULL, 0);
        // Fix for crash SKY-3900 source: https://msdn.microsoft.com/en-us/library/windows/desktop/dd183494%28v=vs.85%29.aspx
        if(NULL == hBitmap ||
            ERROR_INVALID_PARAMETER == (long)hBitmap ||
            ERROR_OUTOFMEMORY == (long)hBitmap ||
            NULL == bitmapData)
        {
            LOG_GLE(LS_ERROR) << "CreateDIBSection Failed!";
            break;
        }
        // select the object into the DC
        HGDIOBJ hOldBitmap = SelectObject(hMemoryDC, hBitmap);
        RECT rcDst = { 0, 0, width, height };
        // Get the icon for the process
        HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, desc.id());
        if (hProc)
        {
            TCHAR szPath[MAX_PATH];
            DWORD pathLen = _countof(szPath);
            if (queryProcImageFunc_)
            {
                if (queryProcImageFunc_(hProc, 0, szPath, &pathLen))
                {
                    processIcon = GetHighResolutionIcon(szPath, width, height);
                    if (processIcon == NULL)
                    {
                        HICON smallIcon = NULL;
                        ExtractIconEx(szPath, 0, &processIcon, &smallIcon, 1);
                        if (processIcon == NULL)
                        {
                            processIcon = smallIcon;
                            smallIcon = NULL;
                        }
                        else
                        {
                            // we don't need the small icon
                            DestroyIcon(smallIcon);
                        }
                    }
                }
            }
            CloseHandle(hProc);
        }
        if (processIcon)
        {
            BITMAP bm = { 0 };
            ICONINFO ic = { 0 };
            GetIconInfo(processIcon, &ic);
            GetObject(ic.hbmColor, sizeof(bm), &bm);
            DrawIconEx(hMemoryDC, 0, 0, processIcon,
                width, height, 0, NULL, DI_NORMAL);

            // SKY-3900 : Main culprit!
            if(ic.hbmColor)
                DeleteObject(ic.hbmColor);
            if(ic.hbmMask)
                DeleteObject(ic.hbmMask);

            DestroyIcon(processIcon);
        }
        // Now select the old bitmap object
        SelectObject(hMemoryDC, hOldBitmap);

        // copy into our frame buffer
        memcpy(buffer, bitmapData, length);
        success = true;
    }while(0);

    if(hBitmap)
        DeleteObject(hBitmap);
    if(hMemoryDC)
        DeleteDC(hMemoryDC);
    if(hdcScreen)
        ReleaseDC(NULL, hdcScreen);  
    if(false == success)
    {
        delete []buffer;
        return 0;
    }
    return buffer;
}

void Win32WindowPicker::FreeThumbnail(uint8_t* thumbnail) {
  delete [] thumbnail;
}

bool Win32WindowPicker::IsVisible(const WindowId& id) {
  return (::IsWindow(id.id()) != FALSE && ::IsWindowVisible(id.id()) != FALSE);
}

bool Win32WindowPicker::MoveToFront(const WindowId& id) {
  // remember the last active foreground window, this will be used in case app capture fails
  prevActiveWindow_ = GetForegroundWindow();
  return SetForegroundWindow(id.id()) != FALSE;
}

int Win32WindowPicker::HasDesktopLayoutChanged() {
  // we compare the last saved desktop list with the current one to
  // check to see if it's just the resolution that has changed
  DesktopDescriptionList currentList;
  GetDesktopList(&currentList);
  int changed = DesktopLayoutHasChanged(lastDesktopList_, currentList, sharedDesktop_);
  lastDesktopList_.clear();
  lastDesktopList_ = currentList;
  return changed;
}

bool Win32WindowPicker::RegisterAppTerminateNotify(ProcessId pid, int desktopId) {
    if (pid == 0) {
        sharedDesktop_ = lastDesktopList_[desktopId].id();
    }
    else {
        // remove any existing watcher
        watcher()->UnregisterAppTerminate(0);
        watcher()->RegisterAppTerminate(pid);
        sharedDesktop_ = DesktopId(0, 0);
    }
    return true;
}

bool Win32WindowPicker::UnregisterAppTerminateNotify(ProcessId pid, int desktopId) {
    watcher()->UnregisterAppTerminate(pid);
    return true;
}

bool Win32WindowPicker::IsApplicationSharingSupported() {
    // supported on Windows 7 or later only
    bool supported = IsWindows7OrLater();
    return supported;
}

Win32DesktopWatcher::Win32DesktopWatcher(Win32WindowPicker* picker)
  : DesktopWatcher(picker)
  , picker_(picker)
  , watchedPid_(0)
  , m_isMetroApplication(false)
  , m_metroAppHwnd(NULL)
{
}

Win32DesktopWatcher::~Win32DesktopWatcher() {
}

bool Win32DesktopWatcher::Start() {
  if (!Create(NULL, _T("libjingle Win32DesktopWatcher Window"),
              0, 0, 0, 0, 0, 0)) {
    return false;
  }
  return true;
}

void Win32DesktopWatcher::Stop() {
  Destroy();
}

bool Win32DesktopWatcher::RegisterAppTerminate(ProcessId pid) {
    watchedPid_ = pid;
    SetTimer(handle(), kProcessWatcherTimerId, 500, NULL);
    return true;
}

bool Win32DesktopWatcher::UnregisterAppTerminate(ProcessId pid) {

    m_isMetroApplication = false;//resetting to default value, false
    if(m_metroAppHwnd){
        m_metroAppHwnd = NULL;
    }

    if (watchedPid_ != 0) {
        KillTimer(handle(), kProcessWatcherTimerId);
        watchedPid_ = 0;
    }
    return true;
}

void AppTerminated(ProcessId pid) {
}

BOOL CALLBACK Win32DesktopWatcher::AppCaptureEnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    AppWindowList* winList = (AppWindowList*) lParam;

    // Get process ID of the window
    DWORD dwProcID = 0;
    GetWindowThreadProcessId(hwnd, &dwProcID);
    if (IsWindowVisible(hwnd))
    {
        AppWindowInfo info = { 0 };
        info.hwnd = hwnd;
        GetWindowRect(hwnd, &info.rcWin);
        info.pid = dwProcID;
        winList->list.push_back(info);
    }
    return TRUE;
}

bool Win32DesktopWatcher::CheckIfMetroApp(ProcessId procId) {

    bool isMetroApp = false;
    AppWindowList winList;
    winList.pid = procId;
    std::vector<HWND>   win_handle_list;
    EnumWindows(AppCaptureEnumWindowsProc, (LPARAM) &winList);
    for (size_t i = 0; i < winList.list.size(); i++)
    {
        char lpClassName[MAX_CLASS_NAME];
        GetClassNameA(winList.list[i].hwnd, lpClassName, MAX_CLASS_NAME);
        //for metro apps window, insert their child window also into the list as it belongs to actual process to be captured.
        if((strcmp(lpClassName, "Windows.UI.Core.CoreWindow") == 0) && winList.list[i].pid == procId)
        {
            isMetroApp = true;
            m_metroAppHwnd = winList.list[i].hwnd;
            break;
        }
        if(strcmp(lpClassName, "ApplicationFrameWindow") == 0)
        {
            HWND childWindow = NULL;
            while (childWindow = FindWindowExA(winList.list[i].hwnd, childWindow, NULL, NULL))
            {
                DWORD childwindowPid;
                GetWindowThreadProcessId(childWindow, &childwindowPid);
                if(childwindowPid == procId)
                {
                    isMetroApp = true;
                    m_metroAppHwnd = childWindow;
                    break;
                }
            }
            if(isMetroApp) break;
        }
    }
    return isMetroApp;
}
bool Win32DesktopWatcher::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam,
  LRESULT& result)
{
  if (uMsg == WM_DISPLAYCHANGE) {
    // if we enumerate the desktop list here we can get into a deadlock
    // to avoid that we kick of a timer message and enumerate the
    // desktops and fire a notification from the timer message.
    // We delay the time slightly as desktop changes take a while
    // so this notification isn't time sensitive
    SetTimer(handle(), kDisplayChangedTimerId, 100, NULL);
    result = 0;
    return true;
  }
  else if ((uMsg == WM_TIMER) && (wParam == kDisplayChangedTimerId)) {
    // kill off the timer
    KillTimer(handle(), kDisplayChangedTimerId);
    int layoutChanged = picker_->HasDesktopLayoutChanged();
    picker_->SignalDesktopsChanged((layoutChanged & kDesktopLayoutChanged),
        (layoutChanged & kDesktopSharingShouldEnd));
    result = 0;
    return true;
  }
  else if ((uMsg == WM_TIMER) && (wParam == kProcessWatcherTimerId)) {
    // Fix for SKY-3294 (Windows Application Sharing With IE Protected Mode Enable)
    // previous implementation was checking process aliveness using CreateToolhelp32Snapshot API,
    // but this API doesn't return all process details! Not sure why!
    // Since we already know the process id, we can use OpenProcess API and query the exit code directly
    bool stillRunning = false;
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, watchedPid_);
    if(NULL != hProc)
    {
        DWORD dwExitCode = 0;
        GetExitCodeProcess(hProc, &dwExitCode);
        if(STILL_ACTIVE == dwExitCode)
        {
            stillRunning = true;
        }
        //else { // process is dead }
        CloseHandle(hProc);
    }
    else
    {
        //if openprocess fails check if shared app is metro app. If yes, then avoid SignalDesktopsChanged.
        if(!m_isMetroApplication)
        {
            m_isMetroApplication = CheckIfMetroApp(watchedPid_);
            ProcessDetails currProcDetails(GetCurrentProcessId());
            IntegrityLevel curProcessIntLevel = currProcDetails.GetProcessIntegrityLevel();
            if(m_isMetroApplication && (curProcessIntLevel==Low))
            {
                stillRunning = true;
            }
        }
        else
        {
            //check if our metro app window exists or not.
            if(IsWindow(m_metroAppHwnd))
                stillRunning = true;
        }
    }

    if (!stillRunning) {
      LOG_GLE(INFO)<<"Process not running, signal desktop change notification, pid:"<<watchedPid_<<" ;Is metro app:"<<m_isMetroApplication;
      picker_->SignalDesktopsChanged(false, true);
    }
  }
  else if(uMsg == WM_MSG_APP_CAPTURED_FAILED) {
      // App capture failed so notify the upper layer and stop app capture
      // also activate the previous active window, which will our browser
      HWND prevActiveWnd = picker_->GetPrevActiveWindow();
      if(NULL != prevActiveWnd)
      {
          SetForegroundWindow(prevActiveWnd);
      }

      picker_->SignalDesktopsChanged(false, true);
  }

  return false;
}

}  // namespace talk_base
