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

#include "../inc/NMPolicyAppManual.h"
#include "../../common/inc/DebugLog.h"

#include <sys/time.h>

using namespace sc;

std::string NMPolicyAppManual::get_stats_string(void) {
  char stats_cstr[256];
  if (this->mWFDMode) {
    snprintf(stats_cstr, 256, "WFD Mode - %3d.%06d (%s)",
             this->mRecentAppLaunchTSSec, this->mRecentAppLaunchTSUsec,
             this->mPresentAppName.c_str());
  } else {
    snprintf(stats_cstr, 256, "BT Mode - %3d.%06d (%s)",
             this->mRecentAppLaunchTSSec, this->mRecentAppLaunchTSUsec,
             this->mPresentAppName.c_str());
  }
  std::string stats_string(stats_cstr);
  return stats_string;
}

void NMPolicyAppManual::on_custom_event(std::string &event_description) {
  this->mPresentAppName.assign(event_description);
  if (event_description.compare("1-navermap") == 0 ||
      event_description.compare("2-news") == 0 ||
      event_description.compare("4-webbrowsing") == 0) {
    this->mWFDMode = true;
  } else if (event_description.compare("3-remotecamera") == 0 ||
             event_description.compare("5-svoice") == 0) {
    this->mWFDMode = false;
  } else {
    LOG_WARN("Invalid custom event for Manual policy: %s",
             event_description.c_str());
  }

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

SwitchBehavior NMPolicyAppManual::decide(const Stats &stats,
                                         bool is_increasable,
                                         bool is_decreasable) {
  if (this->mWFDMode && is_increasable) {
    struct timeval present_tv;
    gettimeofday(&present_tv, NULL);
    long long present_ts =
        ((long long)present_tv.tv_sec * (1000 * 1000) +
         (long long)present_tv.tv_usec) -
        ((long long)this->mFirstAppLaunchTS.tv_sec * (1000 * 1000) +
         (long long)this->mFirstAppLaunchTS.tv_usec);
    int present_ts_sec = (int)(present_ts / (1000 * 1000));
    int present_ts_usec = (int)(present_ts % (1000 * 1000));
    LOG_IMP("Decision: WFD on %3d.%06d", present_ts_sec, present_ts_usec);

    return kIncreaseAdapter;
  } else if (!this->mWFDMode && is_decreasable) {
    struct timeval present_tv;
    gettimeofday(&present_tv, NULL);
    long long present_ts =
        ((long long)present_tv.tv_sec * (1000 * 1000) +
         (long long)present_tv.tv_usec) -
        ((long long)this->mFirstAppLaunchTS.tv_sec * (1000 * 1000) +
         (long long)this->mFirstAppLaunchTS.tv_usec);
    int present_ts_sec = (int)(present_ts / (1000 * 1000));
    int present_ts_usec = (int)(present_ts % (1000 * 1000));
    LOG_IMP("Decision: BT on %3d.%06d", present_ts_sec, present_ts_usec);

    return kDecreaseAdapter;
  } else {
    return kNoBehavior;
  }
}