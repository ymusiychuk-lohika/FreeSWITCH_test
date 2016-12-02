/******************************************************************************
 * BwMgr header file
 *
 * Copyright (C) Bluejeans Network, Inc  All Rights Reserved
 *
 * Author: Brian Ashford
 * Date:   Oct 3, 2013
 *****************************************************************************/

#ifndef OUTGOINGBWMGR_H_
#define OUTGOINGBWMGR_H_

#include "smdataandactions.h"
#include "windowedbitrateandlosstracker.h"

namespace bwmgr {

class OutgoingBwMgr
: public SMDataAndActions
{
public:
    OutgoingBwMgr(const BwMgrContext& ctx);
    virtual ~OutgoingBwMgr();

    void updatePacketLoss(
        BwMgrMediaType mediaType,
        const LossReport &lossReport);

protected:

    void    notifyBitrateChange(uint32_t bitrate);

private:

    WindowedBitrateAndLossTracker mBitrateLossTracker;
};

} // namespace bwmgr

#endif /* BWMGRSM_H_ */
