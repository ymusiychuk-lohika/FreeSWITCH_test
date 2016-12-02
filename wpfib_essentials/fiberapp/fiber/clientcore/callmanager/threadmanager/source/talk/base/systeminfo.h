/*
 * libjingle
 * Copyright 2008 Google Inc.
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

#ifndef TALK_BASE_SYSTEMINFO_H__
#define TALK_BASE_SYSTEMINFO_H__

#include <string>
#include<vector>

#include "talk/base/basictypes.h"

namespace talk_base {

#ifdef WIN32
class WinCPUParmas;
#endif //WIN32

class SystemInfo {
 public:
  enum Architecture {
    ARCH_X86 = 0,
    ARCH_X64 = 1,
    ARCH_ARM = 2
  };

  SystemInfo();
  ~SystemInfo();

  // The number of CPU Cores in the system.
  int GetMaxPhysicalCpus();
  // The number of CPU Threads in the system.
  int GetMaxCpus();
  // The number of CPU Threads currently available to this process.
  int GetCurCpus();
  // Identity of the CPUs.
  Architecture GetCpuArchitecture();
  std::string GetCpuVendor();
  std::string GetCpuProcessor();
  int GetCpuFamily();
  int GetCpuModel();
  int GetCpuStepping();
  // Estimated speed of the CPUs, in MHz.
  int GetMaxCpuSpeed();
  int GetCurCpuSpeed();
  // Total amount of physical memory, in bytes.
  int64 GetMemorySize();
  // The model name of the machine, e.g. "MacBookAir1,1"
  std::string GetMachineModel();

#ifdef WIN32
  // The model name of the machine, fetched from BaseBoardManufacturer and BaseBoardProduct registry entries, only for Windows
  std::string GetBaseBoardMachineModel();
#endif //WIN32

  // The gpu identifier
  struct GpuInfo {
    GpuInfo() : vendor_id(0), device_id(0), displays_attached(0)
    {
        device_name.assign("Not Available");
        description.assign("Not Available");
        driver.assign("Not Available");
        driver_version.assign("Not Available");
    }
    std::string device_name;
    std::string description;
    int vendor_id;
    int device_id;
    //displays_attached - Number of Display outputs connected to the GPU
    unsigned int displays_attached;
    std::string driver;
    std::string driver_version;
  };
  bool GetGpuInfo(std::vector<GpuInfo> *info);

 private:
  int physical_cpus_;
  int logical_cpus_;
  Architecture cpu_arch_;
  std::string cpu_vendor_;
  std::string cpu_processor_;
  int cpu_family_;
  int cpu_model_;
  int cpu_stepping_;
  int cpu_speed_;
  int64 memory_;
  std::string machine_model_;
#ifdef WIN32
  std::string baseboard_machine_model_;
  WinCPUParmas* pwin32_cpu_params_;
#endif //WIN32
};

}

#endif  // TALK_BASE_SYSTEMINFO_H__
