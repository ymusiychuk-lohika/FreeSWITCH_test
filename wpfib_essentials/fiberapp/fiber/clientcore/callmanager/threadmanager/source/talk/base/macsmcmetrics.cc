#include "macsmcmetrics.h"

namespace talk_base {

SMCMetricsMac::SMCMetricsMac():
    _numFans(0),
    _initializationSuccess(false),
    _smcConnection(IO_OBJECT_NULL)
{
    Init();
}

SMCMetricsMac::~SMCMetricsMac()
{
    SMCClose();
}

void SMCMetricsMac::Init()
{
    kern_return_t result = SMCOpen();
    if(kIOReturnSuccess == result)
    {
        _initializationSuccess = true;
        _numFans = GetFanCount(SMC_KEY_FAN_NUM);
        LOG(LS_INFO) << "MacSMC - Number of Fans: " <<  _numFans;

    }
}

kern_return_t SMCMetricsMac::SMCOpen(void)
{
    kern_return_t result;
    mach_port_t   masterPort;
    io_iterator_t iterator;
    io_object_t   device;

    result = IOMasterPort(MACH_PORT_NULL, &masterPort);
    if (result != kIOReturnSuccess)
    {
        LOG(LS_ERROR) << "Error: MacSMC - IOMasterPort() = " << result;
        return result;
    }

    CFMutableDictionaryRef matchingDictionary = IOServiceMatching("AppleSMC");
    if(NULL == matchingDictionary)
    {
        LOG(LS_ERROR) << "Error: MacSMC - Matching Dictionary not created" ;
        return kIOReturnError;
    }
    result = IOServiceGetMatchingServices(masterPort, matchingDictionary, &iterator);

    //matchingDictionary gets consumed by the IOServiceGetMatchingServices.
    //Hence no need to CFRelease
    matchingDictionary = NULL;
    if (result != kIOReturnSuccess)
    {
        LOG(LS_ERROR) << "Error: MacSMC - IOServiceGetMatchingServices() = " << result;
        return result;
    }

    device = IOIteratorNext(iterator);
    IOObjectRelease(iterator);
    if (device == 0)
    {
        LOG(LS_ERROR) << "Error: MacSMC - no SMC found ";
        return kIOReturnError;
    }

    result = IOServiceOpen(device, mach_task_self(), 0, &_smcConnection);
    IOObjectRelease(device);
    if (result != kIOReturnSuccess)
    {
        LOG(LS_ERROR) << "Error: IOServiceOpen() = " << result;
        return result;
    }

    return kIOReturnSuccess;
}

kern_return_t SMCMetricsMac::SMCClose()
{
    if(!_initializationSuccess)
        return kIOReturnSuccess;

    return IOServiceClose(_smcConnection);
}

kern_return_t SMCMetricsMac::SMCCall(int index, SMCParamStruct *inputStructure, SMCParamStruct *outputStructure)
{
    size_t   structureInputSize = sizeof(SMCParamStruct);
    size_t   structureOutputSize = sizeof(SMCParamStruct);

    return IOConnectCallStructMethod( _smcConnection, index,
                                     // inputStructure
                                     inputStructure, structureInputSize,
                                     // ouputStructure
                                     outputStructure, &structureOutputSize );

}

float SMCMetricsMac::GetFanSpeed()
{
    if(!_initializationSuccess)
        return 0.0f;

    if(_numFans == 0)
        return 0.0f;

    SMCOutput val;
    kern_return_t result;
    UInt32Char_t  key;

    float combinedFanSpeed = 0.0f;
    float avgFanSpeed = 0.0f;

    for(int i = 0; i < _numFans; i++)
    {
        sprintf(key, SMC_KEY_FAN_SPEED, i);
        result = SMCReadKey(key, &val);
        float fanSpeed = 0;

        if(result == kIOReturnSuccess)
        {
            fanSpeed = Strtof(val.bytes, val.dataSize, 2);
        }
        else
        {
            return 0.0f;
        }

        combinedFanSpeed = combinedFanSpeed + fanSpeed;
    }

    avgFanSpeed = combinedFanSpeed/_numFans;
    return avgFanSpeed;
}

int SMCMetricsMac::GetFanCount(char *key)
{
    if(!_initializationSuccess)
        return 0;

    SMCOutput val;
    kern_return_t result;
    result = SMCReadKey(key, &val);

    int ret = 0;
    if(result == kIOReturnSuccess)
    {
        ret = Strtoul((char *)val.bytes, val.dataSize, 10);
    }

    return ret;
}

kern_return_t SMCMetricsMac::SMCReadKey(UInt32Char_t key, SMCOutput *val)
{
    kern_return_t result;
    SMCParamStruct  inputStructure;
    SMCParamStruct  outputStructure;

    memset(&inputStructure, 0, sizeof(SMCParamStruct));
    memset(&outputStructure, 0, sizeof(SMCParamStruct));
    memset(val, 0, sizeof(SMCOutput));

    inputStructure.key = Strtoul(key, 4, 16);
    inputStructure.data8 = SMC_CMD_READ_KEYINFO;

    result = SMCCall(KERNEL_INDEX_SMC, &inputStructure, &outputStructure);
    if (result != kIOReturnSuccess)
    {
        LOG(LS_ERROR) << "Error: MacSMC SMCCall() : " << result;
        return result;
    }

    val->dataSize = outputStructure.keyInfo.dataSize;
    Ultostr(val->dataType, outputStructure.keyInfo.dataType);
    inputStructure.keyInfo.dataSize = val->dataSize;
    inputStructure.data8 = SMC_CMD_READ_BYTES;

    result = SMCCall(KERNEL_INDEX_SMC, &inputStructure, &outputStructure);
    if (result != kIOReturnSuccess)
    {
        LOG(LS_ERROR) << "Error: MacSMC SMCCall() : " << result;
        return result;
    }

    memcpy(val->bytes, outputStructure.bytes, sizeof(outputStructure.bytes));

    return kIOReturnSuccess;
}


}
