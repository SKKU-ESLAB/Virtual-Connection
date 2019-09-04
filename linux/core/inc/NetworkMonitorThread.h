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

#ifndef __NETWORK_MONITOR_THREAD_H__
#define __NETWORK_MONITOR_THREAD_H__

#include "API.h"
#include "NMPolicy.h"
#include "NetworkSwitcher.h"
#include "Stats.h"

#include "../../common/inc/Counter.h"

#include <thread>

namespace sc {

class Stats;
class NetworkSwitcher;
class NetworkMonitorThread {
public:
  /* Control netwowrk monitor thread */
  void start(void);
  void stop(void);

private:
  /* Network monitor thread */
  void monitor_thread(void);
  std::thread *mMonitorThread;
  bool mMonitorThreadOn;

  /* Checking statistics and decide switching */
  void print_stats(Stats &stats);
  void check_and_decide_switching(Stats &stats);

private:
  int mPrintCount = 0;

  /* Check condition of increase/decrease */
  bool is_increaseable(void);
  bool is_decreaseable(void);

  /* Increase/decrease adapter */
  bool increase_adapter(void);
  bool decrease_adapter(void);

public:
  /* Policy */
  void set_policy(NMPolicy *policy) {
    std::unique_lock<std::mutex> lck(this->mPolicyLock);
    this->mPolicy = policy;
  }
  NMPolicy *get_policy(void) {
    std::unique_lock<std::mutex> lck(this->mPolicyLock);
    return this->mPolicy;
  }

private:
  NMPolicy *mPolicy;
  std::mutex mPolicyLock;

public:
  /* Singleton */
  static NetworkMonitorThread *singleton(void) {
    if (NetworkMonitorThread::sSingleton == NULL) {
      NetworkMonitorThread::sSingleton = new NetworkMonitorThread();
    }
    return NetworkMonitorThread::sSingleton;
  }

private:
  /* Singleton */
  static NetworkMonitorThread *sSingleton;
  NetworkMonitorThread(void) {
    this->mMonitorThreadOn = false;
    this->mMonitorThread = NULL;
    this->mPolicy = NULL;
  }
}; /* class NetworkMonitorThread */
} /* namespace sc */

#endif /* !defined(__NETWORK_MONITOR_H__)THREAD_ */