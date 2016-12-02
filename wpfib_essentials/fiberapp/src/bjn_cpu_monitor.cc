#include "bjn_cpu_monitor.h"
#include "talk/base/logging_libjingle.h"
#include "talk/base/timeutils.h"
#include "string"
#include "algorithm"

#ifdef FB_MACOSX
#include "talk/base/macutils.h"
#include "h264vtencoder.h"
#endif

#ifdef WIN32
#include "talk/base/win32.h"
#include "mfxvideo++.h"
#endif

static std::string make_cpuid_key(const std::string &vendor, int family, int model, int stepping)
{
    std::stringstream buffer;
    buffer << vendor << "," << family << ","  << model << "," << stepping;
    return buffer.str();
}

// CPU_Monitor implementaion
CPU_Monitor::CPU_Monitor():
    cpu_load_(0),
    process_load_(0),
    cpu_adjustment_strength_(0),
    throttle_log_(0),
    isInitCpuMap_(false),
    useMaxFreqBasedResCap_(false),
    isGPUInfoInitialized_(false),
    isH264CapablePublish_(false),
#ifdef WIN32
    useBaseBoardMachineModel_(false),
#endif //WIN32
    isH264Capable_(false)
{
    sampler_.Init();

    cpu_vendor_     = sys_info_.GetCpuVendor();
    machine_model_  = sys_info_.GetMachineModel();
    memory_         = sys_info_.GetMemorySize();
    physical_cpus_  = sys_info_.GetMaxPhysicalCpus();
    max_cpus_       = sys_info_.GetMaxCpus();
    cur_cpus_       = sys_info_.GetCurCpus();
    cpu_family_     = sys_info_.GetCpuFamily();
    cpu_model_      = sys_info_.GetCpuModel();
    cpu_stepping_   = sys_info_.GetCpuStepping();
    max_cpu_speed_  = sys_info_.GetMaxCpuSpeed();
    cur_cpu_speed_  = sys_info_.GetCurCpuSpeed();
    cpu_arch_       = sys_info_.GetCpuArchitecture();

#ifdef WIN32
    // read the baseboard machine model details and log it
    // will be used for enabling as part of config.xml
    baseboard_machine_model_ = sys_info_.GetBaseBoardMachineModel();
#endif //WIN32
}

void CPU_Monitor::reset()
{
    cpu_load_           = 0;
}

void CPU_Monitor::addCpuParam(std::string machineModel,
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
                              bool dualstream)
{
    CPU_Params params;

    params.machineModel = machineModel;
    params.vendor =  vendor;
    params.family = family;
    params.model = model;
    params.stepping = stepping;
    params.vcores = vcores;
    params.frequency = frequency;
    params.vparams.encWidth = encwidth;
    params.vparams.encHeight = encheight;
    params.vparams.encFps = encfps;
    params.vparams.decWidth = decwidth;
    params.vparams.decHeight = decheight;
    params.vparams.decFps = decfps;
    params.vparams.dualStream = dualstream;
    updateCpuParamsMap(params);
}

void CPU_Monitor::InitGPUInfo()
{
#ifdef WIN32
    // Find gpu info only for platforms which supports Win HW H.264 Accl
    if (talk_base::IsWindows7OrLater())
    {
        cpu_processor_ = sys_info_.GetCpuProcessor();
        sys_info_.GetGpuInfo(&gpuList_);
        isGPUInfoInitialized_ = true;
    }
#endif
}

void CPU_Monitor::InitSMCSampler()
{
        smc_sampler_.Init();
}

//This function will check if system is capable to suppport
//H264 h/w accelertion on mac and windows.
void CPU_Monitor::setH264CapableFlag(){

#if defined(WIN32)|| defined(FB_MACOSX)
    isH264CapablePublish_ = true;
#endif

    //H264 flag set for Win
#ifdef WIN32
    if (talk_base::IsWindows7OrLater())
    {
        MFXVideoSession session;
        // The minium Intel SDK version to be queried will be 1.3
        mfxVersion ver = { { 3, 1 } };
        mfxStatus status = session.Init(MFX_IMPL_HARDWARE_ANY, &ver);
        if (status >= MFX_ERR_NONE)
        {
            isH264Capable_ = true;
            LOG(LS_INFO) << "isH264Capable_ is set to true, Session Init returned: " << status;
            session.Close();
        }
        else
        {
            LOG(LS_INFO) << "isH264Capable_ is set to false, Session Init returned: " << status;
        }
    }
#endif

    //H264 flag set for Mac
#ifdef FB_MACOSX
    int major,minor,build;
    talk_base::GetOsVersion(&major, &minor, &build);
    if(major >= 10 && minor >= 9){
        isH264Capable_ = h264vtencoder::isHWEncodingSupported(DEFAULT_MAX_WIDTH,DEFAULT_MAX_HEIGHT,kCMVideoCodecType_H264);
    }
    if(isH264Capable_){
        LOG(LS_INFO) << "isH264Capable_ is set to true";
    }
    else{
        LOG(LS_INFO) << "isH264Capable_ is set to false";
    }
#endif

}

#ifdef WIN32
void CPU_Monitor::searchWithBaseBoardData()
{
    useBaseBoardMachineModel_ = true;
}
#endif //WIN32

void CPU_Monitor::initCpuMap()
{
    // This function should only be called once
    // after feature enablement of max_freq_based_res_cap
    // and before getCpuParams() and logCpuParams()
    if (isInitCpuMap_)
    {
        LOG(LS_WARNING) << "default cpuMap was already initialized!";
    }

    int comparison_cpu_speed = getComparisonCpuSpeed();

    LOG(LS_INFO) << "In function " << __FUNCTION__
    << ". Determining resolution cap using "
    << ((useMaxFreqBasedResCap_) ? "max_freq = " : "cur_freq = ")
    << comparison_cpu_speed;

    // Decide resolution based on number of cores and frequency of each core
    // We want to do something like, if 4 virtual cores and each of at least 3 GHz or 8 virtual cores and each of atleast 2.0 GHz then do 720p@30 both ways
    // Max frequency is used if max_freq_based_res_cap feature is enabled, otherwise current frequency is used.

#ifdef FORCE_WEB_CAM_720p
	default_param_.vparams.encWidth = 1920;
	default_param_.vparams.encHeight = 1080;
	default_param_.vparams.encFps = 30;
	default_param_.vparams.decWidth = 1920;
	default_param_.vparams.decHeight = 1080;
#else

    if( (3000 <= comparison_cpu_speed && 4 <= this->cur_cpus_) || (2000 <= comparison_cpu_speed && 8 <= this->cur_cpus_) )
    {
        default_param_.vparams.encWidth    = 1280;
        default_param_.vparams.encHeight   = 720;
        default_param_.vparams.encFps      = 30;
        default_param_.vparams.decWidth    = 1280;
        default_param_.vparams.decHeight   = 720;
    }
    else if(2500 <= comparison_cpu_speed && 4 <= this->cur_cpus_)
    {
        default_param_.vparams.encWidth    = 1280;
        default_param_.vparams.encHeight   = 720;
        default_param_.vparams.encFps      = 15;
        default_param_.vparams.decWidth    = 1280;
        default_param_.vparams.decHeight   = 720;
    }
    else if(1800 <= comparison_cpu_speed && 4 <= this->cur_cpus_ )
    {
        default_param_.vparams.encWidth    = 640;
        default_param_.vparams.encHeight   = 360;
        default_param_.vparams.encFps      = 30;
        default_param_.vparams.decWidth    = 1280;
        default_param_.vparams.decHeight   = 720;
    }
    else
    {
        default_param_.vparams.encWidth    = 640;
        default_param_.vparams.encHeight   = 360;
        default_param_.vparams.encFps      = 30;
        default_param_.vparams.decWidth    = 640;
        default_param_.vparams.decHeight   = 360;
    }
#endif
    default_param_.vparams.decFps      = 30;    // Do not try to control decodig fps from client side
    default_param_.vparams.dualStream  = true;

    isInitCpuMap_ = true;
}

void CPU_Monitor::dumpCpuParamsMap() const
{
    MachineModelParamsMap::const_iterator itMachineModelParams;
    CpuIdResolutionsMap::const_iterator itCpuIdRes;
    CpuParamsList::const_iterator itCpuParamsList;

    LOG(INFO)<<"CPU Param Map:";
    for(itMachineModelParams = cpu_params_map_.begin(); cpu_params_map_.end() != itMachineModelParams; ++itMachineModelParams)
    {
        for(itCpuIdRes = itMachineModelParams->second.begin(); itMachineModelParams->second.end() != itCpuIdRes; ++itCpuIdRes)
        {
            for(itCpuParamsList = itCpuIdRes->second.begin(); itCpuIdRes->second.end() != itCpuParamsList; ++itCpuParamsList)
            {
                LOG(INFO)<< "MachineModel " << itCpuParamsList->machineModel << " Vendor: " << itCpuParamsList->vendor <<
                    " Family: "<< itCpuParamsList->family << " Model: "<< itCpuParamsList->model << " Stepping: "<< itCpuParamsList->stepping <<
                    " EncRes: " << itCpuParamsList->vparams.encWidth << "x" << itCpuParamsList->vparams.encHeight << "@" << itCpuParamsList->vparams.encFps <<
                    " DecRes: " << itCpuParamsList->vparams.decWidth << "x" << itCpuParamsList->vparams.decHeight << "@" << itCpuParamsList->vparams.decFps <<
                    " vcores: "<< itCpuParamsList->vcores << " Frequency: "<< itCpuParamsList->frequency << " DualStream: " << itCpuParamsList->vparams.dualStream;

                // Perform sanity on map values
                if(itMachineModelParams->first != itCpuParamsList->machineModel ||
                    itCpuIdRes->first != make_cpuid_key(itCpuParamsList->vendor, itCpuParamsList->family, itCpuParamsList->model, itCpuParamsList->stepping))
                {
                    LOG(LS_ERROR)<<"Map params not consistent. Param Keys, MachineModel: "<<itMachineModelParams->first<<" CPUID Key: "<<itCpuIdRes->first;
                }
            }
        }
    }
}

static bool cpuParamsCompare(const CPU_Monitor::CPU_Params &lhs, const CPU_Monitor::CPU_Params &rhs)
{
    if(lhs.vcores == rhs.vcores)
        return lhs.frequency > rhs.frequency;
    else
        return lhs.vcores > rhs.vcores;
}

CPU_Monitor::MachineModelParamsMap::iterator CPU_Monitor::FindMatchingMachineModel(const std::string& mm) {
    MachineModelParamsMap::iterator itMachineModelParams = cpu_params_map_.find(mm.c_str());
    //Could not find, do wildcard matching
    if (cpu_params_map_.end() == itMachineModelParams)
    {
        MachineModelParamsMap::iterator itWildcardParams = cpu_params_map_.lower_bound(mm);
        while (itWildcardParams != cpu_params_map_.end())
        {
            const std::string& machineKey = itWildcardParams->first;
            //using ~ is because it is the second to last in ASCII table. so lower_bound can find it
            if (machineKey.size() > 0 && machineKey.at(machineKey.size() - 1) == '~')
            {
                if (mm.compare(0, machineKey.size() - 1, machineKey, 0, machineKey.size() - 1) == 0) // prefix match?
                    itMachineModelParams = itWildcardParams;
                break;
            }
            else
            {
                itWildcardParams++;
            }
        }
    }
    return itMachineModelParams;
}

CPU_Monitor::CPU_Params CPU_Monitor::getCpuParams()
{
    int comparison_cpu_speed = getComparisonCpuSpeed();

    LOG(LS_INFO) << "In function " << __FUNCTION__
    << ". Determining resolution cap using "
    << ((useMaxFreqBasedResCap_) ? "max_freq = " : "cur_freq = ")
    << comparison_cpu_speed;

    /* Following is the logic the way we find out encoding and decoding resolution for given cpu.
        * * First lookup is done of cpu vendor. This gives us map of model, family and stepping to encoding resolution
        * * Second lookup is done of cpu model, family and stepping (we ignore stepping as of now by keeping value as 0). This lookup gives us map of encoding resolution to all the cpu params
        * * In above iteration we get values as vector of all the cpu params. We iterate through each cpu param in vector and find cpu param such that current cpus on the box (virtual cores) are >= vcores in cpu params and
        * cpu frequency is >= frequency in cpu param. Max frequency is used if max_freq_based_res_cap feature is enabled, otherwise current frequency is used. We return the first such match assuming its latest entry which might be over written by custom config xml by user by
        * * If we find cpu param matching all above criterias then we return that param. If any of the lookup fails or any of the condition while iterating doesn't match then we return default param which is calculated based
        * on number of vittual cores on the box and frequency of each core.
        */

    MachineModelParamsMap::iterator itMachineModelParams;
    CpuIdResolutionsMap::iterator itCpuIdRes;
    CpuParamsList::reverse_iterator itCpuParamsList;

    CPU_Params param = default_param_;
    int maxDecWidth = 0;

    itMachineModelParams = FindMatchingMachineModel(machine_model_);
#ifdef WIN32
    if (useBaseBoardMachineModel_  && cpu_params_map_.end() == itMachineModelParams &&
        !baseboard_machine_model_.empty()) {
        // If no data is found then search baseboard machine model, only if baseboard data is initialised
        itMachineModelParams = FindMatchingMachineModel(baseboard_machine_model_);
    }
#endif //WIN32

    //If no entry for this machine model lookup in default entry for this cpu id.
    if(cpu_params_map_.end() == itMachineModelParams) {
        itMachineModelParams = cpu_params_map_.find("");
    }

    if(cpu_params_map_.end() != itMachineModelParams)
    {
        itCpuIdRes = itMachineModelParams->second.find(make_cpuid_key(cpu_vendor_, cpu_family_, cpu_model_, 0));    // We ignore the stepping value as of now. We build map also with stepping value as 0
        if(itMachineModelParams->second.end() == itCpuIdRes)
        {
            itCpuIdRes = itMachineModelParams->second.find(make_cpuid_key(cpu_vendor_, 0, 0, 0));
        }

        if(itMachineModelParams->second.end() != itCpuIdRes)
        {
            std::sort(itCpuIdRes->second.rbegin(), itCpuIdRes->second.rend(), cpuParamsCompare);
            for(itCpuParamsList = itCpuIdRes->second.rbegin(); itCpuIdRes->second.rend() != itCpuParamsList; ++itCpuParamsList)
            {
                CPU_Params cur_cpu_params = *itCpuParamsList;
                if(itCpuParamsList->vcores <= cur_cpus_ && itCpuParamsList->frequency <= comparison_cpu_speed)
                {
                    param = *itCpuParamsList;
                    break;
                }
            }
        }
    }
    return param;
}

void CPU_Monitor::curCpuUpdate(int cur_cpu_speed)
{
    cur_cpu_speed_  = cur_cpu_speed;
}

/*
* refreshCpuLoad(): Should be called periodically to check if CPU is running above the max threshold or lower than min threshold to take and action on video quality.
* Returns:  Returns strength as integer. Strength is no. of Units difference from min or max threshold with step of 5. If strength is positive then CPU is running 
*           consistenly higher than max threshold For given duration. If Strength is negative then CPU is consistently lower than min threshold for given duration. 
*           Else returns 0.
*/

int CPU_Monitor::refreshCpuLoad(pj_time_val current_time)
{
    int cpus = sampler_.GetMaxCpus();
    int cur_cpus = sampler_.GetCurrentCpus();
    float proc_load = sampler_.GetProcessLoad();
    float sys_load = sampler_.GetSystemLoad();

    if(cur_cpus != cur_cpus_)
    {
        LOG(INFO) << "Number of CPUs changed for process. Earlier: "<< cur_cpus_ << " now: "<< cur_cpus;
        cur_cpus_ = cur_cpus;
    }

    cpu_load_ = static_cast<int>((sys_load * 100) / cpus);
    process_load_ = static_cast<int>((proc_load * 100) / cpus);
    cpu_adjustment_strength_ = fbr_cpu_monitor_.getCpuStrength(current_time, cpu_load_);

    if(throttle_log_++ % NORMAL_ACTION_DURATION == 0) {
        LOG(INFO) << "Current cpu load " << cpu_load_ << " plugin process cpu load " << process_load_;
        if(cpu_adjustment_strength_ > 0) {
            LOG(INFO) << "CPU utilization consistently higher than " <<  MAX_CPU_THRESHOLD << " for more than" << NORMAL_ACTION_DURATION << " seconds.";
        } else if(cpu_adjustment_strength_ < 0) {
            LOG(INFO) << "CPU utilization consistently lower than " <<  MIN_CPU_THRESHOLD << " for more than" << NORMAL_ACTION_DURATION << " seconds.";
        }
    }
    return cpu_adjustment_strength_;
}

float CPU_Monitor::getFanSpeed()
{
    return smc_sampler_.GetFanSpeed();
}
void CPU_Monitor::updateCpuParamsMap(const CPU_Monitor::CPU_Params& params)
{
    /*
        * We want data structure to store for given vendor, family, family, stepping, virtual cores and frequency of each core what is maximum resolution supported by that cpu.
        * While deciding higher resolution encoding resolution will be given priority over decoding resolution.
        * Following is the data structure chose to store above data.
        * * Map of vendors to model, family, stepping. Key of the map is Vendor string value is map whose key is CpUIdKey (calcualted from model, family and stepping).
        * * This second level map stores mapping of CPU Id to encoding resolution. Key is CpuId (calculated from family, model and stepping) and value is map whose key is vector of all Cpu Params.
        */

    MachineModelParamsMap::iterator itMachineModelParams;
    CpuIdResolutionsMap::iterator itCpuIdRes;

    CpuIdResolutionsMap cpuIdResMap;
    CpuParamsList cpuParamsList;

    std::string cpuidKey = make_cpuid_key(params.vendor, params.family, params.model, params.stepping);

    itMachineModelParams = cpu_params_map_.insert(std::pair<std::string, CpuIdResolutionsMap>(params.machineModel, cpuIdResMap)).first;
    itCpuIdRes = itMachineModelParams->second.insert(std::pair<std::string, CpuParamsList>(cpuidKey, cpuParamsList)).first;
    itCpuIdRes->second.push_back(params);
}

void CPU_Monitor::logCPUParams(const CPU_Params &cur_cpu_params)
{
    dumpCpuParamsMap();
    LOG(LS_INFO) << "CPU Details "<<"\n"<<
        "GetMaxPhysicalCpus: "<<physical_cpus_<<"\n"<<
        "GetMaxCpus: "<<max_cpus_<<"\n"<<
        "GetCurCpus: "<<cur_cpus_<<"\n"<<
        "GetCpuArchitecture: "<<cpu_arch_<<"\n"<<
        "GetCpuVendor: "<<cpu_vendor_<<"\n"<<
        "GetCpuFamily: "<<cpu_family_<<"\n"<<
        "GetCpuModel: "<<cpu_model_<<"\n"<<
        "GetCpuStepping: "<<cpu_stepping_<<"\n"<<
        "GetMaxCpuSpeed: "<<max_cpu_speed_<<"\n"<<
        "GetCurCpuSpeed: "<<cur_cpu_speed_<<"\n"<<
        "GetMemorySize: "<<memory_<<"\n"<<
        "GetMachineModel: "<<machine_model_<<"\n"
#ifdef WIN32
        "GetBaseBoardMachineModel: " << baseboard_machine_model_ << "\n"
#endif //WIN32
        ;

    if (isGPUInfoInitialized_)
    {
        /*  Helpful in identifying CPU Processor and GPU models of the endpoints
        Can be extended to non-windows platforms later if neccessary    */
        LOG(LS_INFO) << "CPU Processor = " << cpu_processor_.c_str();
        if (gpuList_.size())
        {
            LOG(LS_INFO) << "Number of GPUs detected: " << gpuList_.size();
            for (int i = 0; i < gpuList_.size(); i++)
            {
                LOG(LS_INFO) << "GPU adapter: " << gpuList_[i].description << "; displays attached: " << gpuList_[i].displays_attached;
            }
        }
        else
        {
            LOG(LS_INFO) << "GPU Details not available";
        }
    }

    LOG(LS_INFO) << "Default CPU Params: EncRes: " << default_param_.vparams.encWidth << "x" << default_param_.vparams.encHeight << "@" << default_param_.vparams.encFps <<
            " DecRes: " << default_param_.vparams.decWidth << "x" << default_param_.vparams.decHeight << "@" << default_param_.vparams.decFps << " DualStream: " << cur_cpu_params.vparams.dualStream;
    LOG(LS_INFO) << "Current CPU Params: EncRes: " << cur_cpu_params.vparams.encWidth << "x" << cur_cpu_params.vparams.encHeight << "@" << cur_cpu_params.vparams.encFps <<
            " DecRes: " << cur_cpu_params.vparams.decWidth << "x" << cur_cpu_params.vparams.decHeight << "@" << cur_cpu_params.vparams.decFps << " DualStream: " << cur_cpu_params.vparams.dualStream;
}

void CPU_Monitor::curCPUParams(std::string &cpuParams) const
{
    std::ostringstream valStr;

    // Begin the CPU info string
    cpuParams.assign("[");

    // MaxPhysicalCpus
    cpuParams.append("MaxPhysicalCpus:");
    valStr << physical_cpus_;
    cpuParams.append(valStr.str());
    cpuParams.append(", ");
    valStr.str("");
    valStr.clear();

    // MaxCpus
    cpuParams.append("MaxCpus:");
    valStr << max_cpus_;
    cpuParams.append(valStr.str());
    cpuParams.append(", ");
    valStr.str("");
    valStr.clear();

    // CurCpus
    cpuParams.append("CurCpus:");
    valStr << cur_cpus_;
    cpuParams.append(valStr.str());
    cpuParams.append(", ");
    valStr.str("");
    valStr.clear();

    // CpuArchitecture
    cpuParams.append("CpuArchitecture:");
    valStr << cpu_arch_;
    cpuParams.append(valStr.str());
    cpuParams.append(", ");
    valStr.str("");
    valStr.clear();

    // CpuVendor
    cpuParams.append("CpuVendor:");
    cpuParams.append(cpu_vendor_);
    cpuParams.append(", ");

    // CpuFamily
    cpuParams.append("CpuFamily:");
    valStr << cpu_family_;
    cpuParams.append(valStr.str());
    cpuParams.append(", ");
    valStr.str("");
    valStr.clear();

    // CpuModel
    cpuParams.append("CpuModel:");
    valStr << cpu_model_;
    cpuParams.append(valStr.str());
    cpuParams.append(", ");
    valStr.str("");
    valStr.clear();

    // CpuStepping
    cpuParams.append("CpuStepping:");
    valStr << cpu_stepping_;
    cpuParams.append(valStr.str());
    cpuParams.append(", ");
    valStr.str("");
    valStr.clear();

    // MaxCpuSpeed
    cpuParams.append("MaxCpuSpeed:");
    valStr << max_cpu_speed_;
    cpuParams.append(valStr.str());
    cpuParams.append(", ");
    valStr.str("");
    valStr.clear();

    // CurCpuSpeed
    cpuParams.append("CurCpuSpeed:");
    valStr << cur_cpu_speed_;
    cpuParams.append(valStr.str());
    cpuParams.append(", ");
    valStr.str("");
    valStr.clear();

    // MemorySize
    cpuParams.append("MemorySize:");
    valStr << memory_;
    cpuParams.append(valStr.str());
    cpuParams.append(", ");
    valStr.str("");
    valStr.clear();

    // MachineModel
    cpuParams.append("MachineModel:");
    cpuParams.append(machine_model_);

#ifdef WIN32
    // Baseboard MachineModel
    if (useBaseBoardMachineModel_ && !baseboard_machine_model_.empty())
    {
        cpuParams.append(", ");
        cpuParams.append("BaseBoardMachineModel:");
        cpuParams.append(baseboard_machine_model_);
    }
#endif //WIN32

    if (isGPUInfoInitialized_)
    {
        cpuParams.append(", ");
        // CPU Processor
        cpuParams.append("Processor:" + cpu_processor_ + ", ");

        // GPU details
        cpuParams.append("GPU#nDisplays:");
        if (gpuList_.size())
        {
            for (int i = 0; i < gpuList_.size(); i++)
            {
                std::stringstream ss;
                ss << gpuList_[i].description << "#" << gpuList_[i].displays_attached << " ";
                cpuParams.append(ss.str());
            }
        }
        else
        {
            cpuParams.append("Not available");
        }
    }

    if(isH264CapablePublish_){
        cpuParams.append(", ");
        if (isH264Capable_)
            cpuParams.append("isH264Capable:Yes ");
        else
            cpuParams.append("isH264Capable:No ");
    }

    // End the CPU info string
    cpuParams.append("]");

    return;
}

void CPU_Monitor::setMaxFreqBasedResCapFlag(bool flag) {

    if (useMaxFreqBasedResCap_ != flag)
    {
        useMaxFreqBasedResCap_ = flag;
        LOG(LS_INFO) << "Setting useMaxFreqBasedResCap_ to " << useMaxFreqBasedResCap_;
    }

    // Initialise default cpu params map.
    // It is supposed to be initialized after feature enablement flag is set
    // When cleaning feature enablement, initCpuMap() can be moved to constructor
    initCpuMap();
}
