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

#include "TestRunner.h"

#include "../common/inc/Counter.h"
#include "../common/inc/DebugLog.h"
#include "../core/inc/API.h"
#include "../core/inc/EventLogging.h"
#include "../device/inc/NetworkInitializer.h"
#include "../policy/inc/NMPolicyManual.h"

using namespace sc;

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

TestRunner *TestRunner::sTestRunner = NULL;

void TestRunner::start() {
  assert(this->mThroughput > 0);
  assert(this->mSendWhen == 0 || this->mSendWhen == 1 || this->mSendWhen == 2 ||
         this->mSendWhen == 3);

  printf("** TestRunner Start\n");
  printf(" ** Mode: %d\n", this->mSendWhen);
  printf(" ** Throughput: %dKB/s\n", this->mThroughput / 1024);

  // Network Interface Initialization
  sc::NetworkInitializer netInit;
  netInit.initialize();

  // Bluetooth adapter setting
  this->mBtAdapter = sc::BtServerAdapter::singleton(
      0, "Bt", "150e8400-1234-41d4-a716-446655440000");
  this->mBtAdapter->listen_state(this);
  sc::register_adapter(this->mBtAdapter);

  // Wi-fi Direct adapter setting
  this->mWfdAdapter = sc::WfdServerAdapter::singleton(1, "Wfd", 3455, "SelCon");
  this->mWfdAdapter->listen_state(this);
  sc::register_adapter(this->mWfdAdapter);

  // Initial network monitor mode setting
  NMPolicyManual *nm_policy = new NMPolicyManual();
  sc::set_nm_policy(nm_policy);

  // Start selective connection
  TestRunner::sTestRunner = this;
  sc::start_sc(TestRunner::on_start_sc);
}

#define SEND_ON_BT 0
#define SEND_ON_WFD 1
#define SEND_ON_BT_TO_WFD 2
#define SEND_ON_WFD_TO_BT 3

void TestRunner::on_start_sc(bool is_success) {
  TestRunner *self = TestRunner::sTestRunner;

  if (!is_success) {
    printf("[ERROR] Failed to initialize!");
    return;
  }

  // Launch app receiving thread
  self->mThread =
      new std::thread(std::bind(&TestRunner::receiving_thread, self));
  self->mThread->detach();

  // 1. Send test data to warm-up the initial connection
  printf(" ** Send Test Data\n");
  printf(" ** 5 sec. remains\n");
  self->send_test_data();
  ::sleep(5);

  // BT -> WFD
  if (self->mSendWhen == SEND_ON_WFD || self->mSendWhen == SEND_ON_BT_TO_WFD ||
      self->mSendWhen == SEND_ON_WFD_TO_BT) {
    NMPolicy *policy = sc::get_nm_policy();
    policy->send_custom_event("wfd");
  }
  if (self->mSendWhen == SEND_ON_WFD || self->mSendWhen == SEND_ON_WFD_TO_BT) {
    // Wait for 15 sec
    for (int i = 0; i < 3; i++) {
      self->send_test_data();
      ::sleep(5);
    }
  }
  // WFD -> BT
  if (self->mSendWhen == SEND_ON_WFD_TO_BT) {
    NMPolicy *policy = sc::get_nm_policy();
    policy->send_custom_event("bt");
  }

  printf("\e[49;41m ** Start Workload\e[0m\n");

  /// Parse trace file and send data
  self->send_workload();

  printf("\e[49;41m ** Finish Workload...\e[0m\n");
  char eventstr[256];
  snprintf(eventstr, 256, "Total average latency(send RTT): %.3fus\n",
           Core::singleton()->get_average_send_rtt());
  printf(eventstr);
  EventLogging::print_measured_send_rtt();
  EventLogging::print_event(eventstr);
  ::sleep(600);
  sc::stop_logging();
  sc::stop_monitoring();

  // Stop selective connection
  sc::stop_sc(NULL);

  return;
}

void TestRunner::send_test_data() {
  char *temp_buf = (char *)calloc(TEST_DATA_SIZE, sizeof(char));
  sc::send(temp_buf, TEST_DATA_SIZE);
  free(temp_buf);
}

#define MAX_SEC 90
void TestRunner::send_workload(void) {
  this->mSendDataSize.start_measure_speed();
  int max_iter = MAX_SEC;
  int sleep_us = 1000 * 1000;
  int data_size = this->mThroughput;
  // int max_iter = (this->mThroughput <= 5120) ? MAX_SEC * (this->mThroughput /
  // 1024) : MAX_SEC; int sleep_us = (this->mThroughput <= 5120) ? 1000 * 1000 /
  // (this->mThroughput / 1024) : 1000 * 1000; int data_size =
  //     (this->mThroughput <= 5120) ? 1024 : this->mThroughput;

  for (int i = 0; i < max_iter; i++) {
    // Allocate data buffer
    char *data_buffer = (char *)calloc(data_size, sizeof(char));
    if (data_buffer == NULL) {
      fprintf(stderr, "[Error] Data buffer allocation failed: size=%d\n",
              data_size);
      return;
    }

    // Send data
    TestRunner::generate_simple_string(data_buffer, data_size);
    sc::send(data_buffer, data_size);
    free(data_buffer);
    ::usleep(sleep_us);
  }

  this->mSendDataSize.stop_measure_speed();
}

void TestRunner::receiving_thread(void) {
  LOG_THREAD_LAUNCH("App Receiving");

  void *buf = NULL;
  while (true) {
    int ret = sc::receive(&buf);
#if DEBUG_SHOW_DATA == 1
    printf("Recv %d> %s\n\n", ret, reinterpret_cast<char *>(buf));
#endif

    if (buf)
      free(buf);
  }

  LOG_THREAD_FINISH("App Receiving");
}

void TestRunner::onUpdateServerAdapterState(sc::ServerAdapter *adapter,
                                            sc::ServerAdapterState old_state,
                                            sc::ServerAdapterState new_state) {
  sc::ServerAdapterState bt_state = this->mBtAdapter->get_state();
  sc::ServerAdapterState wfd_state = this->mWfdAdapter->get_state();
  std::string bt_state_str(
      sc::ServerAdapter::server_adapter_state_to_string(bt_state));
  std::string wfd_state_str(
      sc::ServerAdapter::server_adapter_state_to_string(wfd_state));

  LOG_VERB("[STATE] BT: %s / WFD: %s", bt_state_str.c_str(),
           wfd_state_str.c_str());
}

char *TestRunner::generate_random_string(char *str, size_t size) {
  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  if (size) {
    --size;
    for (size_t n = 0; n < size; n++) {
      int key = rand() % (int)(sizeof charset - 1);
      str[n] = charset[key];
    }
    str[size] = '\0';
  }
  return str;
}

char *TestRunner::generate_simple_string(char *str, size_t size) {
  memset(str, 'a', size - 1);
  memset(str + size - 1, '\0', 1);
  return str;
}

TestRunner *TestRunner::test_runner(int bt_wfd_mode, int throughput) {
  return new TestRunner(bt_wfd_mode, throughput);
}