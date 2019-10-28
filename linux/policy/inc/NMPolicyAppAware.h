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

#ifndef __NM_POLICY_APP_AWARE_H__
#define __NM_POLICY_APP_AWARE_H__

#include "../../core/inc/NMPolicy.h"
#include "../inc/AppAwareTPT.h"

#include <string>
#include <sys/time.h>

namespace sc {
class NMPolicyAppAware : public NMPolicy {
public:
  NMPolicyAppAware(bool is_fg_bg_mixed_mode) {
    this->mPresentAppName.assign("");
    this->mLastMediaBandwidth = 0.0f;
    this->mRequestSpeedIncCount = 0;
    this->mRequestSpeedDecCount = 0;
    this->mSerialIncCount = 0;
    this->mSerialDecCount = 0;
    this->mEnergyRetain = 0.0f;
    this->mEnergySwitch = 0.0f;
    this->mIsRecentWfdOn = false;

    this->mIsFGBGMixedMode = is_fg_bg_mixed_mode;
    this->mBGAppEntry = NULL;
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
  BWTrafficEntry *get_most_proper_traffic(AppTrafficEntry *appEntry,
                                          float present_request_bandwidth);
  BWTrafficEntry *get_most_proper_traffic_time_only(AppTrafficEntry *appEntry);

  SwitchBehavior decide_internal_fg_only_mode(const Stats &stats,
                                              bool is_increasable,
                                              bool is_decreasable);
  SwitchBehavior decide_internal_fg_bg_mixed_mode(const Stats &stats,
                                                  bool is_increasable,
                                                  bool is_decreasable);
  void update_recent_switch_ts(void);
  void reset_recent_switch_ts(void);

  void check_background_app_entry(void);

private:
  std::string mPresentAppName;

  float mLastMediaBandwidth;
  int mRequestSpeedIncCount;
  int mRequestSpeedDecCount;

  int mSerialIncCount;
  int mSerialDecCount;

  float mEnergyRetain;
  float mEnergySwitch;
  bool mIsRecentWfdOn;

  bool mIsFGBGMixedMode;
  AppTrafficEntry *mBGAppEntry;

  struct timeval mRecentSwitchTS;

  AppAwareTPT mTrafficPredictionTable;
}; /* class NMPolicyAppAware */
} /* namespace sc */

#endif /* defined(__NM_POLICY_APP_AWARE_H__) */