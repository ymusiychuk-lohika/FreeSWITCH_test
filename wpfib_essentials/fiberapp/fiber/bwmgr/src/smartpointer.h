/******************************************************************************
 * This contains a basic smart pointer that should be a subset of an
 * equivalent C++11 pointer, for build environment compatibility
 *
 * Copyright (C) Bluejeans Network, Inc  All Rights Reserved
 *
 * Author: Jeff Wallace
 * Date:   Sep 8, 2015
 *****************************************************************************/

#ifndef BWMGR_SMARTPOINTER_H
#define BWMGR_SMARTPOINTER_H

#include <cassert>
#include <algorithm>
#include "noncopyable.h"

// The inline keyword means nothing, this is what works.
#if _WIN32
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE __attribute__((always_inline))
#endif

namespace bwmgr
{
// This is designed as a subset of std::unique_ptr, for compatibility on
// pre C++11 compilers.  Once we have C++11 available on all platforms, it should be a
// trivial swap to use std::unique_ptr instead
template <typename T>
class UniquePtr : private NonCopyable<UniquePtr<T> >
{
public:
    UniquePtr()
        : mPtr(NULL)
    {
    }

    explicit UniquePtr(T *ptr)
        : mPtr(ptr)
    {
    }

    ~UniquePtr()
    {
        delete mPtr;
    }

    void reset(T *ptr = NULL)
    {
        if (ptr != mPtr)
        {
            delete mPtr;
            mPtr = ptr;
        }
    }

    T *release()
    {
        T *ptr = mPtr;
        mPtr = NULL;
        return ptr;
    }

    void swap(UniquePtr<T> &rhs)
    {
        std::swap(mPtr, rhs.mPtr);
    }

    FORCEINLINE T &operator *() const { return *mPtr; }
    FORCEINLINE T *operator ->() const { return mPtr; }
    FORCEINLINE T *get() const { return mPtr; }

    // This is a safe bool idiom
    typedef T *UniquePtr::*almost_bool;
    operator almost_bool() const
    {
        return mPtr == NULL ? NULL : &UniquePtr<T>::mPtr;
    }

private:
    T *mPtr;
};

template <typename T>
class UniquePtr<T[]> : private NonCopyable<UniquePtr<T[]> >
{
public:
    UniquePtr()
        : mPtr(NULL)
    {
    }

    explicit UniquePtr(T *ptr)
        : mPtr(ptr)
    {
    }

    ~UniquePtr()
    {
        delete []mPtr;
    }

    void reset(T *ptr = NULL)
    {
        if (ptr != mPtr)
        {
            delete mPtr;
            mPtr = ptr;
        }
    }

    T *release()
    {
        T *ptr = mPtr;
        mPtr = NULL;
        return ptr;
    }

    void swap(UniquePtr<T> &rhs)
    {
        std::swap(mPtr, rhs.mPtr);
    }

    FORCEINLINE T *get() const { return mPtr; }

    FORCEINLINE T &operator[](size_t idx) const
    {
        return mPtr[idx];
    }

    // This is a safe bool idiom
    typedef T *UniquePtr::*almost_bool;
    operator almost_bool() const
    {
        return mPtr == NULL ? NULL : &UniquePtr<T[]>::mPtr;
    }

private:
    T *mPtr;
};

// Skipping the relational operators for UniquePtr until a need arises.

class AtomicLong
{
public:
    AtomicLong(long val = 0) : mVal(val) {}

    FORCEINLINE long Increment()
    {
#if _WIN32
        return _InterlockedIncrement(&mVal);
#else
        return __sync_add_and_fetch(&mVal, 1L);
#endif
    }
    FORCEINLINE long Decrement()
    {
#if _WIN32
        return _InterlockedDecrement(&mVal);
#else
        return __sync_sub_and_fetch(&mVal, 1L);
#endif
    }

    /* Note, this function is inherently not thread safe.  Actions should be
     * based on retvals of AddRef/Release.   */
    long Get() const { return mVal; }

private:
    long mVal;
};

template <typename T>
class SharedPtr
{
public:
    SharedPtr() : mPtr(NULL), mRef(NULL) {}

    template <typename U>
    explicit SharedPtr(U *ptr) : mPtr(ptr)
    {
        mRef = ptr != NULL ? new AtomicLong(1) : NULL;
    }

    SharedPtr(const SharedPtr &rhs) : mPtr(rhs.mPtr), mRef(rhs.mRef)
    {
        increment();
    }

    template <typename U>
    SharedPtr(const SharedPtr<U> &rhs) : mPtr(rhs.mPtr), mRef(rhs.mRef)
    {
        increment();
    }

    SharedPtr &operator =(const SharedPtr &rhs)
    {
        if (mPtr == rhs.mPtr) // the self assignment test
        {
            // This had ought to be the same as well
            assert(mRef == rhs.mRef);
            return *this;
        }

        SharedPtr<T>(rhs).swap(*this);
        return *this;
    }

    template <typename U>
    SharedPtr &operator =(const SharedPtr<U> &rhs)
    {
        SharedPtr<T>(rhs).swap(*this);
        return *this;
    }

    ~SharedPtr()
    {
        decrement();
    }

    FORCEINLINE T &operator *() const { return *mPtr; }
    FORCEINLINE T *operator ->() const { return mPtr; }
    FORCEINLINE T *get() const { return mPtr; }

    // This is a safe bool idiom
    typedef T *SharedPtr::*almost_bool;
    operator almost_bool() const
    {
        return mPtr == NULL ? NULL : &SharedPtr<T>::mPtr;
    }

    void reset()
    {
        SharedPtr<T>().swap(*this);
    }

    void reset(T *ptr)
    {
        SharedPtr<T>(ptr).swap(*this);
    }

    void swap(SharedPtr<T> &rhs)
    {
        std::swap(mPtr, rhs.mPtr);
        std::swap(mRef, rhs.mRef);
    }

    long use_count() const { return mRef ? mRef->Get() : 0; }
    bool unique() const { return use_count() == 1; }

private:
    void increment()
    {
        if (mRef)
            mRef->Increment();
    }

    void decrement()
    {
        if (mRef)
        {
            uint32_t newCount = mRef->Decrement();
            if (newCount == 0)
                destroy();
        }
    }

    void destroy()
    {
        delete mPtr;
        mPtr = NULL;
        delete mRef;
        mRef = NULL;
    }

    T *mPtr;
    AtomicLong *mRef;

    template<typename U> friend class SharedPtr;
};

} // bwmgr

#endif /* BWMGR_SMARTPOINTER_H */
