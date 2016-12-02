#ifndef TALK_SESSION_PHONE_ANDROIDDEVICEMANAGER_H_
#define TALK_SESSION_PHONE_ANDROIDDEVICEMANAGER_H_

//This code is only for compatibiity
//dummy functions for android.

#include <string>
#include <vector>

#include "talk/base/sigslot.h"
#include "talk/base/stringencode.h"
#include "talk/session/phone/devicemanager.h"

namespace cricket {

    class DeviceWatcher;

class AndroidDeviceManager : public DeviceManager {
 public:
  AndroidDeviceManager();
  virtual ~AndroidDeviceManager();

    // Initialization
    bool Init(){return true;}
    void Terminate(){return;}
    // Capabilities
    int GetCapabilities(){return 0;}
    
    //Default Device
    bool GetDefaultAudioInputDevices(){return false;}
    bool GetDefaultAudioOutputDevices(){return false;}
    
    // Device enumeration
    bool GetAudioInputDevices(std::vector<Device>* devices){return false;}
    bool GetAudioOutputDevices(std::vector<Device>* devices){return false;}
    
    bool GetAudioInputDevice(const std::string& name, Device* out){return false;}
    bool GetAudioOutputDevice(const std::string& name, Device* out){return false;}
    
    bool GetVideoCaptureDevices(std::vector<Device>* devs){return false;}
    bool GetVideoCaptureDevice(const std::string& name, Device* out){return false;}

};

}  // namespace cricket

#endif  // TALK_SESSION_PHONE_ANDROIDDEVICEMANAGER_H_

