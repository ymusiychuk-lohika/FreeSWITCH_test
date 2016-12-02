//
//  fpslimiter.h
//
#ifndef __FPSLIMITER_H__
#define __FPSLIMITER_H__

#include <stdint.h>
#include <string>
#include <sstream>
#include <pj/log.h>
//
///////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Class FpsLimiter
//
// Purpose: Limit the FPS to the configured rate

#define DEFAULT_FRAMERATE          30
#define MAX_DURATION_SAMPLES       45
#define MAX_DURATION_SAMPLES_PRES  10
class FpsLimiter
{
public:
    FpsLimiter();
    FpsLimiter(double fpsLimit, uint32_t clockFreq);
    virtual ~FpsLimiter() {}

    // Set the frames per second limit, above which frames will be skipped
    void SetFpsLimit(double fps);

    // Set the clock frequency.  Timestamps will be in these units, in Hz.
    void SetClockFreq(uint32_t freq);

    // Call this function for each incoming frame.  Returns true if the frame
    // should be skipped, false otherwise.
    bool SkipThisFrame(uint64_t timestamp, uint64_t avgDuration);

    // Reset the internal tracking and start over.  Does NOT reset the FPS limit or the clock frequency.
    void Reset();

protected:
    void RecalcMaxFrameSkip(uint64_t curTimestamp);
    void RecalcMinDuration();

protected:
    double                   m_fpsLimit;                    // Skip frames whenever this limit is exceeded, subject to internal constraints to avoid excessive consecutive skipped frames
    uint32_t                 m_clockFreq;                   // Timestamps will be in these units, in Hz.
    uint8_t                  m_maxConsecutiveFramesToSkip;  // Skip no more than this number of frames in a row.  See note in SkipThisFrame()
    uint8_t                  m_skipCount;                   // Number of frames skipped in a row
    bool                     m_maxSkipFramesRecalculated;   // True after we've recalculated m_maxConsecutiveFramesToSkip the first time
    uint16_t                 m_totalFramesSinceRecalc;      // Count of the number of frames that have come in from the camera since we last recalcuated m_maxConsecutiveFramesToSkip
    uint64_t                 m_firstFrameTimestamp;         // Used to recalculate m_maxConsecutiveFramesToSkip
    const uint16_t           m_FRAMES_FOR_SKIP_RECALC;      // Number of frames to use in the recalculation of m_maxConsecutiveFramesToSkip
    const uint16_t           m_RECALC_SKIP_RATE_FREQUENCY;  // Recalculate m_maxConsecutiveFramesToSkip every m_RECALC_SKIP_RATE_FREQUENCY frames
    uint64_t                 m_minDuration;                 // Don't let average duration go lower than this.
    std::string              m_LogName;
};
#endif