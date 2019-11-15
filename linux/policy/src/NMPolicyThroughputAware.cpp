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

#include "../inc/NMPolicyThroughputAware.h"
#include "../../common/inc/DebugLog.h"

using namespace sc;

std::string NMPolicyThroughputAware::get_stats_string(void) {
  char stats_cstr[256];
  if (this->mWFDMode) {
    snprintf(stats_cstr, 256, "WFD Mode");
  } else {
    snprintf(stats_cstr, 256, "BT Mode");
  }
  std::string stats_string(stats_cstr);
  return stats_string;
}

void NMPolicyThroughputAware::on_custom_event(std::string &event_description) {
  return;
}

#define INCREASE_THRESHOLD_BUFFER_LENGTH_BYTES 10 * 1000 // 10KB
#define INCREASE_COUNT_THRESHOLD 5                      // 500ms
#define DECREASE_THRESHOLD_BANDWIDTH_BYTES_PER_SEC 62500 // 500kbps = 62.5KB/s
#define DECREASE_COUNT_THRESHOLD 50                       // 5sec

SwitchBehavior NMPolicyThroughputAware::decide(const Stats &stats,
                                               bool is_increasable,
                                               bool is_decreasable) {
  if (is_increasable) {
    if (stats.now_queue_data_size > INCREASE_THRESHOLD_BUFFER_LENGTH_BYTES) {
      this->mIncreaseCount++;
      if (this->mIncreaseCount >= INCREASE_COUNT_THRESHOLD) {
        this->mIncreaseCount = 0;
        this->mWFDMode = true;
        return kIncreaseAdapter;
      } else {
        return kNoBehavior;
      }
    } else {
      this->mIncreaseCount = 0;
      return kNoBehavior;
    }
  } else if (is_decreasable) {
    if (stats.now_total_bandwidth < DECREASE_THRESHOLD_BANDWIDTH_BYTES_PER_SEC) {
      this->mDecreaseCount++;
      if (this->mDecreaseCount >= DECREASE_COUNT_THRESHOLD) {
        this->mDecreaseCount = 0;
        this->mWFDMode = false;
        return kDecreaseAdapter;
      } else {
        return kNoBehavior;
      }
    } else {
      this->mDecreaseCount = 0;
      return kNoBehavior;
    }
  } else {
    return kNoBehavior;
  }
}