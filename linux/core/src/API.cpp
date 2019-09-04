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

#include "../inc/API.h"

#include "../inc/APIInternal.h"
#include "../inc/EventLogging.h"
#include "../inc/MonitorLoggingThread.h"
#include "../inc/NetworkMonitorThread.h"

#include <condition_variable>
#include <mutex>

using namespace sc;

std::mutex g_wait_lock_start_sc;
std::condition_variable g_wait_cond_start_sc;
bool g_is_start_sc_done;
bool g_start_sc_success;

std::mutex g_wait_lock_stop_sc;
std::condition_variable g_wait_cond_stop_sc;
bool g_is_stop_sc_done;
bool g_stop_sc_success;

// TODO: make it into synchronous call
void sc::start_sc(StartCallback startCallback) {
  sc::start_sc(startCallback, true, true, false);
}

void sc::start_sc(StartCallback startCallback, bool is_monitor, bool is_logging, bool is_append) {
  // Core start procedure
  g_is_start_sc_done = false;
  Core::singleton()->start();
  if(is_monitor) {
    NetworkMonitorThread::singleton()->start();
  }
  if(is_logging) {
    MonitorLoggingThread::singleton()->start(is_append);
    EventLogging::initialize();
  }

  // Wait until core start thread ends
  if (!g_is_start_sc_done) {
    std::unique_lock<std::mutex> lck(g_wait_lock_start_sc);
    g_wait_cond_start_sc.wait(lck);
  }

  // Execute callback
  if (startCallback != NULL) {
    startCallback(g_start_sc_success);
  }
}

void sc::start_sc_done(bool is_success) {
  g_is_start_sc_done = true;
  g_start_sc_success = is_success;
  g_wait_cond_start_sc.notify_all();
}

// TODO: make it into synchronous call
void sc::stop_sc(StopCallback stopCallback) {
  sc::stop_sc(stopCallback, true, true);
}

void sc::stop_sc(StopCallback stopCallback, bool is_monitor, bool is_logging) {
  // Core stop procedure
  g_is_stop_sc_done = false;
  if(is_monitor) {
    sc::stop_monitoring();
  }
  if(is_logging) {
    sc::stop_logging();
    Core::singleton()->stop();
  }

  // Wait until core stop thread ends
  if (!g_is_stop_sc_done) {
    std::unique_lock<std::mutex> lck(g_wait_lock_stop_sc);
    g_wait_cond_stop_sc.wait(lck);
  }

  // Execute callback
  if (stopCallback != NULL) {
    stopCallback(g_stop_sc_success);
  }
}

void sc::stop_sc_done(bool is_success) {
  g_is_stop_sc_done = true;
  g_stop_sc_success = is_success;
  g_wait_cond_stop_sc.notify_all();
}

void sc::stop_monitoring(void) { NetworkMonitorThread::singleton()->stop(); }

void sc::stop_logging(void) { MonitorLoggingThread::singleton()->stop(); }

void sc::set_nm_policy(NMPolicy* nm_policy) {
  NetworkMonitorThread::singleton()->set_policy(nm_policy);
}

NMPolicy* sc::get_nm_policy(void) {
  return NetworkMonitorThread::singleton()->get_policy();
}