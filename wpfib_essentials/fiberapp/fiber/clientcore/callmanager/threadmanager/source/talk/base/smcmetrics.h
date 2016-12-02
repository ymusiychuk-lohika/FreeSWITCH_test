#ifndef _SMC_METRICS_H_
#define _SMC_METRICS_H_

#include "talk/base/logging_libjingle.h"

namespace talk_base {

//Base class for platform specific  smcMetrics classes
class SMCMetricsBase
{
public:
    SMCMetricsBase();
    ~SMCMetricsBase();

    static SMCMetricsBase* CreateSMCMetricsObject();
    virtual float GetFanSpeed();
};

class SMCMetrics
{
public:
    SMCMetrics();
    ~SMCMetrics();

    void Init();
    float GetFanSpeed();

private:
    SMCMetricsBase* _SMCMetrics;

};

}
#endif
