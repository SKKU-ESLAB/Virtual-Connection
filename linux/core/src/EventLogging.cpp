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

#include "../inc/EventLogging.h"

#include "../inc/MonitorLoggingThread.h"

#include "../../common/inc/DebugLog.h"

#include <fcntl.h>

using namespace sc;

void EventLogging::initialize(void) {
  FILE *fp = ::fopen(EVENT_LOG_FILE_NAME, "w");
  if (fp == NULL) {
    LOG_ERR("Failed to open events log file");
    return;
  }

  // Prevent file descriptor inheritance
  int fd = fileno(fp);
  int fd_flag = fcntl(fd, F_GETFD, 0);
  fd_flag |= FD_CLOEXEC;
  fcntl(fd, F_SETFD, fd_flag);

  ::fclose(fp);
}

void EventLogging::print_event(const char *eventstr) {
  // Open events logging file
  FILE *fp = ::fopen(EVENT_LOG_FILE_NAME, "a");
  if (fp == NULL) {
    LOG_ERR("Failed to open events log file");
    return;
  }

  // Prevent file descriptor inheritance
  int fd = fileno(fp);
  int val = fcntl(fd, F_GETFD, 0);
  val |= FD_CLOEXEC;
  fcntl(fd, F_SETFD, val);

  ::fprintf(fp, "%s\n", eventstr);
  ::fflush(fp);

  ::fclose(fp);
}

void EventLogging::print_measured_send_rtt(void) {
  MonitorLoggingThread *logging_thread = MonitorLoggingThread::singleton();
  float measured_send_rtt = logging_thread->get_measured_send_rtt();
  char buf[256];
  snprintf(buf, 256, "Total measured average latency: %.3fms\n",
           measured_send_rtt);
  print_event(buf);
  printf(buf);
}