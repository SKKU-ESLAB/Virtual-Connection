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

#include "../inc/Stats.h"

#include "../inc/Core.h"
#include "../inc/SegmentManager.h"

using namespace sc;

void Stats::acquire_values(void) {
  Core *core = Core::singleton();
  SegmentManager *sm = SegmentManager::singleton();

  /* Statistics used to print present status */
  this->ema_queue_arrival_speed = sm->get_ema_queue_arrival_speed();

  /* Now total bandwidth */
  this->now_total_bandwidth = core->get_total_bandwidth();

  /* Send request size */
  this->ema_send_request_size = core->get_ema_send_request_size();

  /* Send request arrival time */
  this->ema_arrival_time_us = core->get_ema_send_arrival_time();

  /* Send queue size */
  this->now_queue_data_size = sm->get_queue_data_size(kSQSendData) +
                              sm->get_queue_data_size(kSQSendControl) +
                              sm->get_failed_sending_queue_data_size();

  /* Statistics used to evaluate the policies */
  this->ema_send_rtt = core->get_ema_send_rtt();
  this->ema_media_rtt = core->get_ema_media_rtt();
}