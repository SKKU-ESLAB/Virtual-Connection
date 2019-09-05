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

#include "TraceRunner.h"

#include "../common/inc/DebugLog.h"
#include "../core/inc/API.h"
#include "../core/inc/EventLogging.h"
#include "../core/inc/NMPolicy.h"
#include "../core/inc/Stats.h"
#include "../device/inc/NetworkInitializer.h"

using namespace sc;

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

TraceRunner *TraceRunner::sTraceRunner = NULL;
TraceRunner *TraceRunner::trace_runner(std::string packet_trace_filename,
                                       std::string event_trace_filename,
                                       bool is_wait_packets_of_prev_event) {
  return new TraceRunner(packet_trace_filename, event_trace_filename,
                         is_wait_packets_of_prev_event);
}

// Initialize network interfaces
void TraceRunner::start(NMPolicy *switch_policy) {
  assert(!this->mPacketTraceFilename.empty());

  printf("** TraceRunner Start\n");
  printf(" ** Packet Trace: %s\n", this->mPacketTraceFilename.c_str());
  printf(" ** Event Trace: %s\n", this->mEventTraceFilename.c_str());

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

  // Initial network monitor policy setting
  sc::set_nm_policy(switch_policy);

  // Start selective connection
  TraceRunner::sTraceRunner = this;
  sc::start_sc(TraceRunner::on_start_sc, true, false, false);
}

// Read trace file and send packets accorting to given workload
void TraceRunner::on_start_sc(bool is_success) {
  TraceRunner *self = TraceRunner::sTraceRunner;

  if (!is_success) {
    printf("[ERROR] Failed to initialize!");
    return;
  }

  char buf[256];
  snprintf(buf, 256, "Packet Trace: %s / Event Trace: %s",
           self->mPacketTraceFilename.c_str(),
           self->mEventTraceFilename.c_str());
  EventLogging::print_event(buf);

  // Launch app receiving thread
  self->mThread =
      new std::thread(std::bind(&TraceRunner::receiving_thread, self));
  self->mThread->detach();

  // 1. Send test data to warm-up the initial connection
  printf(" ** Send Test Data\n");
  printf(" ** 10 sec. remains\n");
  self->send_test_data();
  ::sleep(5);
  printf(" ** 5 sec. remains\n");
  self->send_test_data();
  ::sleep(5);

  /// Parse trace file and send data
  printf("\e[49;41m ** Start Workload\e[0m\n");
  self->send_workload(self->mPacketTraceFilename, self->mEventTraceFilename,
                      self->mIsWaitPacketsOfPrevEvent);
  printf("\e[49;41m ** Finish Workload...\e[0m\n");

  char eventstr[200];
  char eventstr2[200];
  snprintf(eventstr, 256, "Total average latency(send RTT): %.3fus\n",
           Core::singleton()->get_average_send_rtt());
  printf(eventstr);
  EventLogging::print_event(eventstr);
  ::sleep(120);
  snprintf(eventstr2, 256, "Total average latency(send RTT): %.3fus\n",
           Core::singleton()->get_average_send_rtt());
  printf(eventstr2);
  EventLogging::print_event(eventstr2);
  ::sleep(400);
  sc::stop_logging();
  sc::stop_monitoring();

  // Stop selective connection
  sc::stop_sc(NULL, true, false);

  return;
}

void TraceRunner::send_test_data() {
  char *temp_buf = (char *)calloc(TEST_DATA_SIZE, sizeof(char));
  sc::send(temp_buf, TEST_DATA_SIZE);
  free(temp_buf);
}

void TraceRunner::send_workload(std::string &packet_trace_filename,
                                std::string &event_trace_filename,
                                bool is_wait_packets_of_prev_event) {
  // Initialize parser
  PacketTraceReader *packet_trace_reader;
  packet_trace_reader = new PacketTraceReader(packet_trace_filename);
  packet_trace_reader->read_header();

  EventTraceReader *event_trace_reader;
  event_trace_reader = new EventTraceReader(event_trace_filename);
  event_trace_reader->read_header();

// Loop-global variables
#define ACTION_EXIT -1
#define ACTION_INIT 0
#define ACTION_SEND_PACKET 1
#define ACTION_CUSTOM_EVENT 2
  int next_action = ACTION_INIT;
  int num_packets = 0, num_events = 0, num_iters = 0;

  // Next packet
  int next_packet_ts_us = 0;
  int next_packet_payload_length = 0;
  bool packet_ready = false;

  // Next event
  int next_event_ts_us = 0;
  std::string next_event_description("");
  bool event_ready = false;

  // Read packet trace row
  while (next_action > ACTION_EXIT) {
    if (num_iters % 1000 == 0) {
      printf(" ** Send Workload (%d: %d packets/ %d events)\n", num_iters,
             num_packets, num_events);
    }

    // Step 1. Read a line from the trace if packet or event is not ready
    if (!packet_ready) {
      if (packet_trace_reader->read_a_row(next_packet_ts_us,
                                          next_packet_payload_length)) {
        packet_ready = true;
        num_packets++;
#if DEBUG_SHOW_DATA == 1
        printf("Read Packet Trace %d : %d %d\n", num_packets, next_packet_ts_us,
               next_packet_payload_length);
#endif
      }
    }
    if (!event_ready) {
      if (event_trace_reader->read_a_row(next_event_ts_us,
                                         next_event_description)) {
        event_ready = true;
        num_events++;
#if DEBUG_SHOW_DATA == 1
        printf("Read Event Trace %d : %d %s\n", num_events, next_event_ts_us,
               next_event_description);
#endif
      }
    }

    // Step 2. Decide next action
    int next_ts_us = 0;
    if (packet_ready && event_ready) {
      if (next_packet_ts_us < next_event_ts_us) {
        next_action = ACTION_SEND_PACKET;
        next_ts_us = next_packet_ts_us;
        packet_ready = false;
      } else {
        next_action = ACTION_CUSTOM_EVENT;
        next_ts_us = next_event_ts_us;
        event_ready = false;
      }
    } else if (packet_ready) {
      next_action = ACTION_SEND_PACKET;
      next_ts_us = next_packet_ts_us;
      packet_ready = false;
    } else if (event_ready) {
      next_action = ACTION_CUSTOM_EVENT;
      next_ts_us = next_event_ts_us;
      event_ready = false;
    } else {
      next_action = ACTION_EXIT;
    }

    // printf("Trace: packet=(%d / %d) / event=(%d / %s) => %d\n",
    //        next_packet_ts_us, next_packet_payload_length, next_event_ts_us,
    //        next_event_description.c_str(), next_packet_payload_length,
    //        next_action);

    // Step 3. Sleep for wait next action (packet or event)
    if (int res = this->sleep_workload(next_ts_us) < 0) {
      printf("[ERROR] sleep_workload error %d\n", res);
      return;
    }

    // Step 4. Action
    if (next_action == ACTION_SEND_PACKET) {
      char *data_buffer =
          (char *)calloc(next_packet_payload_length, sizeof(char));
      if (data_buffer == NULL) {
        fprintf(stderr, "[Error] Data buffer allocation failed: size=%d\n",
                next_packet_payload_length);
        return;
      }

      // Generate string to data buffer
      TraceRunner::generate_simple_string(data_buffer,
                                          next_packet_payload_length);

      // Send data
      sc::send(data_buffer, next_packet_payload_length);

      free(data_buffer);
    } else if (next_action == ACTION_CUSTOM_EVENT) {
      // Wait until every packets of the previous event are sent
      if (is_wait_packets_of_prev_event) {
        bool is_first_checked = false;
        bool is_queue_dirty = true;
        while (is_queue_dirty) {
          if (!is_first_checked) {
            is_first_checked = true;
          } else {
            usleep(100 * 1000);
          }
          Stats stats;
          stats.acquire_values();
          is_queue_dirty = (stats.now_queue_data_size != 0);
        }
      }

      // Update custom event
      NMPolicy *policy = sc::get_nm_policy();
      policy->send_custom_event(next_event_description);
    }
    num_iters++;
  }
  NMPolicy *policy = sc::get_nm_policy();
  policy->send_custom_event("");
}

int TraceRunner::sleep_workload(int next_ts_us) {
  int sleep_us = next_ts_us - this->mRecentTSUs;
  if (sleep_us < 0) {
    printf("[Error] Invalid sleep time: %d(%d -> %d)\n", sleep_us,
           this->mRecentTSUs, next_ts_us);
    return -1;
  }
  // Sleep
  ::usleep(sleep_us);

  // Update recent timestamp
  this->mRecentTSUs = next_ts_us;

  return 0;
}

void TraceRunner::receiving_thread(void) {
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

void TraceRunner::onUpdateServerAdapterState(sc::ServerAdapter *adapter,
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

char *TraceRunner::generate_simple_string(char *str, size_t size) {
  memset(str, 'a', size - 1);
  memset(str + size - 1, '\0', 1);
  return str;
}