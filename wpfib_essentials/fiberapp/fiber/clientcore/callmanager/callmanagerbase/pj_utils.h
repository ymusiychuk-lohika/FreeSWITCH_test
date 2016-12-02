/******************************************************************************
 * A file for miscellaneous pj_project helpers
 *
 * Copyright (C) 2015 Bluejeans Network, All Rights Reserved
 *****************************************************************************/


#ifndef FIBER_PJ_UTILS_H
#define FIBER_PJ_UTILS_H

#include <pj/os.h>
#include <string.h>

class PJThreadDescRegistration
{
public:
    PJThreadDescRegistration()
        : mThread(NULL)
    {
        memset(mThreadDesc, 0, sizeof(mThreadDesc));
    }

    void Register(const char *threadName);

private:
    pj_thread_t* mThread;
    pj_thread_desc mThreadDesc;
};

class PJDynamicThreadDescRegistration
{
public:
    PJDynamicThreadDescRegistration()
        : mThread(NULL)
        , mThreadDesc(NULL)
    {
    }

    void Register(const char *threadName);

private:
    pj_thread_t* mThread;
    // This is allocated by a pj_pool, which will free the memory at
    // destruction of that pool
    pj_thread_desc *mThreadDesc;
};


extern void CleanAllDynamicThreadRegistries();

#endif // FIBER_PJ_UTILS_H
