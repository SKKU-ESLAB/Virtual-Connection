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

#include "../inc/NMPolicyManual.h"
#include "../../common/inc/DebugLog.h"

using namespace sc;

std::string NMPolicyManual::get_stats_string(void) {
  char stats_cstr[256];
  if (this->mWFDMode) {
    snprintf(stats_cstr, 256, "WFD Mode");
  } else {
    snprintf(stats_cstr, 256, "BT Mode");
  }
  std::string stats_string(stats_cstr);
  return stats_string;
}

void NMPolicyManual::on_custom_event(std::string &event_description) {
  if (event_description.compare("wfd") == 0) {
    this->mWFDMode = true;
  } else if (event_description.compare("bt") == 0) {
    this->mWFDMode = false;
  } else {
    LOG_WARN("Invalid custom event for Manual policy: %s",
             event_description.c_str());
  }
  return;
}

SwitchBehavior NMPolicyManual::decide(const Stats &stats, bool is_increasable,
                                      bool is_decreasable) {
  if (this->mWFDMode && is_increasable) {
    return kIncreaseAdapter;
  } else if (!this->mWFDMode && is_decreasable) {
    return kDecreaseAdapter;
  } else {
    return kNoBehavior;
  }
}