/******************************************************************************
 * Monitor and analyze loss
 *
 * Copyright (C) Bluejeans Network, Inc  All Rights Reserved
 *
 * Author: Brian Ashford
 * Date:   Nov 7, 2013
 *****************************************************************************/


#ifndef LOSSMONITOR_H_
#define LOSSMONITOR_H_

#include "bwmgr_types.h"
#include "rtpstatsutil.h"
#include "logger.h"

namespace bwmgr {

class LossMonitor
: public Logger
{
public:

    LossMonitor(const BwMgrContext& ctx);
    ~LossMonitor();

    int         updatePacketLoss(double loss, uint32_t bw);

    double      getConstantLoss(void) const;

    uint32_t    getZeroLossBwThreshhold(void) const;

protected:

    int         getBandwidthAdjustmentDirection(double loss, uint32_t bw);

    double      detectConstantLoss(double loss, uint32_t bw);

private:

    double      mConstantLoss;

    // used with loss
    uint32_t    mLossVarianceCount;
    uint32_t    mZeroLossCount;
    uint32_t    mZeroLossThreshhold;
    RunningMeanAndVariance  mLossVariance;

    WindowedCorrelationCoeffT<double>    mCorrelation;
    WindowedAverageT<double>             mAveragedCorrelation;
};

} // namespace bwmgr
#endif /* LOSSMONITOR_H_ */
