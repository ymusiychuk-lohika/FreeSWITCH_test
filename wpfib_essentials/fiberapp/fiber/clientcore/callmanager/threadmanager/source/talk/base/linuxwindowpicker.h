/*
 * libjingle
 * Copyright 2010 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TALK_BASE_LINUXWINDOWPICKER_H_
#define TALK_BASE_LINUXWINDOWPICKER_H_

#include "talk/base/basictypes.h"
#include "talk/base/scoped_ptr.h"
#include "talk/base/windowpicker.h"

// Avoid include <X11/Xlib.h>.
struct _XDisplay;
typedef unsigned long Window;

namespace talk_base {

class XWindowEnumerator;

class LinuxWindowPicker : public WindowPicker {
 public:
  LinuxWindowPicker();
  ~LinuxWindowPicker();

  static bool IsDesktopElement(_XDisplay* display, Window window);

  virtual bool Init();
  virtual bool IsVisible(const WindowId& id);
  virtual bool MoveToFront(const WindowId& id);
  virtual bool GetWindowList(WindowDescriptionList* descriptions);
  virtual bool GetDesktopList(DesktopDescriptionList* descriptions);
  virtual bool GetApplicationList(ApplicationDescriptionList* descriptions);
  virtual bool GetDesktopDimensions(const DesktopId& id, int* width,
                                    int* height);
  uint8_t* GetWindowIcon(const WindowId& id, int* width, int* height);
  uint8_t* GetWindowThumbnail(const WindowDescription& desc, int width, int height);
  int GetNumDesktops();
  uint8_t* GetDesktopThumbnail(const DesktopId& id, int width, int height);
  uint8_t* GetApplicationThumbnail(const ApplicationDescription& desc, int width, int height);
  void FreeThumbnail(uint8_t* thumbnail);
  void set_watcher(DesktopWatcher* watcher) { watcher_.reset(watcher); }
  DesktopWatcher* watcher() { return watcher_.get(); }
  void SetDesktopsChanged();
  virtual bool RegisterAppTerminateNotify(ProcessId pid, int desktopId);
  virtual bool UnregisterAppTerminateNotify(ProcessId pid, int desktopId);
  virtual bool IsApplicationSharingSupported() { return false; }

 private:
  scoped_ptr<XWindowEnumerator> enumerator_;
  talk_base::scoped_ptr<DesktopWatcher> watcher_;
};

class LinuxDesktopWatcher
  : public DesktopWatcher {
public:
  explicit LinuxDesktopWatcher(LinuxWindowPicker* wp);
  virtual ~LinuxDesktopWatcher();
  virtual bool Start();
  virtual void Stop();

private:
  static void *watcherThread(void* arg);
  void notifyThread();
  LinuxWindowPicker* picker_;
  pthread_t watcher_thread_;
  bool quit_thread_;
  _XDisplay* display_;
};

}  // namespace talk_base

#endif  // TALK_BASE_LINUXWINDOWPICKER_H_
