// Copyright 2010 Google Inc. All Rights Reserved

//         thorcarpenter@google.com (Thor Carpenter)

#ifndef TALK_BASE_WINDOWPICKER_H_
#define TALK_BASE_WINDOWPICKER_H_

#include <string>
#include <vector>
#include <map>

#include "talk/base/window.h"
#include "talk/base/sigslot.h"
#include "talk/base/logging_libjingle.h"

#define INTEGRITY_LOW       "low"
#define INTEGRITY_MEDIUM    "medium"
#define INTEGRITY_HIGH      "high"
#define INTEGRITY_UNKNOWN   "unknown"

namespace talk_base {

// note this value must match the one defined the in the webrtc capture code
enum { kGreenBorderThickness = 5 };

// Define ProcessId for each platform.
#if defined(LINUX)
typedef pid_t ProcessId;
#elif defined(WIN32)
typedef DWORD ProcessId;
#elif defined(OSX)
typedef int ProcessId;
#else
typedef unsigned int ProcessId;
#endif

class WindowDescription {
 public:
  WindowDescription() : id_(),bSlideShowWnd_(false) {}
  WindowDescription(const WindowId& id, const std::string& title)
      : id_(id), title_(title), monitor_(0), bSlideShowWnd_(false){
  }
  WindowDescription(const WindowId& id, const std::string& title, const uint32_t monitor)
      : id_(id), title_(title), monitor_(monitor), bSlideShowWnd_(false) {
  }
  const WindowId& id() const { return id_; }
  const unsigned int numerical_id() { return (unsigned int) id_.id(); }
  void set_id(const WindowId& id) { id_ = id; }
  const std::string& title() const { return title_; }
  void set_title(const std::string& title) { title_ = title; }
  const uint32_t monitor() const { return monitor_; }
  void set_monitor(const uint32_t monitor) { monitor_ = monitor; }
  void set_SlideShowWnd(const bool slideShow) { bSlideShowWnd_ = slideShow; }
  const bool SlideShowWnd() const { return bSlideShowWnd_; }

 private:
  WindowId id_;
  std::string title_;
  uint32_t monitor_;
  bool bSlideShowWnd_;
};

typedef std::vector<WindowDescription> WindowDescriptionList;

class DesktopDescription {
 public:
  DesktopDescription() : id_() {}
  DesktopDescription(const DesktopId& id, const std::string& title)
      : id_(id), title_(title), primary_(false) {
  }
  const DesktopId& id() const { return id_; }
  void set_id(const DesktopId& id) { id_ = id; }
  const std::string& title() const { return title_; }
  void set_title(const std::string& title) { title_ = title; }
  // Indicates whether it is the primary desktop in the system.
  bool primary() const { return primary_; }
  void set_primary(bool primary) { primary_ = primary; }

 private:
  DesktopId id_;
  std::string title_;
  bool primary_;
};

typedef std::vector<DesktopDescription> DesktopDescriptionList;

class ApplicationDescription {
 public:
  ApplicationDescription() :
        id_(),
        integrity_level_(INTEGRITY_UNKNOWN)
  {}

  ApplicationDescription(const ProcessId& id, const std::string& name)
      : id_(id),
        name_(name),
        integrity_level_(INTEGRITY_UNKNOWN)
  {}

  const ProcessId& id() const { return id_; }
  void set_id(const ProcessId& id) { id_ = id; }
  const std::string& name() const { return name_; }
  void set_name(const std::string& name) { name_ = name; }
  const std::string& file_description() const { return file_description_; }
  void set_file_description(const std::string& desc) { file_description_ = desc; }
  const std::string& product_name() const { return product_name_; }
  void set_product_name(const std::string& name) { product_name_ = name; }
  void add_window(const WindowDescription& window) {
    windows_.push_back(window);
  }
  const WindowDescriptionList& get_windows() { return windows_; }

#if defined(WIN32)
  void set_integrity_level(IntegrityLevel il) {
      switch(il)
      {
      case Low:
          integrity_level_ = INTEGRITY_LOW;
          break;
      case Medium:
          integrity_level_ = INTEGRITY_MEDIUM;
          break;
      case High:
          integrity_level_ = INTEGRITY_HIGH;
          break;
      default:
          integrity_level_ = INTEGRITY_UNKNOWN;
          break;
      }
  }
#endif // WIN32

  const std::string& integrity_level() { return integrity_level_;}

 private:
  ProcessId id_;
  std::string name_;
  std::string file_description_;
  std::string product_name_;
  std::string integrity_level_;
  WindowDescriptionList windows_;
};

typedef std::vector<ApplicationDescription> ApplicationDescriptionList;

class WindowPicker {
 public:
  virtual ~WindowPicker() {}
  virtual bool Init() = 0;

  // TODO: Move this two methods to window.h when we no longer need to load
  // CoreGraphics dynamically.
  virtual bool IsVisible(const WindowId& id) = 0;
  virtual bool MoveToFront(const WindowId& id) = 0;

  // Gets a list of window description and appends to descriptions.
  // Returns true if successful.
  virtual bool GetWindowList(WindowDescriptionList* descriptions) = 0;
  // Gets a list of desktop descriptions and appends to descriptions.
  // Returns true if successful.
  virtual bool GetDesktopList(DesktopDescriptionList* descriptions) = 0;
  // Gets a list of application description and appends to descriptions.
  // Returns true if successful.
  virtual bool GetApplicationList(ApplicationDescriptionList* descriptions) = 0;
  // Gets the width and height of a desktop.
  // Returns true if successful.
  virtual bool GetDesktopDimensions(const DesktopId& id, int* width,
                                    int* height) = 0;
  // Returns an allocated buffer containing a 32bpp snapshot of the desktop
  virtual uint8_t* GetDesktopThumbnail(const DesktopId& id, int width,
                                     int height) = 0;
  // Returns an allocated buffer containing a 32bpp snapshot of the window
  virtual uint8_t* GetWindowThumbnail(const WindowDescription& desc, int width,
                                     int height) = 0;
  // Returns an allocated buffer containing a 32bpp snapshot of the application
  virtual uint8_t* GetApplicationThumbnail(const ApplicationDescription& desc, int width,
                                     int height) = 0;
  virtual void FreeThumbnail(uint8_t* thumbnail) = 0;
  // register for notifications when the app with pid terminates
  virtual bool RegisterAppTerminateNotify(ProcessId pid, int desktopId) = 0;
  virtual bool UnregisterAppTerminateNotify(ProcessId pid, int desktopId) = 0;
  // does this platform support application sharing
  virtual bool IsApplicationSharingSupported() = 0;

  sigslot::signal2<bool, bool> SignalDesktopsChanged;
  DesktopDescriptionList lastDesktopList_;
};

class DesktopWatcher {
 public:
  explicit DesktopWatcher(WindowPicker* wp) {}
  virtual ~DesktopWatcher() {}
  virtual bool Start() { return true; }
  virtual void Stop() {}
  virtual bool RegisterAppTerminate(ProcessId pid) { return true; }
  virtual bool UnregisterAppTerminate(ProcessId pid) { return true; }
  virtual void AppTerminated(ProcessId pid) { }
};

enum {
    kDesktopLayoutChanged       = 0x0001,
    kDesktopSharingShouldEnd    = 0x0002,
};

inline int DesktopLayoutHasChanged(DesktopDescriptionList& previous,
                                    DesktopDescriptionList& current,
                                    DesktopId& currentDesktop) {
    int changedFlags = 0;
    // first check if we had one monitor before and after, in this case
    // we don't end sharing even if the monitor has changed
    changedFlags |= kDesktopSharingShouldEnd;
    if ((previous.size() == 1) && (current.size() == 1))
    {
        LOG(LS_INFO) << "Supressing end sharing as single monitor before and after";
        changedFlags &= ~kDesktopSharingShouldEnd;
    }
    else
    {
        // We check to see if the shared desktop is still
        // in the new list if it is we don't end sharing
        for (size_t i = 0; i < current.size(); i++)
        {
            if (current[i].id().id() == currentDesktop.id())
            {
                LOG(LS_INFO) << "Supressing end sharing as shared monitor is still active";
                changedFlags &= ~kDesktopSharingShouldEnd;
                break;
            }
        }
    }

    // we check to see if the monitor layout has changed, this
    // may not end sharing which is why it's a separate flag
    if (previous.size() != current.size())
    {
        // a monitor added or removed
        changedFlags |= kDesktopLayoutChanged;
    }
    else
    {
        // same number of monitors, has anything changed?
        for (size_t i = 0; i < previous.size(); i++)
        {
            if ((previous[i].primary() != current[i].primary())
                || (previous[i].id().id() != current[i].id().id()))
            {
                changedFlags |= kDesktopLayoutChanged;
                break;
            }
        }
    }

    // log what has changed or not
    LOG(LS_INFO) << "=== Monitor layout has changed, sharedId: " << currentDesktop.id() << " ====";
    for (size_t i = 0; i < previous.size(); i++)
    {
        LOG(LS_INFO) << "Before(" << i << "): primary " << previous[i].primary()
            << " id " << previous[i].id().id() << " index " << previous[i].id().index();
    }
    for (size_t i = 0; i < current.size(); i++)
    {
        LOG(LS_INFO) << "After(" << i << "): primary " << current[i].primary()
            << " id " << current[i].id().id() << " index " << current[i].id().index();
    }
    LOG(LS_INFO) << "=== layoutChanged: " << (changedFlags & kDesktopLayoutChanged)
        << ", endSharing: " << (changedFlags & kDesktopSharingShouldEnd) << " ====";

    return changedFlags;
}

}  // namespace talk_base

#endif  // TALK_BASE_WINDOWPICKER_H_
