/*******************************************************
 * Author : Utkarsh Khokhar
 * Date   : 1 Mar, 2012
 * Purpose: Skinny Log creation
 ******************************************************/
#include "openlog.h"
#include "bjnhelpers.h"
#include "talk/base/logging_libjingle.h"

#ifndef FB_WIN
#include "syslog.h"
#endif

using namespace boost::filesystem;

void new_log::getLogFilePath(std::string logFilePrefix, std::string &filePath, std::string logFileSuffix) {

#if 0
	// Clear log files with old name skinny.
    // TODO: Currently called pretty frequently. try not to call everytime when you get logfile path
    wpath oldAppDataPath = FB::utf8_to_wstring(BJN::getBjnOldLocalAppDataPath());
    if (boost::filesystem::exists(oldAppDataPath) && boost::filesystem::is_directory(oldAppDataPath))
    {
        try
        {
            boost::filesystem::remove_all(oldAppDataPath);
        }
        catch (...)
        {
            std::cerr << "Placeholder Error String";
        }
    }

#endif
    // Get log file path
    std::string appDataPath = BJN::getBjnLogfilePath();
    std::stringstream ss;
    time_t seconds = time(NULL);
    boost::interprocess::ipcdetail::OS_process_id_t procId = boost::interprocess::ipcdetail::get_current_process_id();
    ss << logFilePrefix << "." << seconds << "_" << procId << "." << logFileSuffix;

    if (!(boost::filesystem::exists(appDataPath) && boost::filesystem::is_directory(appDataPath)))
    {
        boost::filesystem::create_directories(appDataPath);
    }
#if 0
	else
    {
        // Erase old log files
        boost::filesystem::directory_iterator end_iter;

        typedef std::multimap<std::time_t, boost::filesystem::wpath> result_set_t;
        result_set_t result_set;
        result_set_t::iterator iter, iter_low;
        boost::filesystem::directory_iterator dir_iter(appDataPath);

        for(; dir_iter != end_iter; ++dir_iter)
        {
            if (boost::filesystem::is_regular_file(dir_iter->status()) )
            {
                result_set.insert(result_set_t::value_type(boost::filesystem::last_write_time(dir_iter->path()), *dir_iter));
            }
        }

        iter_low = result_set.lower_bound(seconds - LOG_CLEANUP_DURATION);

        for(iter = result_set.begin(); iter != iter_low; iter++)
        {
            try
            {
                boost::filesystem::remove(iter->second);
            }
            catch (...)
            {
                std::cerr << "Placeholder Error String";
            }
        }
    }
#endif
	  filePath = appDataPath + "/" + ss.str();
//    filePath = FB::wstring_to_utf8(logPath.wstring());
}

void new_log::openNewLog()
{
    if(!talk_base::LogMessage::getCallLogStatus())
    {
        std::string filePath;
        std::string logConfigOptions;
        
        getLogFilePath("Prism", filePath);
#ifdef FEATURE_ENABLE_LOG_ENCRYPTION
        logConfigOptions = "info file none debug thread encrypt";
#else
        logConfigOptions = "info file none debug thread";
//        filePath += ".dec"; 
#endif

        try {
            talk_base::LogMessage::ConfigureLogging(logConfigOptions.c_str(),filePath.c_str());
            talk_base::LogMessage::setCallLogStatus(true);
        } catch (...) {
#ifndef FB_WIN
            syslog(LOG_ERR,"Blue Jeans Error in creating log file");
#endif
        }
    }
}
