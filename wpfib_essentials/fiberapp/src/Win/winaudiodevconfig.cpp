#include <winsock2.h>
#include <propkeydef.h>

#define INITGUID
#include <Functiondiscoverykeys_devpkey.h>
#include "winaudiodevconfig.h"
#include <MMSystem.h>
#include "talk/base/logging_libjingle.h"

// these are from mmddk.h which we don't have in our build setup so define them here
#define DRV_QUERYDEVICEINTERFACE        0x080C
#define DRV_QUERYDEVICEINTERFACESIZE    0x080D

// this is defined in a webrtc header (audio_device_defines.h) to avoid pulling in all
// the webrtc includes we define it here. It's used as the max size of the device id strings
static const int kAdmMaxGuidSize = 128;

// This PROPERTYKEY isn't defined in any of the public Microsoft header files
// but we need it to map the audio device id to the hardware id string
const GUID HWKEYPROPERTY_GUID
    = { 0xB3F8FA53, 0x0004, 0x438E, { 0x90, 0x03, 0x51, 0xA4, 0x6E, 0x13, 0x9B, 0xFC } };
static const DWORD HWKEYPROPERTY_PID = 2;
static const PROPERTYKEY PKEY_AudioHardwareIdString = { HWKEYPROPERTY_GUID, HWKEYPROPERTY_PID };


bool GUIDStringsAreEqual(const std::string& guid1, const std::string& guid2)
{
    bool equal = false;

    // quick n dirty way is to do a case insensitive compare
    // rather than converting the strings to to GUIDs
    if (stricmp(guid1.c_str(), guid2.c_str()) == 0)
    {
        equal = true;
    }

    return equal;
}

void ExtractGUIDFromDeviceID(const std::string& deviceId, std::string& guid)
{
    // device Id looks like this:
    // {0.0.1.00000000}.{58e7a8dc-a7ff-430c-aa3f-fda3bf5e088a}
    // we just want the guid from it
    const char* guidStart = strrchr(deviceId.c_str(), '{');
    if (guidStart)
    {
        guid = guidStart;
    }
}

void HardwareKeyFromHardwareId(DeviceConfigType type, const std::string& hardwareId,
    unsigned int endpointType, std::string& vendorId, std::string& productId, std::string& classId)
{
    // the input hardware Id look like this:
    // {1}.PCI\VEN_1102&DEV_000B&SUBSYS_00411102&REV_04\4&4F261B2&0&00E1
    // or this:
    // {1}.HDAUDIO\FUNC_01&VEN_10EC&DEV_0887&SUBSYS_102804AA&REV_1003\4&2718705D&0&0001
    // or this:
    // {1}.USB\VID_046D&PID_0821&MI_00\8&333DF4D6&0&0000
    // We need to extract 3 pieces of information that make up the key
    // 1. type PCI or HDAUDIO or USB (we add our own HDMI type)
    // 2. Vendor id: VEN_XXXX or VID_XXXX
    // 3. Product id: PID_XXXX or DEV_XXXX

    // type is always the text following "{1}." and preceeding the first '\'
    const char* startChar = strchr(hardwareId.c_str(), '.');
    if (startChar)
    {
        const char* endChar = strchr(hardwareId.c_str(), '\\');
        if (endChar)
        {
            classId.assign(startChar + 1, (endChar - startChar - 1));
        }
    }
    // we change the type here so we can identify HDMI devices
    if (endpointType == DigitalAudioDisplayDevice)
    {
        classId = "HDMI";
    }

    // get the vendor id
    char* vendorKey = "VEN_";
    if (classId == "USB")
    {
        vendorKey = "VID_";
    }
    startChar = strstr(hardwareId.c_str(), vendorKey);
    if (startChar)
    {
        vendorId.assign(startChar + 4, 4);
    }

    // get the product id
    char* productKey = "DEV_";
    if (classId == "USB")
    {
        productKey = "PID_";
    }
    startChar = strstr(hardwareId.c_str(), productKey);
    if (startChar)
    {
        productId.assign(startChar + 4, 4);
    }
}

void HardwareKeyFromHardwareIdWave(DeviceConfigType type, const std::string& hardwareId,
    std::string& vendorId, std::string& productId, std::string& classId)
{
    // the input hardware Id look like one of these:
    // \\?\HDAUDIO#FUNC_01&VEN_10EC&DEV_0887&SUBSYS_102804AA&REV_1003#4&2718705D&0&0001#{6994AD04-93EF-11D0-A3CC-00A0C9223196}\ESLAVEDHPLINEOUTWAVE
    // \\?\PCI#VEN_1102&DEV_000B&SUBSYS_00411102&REV_04#4&4F261B2&0&00E1#{6994AD04-93EF-11D0-A3CC-00A0C9223196}\SPDIFOUTWAVE
    // \\?\USB#VID_046D&PID_0821&MI_00#8&333DF4D6&0&0000#{6994AD04-93EF-11D0-A3CC-00A0C9223196}\GLOBAL

    // We need to extract 3 pieces of information that make up the key
    // 1. type PCI or HDAUDIO or USB
    // 2. Vendor id: VEN_XXXX or VID_XXXX
    // 3. Product id: PID_XXXX or DEV_XXXX

    // type is always the text following "\\?\" up to the first '#'
    const char* startChar = hardwareId.c_str() + 4;
    if (startChar)
    {
        const char* endChar = strchr(hardwareId.c_str(), '#');
        if (endChar)
        {
            classId.assign(startChar, (endChar - startChar));
        }
    }

    // get the vendor id
    char* vendorKey = "VEN_";
    if (classId == "USB")
    {
        vendorKey = "VID_";
    }
    startChar = strstr(hardwareId.c_str(), vendorKey);
    if (startChar)
    {
        vendorId.assign(startChar + 4, 4);
    }

    // get the product id
    char* productKey = "DEV_";
    if (classId == "USB")
    {
        productKey = "PID_";
    }
    startChar = strstr(hardwareId.c_str(), productKey);
    if (startChar)
    {
        productId.assign(startChar + 4, 4);
    }
}

bool GetAudioHardwareIdFromDeviceIdCore(DeviceConfigType type,
    const std::string& deviceId,
    std::string& vendorId,
    std::string& productId,
    std::string& classId)
{
    // TODO: do we need to make sure CoInit has been called on this thread?
    HRESULT hr;
    CComPtr<IMMDeviceEnumerator> enumerator;
    std::string deviceGUID;

    // get the device Id GUID from the input
    ExtractGUIDFromDeviceID(deviceId, deviceGUID);

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&enumerator));
    bool found = false;
    if (SUCCEEDED(hr) && !found)
    {
        CComPtr<IMMDeviceCollection> devices;
        EDataFlow dir = (type == DeviceConfigTypeMic) ? eCapture : eRender;
        hr = enumerator->EnumAudioEndpoints(dir, DEVICE_STATE_ACTIVE, &devices);
        if (SUCCEEDED(hr))
        {
            unsigned int devCount;
            hr = devices->GetCount(&devCount);
            if (SUCCEEDED(hr))
            {
                for (unsigned int i = 0; i < devCount; i++)
                {
                    CComPtr<IMMDevice> device;

                    if (found)
                    {
                        break;
                    }

                    // Get pointer to endpoint number i.
                    hr = devices->Item(i, &device);
                    if (FAILED(hr))
                    {
                        break;
                    }

                    CComPtr<IPropertyStore> props;
                    HRESULT hr = device->OpenPropertyStore(STGM_READ, &props);
                    if (FAILED(hr))
                    {
                        break;
                    }

                    // get the GUID property
                    PROPVARIANT pvGUID;
                    PropVariantInit(&pvGUID);
                    if (SUCCEEDED(props->GetValue(PKEY_AudioEndpoint_GUID, &pvGUID)))
                    {
                        if(pvGUID.vt != VT_EMPTY && pvGUID.vt != VT_NULL && pvGUID.pwszVal != NULL)
                        {
                            std::string hwGUIDStr = CW2A(pvGUID.bstrVal);
                            if (GUIDStringsAreEqual(deviceGUID, hwGUIDStr))
                            {
                                // get the endpoint form factor (used to detect HDMI devices)
                                unsigned int endpointType = 0;
                                bool hdmi = false;
                                PROPVARIANT pvType;
                                PropVariantInit(&pvType);
                                if (SUCCEEDED(props->GetValue(PKEY_AudioEndpoint_FormFactor, &pvType)))
                                {
                                    endpointType = pvType.uintVal;
                                    PropVariantClear(&pvType);
                                }

                                // found the device, now get the hardware Id
                                PROPVARIANT pvHwKey;
                                PropVariantInit(&pvHwKey);
                                if (SUCCEEDED(props->GetValue(PKEY_AudioHardwareIdString, &pvHwKey)))
                                {
                                    //Jie: Not sure why we use bstrVal instead of pwszVal, but from the debugging, seems that vt is 31, which is VT_LPWSTR
                                    if(pvHwKey.vt != VT_EMPTY && pvHwKey.vt != VT_NULL && pvHwKey.pwszVal != NULL)
                                    {
                                        // got the hardware key we were looking for
                                        std::string hwKey = CW2A(pvHwKey.bstrVal);
                                        HardwareKeyFromHardwareId(type, hwKey, endpointType, vendorId, productId, classId);
                                        PropVariantClear(&pvHwKey);
                                        // quit the loop
                                        found = true;
                                    }
                                    else
                                    {
                                        LOG(LS_INFO) << "Getting PKEY_AudioHardwareIdString with GUID: "
                                            << hwGUIDStr << " failed. Try getting its friendly name.";
                                        PROPVARIANT pvFriendlyName;
                                        PropVariantInit(&pvFriendlyName);
                                        if (SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName, &pvFriendlyName)))
                                        {
                                            if(pvFriendlyName.vt == VT_LPWSTR && pvFriendlyName.pwszVal != NULL)
                                            {
                                                std::string hwFriendlyNameStr = CW2A(pvFriendlyName.pwszVal);
                                                LOG(LS_INFO) << "Device Friendly name: "<<hwFriendlyNameStr;
                                            }
                                        }
                                    }//else
                                }
                            }
                        }
                        PropVariantClear(&pvGUID);
                    }
                }
            }
        }
    }

    if (FAILED(hr)) {
        return false;
    }

    return found;
}

UINT waveInOutGetNumDevs(DeviceConfigType type)
{
    if (type == DeviceConfigTypeMic)
    {
        return waveInGetNumDevs();
    }
    else
    {
        return waveOutGetNumDevs();
    }
}

MMRESULT waveInOutGetDevName(DeviceConfigType type, UINT_PTR id, WCHAR name[MAXPNAMELEN])
{
    MMRESULT ret = 0;
    if (type == DeviceConfigTypeMic)
    {
        WAVEINCAPS caps = { 0 };
        ret = waveInGetDevCaps(id, &caps, sizeof(caps));
        wcscpy(name, caps.szPname);
    }
    else
    {
        WAVEOUTCAPS caps = { 0 };
        ret = waveOutGetDevCaps(id, &caps, sizeof(caps));
        wcscpy(name, caps.szPname);
    }
    return ret;
}

MMRESULT waveInOutMessage(DeviceConfigType type, UINT id, UINT uMsg, DWORD_PTR dw1, DWORD_PTR dw2)
{
    if (type == DeviceConfigTypeMic)
    {
        return waveInMessage((HWAVEIN)id, uMsg, dw1, dw2);
    }
    else
    {
        return waveOutMessage((HWAVEOUT)id, uMsg, dw1, dw2);
    }
}

bool GetAudioHardwareIdFromDeviceIdWave(DeviceConfigType type,
    const std::string& deviceId,
    std::string& vendorId,
    std::string& productId,
    std::string& classId)
{
    bool found = false;

    UINT count = waveInOutGetNumDevs(type);
    for (UINT i = 0; i < count; i++)
    {
        if (found)
            break;

        WCHAR name[MAXPNAMELEN];
        if (waveInOutGetDevName(type, i, name) == MMSYSERR_NOERROR)
        {
            // the deviceId passed in was converted to UTF8 so we must do the same conversion
            char deviceName[kAdmMaxGuidSize];
            WideCharToMultiByte(CP_UTF8, 0, name, -1, deviceName, kAdmMaxGuidSize, NULL, NULL);
            if (deviceId == deviceName)
            {
                // found the device we want, get its hardware identifier
                ULONG bufferLen = 0;
                if (waveInOutMessage(type, i, DRV_QUERYDEVICEINTERFACESIZE, (DWORD_PTR)&bufferLen, 0)
                    == MMSYSERR_NOERROR)
                {
                    WCHAR* buffer = (WCHAR*) new char[bufferLen];
                    buffer[0] = 0;
                    if (waveInOutMessage(type, i, DRV_QUERYDEVICEINTERFACE, (DWORD_PTR)buffer, bufferLen)
                        == MMSYSERR_NOERROR)
                    {
                        // make it all upper case
                        size_t len = wcslen(buffer);
                        for (size_t j = 0; j < len; j++)
                        {
                            buffer[j] = towupper(buffer[j]);
                        }
                        std::string hwKey = CW2A(buffer);
                        HardwareKeyFromHardwareIdWave(type, hwKey, vendorId, productId, classId);
                        found = true;
                    }
                    delete [] buffer;
                }
            }
        }
    }

    return found;
}

bool GetAudioHardwareIdFromDeviceIdWin(DeviceConfigType type,
    const std::string& deviceId,
    std::string& vendorId,
    std::string& productId,
    std::string& classId)
{
    OSVERSIONINFO versionInfo = {0};
    versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
    GetVersionEx(&versionInfo);

    // check for Vista or later, Vista is major OS version 6
    if (versionInfo.dwMajorVersion >= 6)
    {
        return GetAudioHardwareIdFromDeviceIdCore(type, deviceId, vendorId, productId, classId);
    }
    else
    {
        // TODO: currently we can't detect HMDI output devices on XP
        // they will be listed as HDAUDIO devices, on Vista and above
        // we explicitly replace HDAUDIO with HDMI in the hardwareId
        return GetAudioHardwareIdFromDeviceIdWave(type, deviceId, vendorId, productId, classId);
    }
}
