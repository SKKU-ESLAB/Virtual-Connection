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

#include "../inc/NMPolicyBtOnly.h"
#include "../../common/inc/DebugLog.h"

#include <sys/time.h>

using namespace sc;

std::string NMPolicyBtOnly::get_stats_string(void) {
  char stats_cstr[256];
  snprintf(stats_cstr, 256, "BT-only - %3d.%06d", this->mRecentAppLaunchTSSec,
           this->mRecentAppLaunchTSUsec);
  std::string stats_string(stats_cstr);
  return stats_string;
}

void NMPolicyBtOnly::on_custom_event(std::string &event_description) {
  // Update app launch timestamp
  gettimeofday(&this->mRecentAppLaunchTS, NULL);
  if (!this->mFirstAppLaunched) {
    this->mFirstAppLaunched = true;
    this->mFirstAppLaunchTS.tv_sec = this->mRecentAppLaunchTS.tv_sec;
    this->mFirstAppLaunchTS.tv_usec = this->mRecentAppLaunchTS.tv_usec;
  }

  long long recent_app_launch_ts =
      ((long long)this->mRecentAppLaunchTS.tv_sec * (1000 * 1000) +
       (long long)this->mRecentAppLaunchTS.tv_usec) -
      ((long long)this->mFirstAppLaunchTS.tv_sec * (1000 * 1000) +
       (long long)this->mFirstAppLaunchTS.tv_usec);
  this->mRecentAppLaunchTSSec = (int)(recent_app_launch_ts / (1000 * 1000));
  this->mRecentAppLaunchTSUsec = (int)(recent_app_launch_ts % (1000 * 1000));

  return;
}

SwitchBehavior NMPolicyBtOnly::decide(const Stats &stats, bool is_increasable,
                                      bool is_decreasable) {
  if (is_decreasable) {
    return kDecreaseAdapter;
  } else {
    return kNoBehavior;
  }
}