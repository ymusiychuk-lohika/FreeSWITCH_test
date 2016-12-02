#ifndef I_DEVICE_CONTROLLER_H_
#define I_DEVICE_CONTROLLER_H_

namespace bjn_sky {
class skinnySipManager;
}

struct deviceOperationStatus
{
    uint16_t usage;
    bool usageOn;
    bool success;
};

class IDeviceController{

public:
    enum muteTypes {
        localMute,
        remoteMute,
        DeviceMute = 47
    };

    IDeviceController(){}

    virtual ~IDeviceController(){}

    virtual bool setDeviceState(uint16_t usage, bool state) = 0;

    virtual bool setDeviceMute(muteTypes muteType, bool state) = 0;

    virtual bool initialize() = 0;

    virtual bool setEventHandler(bjn_sky::skinnySipManager* sipObj) = 0;

    virtual void processDeviceChange(bool deviceAdded) = 0;

    static IDeviceController* createDeviceController();

    virtual int getDeviceEnumerateWaitTime() = 0;
};

#endif
