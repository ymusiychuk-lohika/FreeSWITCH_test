#ifndef LOCK_H
#define LOCK_H

#include <cassert>
#if _WIN32
#define NOMINMAX
#include <winsock2.h>
#else
#include <errno.h>
#include <pthread.h>
#include "noncopyable.h"
#endif

#include "thread_annotations.h"

// The inline keyword doesn't really mean it'll inline.   This is what works.
#if _WIN32
#define FORCEINLINE __forceinline
#else
#define FORCEINLINE __attribute__((always_inline))
#endif


// Define simple wrappers around a lock
#if _WIN32

// TODO define a recursive lock implementation, but try to not expose windows.h from the header (it's a slow compile)
class RecursiveLock
{
public:
    RecursiveLock()
    {
        InitializeCriticalSection(&mCritSec); 
    }

    ~RecursiveLock()
    {
        DeleteCriticalSection(&mCritSec);
    }

    FORCEINLINE void Lock() EXCLUSIVE_LOCK_FUNCTION()
    {
        EnterCriticalSection(&mCritSec);
    }

    FORCEINLINE void Unlock() UNLOCK_FUNCTION()
    {
        LeaveCriticalSection(&mCritSec);
    }

private:
    RecursiveLock(const RecursiveLock &) = delete;
    RecursiveLock &operator =(const RecursiveLock &) = delete;

    CRITICAL_SECTION mCritSec;
};

#else


class RecursiveLock
    : private bwmgr::NonCopyable<RecursiveLock> // not all compiler targets have C++11, so no deleted functions here
{
public:
    RecursiveLock()
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&mMutex, &attr);
        pthread_mutexattr_destroy(&attr);
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
    ~RecursiveLock()
    {
        int val = pthread_mutex_destroy(&mMutex);
        assert(val != EBUSY && "Mutex is locked at destruction");
    }
#pragma GCC diagnostic pop

    FORCEINLINE void Lock() EXCLUSIVE_LOCK_FUNCTION()
    {
        pthread_mutex_lock(&mMutex);
    }


    FORCEINLINE void Unlock() UNLOCK_FUNCTION()
    {
        pthread_mutex_unlock(&mMutex);
    }

private:
    RecursiveLock(const RecursiveLock &);
    RecursiveLock &operator =(const RecursiveLock &);

    pthread_mutex_t mMutex;
};

#endif

template <typename T>
class SCOPED_LOCKABLE ScopedLock
{
public:
    FORCEINLINE ScopedLock(T &lock) EXCLUSIVE_LOCK_FUNCTION(lock)
        : mLock(&lock)
    {
        lock.Lock();
    }
    FORCEINLINE ScopedLock(T *lock) EXCLUSIVE_LOCK_FUNCTION(lock)
        : mLock(lock)
    {
        lock->Lock();
    }

    FORCEINLINE ~ScopedLock() UNLOCK_FUNCTION()
    {
        mLock->Unlock();
    }

private:
    T *mLock;
};

typedef ScopedLock<RecursiveLock> ScopedRecursiveLock;

#endif

