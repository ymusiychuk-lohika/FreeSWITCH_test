//
//  moving_average.h
//
//  This class calculates a moving average from the given set of numeric samples.
//  It supports setting a maximum number of samples over which the average will be calculated.
//  Once the maximum number of samples is reached, as each new sample is added, the oldest sample
//  is discarded.
//
//  Created by Brian T. Toombs on 1/7/2015.
//  Copyright (c) 2015 Bluejeans Network, Inc. All rights reserved.
//

#ifndef moving_average_h
#define moving_average_h

#include <vector>
#include <pj/log.h>
#include <stdio.h>
#include <string>
#include <sstream>

template <class T>
class MovingAverage
{
public:
    MovingAverage();
    MovingAverage(typename std::vector<T>::size_type maxSize);
    virtual ~MovingAverage() { Clear(); }

    // Set the maximum number of samples to be held in the buffer.  Returns false and fails to change the size if the buffer is NOT empty.
    bool MaxSize(typename std::vector<T>::size_type maxSize);

    // Get the maximum number of samples that can be held in the buffer.  If the MaxSize has not been specified, returns 0.
    typename std::vector<T>::size_type MaxSize() { return m_maxSize; }

    // Add a new sample to the buffer, replacing the oldest one if the buffer is full.  Returns the new average.  Copies the sample as it is added.
    T AddSample(const T &sample);

    // Remove the most recent sample from the buffer.  Returns the removed sample.  If the buffer is empty, returns 0.
    T RemoveSample();

    // Return the moving average, or 0 if the buffer is empty.
    T Mean() const { return ComputeMean(); }

    // Return the number of samples in the buffer
    typename std::vector<T>::size_type Size() const { return m_curSize; }

    // Remove all samples, but keep the existing max size setting
    void Clear();

protected:
    T ComputeMean() const
    {
        return m_curSize ? m_total/m_curSize : 0;
    }

protected:
    std::vector<T>                     m_circBuffer; // Circular buffer
    typename std::vector<T>::size_type m_next;       // Points to the next slot to be used in m_circBuffer
    T                                  m_total;      // Running total of all the values in m_circBuffer to make calculating the average quicker
    typename std::vector<T>::size_type m_maxSize;    // Max number of samples to be held in the buffer
    typename std::vector<T>::size_type m_curSize;    // Current number of samples in the buffer.  This needs to be tracked separately from the vector since
                                                     // sometimes there are zero-sized samples in the buffer that we want to skip when calculating the average
    std::string                        m_LogName;
};

//----------------------------------------------------------------------------//
// Name: MovingAverage default constructor
//----------------------------------------------------------------------------//
template <class T>
MovingAverage<T>::MovingAverage()
    : m_next(0)
    , m_total(0)
    , m_maxSize(0)
    , m_curSize(0)
{
    std::stringstream ss;
    ss << "movAvg" << this;
    m_LogName = ss.str();
}

//----------------------------------------------------------------------------//
// Name: MovingAverage Constructor
//----------------------------------------------------------------------------//
template <class T>
MovingAverage<T>::MovingAverage(typename std::vector<T>::size_type maxSize)
    : m_next(0)
    , m_total(0)
    , m_maxSize(maxSize)
    , m_curSize(0)
{
    std::stringstream ss;
    ss << "movAvg" << this;
    m_LogName = ss.str();

    m_circBuffer.reserve(m_maxSize);
}

//----------------------------------------------------------------------------//
// Name: MaxSize
// Desc: Set the maximum number of samples to be held in the buffer.  The buffer
//       must be empty.
//
// Return: true if successful.
//         false if the buffer is NOT empty
//----------------------------------------------------------------------------//
template <class T>
bool MovingAverage<T>::MaxSize(typename std::vector<T>::size_type maxSize)
{
    if (m_circBuffer.size() == 0)
    {
        m_maxSize = maxSize;
        m_circBuffer.reserve(m_maxSize);
        return true;
    }
    else
    {
        return false;
    }
}

//----------------------------------------------------------------------------//
// Name: AddSample
// Desc: Add a new sample to the buffer, replacing the oldest one if the buffer
//       is full.  Copies the sample as it is added.
//
// Return: The new average, calculated after the sample has been added.
//----------------------------------------------------------------------------//
template <class T>
T MovingAverage<T>::AddSample(const T &sample)
{
    // Once the circular buffer is full, remove the oldest sample from the running total
    if (m_circBuffer.size() == m_maxSize)
    {
        m_total -= m_circBuffer[m_next];

        if (m_curSize < m_maxSize)
            ++m_curSize;

        // Insert new sample
        m_circBuffer[m_next] = sample;
    }
    else
    {
        // Append new sample
        m_circBuffer.push_back(sample);
        ++m_curSize;
    }

    // Adjust running total to include newest sample
    m_total += m_circBuffer[m_next];

    std::stringstream ss;
    ss << "adding sample " << m_circBuffer[m_next] << " to buffer at index " << m_next << ". m_total="
       << m_total << ", buffer size=" << m_curSize << ", avg=" << ComputeMean();
    PJ_LOG(6, (m_LogName.c_str(), "%s", ss.str().c_str()));

    // Adjust index, wrapping around if necessary
    ++m_next;
    if (m_next == m_maxSize)
        m_next = 0;

    // return average
    return ComputeMean();
}

//----------------------------------------------------------------------------//
// Name: RemoveSample
// Desc: Remove the most recent sample from the buffer.
//
// Return: The removed sample, or 0 if the buffer is empty.
//----------------------------------------------------------------------------//

template <class T>
T MovingAverage<T>::RemoveSample()
{
    T oldSample;

    // If there's nothing in the circular buffer yet, there's nothing to do.
    if (m_curSize == 0)
        return 0;

    // Adjust index, wrapping around if necessary
    --m_next;
    if (m_next >= m_maxSize || m_next < 0 ) // m_next is likely unsigned, so wraparound happened when it's a large value, but check the signed case, too, for safety
        m_next = m_maxSize - 1;

    oldSample = m_circBuffer[m_next];

    // Adjust running total: remove newest sample
    m_total -= oldSample;
    if (m_curSize > 0)
        --m_curSize;

    std::stringstream ss;
    ss << "removing sample " << oldSample << " from buffer at index " << m_next << ". m_total="
       << m_total << ", buffer size=" << m_curSize << ", avg=" << ComputeMean();
    PJ_LOG(6, (m_LogName.c_str(), "%s", ss.str().c_str()));


    // if the buffer was full, we need to zero out the element we removed so that it won't be removed
    // again when the next element is added.  We don't want to physically remove it, since it could be
    // in the middle of the buffer and removing it will be expensive, as all the other elements would
    // need to be shifted.
    if (m_circBuffer.size() == m_maxSize)
    {
        m_circBuffer[m_next] = 0;
    }
    else    // If it wasn't full, remove and discard the last element.
    {
        m_circBuffer.pop_back();
    }

    return oldSample;
}

template <class T>
void MovingAverage<T>::Clear()
{
    m_circBuffer.clear();
    m_next = 0;
    m_total = 0;
    m_curSize = 0;
}

#endif