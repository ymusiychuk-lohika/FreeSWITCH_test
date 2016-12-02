// This is a collection of various small webrtc types from various files
// such as common_types.h, common_types.cc, constructor_magic.h,
// module_common_types.h and module.h (and a couple others too).
//
// Each file had a source code copyright notice of the form:
/*
 *  Copyright (c) 2014 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
// Albeit with varying years.

#ifndef WEBRTC_COMMON_H
#define WEBRTC_COMMON_H

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <stdint.h>


#if __GNUG__
#define GCC_VERSION (__GNUC__ * 10000 \
                     + __GNUC_MINOR__ * 100 \
                     + __GNUC_PATCHLEVEL__)
#if GCC_VERSION < 40700
#define override
#endif
#endif // __GNUG__


// This is intended to be common items to get things building.


#define RTC_DISALLOW_ASSIGN(TypeName) \
    private: \
  TypeName &operator =(const TypeName &)

#define RTC_DISALLOW_COPY_AND_ASSIGN(TypeName) \
    private: \
  TypeName(const TypeName&);          \
  RTC_DISALLOW_ASSIGN(TypeName)

#define RTC_DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
private: \
TypeName(); \
RTC_DISALLOW_ASSIGN(TypeName)

#define BWE_TEST_LOGGING_PLOT(arg1, arg2, arg3, arg4)

enum {kRtpCsrcSize = 15}; // RFC 3550 page 13

#define RTC_CHECK_GT(lhs, rhs) assert((lhs) > (rhs))
#define RTC_DCHECK(arg)  assert((arg))
#define RTC_DCHECK_LE(lhs, rhs)  assert((lhs) <= (rhs))

namespace bwmgr {
namespace webrtc {

class ProcessThread;

class Module {
 public:
  // Returns the number of milliseconds until the module wants a worker
  // thread to call Process.
  // This method is called on the same worker thread as Process will
  // be called on.
  // TODO(tommi): Almost all implementations of this function, need to know
  // the current tick count.  Consider passing it as an argument.  It could
  // also improve the accuracy of when the next callback occurs since the
  // thread that calls Process() will also have it's tick count reference
  // which might not match with what the implementations use.
  virtual int64_t TimeUntilNextProcess() = 0;

  // Process any pending tasks such as timeouts.
  // Called on a worker thread.
  virtual int32_t Process() = 0;

  // This method is called when the module is attached to a *running* process
  // thread or detached from one.  In the case of detaching, |process_thread|
  // will be nullptr.
  //
  // This method will be called in the following cases:
  //
  // * Non-null process_thread:
  //   * ProcessThread::RegisterModule() is called while the thread is running.
  //   * ProcessThread::Start() is called and RegisterModule has previously
  //     been called.  The thread will be started immediately after notifying
  //     all modules.
  //
  // * Null process_thread:
  //   * ProcessThread::DeRegisterModule() is called while the thread is
  //     running.
  //   * ProcessThread::Stop() was called and the thread has been stopped.
  //
  // NOTE: This method is not called from the worker thread itself, but from
  //       the thread that registers/deregisters the module or calls Start/Stop.
  virtual void ProcessThreadAttached(ProcessThread* process_thread) {}

 protected:
  virtual ~Module() {}
};

// Interface used by the CallStats class to distribute call statistics.
// Callbacks will be triggered as soon as the class has been registered to a
// CallStats object using RegisterStatsObserver.
class CallStatsObserver {
 public:
  virtual void OnRttUpdate(int64_t avg_rtt_ms, int64_t max_rtt_ms) = 0;

  virtual ~CallStatsObserver() {}
};

// Bandwidth over-use detector options.  These are used to drive
// experimentation with bandwidth estimation parameters.
// See modules/remote_bitrate_estimator/overuse_detector.h
struct OverUseDetectorOptions {
  OverUseDetectorOptions()
      : initial_slope(8.0/512.0),
        initial_offset(0),
        initial_e(),
        initial_process_noise(),
        initial_avg_noise(0.0),
        initial_var_noise(50) {
    initial_e[0][0] = 100;
    initial_e[1][1] = 1e-1;
    initial_e[0][1] = initial_e[1][0] = 0;
    initial_process_noise[0] = 1e-13;
    initial_process_noise[1] = 1e-2;
  }
  double initial_slope;
  double initial_offset;
  double initial_e[2][2];
  double initial_process_noise[2];
  double initial_avg_noise;
  double initial_var_noise;
};


struct RTPHeaderExtension {
  RTPHeaderExtension();

  bool hasTransmissionTimeOffset;
  int32_t transmissionTimeOffset;
  bool hasAbsoluteSendTime;
  uint32_t absoluteSendTime;
  bool hasTransportSequenceNumber;
  uint16_t transportSequenceNumber;

  // Audio Level includes both level in dBov and voiced/unvoiced bit. See:
  // https://datatracker.ietf.org/doc/draft-lennox-avt-rtp-audio-level-exthdr/
  bool hasAudioLevel;
  bool voiceActivity;
  uint8_t audioLevel;

  // For Coordination of Video Orientation. See
  // http://www.etsi.org/deliver/etsi_ts/126100_126199/126114/12.07.00_60/
  // ts_126114v120700p.pdf
  bool hasVideoRotation;
  uint8_t videoRotation;
};

inline RTPHeaderExtension::RTPHeaderExtension()
    : hasTransmissionTimeOffset(false),
      transmissionTimeOffset(0),
      hasAbsoluteSendTime(false),
      absoluteSendTime(0),
      hasTransportSequenceNumber(false),
      transportSequenceNumber(0),
      hasAudioLevel(false),
      voiceActivity(false),
      audioLevel(0),
      hasVideoRotation(false),
      videoRotation(0) {
}

struct RTPHeader {
  RTPHeader();

  bool markerBit;
  uint8_t payloadType;
  uint16_t sequenceNumber;
  uint32_t timestamp;
  uint32_t ssrc;
  uint8_t numCSRCs;
  uint32_t arrOfCSRCs[kRtpCsrcSize];
  size_t paddingLength;
  size_t headerLength;
  int payload_type_frequency;
  RTPHeaderExtension extension;
};

inline RTPHeader::RTPHeader()
    : markerBit(false),
      payloadType(0),
      sequenceNumber(0),
      timestamp(0),
      ssrc(0),
      numCSRCs(0),
      paddingLength(0),
      headerLength(0),
      payload_type_frequency(0),
      extension() {
  memset(&arrOfCSRCs, 0, sizeof(arrOfCSRCs));
}


struct PacketInfo {
  PacketInfo(int64_t arrival_time_ms,
             int64_t send_time_ms,
             uint16_t sequence_number,
             size_t payload_size,
             bool was_paced)
      : arrival_time_ms(arrival_time_ms),
        send_time_ms(send_time_ms),
        sequence_number(sequence_number),
        payload_size(payload_size),
        was_paced(was_paced) {}
  // Time corresponding to when the packet was received. Timestamped with the
  // receiver's clock.
  int64_t arrival_time_ms;
  // Time corresponding to when the packet was sent, timestamped with the
  // sender's clock.
  int64_t send_time_ms;
  // Packet identifier, incremented with 1 for every packet generated by the
  // sender.
  uint16_t sequence_number;
  // Size of the packet excluding RTP headers.
  size_t payload_size;
  // True if the packet was paced out by the pacer.
  bool was_paced;
};

inline bool IsNewerSequenceNumber(uint16_t sequence_number,
                                  uint16_t prev_sequence_number) {
  // Distinguish between elements that are exactly 0x8000 apart.
  // If s1>s2 and |s1-s2| = 0x8000: IsNewer(s1,s2)=true, IsNewer(s2,s1)=false
  // rather than having IsNewer(s1,s2) = IsNewer(s2,s1) = false.
  if (static_cast<uint16_t>(sequence_number - prev_sequence_number) == 0x8000) {
    return sequence_number > prev_sequence_number;
  }
  return sequence_number != prev_sequence_number &&
         static_cast<uint16_t>(sequence_number - prev_sequence_number) < 0x8000;
}

inline bool IsNewerTimestamp(uint32_t timestamp, uint32_t prev_timestamp) {
  // Distinguish between elements that are exactly 0x80000000 apart.
  // If t1>t2 and |t1-t2| = 0x80000000: IsNewer(t1,t2)=true,
  // IsNewer(t2,t1)=false
  // rather than having IsNewer(t1,t2) = IsNewer(t2,t1) = false.
  if (static_cast<uint32_t>(timestamp - prev_timestamp) == 0x80000000) {
    return timestamp > prev_timestamp;
  }
  return timestamp != prev_timestamp &&
         static_cast<uint32_t>(timestamp - prev_timestamp) < 0x80000000;
}

inline uint16_t LatestSequenceNumber(uint16_t sequence_number1,
                                     uint16_t sequence_number2) {
  return IsNewerSequenceNumber(sequence_number1, sequence_number2)
             ? sequence_number1
             : sequence_number2;
}

inline uint32_t LatestTimestamp(uint32_t timestamp1, uint32_t timestamp2) {
  return IsNewerTimestamp(timestamp1, timestamp2) ? timestamp1 : timestamp2;
}

// Utility class to unwrap a sequence number to a larger type, for easier
// handling large ranges. Note that sequence numbers will never be unwrapped
// to a negative value.
class SequenceNumberUnwrapper {
 public:
  SequenceNumberUnwrapper() : last_seq_(-1) {}

  // Get the unwrapped sequence, but don't update the internal state.
  int64_t UnwrapWithoutUpdate(uint16_t sequence_number) {
    if (last_seq_ == -1)
      return sequence_number;

    uint16_t cropped_last = static_cast<uint16_t>(last_seq_);
    int64_t delta = sequence_number - cropped_last;
    if (IsNewerSequenceNumber(sequence_number, cropped_last)) {
      if (delta < 0)
        delta += (1 << 16);  // Wrap forwards.
    } else if (delta > 0 && (last_seq_ + delta - (1 << 16)) >= 0) {
      // If sequence_number is older but delta is positive, this is a backwards
      // wrap-around. However, don't wrap backwards past 0 (unwrapped).
      delta -= (1 << 16);
    }

    return last_seq_ + delta;
  }

  // Only update the internal state to the specified last (unwrapped) sequence.
  void UpdateLast(int64_t last_sequence) { last_seq_ = last_sequence; }

  // Unwrap the sequence number and update the internal state.
  int64_t Unwrap(uint16_t sequence_number) {
    int64_t unwrapped = UnwrapWithoutUpdate(sequence_number);
    UpdateLast(unwrapped);
    return unwrapped;
  }

 private:
  int64_t last_seq_;
};

} // namespace webrtc
} // namespace bwmgr

#endif

