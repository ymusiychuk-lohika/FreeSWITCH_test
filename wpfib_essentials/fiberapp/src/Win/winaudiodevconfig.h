#ifndef __WIN_AUDIO_DEVICE_CONFIG_H
#define __WIN_AUDIO_DEVICE_CONFIG_H

#include "audiodevconfig.h"
#include <Windows.h>
#include <Mmdeviceapi.h>
#include <atlbase.h>

bool GetAudioHardwareIdFromDeviceIdWin(DeviceConfigType type,
    const std::string& deviceId,
    std::string& vendorId,
    std::string& productId,
    std::string& classId);

#endif // __WIN_AUDIO_DEVICE_CONFIG_H
