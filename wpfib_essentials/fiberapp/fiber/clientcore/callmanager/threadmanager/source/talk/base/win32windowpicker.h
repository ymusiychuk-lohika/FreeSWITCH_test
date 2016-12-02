// Copyright 2010 Google Inc. All Rights Reserved


#ifndef TALK_BASE_WIN32WINDOWPICKER_H_
#define TALK_BASE_WIN32WINDOWPICKER_H_

#include <string>
#include <map>
#include "talk/base/scoped_ptr.h"
#include "talk/base/win32.h"
#include "talk/base/windowpicker.h"
#include "talk/base/win32window.h"
#include <shellapi.h>
#include <shobjidl.h>


namespace talk_base {

// some system API calls that don't exist on Windows XP
typedef BOOL (WINAPI *QUERYPROCIMAGE_FUNC)(HANDLE, DWORD, LPWSTR, PDWORD);

class Win32WindowPicker : public WindowPicker {
 public:
  Win32WindowPicker();
  ~Win32WindowPicker();
  virtual bool Init();
  virtual bool IsVisible(const WindowId& id);
  virtual bool MoveToFront(const WindowId& id);
  virtual bool GetWindowList(WindowDescriptionList* descriptions);
  virtual bool GetDesktopList(DesktopDescriptionList* descriptions);
  virtual bool GetApplicationList(ApplicationDescriptionList* descriptions);
  virtual bool GetDesktopDimensions(const DesktopId& id, int* width,
                                    int* height);
  virtual uint8_t* GetDesktopThumbnail(const DesktopId& id, int width,
                                     int height);
  virtual uint8_t* GetWindowThumbnail(const WindowDescription& desc, int width,
                                     int height);
  virtual uint8_t* GetApplicationThumbnail(const ApplicationDescription& desc, int width,
                                     int height);
  virtual void FreeThumbnail(uint8_t* thumbnail);
  virtual bool RegisterAppTerminateNotify(ProcessId pid, int desktopId);
  virtual bool UnregisterAppTerminateNotify(ProcessId pid, int desktopId);
  virtual bool IsApplicationSharingSupported();

  void set_watcher(DesktopWatcher* watcher) { watcher_.reset(watcher); }
  DesktopWatcher* watcher() { return watcher_.get(); }
  int HasDesktopLayoutChanged();
  HWND GetPrevActiveWindow() { return prevActiveWindow_; }
 protected:
  static BOOL CALLBACK EnumProc(HWND hwnd, LPARAM l_param);
  static BOOL CALLBACK MonitorEnumProc(HMONITOR h_monitor,
                                       HDC hdc_monitor,
                                       LPRECT lprc_monitor,
                                       LPARAM l_param);
  static BOOL CALLBACK AppEnumProc(HWND hwnd, LPARAM l_param);
  void LoadNamesFromResource(DWORD pid, std::string& description, std::string& name);
  talk_base::scoped_ptr<DesktopWatcher> watcher_;
  DesktopId sharedDesktop_;
  QUERYPROCIMAGE_FUNC queryProcImageFunc_;
  HWND prevActiveWindow_;
};

class Win32DesktopWatcher
    : public DesktopWatcher,
      public Win32Window {
 public:
  explicit Win32DesktopWatcher(Win32WindowPicker* wp);
  virtual ~Win32DesktopWatcher();
  virtual bool Start();
  virtual void Stop();
  virtual bool RegisterAppTerminate(ProcessId pid);
  virtual bool UnregisterAppTerminate(ProcessId pid);

 private:
  virtual bool OnMessage(UINT msg, WPARAM wp, LPARAM lp, LRESULT& result);
  bool m_isMetroApplication;
  HWND m_metroAppHwnd;
  bool CheckIfMetroApp(ProcessId procId);
  static BOOL CALLBACK AppCaptureEnumWindowsProc(HWND hwnd, LPARAM lParam);
  Win32WindowPicker* picker_;
  ProcessId watchedPid_;
  static const int kDisplayChangedTimerId = 33;
  static const int kProcessWatcherTimerId = 34;
};


}  // namespace talk_base

#endif  // TALK_BASE_WIN32WINDOWPICKER_H_
