#ifndef __AUDIO_DEVICE_CONFIG_H
#define __AUDIO_DEVICE_CONFIG_H

#include <string>
#include <map>
#include <stdint.h>
#include "config_handler.h"

enum DeviceConfigType {
    DeviceConfigTypeMic = 1,
    DeviceConfigTypeSpeaker
};

bool GetAudioHardwareIdFromDeviceId(DeviceConfigType type,
                                    const std::string& deviceName,
                                    const std::string& deviceId,
                                    std::string& vendorId,
                                    std::string& productId,
                                    std::string& classId,
                                    std::string& deviceIdKey,
                                    bool idKeyWithType);

bool GetDeviceConfigProperties(DeviceConfigType type,
                               const std::string& deviceId,
                               const std::string& productId,
                               const std::string& vendorId,
                               const std::string& classId,
                               const std::string& deviceIdKey,
                               std::map<std::string, int>& properties,
                               const Config_Parser_Handler& cph_instance);

bool GetDeviceConfigProperties(DeviceConfigType type,
                               const std::string& deviceName,
                               const std::string& deviceId,
                               std::map<std::string, int>& properties,
                               std::string& deviceIdKey,
                               const Config_Parser_Handler& cph_instance);

std::string DeviceTypeString(DeviceConfigType type);

bool isAudioDeviceBlacklistedForAutomaticSelection(DeviceConfigType type,
                                                   const std::string& deviceId,
                                                   const std::string& productId,
                                                   const std::string& vendorId,
                                                   const std::string& classId,
                                                   const std::string& deviceIdKey,
                                                   const Config_Parser_Handler& cph_instance);
#if defined FB_MACOSX
std::string FixDefaultDeviceId(const std::string& deviceId);
#endif

bool getIntDeviceIdFromHex(const std::string& str, int32_t& val);

bool isUSBDeviceOnThunderbolt(const std::string& classIdStr,
                              const std::string& vendorIdStr,
                              const std::string& productIdStr);


#endif // __AUDIO_DEVICE_CONFIG_H
