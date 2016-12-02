/*******************************************************
 * Author : Utkarsh Khokhar
 * Date   : 1 Mar, 2012
 * Purpose: Skinny Log creation
 ******************************************************/
#ifndef NEW_LOG_OPENLOG_H_
#define NEW_LOG_OPENLOG_H_

#include <iostream>
#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>

namespace new_log {
void getLogFilePath(std::string logFilePrefix, std::string &filePath, std::string logFileSuffix="log");
void openNewLog(void);
}

#endif // NEW_LOG_OPENLOG_H_