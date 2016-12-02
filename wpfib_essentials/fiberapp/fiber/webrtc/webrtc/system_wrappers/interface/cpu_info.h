/*
 *  Copyright (c) 2011 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CPU_INFO_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CPU_INFO_H_

#include "webrtc/typedefs.h"

namespace webrtc {

class CpuInfo {
 public:
  static uint32_t DetectNumberOfCores();
#if defined(BJN_WEBRTC_CHANGES)
  static uint32_t DetectNumberOfPhysicalCores();
#endif

 private:
  CpuInfo() {}
  static uint32_t number_of_cores_;
#if defined(BJN_WEBRTC_CHANGES)
  static uint32_t number_of_physical_cores_;
#endif
};

} // namespace webrtc

#endif // WEBRTC_SYSTEM_WRAPPERS_INTERFACE_CPU_INFO_H_
