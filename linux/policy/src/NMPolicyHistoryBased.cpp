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

#include "../inc/NMPolicyHistoryBased.h"
#include "../inc/EnergyPredictor.h"

#include "../inc/ConfigHistoryBasedPolicy.h"

#include "../../common/inc/DebugLog.h"

#include <assert.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <sstream>

#define PRINT_EVERY_ENERGY_COMPARISON 0

using namespace sc;

std::string NMPolicyHistoryBased::get_stats_string(void) {
  char recent_adapter_name[50];
  if (mIsRecentWfdOn) {
    snprintf(recent_adapter_name, 50, "WFD");
  } else {
    snprintf(recent_adapter_name, 50, "BT");
  }
  char stats_cstr[256];
  snprintf(stats_cstr, 256, "\"%s(%d)/%s(%d)\": %d vs. %d - (%d/%d) - %s",
           this->mPresentBGAppName.c_str(), this->mPresentBGEventType,
           this->mPresentFGAppName.c_str(), this->mPresentFGEventType,
           (int)this->mEnergyRetain, (int)this->mEnergySwitch,
           this->mRequestSpeedIncCount, this->mRequestSpeedDecCount,
           recent_adapter_name);

  std::string stats_string(stats_cstr);
  return stats_string;
}

void NMPolicyHistoryBased::on_custom_event(std::string &event_description) {
  // change current app name (event_description = "app_name event_type")
  std::istringstream sstream(event_description);
  std::string token;
  int i = 0;
  bool is_background_app = false;
  while (std::getline(sstream, token, ' ')) {
    if (i == 0) {
      if (token.find("BG") != std::string::npos) {
        is_background_app = true;
      }
      if (is_background_app) {
        if (this->mPresentBGAppName.compare(token) != 0) {
          gettimeofday(&this->mPresentBGAppStartTS, NULL);
        }
        this->mPresentBGAppName.assign(token);
      } else {
        if (this->mPresentFGAppName.compare(token) != 0) {
          gettimeofday(&this->mPresentFGAppStartTS, NULL);
        }
        this->mPresentFGAppName.assign(token);
      }
    } else if (i == 1) {
      if (is_background_app) {
        this->mPresentBGEventType = std::stoi(token);
      } else {
        this->mPresentFGEventType = std::stoi(token);
      }
    }
    i++;
  }

  // reset recent switch timestamp: start to decide switch
  this->reset_recent_switch_ts();
  return;
}

#define SERIAL_INC_COUNT_THRESHOLD 3
#define SERIAL_DEC_COUNT_THRESHOLD 3
SwitchBehavior NMPolicyHistoryBased::decide(const Stats &stats,
                                            bool is_increasable,
                                            bool is_decreasable) {
  // No decision after switch for prediction window
  if (this->mRecentSwitchTS.tv_sec != 0 && this->mRecentSwitchTS.tv_usec != 0) {
    struct timeval present_tv;
    gettimeofday(&present_tv, NULL);

    long long from_switch_elapsed_time_us =
        ((long long)present_tv.tv_sec * (1000 * 1000) +
         (long long)present_tv.tv_usec) -
        ((long long)this->mRecentSwitchTS.tv_sec * (1000 * 1000) +
         (long long)this->mRecentSwitchTS.tv_usec);

    if (from_switch_elapsed_time_us < PREDICTION_WINDOW_SEC * 1000 * 1000 / 2) {
      return kNoBehavior;
    }
  }

  // Filtering intermittent decision
  SwitchBehavior behavior =
      this->decide_internal(stats, is_increasable, is_decreasable);
  if (behavior == SwitchBehavior::kIncreaseAdapter) {
    if (this->mSerialIncCount < SERIAL_INC_COUNT_THRESHOLD) {
      behavior = kNoBehavior;
    } else {
      this->mIsRecentWfdOn = true;
    }
    this->mSerialIncCount++;
    this->mSerialDecCount = 0;
  } else if (behavior == SwitchBehavior::kDecreaseAdapter) {
    if (this->mSerialDecCount < SERIAL_DEC_COUNT_THRESHOLD) {
      behavior = kNoBehavior;
    } else {
      this->mIsRecentWfdOn = false;
    }
    this->mSerialDecCount++;
    this->mSerialIncCount = 0;
  } else {
    this->mSerialIncCount = 0;
    this->mSerialDecCount = 0;
  }

  if (behavior == SwitchBehavior::kIncreaseAdapter ||
      behavior == SwitchBehavior::kDecreaseAdapter) {
    this->update_recent_switch_ts();
  }
  return behavior;
}

void NMPolicyHistoryBased::update_recent_switch_ts(void) {
  gettimeofday(&this->mRecentSwitchTS, NULL);
}

void NMPolicyHistoryBased::reset_recent_switch_ts(void) {
  this->mRecentSwitchTS.tv_sec = 0;
  this->mRecentSwitchTS.tv_usec = 0;
}

SwitchBehavior NMPolicyHistoryBased::decide_internal(const Stats &stats,
                                                     bool is_increasable,
                                                     bool is_decreasable) {

  float present_media_bandwidth =
      stats.ema_queue_arrival_speed + (float)stats.now_queue_data_size; // B/s

  // Step 1. Filtering

  // Step 1-a. Filtering increase when idle / decrease when busy
  // if (present_media_bandwidth > IDLE_THRESHOLD && is_decreasable) {
  //   // Filtering decrease when busy
  //   return kNoBehavior;
  // } else if (present_media_bandwidth < IDLE_THRESHOLD && is_increasable) {
  //   // Filtering increase when idle
  //   return kNoBehavior;
  // }

  // Step 1-b. detect increasing/decreasing traffic in series
  if (present_media_bandwidth > HIGH_THRESHOLD) {
    this->mRequestSpeedIncCount++;
    this->mRequestSpeedDecCount = 0;
  } else if (present_media_bandwidth < LOW_THRESHOLD) {
    this->mRequestSpeedDecCount++;
    this->mRequestSpeedIncCount = 0;
  } else if (present_media_bandwidth > this->mLastMediaBandwidth) {
    this->mRequestSpeedIncCount++;
    this->mRequestSpeedDecCount = 0;
  } else if (present_media_bandwidth < this->mLastMediaBandwidth) {
    this->mRequestSpeedDecCount++;
    this->mRequestSpeedIncCount = 0;
  }
  this->mLastMediaBandwidth = present_media_bandwidth;

  if (this->mRequestSpeedIncCount < INC_COUNT_THRESHOLD &&
      this->mRequestSpeedDecCount < DEC_COUNT_THRESHOLD) {
    return kNoBehavior;
  }

  if ((is_increasable && (this->mRequestSpeedDecCount > DEC_COUNT_THRESHOLD)) ||
      (is_decreasable && (this->mRequestSpeedIncCount > INC_COUNT_THRESHOLD))) {
    return kNoBehavior;
  }

  // Step 2. get traffic prediction entry for the FG/BG apps
  AppEntry *fgAppEntry = NULL;
  if (!this->mPresentFGAppName.empty()) {
    fgAppEntry = this->mTrafficPredictionTable.getItem(this->mPresentFGAppName);
    if (fgAppEntry == NULL) {
      LOG_WARN("Cannot find traffic prediction: %s",
               this->mPresentFGAppName.c_str());
      return kNoBehavior;
    }
  }
  AppEntry *bgAppEntry = this->mBGAppEntry;
  if (fgAppEntry == NULL && bgAppEntry == NULL) {
    LOG_WARN("No apps: no traffic prediction");
    return kNoBehavior;
  }

  // Step 3-a. predict traffic history for the foreground app
  int predicted_traffic_length = 0;
  TrafficEntry *fg_traffic = NULL;
  if (fgAppEntry != NULL) {
    fg_traffic = this->get_predicted_traffic(fgAppEntry, false);
    if (fg_traffic == NULL) {
      LOG_WARN("No predicted traffic for FG app");
      return kNoBehavior;
    }
    predicted_traffic_length = fg_traffic->getTrafficSequence().size();
  }

  // Step 3-b. predict traffic history for the background app
  TrafficEntry *bg_traffic = NULL;
  if (bgAppEntry != NULL) {
    bg_traffic = this->get_predicted_traffic(bgAppEntry, true);
    if (bg_traffic == NULL) {
      LOG_WARN("No predicted traffic for BG app");
      return kNoBehavior;
    }
    predicted_traffic_length = bg_traffic->getTrafficSequence().size();
  }

  // Step 3-C. merge traffic histories
  std::vector<int> traffic_seq;
  for (int i = 0; i < predicted_traffic_length; i++) {
    int traffic = 0;
    traffic += (fg_traffic) ? fg_traffic->getTrafficSequence()[i] : 0;
    traffic += (bg_traffic) ? bg_traffic->getTrafficSequence()[i] : 0;
    traffic_seq.push_back(traffic);
  }

  // Step 4. predict energy and final decision
  int seg_q_length = stats.now_queue_data_size; // Bytes
  if (is_increasable) {
    float energy_bt = EnergyPredictor::predictEnergy(seg_q_length, traffic_seq,
                                                     SCENARIO_BT); // mJ
    float energy_bt_to_wfd = EnergyPredictor::predictEnergy(
        seg_q_length, traffic_seq, SCENARIO_BT_TO_WFD); // mJ
    this->mEnergyRetain = energy_bt;
    this->mEnergySwitch = energy_bt_to_wfd;
#if PRINT_EVERY_ENERGY_COMPARISON == 1
    LOG_IMP("  => (%f > %f) ? BT : to-WFD", this->mEnergyRetain,
            this->mEnergySwitch);
#endif
    if (energy_bt < 0.0f || energy_bt_to_wfd < 0.0f) {
      return kNoBehavior;
    } else if (energy_bt > energy_bt_to_wfd ||
               energy_bt == std::numeric_limits<float>::max()) {
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
    LOG_IMP("  => (%f > %f) ? WFD : to-BT", this->mEnergyRetain,
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

TrafficEntry *
NMPolicyHistoryBased::get_predicted_traffic(AppEntry *appEntry,
                                            bool isBackgroundApp) {
  struct timeval present_time;
  struct timeval present_app_start_time;
  int event_type;
  if (isBackgroundApp) {
    present_app_start_time = this->mPresentBGAppStartTS;
    event_type = this->mPresentBGEventType;
  } else {
    present_app_start_time = this->mPresentFGAppStartTS;
    event_type = this->mPresentFGEventType;
  }

  gettimeofday(&present_time, NULL);
  long long presentTimeUS =
      (long long)present_time.tv_sec * 1000 * 1000 + present_time.tv_usec;
  long long appStartTimeUs =
      (long long)present_app_start_time.tv_sec * 1000 * 1000 +
      present_app_start_time.tv_usec;

  // Get event type entry
  EventTypeEntry *eventTypeEntry = appEntry->getItem(event_type);
  if (eventTypeEntry == NULL) {
    LOG_WARN("Cannot find event_type_entry: %d", event_type);
    return NULL;
  }

  // Get timestamp relative to app start event
  float present_time_sec = (float)(presentTimeUS - appStartTimeUs) / 1000000.0f;
  TrafficEntry *trafficEntry = eventTypeEntry->findItem(present_time_sec);
  // LOG_ERR("Predicted traffic at : %4.1f (%d)", trafficEntry->getTimeSec(),
  // isBackgroundApp);

  // std::vector<int>& traffic = trafficEntry->getTrafficSequence();
  // for(int i=0; i<traffic.size(); i++) {
  //   printf("%d ", traffic[i]);
  // }
  // printf("\n");
  return trafficEntry;
}

void NMPolicyHistoryBased::check_background_app_entry(void) {
  std::map<std::string, AppEntry> &appEntries =
      this->mTrafficPredictionTable.getMap();
  for (std::map<std::string, AppEntry>::iterator it = appEntries.begin();
       it != appEntries.end(); it++) {
    std::string appName = it->first;
    AppEntry *appEntry = &(it->second);
    if (appName.find("BG") != std::string::npos) {
      // Prevent more than one background apps
      assert(this->mBGAppEntry == NULL);

      this->mBGAppEntry = appEntry;
    }
  }
}