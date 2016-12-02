#ifndef BJN_FBR_CPU_MONITOR_H
#define BJN_FBR_CPU_MONITOR_H

#include <pjlib.h>
//Maximum amount of time to wait to bring resolution up after maximum number of time its dropped.
#define MAX_TIME_UPGRADE 30
#define NORMAL_ACTION_DURATION 5

#define MAX_CPU_THRESHOLD 75
#define MIN_CPU_THRESHOLD 60

//In locked mode to start again improving resolution, CPU utilization should be below MIN_CPU_THRESHOLD_LOCKED_MODE for MAX_TIME_UPGRADE + NORMAL_ACTION_DURATION secs consistently.
#define MIN_CPU_THRESHOLD_LOCKED_MODE 40

enum CpuLoadType {
    CPU_LOAD_NONE,
    CPU_LOAD_HIGH,
    CPU_LOAD_LOW,
};

class FbrCpuMonitor
{
public :
    FbrCpuMonitor();
    int getCpuStrength(pj_time_val currentTime, unsigned cpuLoad);
    CpuLoadType applyCpuLoad(unsigned cpuLoad);

private :
    pj_time_val mCpuOverloadDur;
    pj_time_val mCpuNormalDur;
    int mCpuActionDegradeDuration;
    int mCpuActionUpgradeDuration;
    int mCpuMaxThreshold;
    int mCpuMinThreshold;
    int mCpuMaxDropCnt;
};

#endif //BJN_FBR_CPU_MONITOR_H
