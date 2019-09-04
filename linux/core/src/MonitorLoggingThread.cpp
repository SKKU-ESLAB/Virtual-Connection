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

#include "../inc/MonitorLoggingThread.h"

#include "../inc/Core.h"
#include "../inc/Stats.h"

#include "../../common/inc/DebugLog.h"

#include <fcntl.h>

using namespace sc;

MonitorLoggingThread *MonitorLoggingThread::sSingleton = NULL;

void MonitorLoggingThread::start(void) {
  this->start(false);
}

void MonitorLoggingThread::start(bool is_append) {
  this->mIsAppend = is_append;
  
  this->mMonitorLoggingThreadOn = true;
  this->mMonitorLoggingThread =
      new std::thread(std::bind(&MonitorLoggingThread ::logging_thread, this));
  this->mMonitorLoggingThread->detach();
}

void MonitorLoggingThread::stop(void) { this->mMonitorLoggingThreadOn = false; }

void MonitorLoggingThread::logging_thread(void) {
  // Open monitor logging file
  FILE *monitor_fp;
  
  if(this->mIsAppend) {
    monitor_fp = ::fopen(MONITOR_LOG_FILE_NAME, "a");
  } else {
    monitor_fp = ::fopen(MONITOR_LOG_FILE_NAME, "w");
  }

  if (monitor_fp == NULL) {
    LOG_ERR("Failed to open monitor log file");
    return;
  }

  // Prevent file descriptor inheritance
  int fd = fileno(monitor_fp);
  int fd_flag = fcntl(fd, F_GETFD, 0);
  fd_flag |= FD_CLOEXEC;
  fcntl(fd, F_SETFD, fd_flag);

  // Write header of monitor log
  fprintf(monitor_fp,
          "# Timestamp(sec),"
          " Inter-arrival Time (ms), Request Size (B),"
          " Queue Arrival Speed(KB/s), Send Queue Length(KB),"
          " Bandwidth(KB/s), Latency(ms), BT State, WFD State\n");

  // Setting first timeval
  struct timeval first_tv;
  gettimeofday(&first_tv, NULL);
  long long first_tv_us =
      (long long)first_tv.tv_sec * 1000 * 1000 + (long long)first_tv.tv_usec;

  while (this->mMonitorLoggingThreadOn) {
    // Get statistics
    Core *core = Core::singleton();

    // Get relative now timeval
    struct timeval now_tv;
    gettimeofday(&now_tv, NULL);
    long long now_tv_us =
        (long long)now_tv.tv_sec * 1000 * 1000 + (long long)now_tv.tv_usec;
    long long relative_now_tv_us = now_tv_us - first_tv_us;
    int relative_now_tv_sec = (int)(relative_now_tv_us) / (1000 * 1000);
    int relative_now_tv_usec = (int)(relative_now_tv_us) % (1000 * 1000);

    // Get EMA send RTT
    Stats stats;
    stats.acquire_values();

    ServerAdapterState btState = core->get_adapter(0)->get_state();
    ServerAdapterState wfdState = core->get_adapter(1)->get_state();
    int bt_state_code, wfd_state_code;

    if (btState == ServerAdapterState::kConnecting) {
      bt_state_code = 1;
    } else if (btState == ServerAdapterState::kActive) {
      bt_state_code = 2;
    } else if (btState == ServerAdapterState::kDisconnecting) {
      bt_state_code = 3;
    } else {
      bt_state_code = 0;
    }
    if (wfdState == ServerAdapterState::kConnecting) {
      wfd_state_code = 1;
    } else if (wfdState == ServerAdapterState::kActive) {
      wfd_state_code = 2;
    } else if (wfdState == ServerAdapterState::kDisconnecting) {
      wfd_state_code = 3;
    } else {
      wfd_state_code = 0;
    }

    this->mMeasuredSendRTT.set_value(stats.ema_send_rtt);
    ::fprintf(monitor_fp,
              "%ld.%ld, %8.3f, %4d, %8.3f, %8.3f, %8.3f, %8.3f, %8.3f, %d, %d\n",
              relative_now_tv_sec, relative_now_tv_usec,
              ((float)stats.ema_arrival_time_us / 1000),
              (int)stats.ema_send_request_size,
              ((float)stats.ema_queue_arrival_speed / 1000),
              ((float)stats.now_queue_data_size / 1000),
              ((float)stats.now_total_bandwidth / 1000),
              (float)stats.ema_send_rtt, bt_state_code, wfd_state_code);
    ::fflush(monitor_fp);
    ::usleep(250 * 1000);
  }

  ::fclose(monitor_fp);
}