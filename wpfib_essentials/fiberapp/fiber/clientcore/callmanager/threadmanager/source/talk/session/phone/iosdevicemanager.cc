//This code is only for compatibiity
//dummy functions for IOS.


#include "talk/session/phone/iosdevicemanager.h"

#include "talk/base/logging_libjingle.h"
#include "talk/base/stringutils.h"
#include "talk/base/thread.h"
#include "talk/session/phone/mediacommon.h"



namespace cricket {

DeviceManagerInterface* DeviceManagerFactory::Create() {
  return new IosDeviceManager();
}
    class IosDeviceWatcher : public DeviceWatcher {
    public:
        explicit IosDeviceWatcher(DeviceManagerInterface* dm);
        virtual ~IosDeviceWatcher();
        bool Start(){return true;}
        void Stop(){return;}
    };
    
    IosDeviceManager::IosDeviceManager(){}
    IosDeviceManager::~IosDeviceManager(){}

    
};

