/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_REMOTE_BITRATE_ESTIMATOR_ABS_SEND_TIME_H_
#define WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_REMOTE_BITRATE_ESTIMATOR_ABS_SEND_TIME_H_

#include <list>
#include <map>
#include <vector>

#include "webrtc_common.h"
#include "lock.h"
#include "smartpointer.h"
#include "aimd_rate_control.h"
#include "remote_bitrate_estimator.h"
#include "inter_arrival.h"
#include "overuse_detector.h"
#include "overuse_estimator.h"
#include "rate_statistics.h"

namespace bwmgr {
class Logger;

namespace webrtc {

struct Probe {
  Probe(int64_t send_time_ms, int64_t recv_time_ms, size_t payload_size)
      : send_time_ms(send_time_ms),
        recv_time_ms(recv_time_ms),
        payload_size(payload_size) {}
  int64_t send_time_ms;
  int64_t recv_time_ms;
  size_t payload_size;
};

struct Cluster {
  Cluster()
      : send_mean_ms(0.0f),
        recv_mean_ms(0.0f),
        mean_size(0),
        count(0),
        num_above_min_delta(0) {}

  int GetSendBitrateBps() const {
    RTC_CHECK_GT(send_mean_ms, 0.0f);
    return mean_size * 8 * 1000 / send_mean_ms;
  }

  int GetRecvBitrateBps() const {
    RTC_CHECK_GT(recv_mean_ms, 0.0f);
    return mean_size * 8 * 1000 / recv_mean_ms;
  }

  float send_mean_ms;
  float recv_mean_ms;
  // TODO(holmer): Add some variance metric as well?
  size_t mean_size;
  int count;
  int num_above_min_delta;
};

class RemoteBitrateEstimatorAbsSendTime : public RemoteBitrateEstimator {
 public:
  RemoteBitrateEstimatorAbsSendTime(RemoteBitrateObserver* observer,
                                    bwmgr::Clock* clock, bwmgr::Logger *logger);
  virtual ~RemoteBitrateEstimatorAbsSendTime() {}

  void IncomingPacketFeedbackVector(
      const std::vector<PacketInfo>& packet_feedback_vector) override;

  void IncomingPacket(int64_t arrival_time_ms,
                      size_t payload_size,
                      const RTPHeader& header,
                      bool was_paced) override;
  // This class relies on Process() being called periodically (at least once
  // every other second) for streams to be timed out properly. Therefore it
  // shouldn't be detached from the ProcessThread except if it's about to be
  // deleted.
  int32_t Process() override;
  int64_t TimeUntilNextProcess() override;
  void OnRttUpdate(int64_t avg_rtt_ms, int64_t max_rtt_ms) override;
  void RemoveStream(unsigned int ssrc) override;
  bool LatestEstimate(std::vector<unsigned int>* ssrcs,
                      unsigned int* bitrate_bps) const override;
  bool GetStats(ReceiveBandwidthEstimatorStats* output) const override;
  void SetMinBitrate(int min_bitrate_bps) override;

 private:
  typedef std::map<unsigned int, int64_t> Ssrcs;

  static bool IsWithinClusterBounds(int send_delta_ms,
                                    const Cluster& cluster_aggregate);

  static void AddCluster(std::list<Cluster>* clusters, Cluster* cluster);

  int Id() const;

  void IncomingPacketInfo(int64_t arrival_time_ms,
                          uint32_t send_time_24bits,
                          size_t payload_size,
                          uint32_t ssrc,
                          bool was_paced);

  bool IsProbe(int64_t send_time_ms, int payload_size) const
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_.get());

  // Triggers a new estimate calculation.
  void UpdateEstimate(int64_t now_ms)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_.get());

  void UpdateStats(int propagation_delta_ms, int64_t now_ms)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_.get());

  void ComputeClusters(std::list<Cluster>* clusters) const;

  std::list<Cluster>::const_iterator FindBestProbe(
      const std::list<Cluster>& clusters) const
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_.get());

  void ProcessClusters(int64_t now_ms)
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_.get());

  bool IsBitrateImproving(int probe_bitrate_bps) const
      EXCLUSIVE_LOCKS_REQUIRED(crit_sect_.get());

  bwmgr::UniquePtr<RecursiveLock> crit_sect_;
  RemoteBitrateObserver* observer_ GUARDED_BY(crit_sect_.get());
  bwmgr::Clock* clock_;
  Ssrcs ssrcs_ GUARDED_BY(crit_sect_.get());
  bwmgr::UniquePtr<InterArrival> inter_arrival_ GUARDED_BY(crit_sect_.get());
  OveruseEstimator estimator_ GUARDED_BY(crit_sect_.get());
  OveruseDetector detector_ GUARDED_BY(crit_sect_.get());
  RateStatistics incoming_bitrate_ GUARDED_BY(crit_sect_.get());
  AimdRateControl remote_rate_ GUARDED_BY(crit_sect_.get());
  int64_t last_process_time_;
  std::vector<int> recent_propagation_delta_ms_ GUARDED_BY(crit_sect_.get());
  std::vector<int64_t> recent_update_time_ms_ GUARDED_BY(crit_sect_.get());
  int64_t process_interval_ms_ GUARDED_BY(crit_sect_.get());
  int total_propagation_delta_ms_ GUARDED_BY(crit_sect_.get());

  std::list<Probe> probes_;
  size_t total_probes_received_;
  int64_t first_packet_time_ms_;

  bwmgr::Logger *logger_;

  RTC_DISALLOW_IMPLICIT_CONSTRUCTORS(RemoteBitrateEstimatorAbsSendTime);
};

}  // namespace webrtc
}  // namespace bwmgr

#endif  // WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_REMOTE_BITRATE_ESTIMATOR_ABS_SEND_TIME_H_