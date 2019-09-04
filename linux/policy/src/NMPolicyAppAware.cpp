/* Copyright 2017-2018 All Rights Reserved.
 *  Gyeonghwan Hong (redcarrottt@gmail.com)
 *
 * [Contact]
 *  Gyeonghwan Hong (redcarrottt@gmail.com)
 *
 * Licensed under the Apache License, Version 2.0(the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "../inc/NMPolicyAppAware.h"
#include "../inc/EnergyPredictor.h"

#include "../inc/ConfigAppAwarePolicy.h"

#include "../../common/inc/DebugLog.h"

#include <cmath>
#include <cstdlib>
#include <limits>

// #define PRINT_EVERY_ENERGY_COMPARISON 1

using namespace sc;

std::string NMPolicyAppAware::get_stats_string(void) {
  char stats_cstr[256];
  snprintf(stats_cstr, 256, "\"%s\": %d vs. %d - (%d/%d)",
           this->mPresentAppName.c_str(), (int)this->mEnergyRetain,
           (int)this->mEnergySwitch, this->mRequestSpeedIncCount,
           this->mRequestSpeedDecCount);

  std::string stats_string(stats_cstr);
  return stats_string;
}

void NMPolicyAppAware::on_custom_event(std::string &event_description) {
  // change current app name (event_description = "app_name")
  this->mPresentAppName.assign(event_description);

  return;
}

SwitchBehavior NMPolicyAppAware::decide(const Stats &stats, bool is_increasable,
                                        bool is_decreasable) {
  // Step 1. detect increasing/decreasing traffic in series
  float request_speed =
      stats.ema_queue_arrival_speed + (float)stats.now_queue_data_size; // B/s
  if (request_speed < this->mLastRequestSpeed ||
      request_speed < IDLE_THRESHOLD) {
    this->mRequestSpeedDecCount++;
    this->mRequestSpeedIncCount = 0;
  } else if (request_speed > this->mLastRequestSpeed) {
    this->mRequestSpeedIncCount++;
    this->mRequestSpeedDecCount = 0;
  }
  this->mLastRequestSpeed = request_speed;

  if (this->mRequestSpeedIncCount < INC_COUNT_THRESHOLD &&
      this->mRequestSpeedDecCount < DEC_COUNT_THRESHOLD) {
    return kNoBehavior;
  }

  // Step 2. get traffic prediction entry for the app
  if (this->mPresentAppName.empty()) {
    return kNoBehavior;
  }
  AppTrafficEntry *appEntry =
      this->mTrafficPredictionTable.getItem(this->mPresentAppName);
  if (appEntry == NULL) {
    LOG_WARN("Cannot find traffic prediction: %s",
             this->mPresentAppName.c_str());
    return kNoBehavior;
  }

  // Step 3. select most proper traffic history (nearest bandwidth)
  BWTrafficEntry *most_proper_traffic = NULL;
  float min_diff = std::numeric_limits<float>::max();
  std::vector<BWTrafficEntry> &bwTrafficList = appEntry->getList();
  for (std::vector<BWTrafficEntry>::iterator it = bwTrafficList.begin();
       it != bwTrafficList.end(); it++) {
    BWTrafficEntry &bw_traffic = *it;
    float bandwidth = bw_traffic.getBandwidth(); // B/s

    if (this->mRequestSpeedDecCount >= DEC_COUNT_THRESHOLD &&
        bw_traffic.isIncrease()) {
      continue;
    } else if (this->mRequestSpeedIncCount >= INC_COUNT_THRESHOLD &&
               !bw_traffic.isIncrease()) {
      continue;
    }

    float newDiff = std::abs(bandwidth - request_speed);
    if (newDiff < min_diff) {
      min_diff = newDiff;
      most_proper_traffic = &(bw_traffic);
    }
  }
  if (most_proper_traffic == NULL) {
    LOG_WARN("No most proper traffic");
    return kNoBehavior;
  }

  // Step 4. predict energy and final decision
  int seg_q_length = stats.now_queue_data_size; // Bytes
  std::vector<int> &traffic_seq = most_proper_traffic->getTrafficSequence();
  if (is_increasable) {
    float energy_bt = EnergyPredictor::predictEnergy(seg_q_length, traffic_seq,
                                                     SCENARIO_BT); // mJ
    float energy_bt_to_wfd = EnergyPredictor::predictEnergy(
        seg_q_length, traffic_seq, SCENARIO_BT_TO_WFD); // mJ
    this->mEnergyRetain = energy_bt;
    this->mEnergySwitch = energy_bt_to_wfd;
#if PRINT_EVERY_ENERGY_COMPARISON == 1
    LOG_IMP("(%f > %f) ? BT : to-WFD", this->mEnergyRetain,
            this->mEnergySwitch);
#endif
    if (energy_bt < 0.0f || energy_bt_to_wfd < 0.0f) {
      return kNoBehavior;
    } else if (energy_bt > energy_bt_to_wfd) {
      return kIncreaseAdapter;
    } else {
      return kNoBehavior;
    }
  } else if (is_decreasable) {
    float energy_wfd = EnergyPredictor::predictEnergy(seg_q_length, traffic_seq,
                                                      SCENARIO_WFD); // mJ
    float energy_wfd_to_bt = EnergyPredictor::predictEnergy(
        seg_q_length, traffic_seq, SCENARIO_WFD_TO_BT); // mJ
    this->mEnergyRetain = energy_wfd;
    this->mEnergySwitch = energy_wfd_to_bt;
#if PRINT_EVERY_ENERGY_COMPARISON == 1
    LOG_IMP("(%f > %f) ? WFD : to-BT", this->mEnergyRetain,
            this->mEnergySwitch);
#endif
    if (energy_wfd < 0.0f || energy_wfd_to_bt < 0.0f) {
      return kNoBehavior;
    } else if (energy_wfd > energy_wfd_to_bt) {
      return kDecreaseAdapter;
    } else {
      return kNoBehavior;
    }
  } else {
    // If there is only one network adapter
    return kNoBehavior;
  }
}