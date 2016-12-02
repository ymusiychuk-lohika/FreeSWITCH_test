//The System Management Controller (SMC) is a subsystem of Intel processor-based Macintosh computers.
//This file contains class SMCMetricsMac meant for SMC related queries like fan speed,
//CPU core temperature etc. It uses IO Kit framework for hardware & kernel level queries.

#ifndef _MAC_SMC_METRICS_H_
#define _MAC_SMC_METRICS_H_

#include <stdio.h>
#include <string.h>
#include <IOKit/IOKitLib.h>
#include "macutils.h"
#include "smcmetrics.h"

namespace talk_base {

#define KERNEL_INDEX_SMC      2

#define SMC_CMD_READ_BYTES    5
#define SMC_CMD_READ_KEYINFO  9

// key values
#define SMC_KEY_FAN_SPEED     "F%dAc"
#define SMC_KEY_FAN_NUM       "FNum"

//structs holds SMC version.
typedef struct {
    char                  major;
    char                  minor;
    char                  build;
    char                  reserved[1];
    UInt16                release;
} SMCVersion;

typedef struct {
    UInt16                version;
    UInt16                length;
    UInt32                cpuPLimit;
    UInt32                gpuPLimit;
    UInt32                memPLimit;
} SMCPLimitData;

typedef struct {
    UInt32                dataSize;
    UInt32                dataType;
    char                  dataAttributes;
} SMCKeyInfoData;

typedef char              SMCBytes_t[32];


/*
structure that needs to be passed to IOConnectCallStructMethod.
key – kind of value is accessed
keyInfo
    dataType – The data type of the data returned
    dataSize – The number of bytes for that key’s data
data8 – The kind of operation we want to perform for key
bytes – The data
*/

typedef struct {
    UInt32                  key;
    SMCVersion              vers;
    SMCPLimitData           pLimitData;
    SMCKeyInfoData          keyInfo;
    char                    result;
    char                    status;
    char                    data8;
    UInt32                  data32;
    SMCBytes_t              bytes;
} SMCParamStruct;

typedef char              UInt32Char_t[5];

//Holds the final output. Data is stored bytes.
typedef struct {
    UInt32Char_t            key;
    UInt32                  dataSize;
    UInt32Char_t            dataType;
    SMCBytes_t              bytes;
} SMCOutput;

class SMCMetricsMac : public SMCMetricsBase
{
public:
    SMCMetricsMac();
    ~SMCMetricsMac();

    float GetFanSpeed();

private:

    bool _initializationSuccess;
    int  _numFans;
    io_connect_t _smcConnection;

    void Init();
    int GetFanCount(char *key);

    kern_return_t SMCOpen(void);
    kern_return_t SMCClose();
    kern_return_t SMCCall(int index, SMCParamStruct *inputStructure, SMCParamStruct *outputStructure);
    kern_return_t SMCReadKey(UInt32Char_t key, SMCOutput *val);
};

}
#endif
