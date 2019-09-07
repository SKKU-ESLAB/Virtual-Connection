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

#ifndef __COUNTER_H__
#define __COUNTER_H__

#include "DebugLog.h"

#include <assert.h>
#include <map>
#include <mutex>
#include <sys/time.h>
#include <thread>

namespace sc {
class Counter {
public:
#define DEFAULT_SIMPLE_MOVING_AVERAGE_LENGTH 12
#define DEFAULT_EXPONENTIAL_MOVING_AVERAGE_MOMENTUM 0.1f
#define PREV_VALUES_SIZE 10

  Counter();

  ~Counter() { delete this->mSimpleHistoryValues; }

  void add(int diff) {
    std::unique_lock<std::mutex> lock(this->mValueLock);
    this->set_value_locked(this->get_value_locked() + diff);
  }

  void sub(int diff) {
    std::unique_lock<std::mutex> lock(this->mValueLock);
    this->set_value_locked(this->get_value_locked() - diff);
  }

  void increase(void) {
    std::unique_lock<std::mutex> lock(this->mValueLock);
    this->set_value_locked(this->get_value_locked() + 1);
  }

  void decrease(void) {
    std::unique_lock<std::mutex> lock(this->mValueLock);
    this->set_value_locked(this->get_value_locked() - 1);
  }

  void set_value(int new_value) {
    std::unique_lock<std::mutex> lock(this->mValueLock);
    this->set_value_locked(new_value);
  }

  int get_value() {
    std::unique_lock<std::mutex> lock(this->mValueLock);
    return this->get_value_locked();
  }

private:
  void set_value_locked(int new_value);
  int get_value_locked() { return this->mValue; }

public:
  int get_sm_average(void) {
    std::unique_lock<std::mutex> lock(this->mValueLock);
    return this->get_sm_average_locked();
  }

  float get_em_average(void) {
    std::unique_lock<std::mutex> lock(this->mValueLock);
    return this->get_em_average_locked();
  }

  float get_total_average(void) {
    return (float)((double)this->mTotal / this->mTotalCount);
  }

private:
  int get_sm_average_locked(void);
  float get_em_average_locked(void) { return this->mEma; }

public:
  int get_speed() {
    std::unique_lock<std::mutex> lock(this->mValueLock);
    return this->get_speed_locked();
  };

  void update_speed() {
    std::unique_lock<std::mutex> lock(this->mValueLock);
    this->update_speed_locked();
  }
  void start_measure_speed(void);
  void stop_measure_speed(void) { this->mIsSpeedThreadOn = false; }

private:
  void speed_thread_loop();
  int get_speed_locked();
  void update_speed_locked();

private:
  /* Value */
  std::mutex mValueLock;
  int mValue;

  /* Speed */
  int mPrevValues[PREV_VALUES_SIZE];
  uint64_t mPrevValueTSs[PREV_VALUES_SIZE];
  int mPrevValuePointer;
  struct timeval mLastAccessedTS;
  std::thread *mSpeedThread;
  bool mIsSpeedThreadOn;

  /* Total average */
  long long mTotal;
  int mTotalCount;

  /* Simple moving average (SMA) */
  int *mSimpleHistoryValues; /* History values for simple moving average */
  int mSimpleHistoryCursor;  /* Cursor on history values */
  int mSmaLength;            /* Length for simple moving average */

  /* Exponential moving average (EMA) */
  float mEma;         /* Exponential moving average */
  float mEmaMomentum; /* Momentum for Exponential moving average */
};                    /* class Counter */
} /* namespace sc */

#endif /* !defined(__COUNTER_H__) */
