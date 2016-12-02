/******************************************************************************
 * Assorted functions to aid in RTP processing
 *
 * Copyright (C) Bluejeans Network, All Rights Reserved
 *
 * Author: jwallace
 * Date:   Oct 31, 2013
 *****************************************************************************/

#ifndef RTPUTIL_H
#define RTPUTIL_H

#include <stdint.h>
#include <limits>

// Comparison functions for sequence numbers and timestamps
template <typename T>
bool rtpLess(T lhs, T rhs)
{
    T delta = lhs - rhs;
    return delta >= std::numeric_limits<T>::max()/2;
}

template <typename T>
bool rtpGreater(T lhs, T rhs)
{
    T delta = rhs - lhs;
    return delta >= std::numeric_limits<T>::max()/2;
}

template <typename T>
bool rtpLtEq(T lhs, T rhs)
{
    return !rtpGreater(lhs, rhs);
}

template <typename T>
bool rtpGtEq(T lhs, T rhs)
{
    return !rtpLess(lhs, rhs);
}

inline int16_t seqDelta(uint16_t cur, uint16_t prev)
{
    return cur - prev;
}

#endif // ifndef RTPUTIL_H
