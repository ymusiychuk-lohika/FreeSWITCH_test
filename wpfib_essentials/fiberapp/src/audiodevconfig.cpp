#include "audiodevconfig.h"
#include "bjnhelpers.h"
#include "config_handler.h"
#include "talk/base/logging_libjingle.h"
#include "talk/base/common.h"
#include <boost/algorithm/string.hpp>
#if defined FB_WIN
#include "win/winaudiodevconfig.h"
#elif defined FB_MACOSX
#include "Mac/macaudiodevconfig.h"
#elif defined FB_X11
#include "X11/x11audiodevconfig.h"
#endif


std::string DeviceTypeString(DeviceConfigType type)
{
    if (type == DeviceConfigTypeMic)
        return "MIC";
    else
        return "SPK";
}


std::string GetOSDeviceKeyPrefix()
{
    std::string prefix;
#if defined FB_WIN
    prefix = "WIN:";
#elif defined FB_MACOSX
    prefix = "MAC:";
#elif defined FB_X11
    prefix = "LNX:";
#endif
    return prefix;
}

bool GetAudioHardwareIdFromDeviceId(DeviceConfigType type,
                                    const std::string& deviceName,
                                    const std::string& deviceId,
                                    std::string& vendorId,
                                    std::string& productId,
                                    std::string& classId,
                                    std::string& deviceIdKey,
                                    bool idKeyWithType)
{
    bool ok = false;
    std::string deviceKey;

    // NOTE: windows uses the deviceId, but other platforms use the deviceName
#if defined FB_WIN
    ok = GetAudioHardwareIdFromDeviceIdWin(type, deviceId, vendorId, productId, classId);
#elif defined FB_MACOSX
    ok = GetAudioHardwareIdFromDeviceIdMac(type, deviceName, vendorId, productId, classId);
#elif defined FB_X11
    ok = GetAudioHardwareIdFromDeviceIdLinux(type, deviceId, vendorId, productId, classId);
#endif

    // make our vendor, product and class keys uppercase
    boost::to_upper(vendorId);
    boost::to_upper(productId);
    boost::to_upper(classId);

    // assemble the deviceKey;
    if(idKeyWithType) {
        deviceKey = DeviceTypeString(type);
        deviceKey += ":";
    }
    deviceKey += classId;
    deviceKey += ":";
    deviceKey += vendorId;
    deviceKey += ":";
    deviceKey += productId;

    // return the device id
    deviceIdKey = deviceKey;

    return ok;
}

bool GetDeviceConfigProperties(DeviceConfigType type,
                               const std::string& deviceId,
                               const std::string& productId,
                               const std::string& vendorId,
                               const std::string& classId,
                               const std::string& deviceIdKey,
                               std::map<std::string, int>& properties,
                               const Config_Parser_Handler& cph_instance)
{
    bool found = false;

    Config_Parser_Handler::DeviceConfigMap devMap = cph_instance.getDeviceConfig();
    Config_Parser_Handler::DeviceConfigMap::const_iterator it;

    // we try the following to get config details for the device, once we
    // find a match we return the results and look no further
    // 1. OS and device specifc key e.g. MAC:MIC:USB:045E:076D
    // 2. device specific key e.g. MIC:USB:045E:076D
    // 3. OS and vendor specifc key e.g. MAC:MIC:USB:045E
    // 4. vendor specific key e.g. MIC:USB:045E
    // 5. OS and class specific key e.g MAC:MIC:USB
    // 6. class specific key e.g. MIC:USB
    // 7. OS and type, eg WIN:MIC or MAC:SPK
    // 8. type, either MIC or SPK

    // create a list of the keys that we try in the correct order
    std::vector<std::string> keyList;

    // 1. OS and device specifc key e.g. MAC:MIC:USB:045E:076D
    keyList.push_back(GetOSDeviceKeyPrefix() + deviceIdKey);
    // 2. device specific key e.g. MIC:USB:045E:076D
    keyList.push_back(deviceIdKey);
    // 3. OS and vendor specifc key e.g. MAC:MIC:USB:045E
    keyList.push_back(GetOSDeviceKeyPrefix() + DeviceTypeString(type) + ":" + classId + ":" + vendorId);
    // 4. vendor specific key e.g. MIC:USB:045E
    keyList.push_back(DeviceTypeString(type) + ":" + classId + ":" + vendorId);
    // 5. OS and class specific key e.g MAC:MIC:USB
    keyList.push_back(GetOSDeviceKeyPrefix() + DeviceTypeString(type) + ":" + classId);
    // 6. class specific key e.g. MIC:USB
    keyList.push_back(DeviceTypeString(type) + ":" + classId);
    // 7. Os and type, eg WIN:MIC or MAC:SPK
    keyList.push_back(GetOSDeviceKeyPrefix() + DeviceTypeString(type));
    // 8. type, either MIC or SPK
    keyList.push_back(DeviceTypeString(type));

    // now see if any of the keys on the list are present, stop if we match
    for (size_t i = 0; i < keyList.size(); i++)
    {
        it = devMap.find(keyList[i]);
        if (it != devMap.end())
        {
            // config info for this os/device exists
            properties = it->second;
            found = true;

            LOG(LS_INFO) << __FUNCTION__ << " Loaded audio device config for: "
            "device. id: " << deviceId
            << " from key: " << it->first << " with device key: " << keyList[i];

            break;
        }
    }

    if (!found)
    {
        LOG(LS_INFO) << __FUNCTION__ << " Failed to find audio device config for: "
        << deviceId << " with class: " << classId << " and device key: " << deviceIdKey;
    }

    return found;
}

bool GetDeviceConfigProperties(DeviceConfigType type,
                               const std::string& deviceName,
                               const std::string& deviceId,
                               std::map<std::string, int>& properties,
                               std::string &deviceIdKey,
                               const Config_Parser_Handler& cph_instance)
{
    bool found = false;
    std::string vendorId;
    std::string productId;
    std::string classId;

    if (GetAudioHardwareIdFromDeviceId(type, deviceName, deviceId, vendorId, productId, classId, deviceIdKey, true))
    {

        found = GetDeviceConfigProperties(type,
                                          deviceId,
                                          productId,
                                          vendorId,
                                          classId,
                                          deviceIdKey,
                                          properties,
                                          cph_instance);
    }

    return found;
}

bool isAudioDeviceBlacklistedForAutomaticSelection(DeviceConfigType type,
                                                   const std::string& deviceId,
                                                   const std::string& productId,
                                                   const std::string& vendorId,
                                                   const std::string& classId,
                                                   const std::string& deviceIdKey,
                                                   const Config_Parser_Handler& cph_instance)
{
    bool isDeviceNotAutomatic = false;
    std::map<std::string, int> deviceProperties;
    std::map<std::string, int>::iterator it;
    if(GetDeviceConfigProperties(type,
                                 deviceId,
                                 productId,
                                 vendorId,
                                 classId,
                                 deviceIdKey,
                                 deviceProperties,
                                 cph_instance))
    {
        it = deviceProperties.find("automatic");
        if (it != deviceProperties.end() && it->second == 0)
            isDeviceNotAutomatic = true; // device is blacklisted,
                                         // and should not be used for automatic
    }
    return isDeviceNotAutomatic;
}

bool getIntDeviceIdFromHex(const std::string& str, int32_t& val)
{
    bool success = false;

    if(str.length() == 4) // Device ID is represented by 16-bit hex string
    {
        const char* start = str.c_str();
        char *endPtr;
        errno = 0;

        long longVal = strtol(start, &endPtr, 16);

        // A range error check below seems redundant as we check
        // for device ID to be 4 HEX digits above, and LONG_MAX/MIN are
        // decidedly wider. Hence including as only a comment.

        if (!(val == 0 && errno != 0) // Check for failure
            && endPtr != start // Ensure non-empty string
            && *endPtr == '\0' // errno not set if latter part of string is invalid
            /*&& !((val == LONG_MIN || val == LONG_MAX) && errno == ERANGE)*/)
        {
            val = static_cast<int32_t>(longVal); // Down cast fine, since
                                                 // ID is 16-bit Hex.
            success = true;
        }
        else
        {
            if(*endPtr != '\n')
                LOG(LS_WARNING) << __FUNCTION__ << ": Unable to convert " << str
                    << " to Hex as endptr is " << *endPtr << " and not NUL";
            else if(errno)
                LOG(LS_WARNING) << __FUNCTION__ << ": Unable to convert " << str
                    << " to Hex" << ". Error:" << errno;
            else
                LOG(LS_WARNING) << __FUNCTION__ << ": Unable to convert " << str
                    << " to Hex" << " as endPtr is same as start";
        }
    }
    else
    {
        LOG(LS_WARNING) << __FUNCTION__ << ": Error! Device id " << str
            << " should be 4 Hex digits long.";
    }
    return success;
}

bool isUSBDeviceOnThunderbolt(const std::string& classIdStr,
                              const std::string& vendorIdStr,
                              const std::string& productIdStr)
{
    LOG(LS_INFO) << __FUNCTION__;
    bool isOnThn = false;
#ifdef FB_MACOSX
    int32_t vendorId, productId;
    if(classIdStr == "USB")
    {
        if(getIntDeviceIdFromHex(vendorIdStr, vendorId) &&
            getIntDeviceIdFromHex(productIdStr, productId))
        {
            isOnThn = isUSBDeviceOnThunderboltMac(vendorId, productId);
        }
    }
#endif
    LOG(LS_INFO) << "Is device with ID " << classIdStr << ":"
        << vendorIdStr << ":" << productIdStr << " on Thunderbolt? "
        << (isOnThn ? "Yes" : "No");
    return isOnThn;
}
