/****************************************************************************
 * delayanalyzer.h
 *
 * Copyright (C) Bluejeans Network, Inc  All Rights Reserved
 *
 * Author: Brian Ashford
 * Date:   06/21/2013
 *****************************************************************************/

#ifndef DELAYANALYZER_H_
#define DELAYANALYZER_H_

#include <pj/types.h>
#include <pj/pool.h>

typedef struct delay_analyzer_t delay_analyzer_t;

delay_analyzer_t* delayAnalyzerCreate(pj_pool_t* pool);
void        delayAnalyzerUpdatePacketDelay(delay_analyzer_t* analyzer, pj_uint64_t recvtime, pj_uint32_t timestamp);
pj_bool_t   delayAnalyzerGetPacketDelay(delay_analyzer_t* analyzer, pj_uint32_t* delay);
void        delayAnalyzerUpdateReferencePacket(delay_analyzer_t* analyzer, pj_bool_t keyframe);

#endif /* DELAYANALYZER_H_ */
