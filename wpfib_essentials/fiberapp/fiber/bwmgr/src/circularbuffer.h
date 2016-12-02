/******************************************************************************
 * Implementation of a circular buffer template
 *
 * Copyright (C) Bluejeans Network, All Rights Reserved
 *
 * Author: jwallace
 * Date:   Nov 1, 2013
 *****************************************************************************/

#ifndef CIRCULARBUFFER_H_
#define CIRCULARBUFFER_H_

#include <iostream>
#include <cassert>
#include <vector>

namespace bwmgr
{

// a push-only buffer that wraps around std::vector, with functions akin to the
// STL counterpart.
// maybe someday it'll implement iterators
template <typename T>
class CircularBuffer
{
public:
    typedef size_t size_type;

    CircularBuffer()
        : mCapacity(0)
        , mTail(0)
    {
    }

    CircularBuffer(size_type capacity)
        : mCapacity(capacity)
        , mTail(0)
    {
        mBuffer.reserve(capacity);
    }

    void clear()
    {
        mBuffer.clear();
        mTail = 0;
    }

    // pops old element once at capacity
    void push_back(const T &elem)
    {
        if (mCapacity == 0)
            return;

        if (mBuffer.size() < mCapacity)
        {
            mBuffer.push_back(elem);
        }
        else
        {
            mBuffer[mTail] = elem;
            ++mTail;
            if (mTail == mCapacity)
                mTail = 0;
        }
    }

    T &front()
    {
        assert(mCapacity != 0 && "There isn't much functionality available in a 0-sized circular buffer");
        return mBuffer[mTail];
    }

    const T &front() const
    {
        assert(mCapacity != 0 && "There isn't much functionality available in a 0-sized circular buffer");
        return mBuffer[mTail];
    }

    T &back()
    {
        assert(mCapacity != 0 && "There isn't much functionality available in a 0-sized circular buffer");
        return mBuffer[convertIndex(mBuffer.size() - 1)];
    }

    const T &back() const
    {
        assert(mCapacity != 0 && "There isn't much functionality available in a 0-sized circular buffer");
        return mBuffer[convertIndex(mBuffer.size() - 1)];
    }

    T &operator[](size_type idx)
    {
        assert(mCapacity != 0 && "There isn't much functionality available in a 0-sized circular buffer");
        return mBuffer[convertIndex(idx)];
    }

    const T &operator[](size_type idx) const
    {
        assert(mCapacity != 0 && "There isn't much functionality available in a 0-sized circular buffer");
        return mBuffer[convertIndex(idx)];
    }

    T &at(size_type idx)
    {
        // not asserting as at() is designed to throw when out of bounds
        return mBuffer.at(convertIndex(idx));
    }

    const T &at(size_type idx) const
    {
        // not asserting as at() is designed to throw when out of bounds
        return mBuffer.at(convertIndex(idx));
    }

    size_t size() const
    {
        return mBuffer.size();
    }

    bool empty() const
    {
        return mBuffer.empty();
    }

    void clearAndSetCapacity(size_type capacity)
    {
        clear();

        if (mBuffer.capacity() > capacity)
        {
            // shrink to fit
            std::vector<T> newBuffer;
            mBuffer.swap(newBuffer);
        }

        mBuffer.reserve(capacity);
        mCapacity = capacity;
    }

private:

    // maps the index that a user sees to the real underlying index
    size_type convertIndex(size_type idx) const
    {
        assert(idx < mCapacity);
        size_type realIdx = idx + mTail;
        if (realIdx >= mCapacity)
            realIdx -= mCapacity;
        return realIdx;
    }

    std::vector<T> mBuffer;
    size_type mCapacity;
    size_type mTail;
};
} // namespace bwmgr

#endif // ifndef CIRCULARBUFFER_H_
