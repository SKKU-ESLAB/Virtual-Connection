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

#ifndef __NM_POLICY_H__
#define __NM_POLICY_H__

#include "../inc/Stats.h"

#include <string>

namespace sc {

typedef enum {
  kNoBehavior = 0,
  kIncreaseAdapter = 1,
  kDecreaseAdapter = 2,
  kPrepareIncreaseAdapter = 3,
  kPrepareDecreaseAdapter = 4,
  kCancelPrepareSwitchAdapter = 5
} SwitchBehavior;

class NMPolicy {
public:
  virtual std::string get_stats_string(void) = 0;
  void on_custom_event(const char *event_cstring) {
    std::string event_string(event_cstring);
    on_custom_event(event_string);
  }
  virtual void on_custom_event(std::string &event_string) = 0;
  virtual SwitchBehavior decide(const Stats &stats, bool is_increasable,
                                bool is_decreasable) = 0;
}; /* class NMPolicy */

} /* namespace sc */

#endif /* !defined(__NM_POLICY_H__) */