#ifndef __BJN_DEVICES_H_
#define __BJN_DEVICES_H_
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/exception/to_string.hpp>

#define NUMBER_OF_PROPERTY_MAP_DEVICES 1

namespace BJN
{
    /**
    * This class is used basically for special handling of specific devices. Currently majorly we are using it to set visibility property of specific devices. But it can be extended for more 
    * properties as and when needed.
    */

    enum DeviceType
    {
        DEVICE_VIDEO_CAPTURER,
        DEVICE_AUDIO_CAPTURER,
        DEVICE_AUDIO_PLAYOUT,
    };

    struct DeviceProperties
    {
        bool visibility;
    };

    struct DeviceProperyMapping
    {
        const char *        deviceName;
        const char *        deviceUniqueId;
        DeviceProperties    properties;
    };

    class DevicePropertyManager
    {
        public:
            //static bool updateDevicePropertyByUniqueId(SkinnyLocalMediaInfoPtr mediaInfo, DeviceType);
            static std::string getScreenSharingDeviceFriendlyName(int index = 0);
            static std::string getScreenSharingDeviceUniqueID(int index = 0, uint64_t devId = 0);
            static bool isScreenSharingDevice(const std::string& id);
            static int  getScreenSharingDeviceIndex(const std::string& id);

        private:
            
            static DeviceProperyMapping DEVICE_PROPERTIES_MAP[NUMBER_OF_PROPERTY_MAP_DEVICES];
    };

}   // namespace BJN

#endif  //__BJN_DEVICES_H_