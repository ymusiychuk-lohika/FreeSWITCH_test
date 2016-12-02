#include <stdint.h>
#include <stddef.h>
#include "IDeviceController.h"

#ifdef FB_WIN
#include "win/deviceController.h"
#endif

IDeviceController* IDeviceController::createDeviceController()
{
    IDeviceController* device_controller = NULL;
#ifdef FB_WIN
    device_controller = DeviceController::createLogitechSRKObject();
#endif
    return device_controller;
}
