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

#ifndef __MONITOR_LOGGING_THREAD_H__
#define __MONITOR_LOGGING_THREAD_H__

#include "../../common/inc/Counter.h"

#include <thread>

#define MONITOR_LOG_FILE_NAME "monitor-log.csv"

namespace sc {
class MonitorLoggingThread {
public:
  /* Control this thread */
  void start(void);
  void start(bool is_append);
  void stop(void);

private:
  void logging_thread(void);
  std::thread *mMonitorLoggingThread;
  bool mMonitorLoggingThreadOn;

public:
  float get_measured_send_rtt(void) {
    return this->mMeasuredSendRTT.get_total_average();
  }

private:
  bool mIsAppend = false;
  Counter mMeasuredSendRTT;

public:
  /* singleton */
  static MonitorLoggingThread *singleton(void) {
    if (MonitorLoggingThread::sSingleton == NULL) {
      MonitorLoggingThread::sSingleton = new MonitorLoggingThread();
    }
    return MonitorLoggingThread::sSingleton;
  }

private:
  static MonitorLoggingThread *sSingleton;
};
} /* namespace sc */

#endif /* !defined(__MONITOR_LOGGING_THREAD_H__) */