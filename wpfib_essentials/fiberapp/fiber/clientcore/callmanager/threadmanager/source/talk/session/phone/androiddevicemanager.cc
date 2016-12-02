//This code is only for compatibiity
//dummy functions for Android.


#include "talk/session/phone/androiddevicemanager.h"

#include "talk/base/logging_libjingle.h"
#include "talk/base/stringutils.h"
#include "talk/base/thread.h"
#include "talk/session/phone/mediacommon.h"



namespace cricket {

DeviceManagerInterface* DeviceManagerFactory::Create() {
  return new AndroidDeviceManager();
}
    class AndroidDeviceWatcher : public DeviceWatcher {
    public:
        explicit AndroidDeviceWatcher(DeviceManagerInterface* dm);
        virtual ~AndroidDeviceWatcher();
        bool Start(){return true;}
        void Stop(){return;}
    };
    
    AndroidDeviceManager::AndroidDeviceManager(){}
    AndroidDeviceManager::~AndroidDeviceManager(){}

    
};

