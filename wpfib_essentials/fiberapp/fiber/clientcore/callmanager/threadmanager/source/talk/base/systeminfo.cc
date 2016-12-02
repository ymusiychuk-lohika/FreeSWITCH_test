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

#ifdef WIN32
#include "talk/base/scoped_ptr.h"
#include "talk/base/win32.h"  // first because it brings in win32 stuff
#include <tchar.h>
#include <memory>
#include "win32cpuparams.h"
#ifndef EXCLUDE_D3D9
#include <d3d9.h>
#endif
#include <dxgi.h>

#elif defined(OSX)
#include <ApplicationServices/ApplicationServices.h>
#include <CoreServices/CoreServices.h>
#include <sys/sysctl.h>
#include "talk/base/macconversion.h"
#elif defined(IOS)
#include <sys/sysctl.h>
#elif defined(LINUX) || defined(ANDROID)
#include <unistd.h>
#include "talk/base/linux.h"
#endif

#include "talk/base/common.h"
#include "talk/base/cpuid.h"
#include "talk/base/logging_libjingle.h"
#include "talk/base/stringutils.h"
#include "talk/base/systeminfo.h"

namespace talk_base {

// See Also: http://msdn.microsoft.com/en-us/library/ms683194(v=vs.85).aspx
#ifdef WIN32
typedef BOOL (WINAPI *LPFN_GLPI)(
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,
    PDWORD);

static int NumCores() {
  // GetLogicalProcessorInformation() is available on Windows XP SP3 and beyond.
  LPFN_GLPI glpi = reinterpret_cast<LPFN_GLPI>(GetProcAddress(
      GetModuleHandle(L"kernel32"),
      "GetLogicalProcessorInformation"));
  if (NULL == glpi) {
    return -1;
  }
  // Determine buffer size, allocate and get processor information.
  // Size can change between calls (unlikely), so a loop is done.
  DWORD return_length = 0;
  scoped_array<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> infos;
  while (!glpi(infos.get(), &return_length)) {
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
      infos.reset(new SYSTEM_LOGICAL_PROCESSOR_INFORMATION[
          return_length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION)]);
    } else {
      return -1;
    }
  }
  int processor_core_count = 0;
  for (size_t i = 0;
      i < return_length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++i) {
    if (infos[i].Relationship == RelationProcessorCore) {
      ++processor_core_count;
    }
  }
  return processor_core_count;
}
#endif //WIN32

// Note(fbarchard):
// Family and model are extended family and extended model.  8 bits each.
SystemInfo::SystemInfo()
    : physical_cpus_(1), logical_cpus_(1),
      cpu_family_(0), cpu_model_(0), cpu_stepping_(0),
      cpu_speed_(0), memory_(0) {
  // Initialize the basic information.

#if defined(__arm__) || defined(__aarch64__)
  cpu_arch_ = ARCH_ARM;
#elif defined(CPU_X86)
  cpu_arch_ = ARCH_X86;
#else
#error "Unknown architecture."
#endif

#ifdef WIN32
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  logical_cpus_ = si.dwNumberOfProcessors;
  physical_cpus_ = NumCores();
  if (physical_cpus_ <= 0) {
    physical_cpus_ = logical_cpus_;
  }
  cpu_family_ = si.wProcessorLevel;
  cpu_model_ = si.wProcessorRevision >> 8;
  cpu_stepping_ = si.wProcessorRevision & 0xFF;
  pwin32_cpu_params_ = new WinCPUParmas();
#elif defined(OSX) || defined(IOS)
  uint32_t sysctl_value;
  size_t length = sizeof(sysctl_value);
  if (!sysctlbyname("hw.physicalcpu_max", &sysctl_value, &length, NULL, 0)) {
    physical_cpus_ = static_cast<int>(sysctl_value);
  }
  length = sizeof(sysctl_value);
  if (!sysctlbyname("hw.logicalcpu_max", &sysctl_value, &length, NULL, 0)) {
    logical_cpus_ = static_cast<int>(sysctl_value);
  }
  length = sizeof(sysctl_value);
  if (!sysctlbyname("machdep.cpu.family", &sysctl_value, &length, NULL, 0)) {
    cpu_family_ = static_cast<int>(sysctl_value);
  }
  length = sizeof(sysctl_value);
  if (!sysctlbyname("machdep.cpu.model", &sysctl_value, &length, NULL, 0)) {
    cpu_model_ = static_cast<int>(sysctl_value);
  }
  length = sizeof(sysctl_value);
  if (!sysctlbyname("machdep.cpu.stepping", &sysctl_value, &length, NULL, 0)) {
    cpu_stepping_ = static_cast<int>(sysctl_value);
  }
#else // LINUX || ANDROID
  ProcCpuInfo proc_info;
  if (proc_info.LoadFromSystem()) {
    proc_info.GetNumCpus(&logical_cpus_);
    proc_info.GetNumPhysicalCpus(&physical_cpus_);
    proc_info.GetCpuFamily(&cpu_family_);
#if !defined(__arm__)
    // These values aren't found on ARM systems.
    proc_info.GetSectionIntValue(0, "model", &cpu_model_);
    proc_info.GetSectionIntValue(0, "stepping", &cpu_stepping_);
    proc_info.GetSectionIntValue(0, "cpu MHz", &cpu_speed_);
#endif
  }

  // ProcCpuInfo reads cpu speed from "cpu MHz" under /proc/cpuinfo.
  // But that number is a moving target which can change on-the-fly according to
  // many factors including system workload.
  // See /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors.
  // The one in /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq is more
  // accurate. We use it as our cpu speed when it is available.
  int max_freq = talk_base::ReadCpuMaxFreq() / 1000;   // /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq has it in KHz and we want in MHz
  if (max_freq > 0) {
    cpu_speed_ = max_freq;
  }
#endif
}

SystemInfo::~SystemInfo()
{
#ifdef WIN32
    if(pwin32_cpu_params_)
        delete pwin32_cpu_params_;
#endif //WIN32
}


// Return the number of cpu threads available to the system.
int SystemInfo::GetMaxCpus() {
  return logical_cpus_;
}

// Return the number of cpu cores available to the system.
int SystemInfo::GetMaxPhysicalCpus() {
  return physical_cpus_;
}

// Return the number of cpus available to the process.  Since affinity can be
// changed on the fly, do not cache this value.
// Can be affected by heat.
int SystemInfo::GetCurCpus() {
  int cur_cpus;
#ifdef WIN32
  DWORD_PTR process_mask, system_mask;
  ::GetProcessAffinityMask(::GetCurrentProcess(), &process_mask, &system_mask);
  for (cur_cpus = 0; process_mask; ++cur_cpus) {
    // Sparse-ones algorithm. There are slightly faster methods out there but
    // they are unintuitive and won't make a difference on a single dword.
    process_mask &= (process_mask - 1);
  }
#elif defined(OSX)
  // Find number of _available_ cores
  cur_cpus = MPProcessorsScheduled();
#elif defined(IOS)
  uint32_t sysctl_value;
  size_t length = sizeof(sysctl_value);
  int error = sysctlbyname("hw.ncpu", &sysctl_value, &length, NULL, 0);
  cur_cpus = !error ? static_cast<int>(sysctl_value) : 1;
#else
  // Linux, Solaris, ANDROID
  cur_cpus = static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
#endif
  return cur_cpus;
}

// Return the type of this CPU.
SystemInfo::Architecture SystemInfo::GetCpuArchitecture() {
  return cpu_arch_;
}

// Returns the vendor string from the cpu, e.g. "GenuineIntel", "AuthenticAMD".
// See "Intel Processor Identification and the CPUID Instruction"
// (Intel document number: 241618)
std::string SystemInfo::GetCpuVendor() {
  if (cpu_vendor_.empty()) {
    cpu_vendor_ = talk_base::CpuInfo::GetCpuVendor();
  }
  return cpu_vendor_;
}

// Returns the CPU Processor string. Eg: "Intel(R) Core(TM) i5-4310M CPU @ 2.70GHz"
std::string SystemInfo::GetCpuProcessor()
{
    if(cpu_processor_.empty())
    {
        cpu_processor_ = talk_base::CpuInfo::GetCpuProcessor();
    }
    return cpu_processor_;
}

// Return the "family" of this CPU.
int SystemInfo::GetCpuFamily() {
  return cpu_family_;
}

// Return the "model" of this CPU.
int SystemInfo::GetCpuModel() {
  return cpu_model_;
}

// Return the "stepping" of this CPU.
int SystemInfo::GetCpuStepping() {
  return cpu_stepping_;
}

// Return the clockrate of the primary processor in Mhz.  This value can be
// cached.  Returns -1 on error.
int SystemInfo::GetMaxCpuSpeed() {
  if (cpu_speed_) {
    return cpu_speed_;
  }

#ifdef WIN32
  HKEY key;
  static const WCHAR keyName[] =
      L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0";

  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyName , 0, KEY_QUERY_VALUE, &key)
      == ERROR_SUCCESS) {
    DWORD data, len;
    len = sizeof(data);

    if (RegQueryValueEx(key, L"~Mhz", 0, 0, reinterpret_cast<LPBYTE>(&data),
                        &len) == ERROR_SUCCESS) {
      cpu_speed_ = data;
    } else {
      LOG(LS_WARNING) << "Failed to query registry value HKLM\\" << keyName
                      << "\\~Mhz";
      cpu_speed_ = -1;
    }

    RegCloseKey(key);
  } else {
    LOG(LS_WARNING) << "Failed to open registry key HKLM\\" << keyName;
    cpu_speed_ = -1;
  }
#elif defined(IOS) || defined(OSX)
  uint64_t sysctl_value;
  size_t length = sizeof(sysctl_value);
  int error = sysctlbyname("hw.cpufrequency_max", &sysctl_value, &length, NULL, 0);
  cpu_speed_ = !error ? static_cast<int>(sysctl_value/1000000) : -1;
#else
  // TODO: Implement using proc/cpuinfo
  cpu_speed_ = 0;
#endif
  return cpu_speed_;
}

#if defined (LINUX)
// For Linux
// Reads the number of cores and their current frequencies from proc/cpuinfo
// Calculates the average frequency
// Rounds the average and returns the average frequency as an integer
// Returns -1 if there is an error in reading cpu/proc_info
int GetAverageOfSpeedOfAllCoresForLinux() {
  std::string key = "cpu MHz";
  float sumOfCoreFrequencies = 0;
  int coreCount = 0;
  int val = 0;
  ProcCpuInfo proc_info;
  if (proc_info.LoadFromSystem()) {
      size_t section_count;
      proc_info.GetSectionCount(&section_count);
      for (size_t i = 0; i < section_count; ++i) {
          if (proc_info.GetSectionIntValue(i, key, &val)) {
              sumOfCoreFrequencies += val;
              coreCount++;
          };
      }
      sumOfCoreFrequencies = sumOfCoreFrequencies / coreCount;
      // Flooring the value as it is cast from float to int
      return static_cast<int>(sumOfCoreFrequencies);
  }
  else {
	  LOG(LS_WARNING) << "Failed to read /proc/cpuinfo to obtain current clock"
                         "frequencies of each core";
      return -1;
  }
}
#endif // LINUX

// Dynamically check the current clockrate, which could be reduced because of
// powersaving profiles.  Eventually for windows we want to query WMI for
// root\WMI::ProcessorPerformance.InstanceName="Processor_Number_0".frequency
int SystemInfo::GetCurCpuSpeed() {
#ifdef WIN32
  // Read the frequency value from Win32 performance counters API
  return pwin32_cpu_params_->GetFrequency();
#elif defined(IOS) || defined(OSX)
  uint64_t sysctl_value;
  size_t length = sizeof(sysctl_value);
  int error = sysctlbyname("hw.cpufrequency", &sysctl_value, &length, NULL, 0);
  return !error ? static_cast<int>(sysctl_value/1000000) : GetMaxCpuSpeed();
#elif defined(LINUX)
  // LINUX
  // Using proc/cpuinfo for Cur speed on Linux
  // Returns -1 if there is an error
  int clockRate = GetAverageOfSpeedOfAllCoresForLinux();
  return clockRate;//-1 is returned if there was an error in reading the freq
#else //ANDROID
  // TODO: Implementation to get current cpu frequency for Android.
  // Look into /sys/devices/system/cpu/cpu<coreIndex>/cpufreq/scaling_cur_freq
  return GetMaxCpuSpeed();
#endif
}

// Returns the amount of installed physical memory in Bytes.  Cacheable.
// Returns -1 on error.
int64 SystemInfo::GetMemorySize() {
  if (memory_) {
    return memory_;
  }

#ifdef WIN32
  MEMORYSTATUSEX status = {0};
  status.dwLength = sizeof(status);

  if (GlobalMemoryStatusEx(&status)) {
    memory_ = status.ullTotalPhys;
  } else {
    LOG_GLE(LS_WARNING) << "GlobalMemoryStatusEx failed.";
    memory_ = -1;
  }

#elif defined(OSX) || defined(IOS)
  size_t len = sizeof(memory_);
  int error = sysctlbyname("hw.memsize", &memory_, &len, NULL, 0);
  if (error || memory_ == 0) {
    memory_ = -1;
  }
#else
  memory_ = static_cast<int64>(sysconf(_SC_PHYS_PAGES)) *
      static_cast<int64>(sysconf(_SC_PAGESIZE));
  if (memory_ < 0) {
    LOG(LS_WARNING) << "sysconf(_SC_PHYS_PAGES) failed."
                    << "sysconf(_SC_PHYS_PAGES) " << sysconf(_SC_PHYS_PAGES)
                    << "sysconf(_SC_PAGESIZE) " << sysconf(_SC_PAGESIZE);
    memory_ = -1;
  }
#endif

  return memory_;
}

#if _WIN32

static LRESULT FetchRegistryStrValue(HKEY key, const TCHAR *name, std::string &value) {
  DWORD type;
  DWORD bufferLen = 0;

  value.clear();

  // fetch the buffer len needed
  LRESULT retVal = RegQueryValueEx(key, name, 0, &type, NULL, &bufferLen);

  if (retVal != ERROR_SUCCESS || bufferLen == 0 || type != REG_SZ)
    return retVal;

  // there is a warning in the docs that the string *might* not be NULL terminated
  // hence we tack on an additonal character
  size_t realBufferLen = bufferLen + sizeof(TCHAR);
  std::unique_ptr<BYTE[]> buffer(new BYTE[realBufferLen]);
  memset(buffer.get(), 0, realBufferLen);

  // since this is the registry, it is possible the entry could change...
  // since this is used for getting machine info, we'll simply give up if it did.
  DWORD secondPassBufferLen = bufferLen;
  retVal = RegQueryValueEx(key, name, 0, &type, buffer.get(), &secondPassBufferLen);

  if (retVal != ERROR_SUCCESS)
    return retVal;

  const TCHAR *regStr = reinterpret_cast<const TCHAR *>(buffer.get());
  value = ToUtf8(regStr, _tcsnlen(regStr, realBufferLen));
  return ERROR_SUCCESS;
}

static std::string GetMachineModelWin32() {
  std::string makeAndModel;
  HKEY regKey;

  LONG regResult = RegOpenKeyEx(
    HKEY_LOCAL_MACHINE
    , TEXT("HARDWARE\\DESCRIPTION\\System\\BIOS")
    , 0
    , KEY_QUERY_VALUE
    , &regKey);

  if (regResult != ERROR_SUCCESS) {
    LOG_GLE(LS_INFO) << "RegOpenKeyEx for BIOS info failed.";
    return makeAndModel;
  }

  FetchRegistryStrValue(regKey, TEXT("SystemManufacturer"), makeAndModel);

  std::string modelName;
  if (0 == FetchRegistryStrValue(regKey, TEXT("SystemProductName"), modelName)) {
    makeAndModel.reserve(makeAndModel.size() + modelName.size() + 2); // 2 for space and trailing NULL
    if (!makeAndModel.empty())
      makeAndModel += ' ';
    makeAndModel += modelName;
  }

  RegCloseKey(regKey);
  return makeAndModel;
}

#ifdef _WIN32
static std::string GetBaseBoardMachineModelWin32() {
    std::string makeAndModel;
    HKEY regKey;

    LONG regResult = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE
        , TEXT("HARDWARE\\DESCRIPTION\\System\\BIOS")
        , 0
        , KEY_QUERY_VALUE
        , &regKey);

    if (regResult != ERROR_SUCCESS) {
        LOG_GLE(LS_INFO) << "RegOpenKeyEx for BIOS info failed.";
        return makeAndModel;
    }

    FetchRegistryStrValue(regKey, TEXT("BaseBoardManufacturer"), makeAndModel);

    std::string modelName;
    if (0 == FetchRegistryStrValue(regKey, TEXT("BaseBoardProduct"), modelName)) {
        makeAndModel.reserve(makeAndModel.size() + modelName.size() + 2); // 2 for space and trailing NULL
        if (!makeAndModel.empty())
            makeAndModel += ' ';
        makeAndModel += modelName;
    }

    RegCloseKey(regKey);
    return makeAndModel;
}
#endif //_WIN32

#elif defined(LINUX)

// Applicable to files where the content is a single line string
static bool GetStringFromFile(const char *fileName, std::string &output)
{
    std::ifstream file(fileName);
    if (file.good())
        std::getline(file, output);
    return file.good() && !output.empty();
}

static std::string GetMachineModelLinux()
{
    // /sys/class/dmi/id/ has a number of virtual files that describe the
    // system.  We'll try to get the sys_vendor and product_name from there.

    std::string vendorStr, productStr;
    if (!GetStringFromFile("/sys/class/dmi/id/sys_vendor", vendorStr))
    {
        LOG(LS_INFO) << "Couldn't get info from /sys/class/dmi/id/sys_vendor";
        return std::string();
    }

    if (!GetStringFromFile("/sys/class/dmi/id/product_name", productStr))
    {
        LOG(LS_INFO) << "Couldn't get info from /sys/class/dmi/id/product_name";
        return std::string();
    }

    return vendorStr + ' ' + productStr;
}

#endif

// Return the name of the machine model we are currently running on.
// This is a human readable string that consists of the name and version
// number of the hardware, i.e 'MacBookAir1,1'. Returns an empty string if
// model can not be determined. The string is cached for subsequent calls.
std::string SystemInfo::GetMachineModel() {
  if (!machine_model_.empty()) {
    return machine_model_;
  }

#if defined(OSX) || defined(IOS)
  char buffer[128];
  size_t length = sizeof(buffer);
  int error = sysctlbyname("hw.model", buffer, &length, NULL, 0);
  if (!error) {
    machine_model_.assign(buffer, length - 1);
  } else {
    machine_model_.clear();
  }
#elif _WIN32
  machine_model_ = GetMachineModelWin32();
#elif defined(LINUX)
  machine_model_ = GetMachineModelLinux();
#endif
  if (machine_model_.empty())
    machine_model_ = "Not available";

  return machine_model_;
}

#ifdef _WIN32
std::string SystemInfo::GetBaseBoardMachineModel() {
    if (!baseboard_machine_model_.empty()) {
        return baseboard_machine_model_;
    }

    baseboard_machine_model_ = GetBaseBoardMachineModelWin32();
    if (baseboard_machine_model_.empty())
        baseboard_machine_model_ = "Not available";

    return baseboard_machine_model_;
}
#endif // WIN32

#ifdef OSX
// Helper functions to query IOKit for video hardware properties.
static CFTypeRef SearchForProperty(io_service_t port, CFStringRef name) {
  return IORegistryEntrySearchCFProperty(port, kIOServicePlane,
      name, kCFAllocatorDefault,
      kIORegistryIterateRecursively | kIORegistryIterateParents);
}

static void GetProperty(io_service_t port, CFStringRef name, int* value) {
  if (!value) return;
  CFTypeRef ref = SearchForProperty(port, name);
  if (ref) {
    CFTypeID refType = CFGetTypeID(ref);
    if (CFNumberGetTypeID() == refType) {
      CFNumberRef number = reinterpret_cast<CFNumberRef>(ref);
      p_convertCFNumberToInt(number, value);
    } else if (CFDataGetTypeID() == refType) {
      CFDataRef data = reinterpret_cast<CFDataRef>(ref);
      if (CFDataGetLength(data) == sizeof(UInt32)) {
        *value = *reinterpret_cast<const UInt32*>(CFDataGetBytePtr(data));
      }
    }
    CFRelease(ref);
  }
}

static void GetProperty(io_service_t port, CFStringRef name,
                        std::string* value) {
  if (!value) return;
  CFTypeRef ref = SearchForProperty(port, name);
  if (ref) {
    CFTypeID refType = CFGetTypeID(ref);
    if (CFStringGetTypeID() == refType) {
      CFStringRef stringRef = reinterpret_cast<CFStringRef>(ref);
      p_convertHostCFStringRefToCPPString(stringRef, *value);
    } else if (CFDataGetTypeID() == refType) {
      CFDataRef dataRef = reinterpret_cast<CFDataRef>(ref);
      *value = std::string(reinterpret_cast<const char*>(
          CFDataGetBytePtr(dataRef)), CFDataGetLength(dataRef));
    }
    CFRelease(ref);
  }
}
#endif

// Fills a struct with information on the graphics adapater and returns true
// iff successful.
bool SystemInfo::GetGpuInfo(std::vector<GpuInfo> *infoList) {
  if (!infoList) return false;
  infoList->clear();
#if defined(WIN32) && !defined(EXCLUDE_D3D9)
  D3DADAPTER_IDENTIFIER9 identifier;
  HRESULT hr = E_FAIL;
  if (IsWindows7OrLater())
  {
      HMODULE hDxgi = LoadLibraryW(L"dxgi.dll");
      if (hDxgi)
      {
          typedef HRESULT(WINAPI *DXGIPROC)(REFIID riid, void **ppFactory);
          DXGIPROC create_dxgi_factory_1 = reinterpret_cast<DXGIPROC>(GetProcAddress(hDxgi, "CreateDXGIFactory1"));
          if (create_dxgi_factory_1)
          {
              IDXGIFactory1 *pFactory = NULL;
              HRESULT hr = create_dxgi_factory_1(__uuidof(IDXGIFactory1), (void**)(&pFactory));
              if (SUCCEEDED(hr))
              {
                  UINT adapterIndex = 0;
                  IDXGIAdapter1 *pAdapter = NULL;
                  while (pFactory->EnumAdapters1(adapterIndex, &pAdapter) != DXGI_ERROR_NOT_FOUND)
                  {
                      GpuInfo info;

                      // Identify GPU name
                      DXGI_ADAPTER_DESC desc;
                      pAdapter->GetDesc(&desc);
                      std::wstring gpuName = desc.Description;
                      info.description = ToUtf8(gpuName);

                      // Find the number of displays attached
                      UINT nDisplays = 0;
                      IDXGIOutput * pOutput;
                      while (pAdapter->EnumOutputs(nDisplays, &pOutput) != DXGI_ERROR_NOT_FOUND)
                      {
                          pOutput->Release();
                          ++nDisplays;
                      }
                      info.displays_attached = nDisplays;

                      infoList->push_back(info);
                      pAdapter->Release();
                      ++adapterIndex;
                  }
                  pFactory->Release();
              }
              else
              {
                  LOG(LS_ERROR) << "GetGpuInfo failed due to create_dxgi_factory_1 returned = " << hr << " error = " << GetLastError();
              }
          }
          else
          {
              LOG(LS_ERROR) << "GetGpuInfo failed due to GetProcAddress(CreateDXGIFactory1) error = " << GetLastError();
          }
          FreeLibrary(hDxgi);
      }
      else
      {
          LOG(LS_ERROR) << "GetGpuInfo failed due to LoadLibraryW(dxgi) error = " << GetLastError();
      }
  }
#if 0 // Can be enabled if we need gpu info for Win xp and Vista in future
  if(!IsWindows7OrLater())
  {
      HINSTANCE d3d_lib = LoadLibrary(L"d3d9.dll");
      if (d3d_lib)
      {
          typedef IDirect3D9* (WINAPI *D3DCreate9Proc)(UINT);
          D3DCreate9Proc d3d_create_proc = reinterpret_cast<D3DCreate9Proc>(
              GetProcAddress(d3d_lib, "Direct3DCreate9"));
          if (d3d_create_proc)
          {
              IDirect3D9* d3d = d3d_create_proc(D3D_SDK_VERSION);
              if (d3d)
              {
                  UINT numAdapters = d3d->GetAdapterCount();
                  for (int i = 0; i < numAdapters; i++)
                  {
                      hr = d3d->GetAdapterIdentifier(i, 0, &identifier);
                      if (SUCCEEDED(hr))
                      {
                          GpuInfo info;
                          info.device_name = ToUtf8(identifier.DeviceName);
                          info.description = ToUtf8(identifier.Description);
                          info.vendor_id = identifier.VendorId;
                          info.device_id = identifier.DeviceId;
                          info.driver = ToUtf8(identifier.Driver);
                          // driver_version format: product.version.subversion.build
                          std::stringstream ss;
                          ss << HIWORD(identifier.DriverVersion.HighPart) << "."
                              << LOWORD(identifier.DriverVersion.HighPart) << "."
                              << HIWORD(identifier.DriverVersion.LowPart) << "."
                              << LOWORD(identifier.DriverVersion.LowPart);
                          info.driver_version = ss.str();

                          infoList->push_back(info);
                      }
                      else
                      {
                          LOG(LS_ERROR) << "GetAdapterIdentifier failed due to GetAdapterIdentifier returned = " << hr << " error = " << GetLastError();
                      }
                  }
                  d3d->Release();
              }
              else
              {
                  LOG(LS_ERROR) << "GetGpuInfo failed due to d3d_create_proc returned = " << hr << " error = " << GetLastError();
              }
          }
          else
          {
              LOG(LS_ERROR) << "GetGpuInfo failed due to GetProcAddress(Direct3DCreate9) error = " << GetLastError();
          }
          FreeLibrary(d3d_lib);
      }
      else
      {
          LOG(LS_ERROR) << "GetGpuInfo failed due to LoadLibraryW(d3d9) error = " << GetLastError();
      }
  }
#endif
  return true;
#elif defined(OSX)
  GpuInfo info;
  // We'll query the IOKit for the gpu of the main display.
  io_service_t display_service_port = CGDisplayIOServicePort(
      kCGDirectMainDisplay);
  GetProperty(display_service_port, CFSTR("vendor-id"), &info.vendor_id);
  GetProperty(display_service_port, CFSTR("device-id"), &info.device_id);
  GetProperty(display_service_port, CFSTR("model"), &info.description);
  infoList->push_back(info);
  return true;
#else // LINUX || ANDROID
  // TODO: Implement this on Linux
  return false;
#endif
}
} // namespace talk_base
