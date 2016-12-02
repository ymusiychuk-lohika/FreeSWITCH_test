// Copyright 2010 Google Inc. All Rights Reserved


#include "talk/base/macwindowpicker.h"
#include "talk/base/macwindowpickerobjc.h"

#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <dlfcn.h>

#include "talk/base/logging_libjingle.h"
#include "talk/base/macutils.h"

namespace talk_base {
    
static const uint32_t kMaxDisplays = 16;

MacWindowPicker::MacWindowPicker() {
  set_watcher(new MacDesktopWatcher(this));
  GetDesktopList(&lastDesktopList_);
}

MacWindowPicker::~MacWindowPicker() {
  watcher()->Stop();
}

bool MacWindowPicker::Init() {
  watcher()->Start();
  return true;
}

bool MacWindowPicker::IsVisible(const WindowId& id) {
  // Init if we're not already inited.
  if (!Init()) {
    return false;
  }
  CGWindowID ids[1];
  ids[0] = id.id();
  CFArrayRef window_id_array =
      CFArrayCreate(NULL, reinterpret_cast<const void **>(&ids), 1, NULL);

  CFArrayRef window_array = CGWindowListCreateDescriptionFromArray(window_id_array);
  if (window_array == NULL || 0 == CFArrayGetCount(window_array)) {
    // Could not find the window. It might have been closed.
    LOG(LS_INFO) << "Window not found";
    CFRelease(window_id_array);
    return false;
  }

  CFDictionaryRef window = reinterpret_cast<CFDictionaryRef>(
      CFArrayGetValueAtIndex(window_array, 0));
  CFBooleanRef is_visible = reinterpret_cast<CFBooleanRef>(
      CFDictionaryGetValue(window, kCGWindowIsOnscreen));

  // Check that the window is visible. If not we might crash.
  bool visible = false;
  if (is_visible != NULL) {
    visible = CFBooleanGetValue(is_visible);
  }
  CFRelease(window_id_array);
  CFRelease(window_array);
  return visible;
}

bool MacWindowPicker::MoveToFront(const WindowId& id) {
  // Init if we're not already initialized.
  if (!Init()) {
    return false;
  }
  CGWindowID ids[1];
  ids[0] = id.id();
  CFArrayRef window_id_array =
      CFArrayCreate(NULL, reinterpret_cast<const void **>(&ids), 1, NULL);

  CFArrayRef window_array = CGWindowListCreateDescriptionFromArray(window_id_array);
  if (window_array == NULL || 0 == CFArrayGetCount(window_array)) {
    // Could not find the window. It might have been closed.
    LOG(LS_INFO) << "Window not found";
    CFRelease(window_id_array);
    return false;
  }

  CFDictionaryRef window = reinterpret_cast<CFDictionaryRef>(
      CFArrayGetValueAtIndex(window_array, 0));
  CFStringRef window_name_ref = reinterpret_cast<CFStringRef>(
      CFDictionaryGetValue(window, kCGWindowName));
  CFNumberRef application_pid = reinterpret_cast<CFNumberRef>(
      CFDictionaryGetValue(window, kCGWindowOwnerPID));

  // bring the application to the front
  int pid_val;
  CFNumberGetValue(application_pid, kCFNumberIntType, &pid_val);
  ProcessSerialNumber psn;
  OSStatus osErr = GetProcessForPID(pid_val, &psn);
  if (osErr == noErr)
  {
    SetFrontProcess(&psn);
  }
  return true;
}

bool MacWindowPicker::GetDesktopList(DesktopDescriptionList* descriptions) {
  CGDirectDisplayID active_displays[kMaxDisplays];
  uint32_t display_count = 0;

  CGError err = CGGetActiveDisplayList(kMaxDisplays,
                                       active_displays,
                                       &display_count);
  if (err != kCGErrorSuccess) {
    LOG_E(LS_ERROR, OS, err) << "Failed to enumerate the active displays.";
    return false;
  }
  for (uint32_t i = 0; i < display_count; ++i) {
    DesktopId id(active_displays[i], static_cast<int>(i));
    // TODO: Figure out an appropriate desktop title.
    DesktopDescription desc(id, "");
    desc.set_primary(CGDisplayIsMain(id.id()));
    descriptions->push_back(desc);
  }
  return display_count > 0;
}

bool MacWindowPicker::GetDesktopDimensions(const DesktopId& id,
                                           int* width,
                                           int* height) {
  *width = CGDisplayPixelsWide(id.id());
  *height = CGDisplayPixelsHigh(id.id());
  return true;
}

uint8_t* MacWindowPicker::GetDesktopThumbnail(const DesktopId& id,
                                              int width,
                                              int height) {
  uint8_t* buffer = NULL;

  // get the display bounds
  CGRect displayBounds = CGDisplayBounds(id.id());
  // adjust the display bound so we clip out the green border
  displayBounds.origin.x += kGreenBorderThickness;
  displayBounds.origin.y += kGreenBorderThickness;
  displayBounds.size.width -= (kGreenBorderThickness * 2);
  displayBounds.size.height -= (kGreenBorderThickness * 2);
  // grab a screen shot
  CGImageRef screenShot = CGWindowListCreateImage(displayBounds,
                                                  kCGWindowListOptionOnScreenOnly,
                                                  kCGNullWindowID,
                                                  kCGWindowImageDefault);
  if (screenShot)
  {
      // scale it to a thumbnail
      CGImageRef thumbnail = NULL;
      CGRect thumbRect = CGRectMake(0, 0, width, height);
      // create a context so we can scale the source image
      CGContextRef context = CGBitmapContextCreate(NULL,
                                                   thumbRect.size.width,
                                                   thumbRect.size.height,
                                                   8,
                                                   thumbRect.size.width * 4,
                                                   CGColorSpaceCreateDeviceRGB(),
                                                   kCGImageAlphaNoneSkipFirst);
      if (context)
      {
          CGContextDrawImage(context, thumbRect, screenShot);
          thumbnail = CGBitmapContextCreateImage(context);
          CGContextRelease(context);
          
          // TODO: fix this, this is temp code, return the CGImageRef
          buffer = (uint8_t*) thumbnail;
      }
      CFRelease(screenShot);
  }

  return buffer;
}

uint8_t* MacWindowPicker::GetWindowThumbnail(const WindowDescription& desc,
                                             int width,
                                             int height)
{
    uint8_t* buffer = NULL;

    // get the display bounds
    CGRect displayBounds = CGDisplayBounds(desc.id().id());
    // adjust the display bound so we clip out the green border

    // grab a screen shot
    CGImageRef screenShot = CGWindowListCreateImage(displayBounds,
                                                    kCGWindowListOptionIncludingWindow,
                                                    desc.id().id(),
                                                    kCGWindowImageDefault);
    if (screenShot)
    {
        // scale it to a thumbnail
        CGImageRef thumbnail = NULL;
        CGRect thumbRect = CGRectMake(0, 0, width, height);
        // create a context so we can scale the source image
        CGContextRef context = CGBitmapContextCreate(NULL,
                                                     thumbRect.size.width,
                                                     thumbRect.size.height,
                                                     8,
                                                     thumbRect.size.width * 4,
                                                     CGColorSpaceCreateDeviceRGB(),
                                                     kCGImageAlphaNoneSkipFirst);
        if (context)
        {
            CGContextDrawImage(context, thumbRect, screenShot);
            thumbnail = CGBitmapContextCreateImage(context);
            CGContextRelease(context);
            
            // TODO: fix this, this is temp code, return the CGImageRef
            buffer = (uint8_t*) thumbnail;
        }
        CFRelease(screenShot);
    }

    return buffer;
}
    
uint8_t* MacWindowPicker::GetApplicationThumbnail(const ApplicationDescription& desc,
                                             int width,
                                             int height)
{
    uint8_t* buffer = NULL;
    
    CGImageRef appIcon = GetApplicationIcon(desc.id(), width, height);
    if (appIcon)
    {
        // check to see if the size returned was what we requested
        if ((width != (int) CGImageGetWidth(appIcon)) || (height != CGImageGetHeight(appIcon)))
        {
            // we have to scale the image
            CGImageRef thumbnail = NULL;
            CGRect thumbRect = CGRectMake(0, 0, width, height);
            // create a context so we can scale the source image
            CGContextRef context = CGBitmapContextCreate(NULL,
                                                         thumbRect.size.width,
                                                         thumbRect.size.height,
                                                         8,
                                                         thumbRect.size.width * 4,
                                                         CGColorSpaceCreateDeviceRGB(),
                                                         kCGImageAlphaPremultipliedLast);
            if (context)
            {
                CGContextDrawImage(context, thumbRect, appIcon);
                thumbnail = CGBitmapContextCreateImage(context);
                CGContextRelease(context);
                
                // TODO: fix this, this is temp code, return the CGImageRef
                buffer = (uint8_t*) thumbnail;
            }
            CFRelease(appIcon);
        }
        else
        {
            // return the image as it's the requested size
            buffer = (uint8_t*) appIcon;
        }
    }

    return buffer;
}

void MacWindowPicker::FreeThumbnail(uint8_t* thumbnail) {
  CGImageRelease((CGImageRef)thumbnail);
}

bool MacWindowPicker::GetWindowList(WindowDescriptionList* descriptions) {
  // Init if we're not already inited.
  if (!Init()) {
    return false;
  }

  // Only get onscreen, non-desktop windows.
  CFArrayRef window_array = CGWindowListCopyWindowInfo(
          kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
          kCGNullWindowID);
  if (window_array == NULL) {
    return false;
  }

  // Check windows to make sure they have an id, title, and use window layer 0.
  CFIndex i;
  CFIndex count = CFArrayGetCount(window_array);
  for (i = 0; i < count; ++i) {
    CFDictionaryRef window = reinterpret_cast<CFDictionaryRef>(
        CFArrayGetValueAtIndex(window_array, i));
    CFStringRef window_title = reinterpret_cast<CFStringRef>(
        CFDictionaryGetValue(window, kCGWindowName));
    CFNumberRef window_id = reinterpret_cast<CFNumberRef>(
        CFDictionaryGetValue(window, kCGWindowNumber));
    CFNumberRef window_layer = reinterpret_cast<CFNumberRef>(
        CFDictionaryGetValue(window, kCGWindowLayer));
    if (window_title != NULL && window_id != NULL && window_layer != NULL) {
      std::string title_str;
      int id_val, layer_val;
      ToUtf8(window_title, &title_str);
      CFNumberGetValue(window_id, kCFNumberIntType, &id_val);
      CFNumberGetValue(window_layer, kCFNumberIntType, &layer_val);

      // Discard windows without a title.
      if (layer_val == 0 && title_str.length() > 0) {
        WindowId id(static_cast<CGWindowID>(id_val));
        WindowDescription desc(id, title_str);
        descriptions->push_back(desc);
      }
    }
  }

  CFRelease(window_array);
  return true;
}

bool MacWindowPicker::GetApplicationList(ApplicationDescriptionList* descriptions) {
    // Init if we're not already inited.
    if (!Init()) {
        return false;
    }
    // we get the list of windows in 2 step to preserve the MRU order as best we can
    // the first call gets all non minimized windows in MRU order, but as this list
    // doesn't include minimized windows we must then add the minimized windows.
    // If we only get the list once with all Windows then the list isn't ordered

    // get all the on screen windows that are visible
    bool ok = GetApplicationListInternal(descriptions, false);
    if (ok) {
      // add in any minimized windows
      ok = GetApplicationListInternal(descriptions, true);
    }

    return ok;
}
    
bool MacWindowPicker::GetApplicationListInternal(ApplicationDescriptionList* descriptions, bool minimized) {    
    // Only get onscreen, non-desktop windows.
    CGWindowListOption option = kCGWindowListExcludeDesktopElements;
    if (!minimized)
    {
        // this returns all non minimized windows in MRU order
        option |= kCGWindowListOptionOnScreenOnly;
    }
    CFArrayRef window_array = CGWindowListCopyWindowInfo(
        option,
        kCGNullWindowID);
    if (window_array == NULL) {
        return false;
    }
    
    CGDirectDisplayID activeList[kMaxDisplays];
    uint32_t activeCount = 0;
    CGGetActiveDisplayList(kMaxDisplays, activeList, &activeCount);
    
    // Check windows to make sure they have an id, title, and use window layer 0.
    CFIndex i;
    CFIndex count = CFArrayGetCount(window_array);
    for (i = 0; i < count; ++i) {
        CFDictionaryRef window = reinterpret_cast<CFDictionaryRef>(
                                                                   CFArrayGetValueAtIndex(window_array, i));
        CFStringRef window_title = reinterpret_cast<CFStringRef>(
                                                                 CFDictionaryGetValue(window, kCGWindowName));
        CFStringRef window_owner = reinterpret_cast<CFStringRef>(
                                                                 CFDictionaryGetValue(window, kCGWindowOwnerName));
        CFNumberRef window_id = reinterpret_cast<CFNumberRef>(
                                                              CFDictionaryGetValue(window, kCGWindowNumber));
        CFNumberRef window_layer = reinterpret_cast<CFNumberRef>(
                                                                 CFDictionaryGetValue(window, kCGWindowLayer));
        if (window_title != NULL && window_id != NULL && window_layer != NULL) {
            std::string title_str;
            std::string owner_str;
            int id_val, layer_val;
            ToUtf8(window_title, &title_str);
            ToUtf8(window_owner, &owner_str);
            CFNumberGetValue(window_id, kCFNumberIntType, &id_val);
            CFNumberGetValue(window_layer, kCFNumberIntType, &layer_val);
            
            // Discard windows with a layer other than 0
            if (layer_val == 0) {
                CFNumberRef window_pid = reinterpret_cast<CFNumberRef>(
                    CFDictionaryGetValue(window, kCGWindowOwnerPID));
                pid_t pid = 0;
                CFNumberGetValue(window_pid, kCFNumberIntType, &pid);
                CFDictionaryRef window_bounds = reinterpret_cast<CFDictionaryRef>(
                    CFDictionaryGetValue(window, kCGWindowBounds));
                CGRect windowBounds;
                CGRectMakeWithDictionaryRepresentation(window_bounds, &windowBounds);
                
                uint32_t monitorIndex = 0;
                // if we have more than 1 monitor work out which monitor the majority of the window is on
                if (activeCount > 1)
                {
                    CGRect activeListRects[kMaxDisplays];
                    unsigned int activeListArea[kMaxDisplays] = { 0 };
                    for (uint32_t i = 0; i < activeCount; i++)
                    {
                        activeListRects[i] = CGDisplayBounds(activeList[i]);
                        CGRect intersectRect = CGRectIntersection(windowBounds, activeListRects[i]);
                        if (!CGRectIsNull(intersectRect))
                        {
                            activeListArea[i] += (intersectRect.size.width * intersectRect.size.height);
                        }
                    }
                    uint32_t maxArea = 0;
                    for (uint32_t i = 0; i < activeCount; i++)
                    {
                        if (activeListArea[i] > maxArea)
                        {
                            maxArea = activeListArea[i];
                            monitorIndex = i;
                        }
                    }
                }
                
                ProcessId id(pid);
                WindowDescription windowId(id_val, title_str, monitorIndex);
                // as we are enumerating windows not process we may have already added
                // this process to our list, check first before adding
                // TODO: which window title should we use if there is more than 1 window?
                ApplicationDescriptionList::iterator it;
                for (it = descriptions->begin(); it != descriptions->end(); ++it)
                {
                    if (it->id() == id)
                    {
                        // we already have the process, skip
                        it->add_window(windowId);
                        break;
                    }
                }
                if (it == descriptions->end())
                {
                    ApplicationDescription desc(id, owner_str);
                    desc.add_window(windowId);
                    descriptions->push_back(desc);
                }
            }
        }
    }
    
    CFRelease(window_array);
    return true;
}
    

int MacWindowPicker::GetDesktopLayoutChangedFlags() {
    // we compare the last saved desktop list with the current one to
    // check to see if it's just the resolution that has changed
    DesktopDescriptionList currentList;
    GetDesktopList(&currentList);
    int changed = DesktopLayoutHasChanged(lastDesktopList_, currentList, sharedDesktop_);
    // update the last list
    lastDesktopList_.clear();
    lastDesktopList_ = currentList;
    
    return changed;
}

bool MacWindowPicker::RegisterAppTerminateNotify(ProcessId pid, int desktopId) {
    if (pid == 0) {
        sharedDesktop_ = lastDesktopList_[desktopId].id();
    } else {
        watcher()->RegisterAppTerminate(pid);
    }
    return true;
}
    
bool MacWindowPicker::UnregisterAppTerminateNotify(ProcessId pid, int desktopId) {
    if (pid == 0) {
        sharedDesktop_ = DesktopId(0, 0);
    } else {
        watcher()->UnregisterAppTerminate(pid);
    }
    return true;
}

MacDesktopWatcher::MacDesktopWatcher(MacWindowPicker* picker)
  : DesktopWatcher(picker)
  , picker_(picker)
  , termNotify_(NULL)
{
}

MacDesktopWatcher::~MacDesktopWatcher() {
}

bool MacDesktopWatcher::Start() {
  // register for display change callbacks
  CGDisplayRegisterReconfigurationCallback(DisplayChangeCallback, (void*)this);
  return true;
}

void MacDesktopWatcher::Stop() {
  // remove display change callback registration
  CGDisplayRemoveReconfigurationCallback(DisplayChangeCallback, (void*)this);
}

bool MacDesktopWatcher::RegisterAppTerminate(ProcessId pid) {
    UnregisterAppTerminate(0);
    LOG(LS_INFO) << "AddAppTermNotify(" << pid << ")";
    termNotify_ = AddAppTermNotify(pid, this);
    return true;
}

bool MacDesktopWatcher::UnregisterAppTerminate(ProcessId pid) {
    if (termNotify_) {
        LOG(LS_INFO) << "RemoveAppTermNotify(" << pid << ")";
        RemoveAppTermNotify(termNotify_);
        termNotify_ = NULL;
    }
    return true;
}

void MacDesktopWatcher::AppTerminated(ProcessId pid) {
    picker_->SignalDesktopsChanged(false, true);
}
    
void MacDesktopWatcher::SendChangeNotification() {
  // check to see if the desktop layout has changed
  int layoutChanged = picker_->GetDesktopLayoutChangedFlags();
  picker_->SignalDesktopsChanged((layoutChanged & kDesktopLayoutChanged),
        (layoutChanged & kDesktopSharingShouldEnd));
}
    
void MacDesktopWatcher::DisplayChangeCallback(CGDirectDisplayID display, CGDisplayChangeSummaryFlags flags, void* userInfo)
{
  // we get this notification for each screen, we just check when the main screen is notified
  if (kCGDirectMainDisplay == display)
  {
    if (flags & kCGDisplayBeginConfigurationFlag)
    {
        // do nothing, this is called Before any changes take place
    }
    else
    {
      MacDesktopWatcher* obj = (MacDesktopWatcher*) userInfo;
      if (userInfo)
      {
        obj->SendChangeNotification();
      }
    }
  }
}

}  // namespace talk_base
