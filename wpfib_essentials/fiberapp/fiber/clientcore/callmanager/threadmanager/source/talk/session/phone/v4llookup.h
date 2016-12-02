/*
 * Copyright 2009, Google Inc.
 * Author: lexnikitin@google.com (Alexey Nikitin)
 *
 * V4LLookup provides basic functionality to work with V2L2 devices in Linux
 * The functionality is implemented as a class with virtual methods for
 * the purpose of unit testing.
 */
#ifndef TALK_SESSION_PHONE_V4LLOOKUP_H_
#define TALK_SESSION_PHONE_V4LLOOKUP_H_

#include <string>

#ifdef LINUX
namespace cricket {
class V4LLookup {
 public:
  virtual ~V4LLookup() {}

  static bool IsV4L2Device(const std::string& device_path,
                           std::string &caps_bus_info,
                           std::string &device_display_name) {
    return GetV4LLookup()->CheckIsV4L2Device(device_path, caps_bus_info, device_display_name);
  }

  static void SetV4LLookup(V4LLookup* v4l_lookup) {
    v4l_lookup_ = v4l_lookup;
  }

  static V4LLookup* GetV4LLookup() {
    if (!v4l_lookup_) {
      v4l_lookup_ = new V4LLookup();
    }
    return v4l_lookup_;
  }

 protected:
  static V4LLookup* v4l_lookup_;
  // Making virtual so it is easier to mock
  virtual bool CheckIsV4L2Device(const std::string& device_path,
                                 std::string &caps_bus_info,
                                 std::string &device_display_name);
};

}  // namespace cricket

#endif  // LINUX
#endif  // TALK_SESSION_PHONE_V4LLOOKUP_H_
