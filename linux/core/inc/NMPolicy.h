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

#include "../inc/EventLogging.h"
#include "../inc/Stats.h"

#include "../../common/inc/DebugLog.h"

#include <string>
#include <sys/time.h>

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
  NMPolicy(void) { this->mIsFirstCustomEvent = false; }

  virtual std::string get_stats_string(void) = 0;

  void send_custom_event(const char *event_cstring) {
    std::string event_string(event_cstring);
    this->send_custom_event(event_string);
  }

  void send_custom_event(std::string &event_string) {
    this->on_custom_event(event_string);
    this->print_custom_event(event_string);
  }

  SwitchBehavior decide_switch(const Stats &stats, bool is_increasable,
                               bool is_decreasable) {
    SwitchBehavior behavior = decide(stats, is_increasable, is_decreasable);
    print_switch_decision(behavior);
    return behavior;
  }

protected:
  virtual void on_custom_event(std::string &event_string) = 0;
  virtual SwitchBehavior decide(const Stats &stats, bool is_increasable,
                                bool is_decreasable) = 0;

private:
  void print_custom_event(std::string &event_string) {
    // Update app launch timestamp
    struct timeval recent_custom_event_ts;
    gettimeofday(&recent_custom_event_ts, NULL);
    if (!this->mIsFirstCustomEvent) {
      this->mIsFirstCustomEvent = true;
      this->mFirstCustomEventTS.tv_sec = recent_custom_event_ts.tv_sec;
      this->mFirstCustomEventTS.tv_usec = recent_custom_event_ts.tv_usec;
    }

    long long recent_app_launch_ts =
        ((long long)recent_custom_event_ts.tv_sec * (1000 * 1000) +
         (long long)recent_custom_event_ts.tv_usec) -
        ((long long)this->mFirstCustomEventTS.tv_sec * (1000 * 1000) +
         (long long)this->mFirstCustomEventTS.tv_usec);
    int recent_custom_event_ts_sec =
        (int)(recent_app_launch_ts / (1000 * 1000));
    int recent_custom_event_ts_usec =
        (int)(recent_app_launch_ts % (1000 * 1000));

    char eventstr[100] = {
        '\0',
    };
    snprintf(eventstr, 100, "%3d.%06d [Event] %s", recent_custom_event_ts_sec,
             recent_custom_event_ts_usec, event_string.c_str());
    EventLogging::print_event(eventstr);
    printf("%s\n", eventstr);
  }

  void print_switch_decision(SwitchBehavior behavior) {
    if (behavior == kIncreaseAdapter || behavior == kDecreaseAdapter) {
      struct timeval present_tv;
      gettimeofday(&present_tv, NULL);
      long long present_ts =
          ((long long)present_tv.tv_sec * (1000 * 1000) +
           (long long)present_tv.tv_usec) -
          ((long long)this->mFirstCustomEventTS.tv_sec * (1000 * 1000) +
           (long long)this->mFirstCustomEventTS.tv_usec);
      int present_ts_sec = (int)(present_ts / (1000 * 1000));
      int present_ts_usec = (int)(present_ts % (1000 * 1000));
      char eventstr[100] = {
          '\0',
      };
      if (behavior == kIncreaseAdapter) {
        snprintf(eventstr, 100, "%3d.%06d [Switch] WFD on", present_ts_sec,
                 present_ts_usec);
      } else if (behavior == kDecreaseAdapter) {
        snprintf(eventstr, 100, "%3d.%06d [Switch] BT on", present_ts_sec,
                 present_ts_usec);
      }
      EventLogging::print_event(eventstr);
      printf("%s\n", eventstr);
    }
  }

protected:
  bool mIsFirstCustomEvent;
  struct timeval mFirstCustomEventTS;
  struct timeval mRecentCustomEventTS;
}; /* class NMPolicy */

} /* namespace sc */

#endif /* !defined(__NM_POLICY_H__) */