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

#ifndef __NM_POLICY_history_based_H__
#define __NM_POLICY_history_based_H__

#include "../../core/inc/NMPolicy.h"
#include "../inc/TrafficHistoryTable.h"

#include <string>
#include <sys/time.h>

namespace sc {
class NMPolicyHistoryBased : public NMPolicy {
public:
  NMPolicyHistoryBased() {
    this->mPresentAppName.assign("");
    this->mPresentAppStartTS.tv_sec = 0;
    this->mPresentAppStartTS.tv_usec = 0;
    this->mLastMediaBandwidth = 0.0f;
    this->mRequestSpeedIncCount = 0;
    this->mRequestSpeedDecCount = 0;
    this->mSerialIncCount = 0;
    this->mSerialDecCount = 0;
    this->mEnergyRetain = 0.0f;
    this->mEnergySwitch = 0.0f;
    this->mIsRecentWfdOn = false;

    this->mBGAppEntry = NULL;
    this->mRecentSwitchTS.tv_sec = 0;
    this->mRecentSwitchTS.tv_usec = 0;
    this->mTrafficPredictionTable.initialize();
    this->check_background_app_entry();

    this->reset_recent_switch_ts();
  }
  virtual std::string get_stats_string(void);
  virtual std::string get_name(void) {
    std::string str("App-aware");
    return str;
  }
  virtual void on_custom_event(std::string &event_description);
  virtual SwitchBehavior decide(const Stats &stats, bool is_increasable,
                                bool is_decreasable);

private:
  SwitchBehavior decide_internal(const Stats &stats, bool is_increasable,
                                 bool is_decreasable);
  TrafficEntry *get_predicted_traffic(AppEntry *appEntry);

  void update_recent_switch_ts(void);
  void reset_recent_switch_ts(void);

  void check_background_app_entry(void);

private:
  std::string mPresentAppName;
  struct timeval mPresentAppStartTS;
  int mPresentEventType;

  float mLastMediaBandwidth;
  int mRequestSpeedIncCount;
  int mRequestSpeedDecCount;

  int mSerialIncCount;
  int mSerialDecCount;

  float mEnergyRetain;
  float mEnergySwitch;
  bool mIsRecentWfdOn;

  AppEntry *mBGAppEntry;

  struct timeval mRecentSwitchTS;

  TrafficHistoryTable mTrafficPredictionTable;
}; /* class NMPolicyHistoryBased */
} /* namespace sc */

#endif /* defined(__NM_POLICY_history_based_H__) */