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

#include "../inc/NetworkMonitorThread.h"

#include "../inc/Core.h"
#include "../inc/NetworkSwitcher.h"
#include "../inc/Stats.h"

#include "../../configs/ExpConfig.h"
#include "../../configs/NetworkSwitcherConfig.h"

#include <fcntl.h>
#include <math.h>
#include <string.h>

using namespace sc;

NetworkMonitorThread *NetworkMonitorThread::sSingleton = NULL;

void NetworkMonitorThread::start(void) {
  this->mMonitorThreadOn = true;
  this->mMonitorThread =
      new std::thread(std::bind(&NetworkMonitorThread ::monitor_thread, this));
  this->mMonitorThread->detach();
}

void NetworkMonitorThread::stop(void) { this->mMonitorThreadOn = false; }

void NetworkMonitorThread::monitor_thread(void) {
  int count = 0;
  while (this->mMonitorThreadOn) {
    // Get statistics
    Stats stats;
    stats.acquire_values();

    // Check and switch
    // If the switcher is already switching,
    NSState switcher_state = NetworkSwitcher::singleton()->get_state();
    if (switcher_state == NSState::kNSStateReady) {
      this->check_and_decide_switching(stats);
    }

    // Print statistics
    this->print_stats(stats);

    usleep(NETWORK_MONITOR_SLEEP_USECS);
  }
}

void NetworkMonitorThread::print_stats(Stats &stats) {
#ifndef PRINT_NETWORK_MONITOR_STATISTICS
  return;
#else
  if (Core::singleton()->get_state() != CoreState::kCoreStateReady) {
    return;
  }

  this->mPrintCount++;
  if(this->mPrintCount == 10) {
    this->mPrintCount = 0;
  } else {
    return;
  }

  char strline[512];
  snprintf(strline, 256,
           "%8.3fms %4dB / %8.3fKB/s => %s[%4dKB]\e[0m => %s%6.1fKB/s\e[0m / "
           "RTT=%4dms",
           ((float)stats.ema_arrival_time_us / 1000),
           (int)stats.ema_send_request_size,
           (stats.ema_queue_arrival_speed / 1000),
           (stats.now_queue_data_size == 0) ? "\e[44;97m" : "",
           (int)(stats.now_queue_data_size / 1000),
           (stats.now_total_bandwidth / 1000 > 3500) ? "\e[44;97m" : "",
           (float)stats.now_total_bandwidth / 1000, (int)stats.ema_send_rtt);

  NMPolicy *policy = this->get_policy();
  if (policy != NULL) {
    std::string policy_stats_string;
    policy_stats_string = policy->get_stats_string();
    strncat(strline, " ", 512);
    strncat(strline, policy_stats_string.c_str(), 512);
    strncat(strline, "\n", 512);
  } else {
    strncat(strline, "\n", 512);
  }

  printf(strline);
#endif
}

void NetworkMonitorThread::check_and_decide_switching(Stats &stats) {
  /* Determine Increasing/Decreasing adapter */
  NMPolicy *policy = this->get_policy();
  SwitchBehavior behavior = SwitchBehavior::kNoBehavior;
  bool is_increasable = this->is_increaseable();
  bool is_decreasable = this->is_decreaseable();
  behavior = policy->decide_switch(stats, is_increasable, is_decreasable);

  Core *core = Core::singleton();
  int prev_index = core->get_active_adapter_index();
  int next_index = (is_increasable) ? prev_index + 1 : prev_index - 1;

  switch (behavior) {
  case SwitchBehavior::kIncreaseAdapter: {
    this->increase_adapter();
    break;
  }
  case SwitchBehavior::kDecreaseAdapter: {
    this->decrease_adapter();
    break;
  }
  case SwitchBehavior::kPrepareIncreaseAdapter:
  // After using USB Bluetooth, thresholding is no more required.
  // if(this->is_increasable()) {
  //   core->prepare_switch(prev_index, next_index);
  // }
  // break;
  case SwitchBehavior::kPrepareDecreaseAdapter: {
    // After using USB Bluetooth, thresholding is no more required.
    // if(this->is_decreasable()) {
    //   core->prepare_switch(prev_index, next_index);
    // }
    // break;
  }
  case SwitchBehavior::kCancelPrepareSwitchAdapter: {
    core->cancel_preparing_switch(prev_index, next_index);
    break;
  }
  case SwitchBehavior::kNoBehavior:
  default:
    /* No behavior */
    break;
  }
}

bool NetworkMonitorThread::is_increaseable(void) {
  /* Check the minimum condition of adapter increase such as adapters' count
   */
  Core *core = Core::singleton();
  int adapter_count = core->get_adapter_count();
  int active_adapter_index = core->get_active_adapter_index();
  return ((adapter_count > 1) && (active_adapter_index < (adapter_count - 1)));
}
bool NetworkMonitorThread::is_decreaseable(void) {
  /* Check the minimum condition of adapter decrease such as adapters' count
   */
  Core *core = Core::singleton();
  int adapter_count = core->get_adapter_count();
  int active_adapter_index = core->get_active_adapter_index();
  return ((adapter_count > 1) && (active_adapter_index > 0));
}

bool NetworkMonitorThread::increase_adapter(void) {
  Core *core = Core::singleton();
  if (core->get_adapter_count() == 0) {
    LOG_ERR("No adapter is registered!");
    return false;
  } else if (!this->is_increaseable()) {
    LOG_WARN("Cannot increase adapter!");
    return false;
  }

  int prev_index = core->get_active_adapter_index();
  int next_index = prev_index + 1;

  return NetworkSwitcher::singleton()->switch_adapters(prev_index, next_index);
}

bool NetworkMonitorThread::decrease_adapter(void) {
  Core *core = Core::singleton();
  if (core->get_adapter_count() == 0) {
    LOG_ERR("No adapter is registered!");
    return false;
  } else if (!this->is_decreaseable()) {
    LOG_WARN("Cannot deccrease adapter!");
    return false;
  }

  int prev_index = core->get_active_adapter_index();
  int next_index = prev_index - 1;

  return NetworkSwitcher::singleton()->switch_adapters(prev_index, next_index);
}