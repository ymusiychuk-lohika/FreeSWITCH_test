#ifndef rtp_logger_h
#define rtp_logger_h
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <pjsua-lib/pjsua.h>
class RtpLogger {
public:
    RtpLogger();
    ~RtpLogger();
    void StartLogger(std::string logPath, pjsua_call_id callId);
    void StopLogger();
    void logStats();
    std::vector<std::string> getTransportAddess(int med_idx);
private:
    std::ofstream   mLogFile;
    pjsua_call_id   mCallId;
};
#endif
