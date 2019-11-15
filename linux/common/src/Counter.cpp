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

#include "../inc/Counter.h"

using namespace sc;

Counter::Counter() {
  int simple_moving_average_length = DEFAULT_SIMPLE_MOVING_AVERAGE_LENGTH;
  float exponential_moving_average_momentum =
      DEFAULT_EXPONENTIAL_MOVING_AVERAGE_MOMENTUM;

  this->mValue = 0;

  /* Speed */
  for (int i = 0; i < PREV_VALUES_SIZE; i++) {
    this->mPrevValues[i] = 0;
    this->mPrevValueTSs[i] = 0;
  }

  /* Simple moving average */
  assert(simple_moving_average_length > 0);
  this->mSmaLength = simple_moving_average_length;
  this->mSimpleHistoryValues = new int[this->mSmaLength];
  for (int i = 0; i < this->mSmaLength; i++) {
    this->mSimpleHistoryValues[i] = 0;
  }
  this->mSimpleHistoryCursor = 0;

  /* Exponential moving average */
  this->mEma = 0.0f;
  this->mEmaMomentum = exponential_moving_average_momentum;
  assert(exponential_moving_average_momentum >= 0 &&
         exponential_moving_average_momentum <= 1);
}

void Counter::set_value_locked(int new_value) {
  /* Update new value */
  this->mValue = new_value;

  /* Update history for simple moving average */
  this->mSimpleHistoryValues[this->mSimpleHistoryCursor] = new_value;
  this->mSimpleHistoryCursor =
      (this->mSimpleHistoryCursor + 1) % this->mSmaLength;

  /* Update exponential moving average */
  this->mEma = ((float)this->mEma * this->mEmaMomentum) +
               ((float)this->mValue * (1.0f - this->mEmaMomentum));

  /* Total average */
  this->mTotal += (long long)new_value;
  this->mTotalCount++;
}

void Counter::start_measure_speed(void) {
  if (!this->mIsSpeedThreadOn) {
    this->mIsSpeedThreadOn = true;
    this->mSpeedThread =
        new std::thread(std::bind(&Counter::speed_thread_loop, this));
    this->mSpeedThread->detach();
  }
}

void Counter::speed_thread_loop() {
  while (this->mIsSpeedThreadOn) {
    this->update_speed();
    usleep(250 * 1000);
  }
}

#define MAX_SPEED_WINDOW_SEC 1
int Counter::get_speed_locked() {
  struct timeval nowTStv;
  gettimeofday(&nowTStv, NULL);
  uint64_t now_ts_us = (uint64_t)nowTStv.tv_sec * 1000 * 1000 + nowTStv.tv_usec;

  // Circular queue boundary
  int start_cq_idx = (this->mPrevValuePointer + 1) % PREV_VALUES_SIZE;
  int end_cq_idx = (this->mPrevValuePointer) % PREV_VALUES_SIZE;

  // Desired timestamps
  uint64_t start_desired_ts_us = now_ts_us - 1000 * 1000 * MAX_SPEED_WINDOW_SEC;
  uint64_t end_desired_ts_us = now_ts_us;

  // Find speed window boundary's start point (within 1 sec)
  int i = end_cq_idx;
  bool is_pass_once = false;
  while (i != start_cq_idx) {
    uint64_t ts_us = this->mPrevValueTSs[i];
    if (ts_us > end_desired_ts_us && ts_us < start_desired_ts_us) {
      i = (i + 1) % PREV_VALUES_SIZE;
      break;
    }
    is_pass_once = true;
    i = (i - 1 >= 0) ? (i - 1) : (PREV_VALUES_SIZE - 1);
  }
  if (!is_pass_once) {
    return 0;
  }

  // Speed window boundary
  int start_sw_idx = i;
  int end_sw_idx = end_cq_idx;

  uint64_t start_ts_us = this->mPrevValueTSs[start_sw_idx];
  uint64_t end_ts_us = this->mPrevValueTSs[end_sw_idx];

  int start_value = this->mPrevValues[start_sw_idx];
  int end_value = this->mPrevValues[end_sw_idx];

  int speed;
  if (end_ts_us == 0 || start_ts_us == 0) {
    speed = 0;
  } else {
    speed = (int)(((float)(end_value - start_value)) /
                  ((float)(end_ts_us - start_ts_us) / 1000000));
  }

  return speed;
}

void Counter::update_speed_locked() {
  struct timeval nowTStv;
  gettimeofday(&nowTStv, NULL);

  this->mPrevValuePointer = (this->mPrevValuePointer + 1) % PREV_VALUES_SIZE;
  this->mPrevValues[this->mPrevValuePointer] = this->get_value_locked();
  this->mPrevValueTSs[this->mPrevValuePointer] =
      (uint64_t)nowTStv.tv_sec * 1000 * 1000 + nowTStv.tv_usec;
}

int Counter::get_sm_average_locked(void) {
  int simple_mavg = 0;
  for (int i = 0; i < this->mSmaLength; i++) {
    simple_mavg += this->mSimpleHistoryValues[i];
  }
  simple_mavg /= this->mSmaLength;
  return simple_mavg;
}