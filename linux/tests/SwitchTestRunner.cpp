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

#include "SwitchTestRunner.h"

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

SwitchTestRunner *SwitchTestRunner::sTestRunner = NULL;

void SwitchTestRunner::start() {
  assert(this->mThroughput > 0);
  assert(this->mIterations > 0);

  printf("** SwitchTestRunner Start\n");
  printf(" ** Throughput: %dKB/s\n", this->mThroughput / 1024);
  printf(" ** Iterations: %d\n", this->mIterations);

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
  SwitchTestRunner::sTestRunner = this;
  sc::start_sc(SwitchTestRunner::on_start_sc, true, true, false);
}

#define SEND_ON_BT 0
#define SEND_ON_WFD 1
#define SEND_ON_BT_TO_WFD 2
#define SEND_ON_WFD_TO_BT 3

void SwitchTestRunner::on_start_sc(bool is_success) {
  SwitchTestRunner *self = SwitchTestRunner::sTestRunner;

  if (!is_success) {
    printf("[ERROR] Failed to initialize!");
    return;
  }

  // Launch app receiving thread
  self->mThread =
      new std::thread(std::bind(&SwitchTestRunner::receiving_thread, self));
  self->mThread->detach();

  // Send test data to warm-up the initial connection
  printf(" ** Send Test Data\n");
  printf(" ** 5 sec. remains\n");
  self->send_test_data();
  ::sleep(1);
  self->send_test_data();
  ::sleep(1);
  self->send_test_data();
  ::sleep(1);
  self->send_test_data();
  ::sleep(1);
  self->send_test_data();
  ::sleep(1);

  for(int i=0; i<self->mIterations; i++) {
    // BT -> WFD
    NMPolicy *policy = sc::get_nm_policy();
    policy->on_custom_event("wfd");
    
    
    // Wait for 30 sec
    for (int i = 0; i < 30; i++) {
      self->send_test_data();
      ::usleep(1000000);
    }
    
    // WFD -> BT
    policy = sc::get_nm_policy();
    policy->on_custom_event("bt");

    // Wait for 30 sec
    for (int i = 0; i < 30; i++) {
      self->send_test_data();
      ::usleep(1000000);
    }
  }

  printf("\e[49;41m ** Finish Workload... Wait for 5 sec...\e[0m\n");
  ::sleep(5);
  sc::stop_logging();
  sc::stop_monitoring();

  // Stop selective connection
  sc::stop_sc(NULL);

  return;
}

void SwitchTestRunner::send_test_data() {
  char *temp_buf = (char *)calloc(TEST_DATA_SIZE, sizeof(char));
  sc::send(temp_buf, TEST_DATA_SIZE);
  free(temp_buf);
}

#define MAX_SEC 5
void SwitchTestRunner::send_workload(void) {
  this->mSendDataSize.start_measure_speed();
  int max_iter = MAX_SEC;
  int sleep_us = 1000 * 1000;
  int data_size = this->mThroughput;

  for (int i = 0; i < max_iter; i++) {
    // Allocate data buffer
    char *data_buffer = (char *)calloc(data_size, sizeof(char));
    if (data_buffer == NULL) {
      fprintf(stderr, "[Error] Data buffer allocation failed: size=%d\n",
              data_size);
      return;
    }

    // Send data
    SwitchTestRunner::generate_simple_string(data_buffer, data_size);
    sc::send(data_buffer, data_size);
    free(data_buffer);
    ::usleep(sleep_us);
  }

  this->mSendDataSize.stop_measure_speed();
}

void SwitchTestRunner::receiving_thread(void) {
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

void SwitchTestRunner::onUpdateServerAdapterState(sc::ServerAdapter *adapter,
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

char *SwitchTestRunner::generate_random_string(char *str, size_t size) {
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

char *SwitchTestRunner::generate_simple_string(char *str, size_t size) {
  memset(str, 'a', size - 1);
  memset(str + size - 1, '\0', 1);
  return str;
}

SwitchTestRunner *SwitchTestRunner::test_runner(int throughput, int iterations) {
  return new SwitchTestRunner(throughput, iterations);
}