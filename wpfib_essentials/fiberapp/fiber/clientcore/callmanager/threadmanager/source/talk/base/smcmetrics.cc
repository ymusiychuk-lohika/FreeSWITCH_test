
#include "smcmetrics.h"

#ifdef OSX
#include "macsmcmetrics.h"
#endif

namespace talk_base {

//SMCMetricsBase class member functions definitions
SMCMetricsBase::SMCMetricsBase()
{
};

SMCMetricsBase:: ~SMCMetricsBase()
{
};

SMCMetricsBase* SMCMetricsBase::CreateSMCMetricsObject()
{
#ifdef OSX
    return new SMCMetricsMac();
#else
    return NULL;
#endif
}

float SMCMetricsBase:: GetFanSpeed()
{
    return 0;
}


//smcMetrics class member functions definitions
SMCMetrics::SMCMetrics():
    _SMCMetrics(NULL)
{
};

SMCMetrics:: ~SMCMetrics()
{
    if(_SMCMetrics)
    {
        delete _SMCMetrics;
        _SMCMetrics = NULL;
    }
};

float SMCMetrics:: GetFanSpeed()
{
    float ret = 0.0f;
    if(_SMCMetrics)
    {
        ret = _SMCMetrics->GetFanSpeed();
        LOG(LS_INFO) << "Mac Fan speed: " << ret;
    }

    return ret;
}

void SMCMetrics::Init()
{
    _SMCMetrics = SMCMetricsBase::CreateSMCMetricsObject();
}
}

