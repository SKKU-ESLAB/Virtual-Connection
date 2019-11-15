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

#ifndef __NM_POLICY_WFD_ONLY_H__
#define __NM_POLICY_WFD_ONLY_H__

#include "../../core/inc/NMPolicy.h"

#include <sys/time.h>

namespace sc {
class NMPolicyWfdOnly : public NMPolicy {
public:
  NMPolicyWfdOnly(void) {
    this->mWFDMode = false;
    this->mIsAppStarted = false;
  }
  virtual std::string get_stats_string(void);
  virtual std::string get_name(void) {
    std::string str("WFD-only");
    return str;
  }
  virtual void on_custom_event(std::string &event_description);
  virtual SwitchBehavior decide(const Stats &stats, bool is_increasable,
                                bool is_decreasable);

private:
  bool mWFDMode;

  bool mIsAppStarted;
}; /* class NMPolicyWfdOnly */
} /* namespace sc */

#endif /* defined(__NM_POLICY_WFD_ONLY_H__) */