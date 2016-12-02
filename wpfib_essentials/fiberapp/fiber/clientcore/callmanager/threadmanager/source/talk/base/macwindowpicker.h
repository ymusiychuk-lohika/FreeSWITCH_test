// Copyright 2010 Google Inc. All Rights Reserved


#ifndef TALK_BASE_MACWINDOWPICKER_H_
#define TALK_BASE_MACWINDOWPICKER_H_

#include "talk/base/scoped_ptr.h"
#include "talk/base/windowpicker.h"
#include "macwindowpickerobjc.h"
#include <CoreVideo/CoreVideo.h>

namespace talk_base {

class MacWindowPicker : public WindowPicker {
 public:
  MacWindowPicker();
  ~MacWindowPicker();
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
  virtual uint8_t* GetApplicationThumbnail(const ApplicationDescription& desc,
                                           int width, int height);
  virtual void FreeThumbnail(uint8_t* thumbnail);
  virtual bool RegisterAppTerminateNotify(ProcessId pid, int desktopId);
  virtual bool UnregisterAppTerminateNotify(ProcessId pid, int desktopId);
  virtual bool IsApplicationSharingSupported() { return true; }

  void set_watcher(DesktopWatcher* watcher) { watcher_.reset(watcher); }
  DesktopWatcher* watcher() { return watcher_.get(); }
  int GetDesktopLayoutChangedFlags();

 private:
  bool GetApplicationListInternal(ApplicationDescriptionList* descriptions, bool minimized);
  talk_base::scoped_ptr<DesktopWatcher> watcher_;
  DesktopId sharedDesktop_;
};
    
class MacDesktopWatcher
  : public DesktopWatcher {
public:
  explicit MacDesktopWatcher(MacWindowPicker* wp);
  virtual ~MacDesktopWatcher();
  virtual bool Start();
  virtual void Stop();
  virtual bool RegisterAppTerminate(ProcessId pid);
  virtual bool UnregisterAppTerminate(ProcessId pid);
  virtual void AppTerminated(ProcessId pid);

private:
  static void DisplayChangeCallback(CGDirectDisplayID display, CGDisplayChangeSummaryFlags flags, void* userInfo);
  void SendChangeNotification();
  MacWindowPicker* picker_;
  BJNApplicationNotification* termNotify_;
};

}  // namespace talk_base

#endif  // TALK_BASE_MACWINDOWPICKER_H_
