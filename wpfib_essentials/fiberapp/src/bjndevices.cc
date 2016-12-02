#include "bjndevices.h"
#include "modules/video_capture/screen_capturer_defines.h"
#include "talk/base/logging_libjingle.h"


namespace BJN
{
    // While comparing device name or device unique id we will use strncmp and compare if device unique id starts with given id. This helps in selecting a particular type of device.
    // For e.g. For Microsoft LifeCam Cinema give UniqueId as "\\?\usb##vid_045e&pid_075d". So all devices Microsof LifeCam Cinema devices will be selected"

    DeviceProperyMapping DevicePropertyManager::DEVICE_PROPERTIES_MAP[NUMBER_OF_PROPERTY_MAP_DEVICES] = {{SCREEN_SHARING_FRIENDLY_NAME, SCREEN_SHARING_UNIQUE_ID, {false}}};

    /*bool DevicePropertyManager::updateDevicePropertyByUniqueId(SkinnyLocalMediaInfoPtr mediaInfo, DeviceType type)
    {
        for(int i = 0; NUMBER_OF_PROPERTY_MAP_DEVICES > i; ++i)
        {
            switch(type)
            {
                case DEVICE_VIDEO_CAPTURER:
                    if(!strncmp(DEVICE_PROPERTIES_MAP[i].deviceUniqueId, mediaInfo->get_CameraID().c_str(), strlen(DEVICE_PROPERTIES_MAP[i].deviceUniqueId)))
                    {
                        mediaInfo->setVideocapDeviceVisibility(DEVICE_PROPERTIES_MAP[i].properties.visibility);
                        return true;
                    }
                    break;
                case DEVICE_AUDIO_CAPTURER:
                    if(!strncmp(DEVICE_PROPERTIES_MAP[i].deviceUniqueId, mediaInfo->get_MicrophoneID().c_str(), strlen(DEVICE_PROPERTIES_MAP[i].deviceUniqueId)))
                    {
                        mediaInfo->setAudiocapDeviceVisibility(DEVICE_PROPERTIES_MAP[i].properties.visibility);
                        return true;
                    }
                    break;
                case DEVICE_AUDIO_PLAYOUT:
                    if(!strncmp(DEVICE_PROPERTIES_MAP[i].deviceUniqueId, mediaInfo->get_SpeakerID().c_str(), strlen(DEVICE_PROPERTIES_MAP[i].deviceUniqueId)))
                    {
                        mediaInfo->setAudioplayoutDeviceVisibility(DEVICE_PROPERTIES_MAP[i].properties.visibility);
                        return true;
                    }
                    break;
                default:
                    LOG(LS_ERROR)<<"Invalid device type passed";
                    break;
            }
        }
        return false;
    }*/

    std::string DevicePropertyManager::getScreenSharingDeviceFriendlyName(int index)
    {
        std::string name(SCREEN_SHARING_FRIENDLY_NAME);
        name += " ";
        // add one to the zero based index
        name += boost::to_string(index + 1);
        return name;
    }

    std::string DevicePropertyManager::getScreenSharingDeviceUniqueID(int index, uint64_t appId)
    {
        std::string id(SCREEN_SHARING_UNIQUE_ID);
        id += "\\";
        id += boost::to_string(index);
        if (appId != 0)
        {
            id += "\\";
            id += boost::to_string(appId);
        }
        return id;
    }

    bool DevicePropertyManager::isScreenSharingDevice(const std::string& id)
    {
        bool isSharingDevice = false;

        // the screen sharing device id is a guid followed by the screen index (zero based)
        // e.g "{7306149c-b8c7-4227-9946-6d6316edc64f}\0"
        // we just compare the GUID part here
        if (strncmp(SCREEN_SHARING_UNIQUE_ID, id.c_str(), strlen(SCREEN_SHARING_UNIQUE_ID)) == 0)
        {
            isSharingDevice = true;
        }

        return isSharingDevice;
    }

    int DevicePropertyManager::getScreenSharingDeviceIndex(const std::string& id)
    {
        int index = 0;

        // parse the string to get the integer after the last '\'
        const char* pos = strrchr(id.c_str(), '\\');
        if (pos)
        {
            index = atoi(pos + 1);
        }

        return index;
    }

}
