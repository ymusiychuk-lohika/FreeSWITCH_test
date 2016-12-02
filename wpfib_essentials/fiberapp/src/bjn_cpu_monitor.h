#ifndef __BJN_CPU_MONITOR_H
#define __BJN_CPU_MONITOR_H

#include "talk/base/basictypes.h"
#include "talk/base/systeminfo.h"
#include "talk/base/scoped_ptr.h"
#include "talk/base/cpumonitor.h"
#include "talk/base/smcmetrics.h"
#include <pjlib.h>
#include <map>
#include <vector>
#include <sstream>
#include "fbr_cpu_monitor.h"

#define MAKE_RESOLUTION_KEY(width, height, fps) (width * fps)        // ignore height for now as we only support 16x9 resolution

class CPU_Monitor
{
public:
    struct CPU_Params
    {
        std::string machineModel;
        std::string vendor;
        int family;
        int model;
        int stepping;
        int vcores;
        int frequency;
        struct VideoParams
        {
            int encWidth;
            int encHeight;
            int encFps;
            int decWidth;
            int decHeight;
            int decFps;
            bool dualStream;
        };

        VideoParams vparams;
    };

    CPU_Monitor();
    ~CPU_Monitor() { }

    void curCpuUpdate(int cur_cpu_speed);

    // refreshes the cpu load monitoring, which in turns updates
    // assorted system/process load variables and adjustment strength
    int refreshCpuLoad(pj_time_val current_time);

    void updateCpuParamsMap(const CPU_Monitor::CPU_Params& params);
    CPU_Params getCpuParams();
    void reset();
    void logCPUParams(const CPU_Params &cur_cpu_params);

    void addCpuParam(std::string machineModel,
                     std::string vendor,
                     int family,
                     int model,
                     int stepping,
                     int vcores,
                     int frequency,
                     int encwidth,
                     int encheight,
                     int encfps,
                     int decwidth,
                     int decheight,
                     int decfps,
                     bool dualstream);

    void curCPUParams(std::string&) const;
    const std::string &getMachineModel() const {
        return machine_model_;
    }

    // System load normalized across the available CPUs
    int getSystemLoad() const {
        return cpu_load_;
    }

    // CPU load of the process (divided by available CPUs)
    int getProcessLoad() const {
        return process_load_;
    }

    // Strength is # units delta from min or max threshold with step of 5.
    // If strength is positive then CPU is running consistenly higher than max
    // threshold for some duration. If Strength is negative then CPU is consistently
    // lower than min threshold for some duration.  Otherwise, returns 0
    int getAdjustmentStrength() const
    {
        return cpu_adjustment_strength_;
    }

    float getFanSpeed();

#if defined(WIN32) || defined(LINUX)
    int getCurCPUFreqUsage() {
        return sys_info_.GetCurCpuSpeed();
    }
#endif //WIN32 or Linux

    void InitGPUInfo();
    void setMaxFreqBasedResCapFlag(bool flag);
    void setH264CapableFlag();
    void InitSMCSampler();
#ifdef WIN32
    void searchWithBaseBoardData();
#endif //WIN32

private:
    void initCpuMap();
    void dumpCpuParamsMap() const;
    CPU_Monitor(const CPU_Monitor &cpuMonitor) { }
    CPU_Monitor& operator=(const CPU_Monitor &cpuMonitor) { }

    int getComparisonCpuSpeed() {
        if (useMaxFreqBasedResCap_) {
            // For overclocked machines, current speed might be greater
            // than the devices native "max" speed
            // Hence, return greater of the above two to account for such cases
            return (max_cpu_speed_ > cur_cpu_speed_) ? max_cpu_speed_ : cur_cpu_speed_;
        }
        else {
            return cur_cpu_speed_;
        }
    }

    typedef std::vector<CPU_Params> CpuParamsList;
    typedef std::map<std::string, CpuParamsList> CpuIdResolutionsMap;     // Map of CPU Id to supported resolution for it
    typedef std::map<std::string, CpuIdResolutionsMap> MachineModelParamsMap; // Map of model to CPU types of the vendor

    MachineModelParamsMap::iterator FindMatchingMachineModel(const std::string& mm);

    CPU_Params default_param_;
    int64 memory_;
    int physical_cpus_;
    int max_cpus_;
    int cur_cpus_;
    int cpu_family_;
    int cpu_model_;
    int cpu_stepping_;
    int max_cpu_speed_;
    int cur_cpu_speed_;
    talk_base::SystemInfo::Architecture cpu_arch_;
    std::string cpu_vendor_;
    std::string machine_model_;
#ifdef WIN32
    std::string baseboard_machine_model_;
#endif //WIN32
    std::string cpu_processor_;

    int cpu_load_;
    int process_load_;
    int cpu_adjustment_strength_;

    MachineModelParamsMap cpu_params_map_;
    FbrCpuMonitor fbr_cpu_monitor_;
    talk_base::CpuSampler sampler_;
    int throttle_log_;
    talk_base::SystemInfo sys_info_;
    std::vector<talk_base::SystemInfo::GpuInfo> gpuList_;
    talk_base::SMCMetrics smc_sampler_;


    bool isInitCpuMap_;
    bool useMaxFreqBasedResCap_;
    bool isGPUInfoInitialized_;

    //This will enable pubishing for h264 capability
    //to indigo for mac and windows.
    bool isH264CapablePublish_;

    /* isH264Capable_ flag helps to identify Skinny endpoints which is capable
       of H.264 Accl.This is needed as the current implementation for Windows and Mac,
       In windows it requires a monitor (display) should be connected to an Intel GPU.
       Right now only used for Windows 7 & above machines and Mac OS and this flag
       will be sent to indigo as part of CPU params */
    bool isH264Capable_;

#ifdef WIN32
    // This flag will be used to search the cpu params using both 
    // manufacturer/model and baseboard manufacturer/model
    bool useBaseBoardMachineModel_;
#endif //WIN32
};

#endif // __BJN_CPU_MONITOR_H
