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

#ifndef __TEST_RUNNER_H__
#define __TEST_RUNNER_H__

#include "../core/inc/API.h"
#include "../core/inc/NMPolicy.h"
#include "../core/inc/ServerAdapterStateListener.h"

#include "../common/inc/DebugLog.h"
#include "../common/inc/Counter.h"
#include "../device/inc/BtServerAdapter.h"
#include "../device/inc/WfdServerAdapter.h"

#include <string>
#include <thread>

#define DEBUG_SHOW_DATA 0
#define DEBUG_SHOW_TIME 0

#define TEST_DATA_SIZE (128)

class TestRunner : sc::ServerAdapterStateListener {
public:
  void start();
  static void on_start_sc(bool is_success);

private:
  static TestRunner *sTestRunner;

private:
  void send_test_data(void);

  void send_workload(void);

  void receiving_thread(void);

private:
  virtual void onUpdateServerAdapterState(sc::ServerAdapter *adapter,
                                          sc::ServerAdapterState old_state,
                                          sc::ServerAdapterState new_state);

private:
  static char *generate_random_string(char *str, size_t size);
  static char *generate_simple_string(char *str, size_t size);

public:
  static TestRunner *test_runner(int send_when, int throughput);

private:
  TestRunner(int send_when, int throughput) {
    this->mSendWhen = send_when;
    this->mThroughput = throughput;
  }

private:
  /* Attributes */
  int mSendWhen = 0;
  int mThroughput = 0;

  sc::Counter mSendDataSize;

  /* Components */
  std::thread *mThread;
  sc::BtServerAdapter *mBtAdapter;
  sc::WfdServerAdapter *mWfdAdapter;
}; /* class TestRunner */

#endif /* !defined(__TEST_RUNNER_H__) */