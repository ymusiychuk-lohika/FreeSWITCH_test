/******************************************************************************
 * Common Logging header file
 *
 * Copyright (C) Bluejeans Network, Inc  All Rights Reserved
 *
 * Author: Brian Ashford
 * Date:   Oct 29, 2013
 *****************************************************************************/

#ifndef LOGGER_H_
#define LOGGER_H_

#include <stdarg.h>
#include "bwmgr_types.h"

#if defined(__clang__) || (__GNUC__)
#define PRINTFATTR(FMT_ARG, ELLIPSES_ARG) __attribute__((format(printf, FMT_ARG, ELLIPSES_ARG)))
#else
#define PRINTFATTR(FMT_ARG, ELLIPSES_ARG)
#endif

namespace bwmgr {

enum    LoggerLevels {
    LOGGER_ERROR    = 0,
    LOGGER_WARNING  = 1,
    LOGGER_INFO     = 2,
    LOGGER_DEBUG    = 3,
    LOGGER_NUM_LEVELS = 4
};

class Logger
{
public:
    Logger(void);
    Logger(void *userData, UserDefinedLogFunc *logFunc)
        : mLogFunc(logFunc)
        , mLogData(userData)
    {
    }

    virtual void setLoggingInfo(
            void* data,
            UserDefinedLogFunc* func);

    void loggerLog(LoggerLevels logLevel, const char *format, ...) PRINTFATTR(3, 4);

    void error(const char *format, ...) PRINTFATTR(2, 3);
    void warn(const char *format, ...) PRINTFATTR(2, 3);
    void info(const char *format, ...) PRINTFATTR(2, 3);
    void debug(const char *format, ...) PRINTFATTR(2, 3);

protected:
    void log(LoggerLevels logLevel, const char *format, va_list args) PRINTFATTR(3, 0);

    UserDefinedLogFunc*    mLogFunc;
    void*               mLogData;
};

} // namespace bwmgr

#endif /* LOGGER_H_ */
