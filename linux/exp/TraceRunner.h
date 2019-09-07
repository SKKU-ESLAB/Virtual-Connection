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

#ifndef __TRACE_RUNNER_H__
#define __TRACE_RUNNER_H__

#include "../common/inc/csv.h"

#include "../core/inc/API.h"
#include "../core/inc/NMPolicy.h"
#include "../core/inc/ServerAdapterStateListener.h"

#include "../common/inc/DebugLog.h"
#include "../device/inc/BtServerAdapter.h"
#include "../device/inc/WfdServerAdapter.h"

#include <string>
#include <thread>

#define DEBUG_SHOW_DATA 0
#define DEBUG_SHOW_TIME 0

#define TEST_DATA_SIZE (128)

class PacketTraceReader {
public:
  void read_header() {
    this->mCSVReader->read_header(io::ignore_extra_column, "TimeUS", "Length");
  }

  bool read_a_row(int &ts_us, int &payload_length) {
    std::string ts_us_str, payload_length_str;
    bool ret = this->mCSVReader->read_row(ts_us_str, payload_length_str);
    if (ret) {
      // LOG_VERB("ROW: %s / %s", ts_us_str.c_str(),
      // payload_length_str.c_str());
      ts_us = std::stoi(ts_us_str);
      payload_length = std::stoi(payload_length_str);
    }

    return ret;
  }

  PacketTraceReader(std::string filename) {
    this->mCSVReader =
        new io::CSVReader<2, io::trim_chars<>,
                          io::double_quote_escape<',', '\"'>>(filename.c_str());
  }

private:
  io::CSVReader<2, io::trim_chars<>, io::double_quote_escape<',', '\"'>>
      *mCSVReader;
};

class EventTraceReader {
public:
  void read_header() {
    this->mCSVReader->read_header(io::ignore_extra_column, "TimeUS", "Event");
  }

  bool read_a_row(int &ts_us, std::string &event_description) {
    std::string ts_us_str;
    bool ret = this->mCSVReader->read_row(ts_us_str, event_description);
    if (ret) {
      LOG_VERB("ROW: %s / %s", ts_us_str.c_str(), event_description.c_str());
      ts_us = std::stoi(ts_us_str);
    }

    return ret;
  }

  EventTraceReader(std::string filename) {
    this->mCSVReader =
        new io::CSVReader<2, io::trim_chars<>,
                          io::double_quote_escape<',', '\"'>>(filename.c_str());
  }

private:
  io::CSVReader<2, io::trim_chars<>, io::double_quote_escape<',', '\"'>>
      *mCSVReader;
}; /* class EventTraceReader */

class TraceRunner : sc::ServerAdapterStateListener {
public:
  void start(sc::NMPolicy *switch_policy);
  static void on_start_sc(bool is_success);

private:
  static TraceRunner *sTraceRunner;

private:
  void send_test_data(void);

  void send_workload(std::string &packet_trace_filename,
                     std::string &event_trace_filename,
                     bool is_wait_packets_of_prev_event);
  bool check_event(EventTraceReader *event_trace_reader, int next_packet_ts_us,
                   bool &next_event_ts_ready, int &next_event_ts_us);
  int sleep_workload(int next_ts_us);

  void receiving_thread(void);

private:
  void start_im_alive_thread(void);
  void im_alive_thread(void);
  bool mIsFirstPacketSent = false;

private:
  /* implement sc::ServerAdapterStateListener */
  virtual void onUpdateServerAdapterState(sc::ServerAdapter *adapter,
                                          sc::ServerAdapterState old_state,
                                          sc::ServerAdapterState new_state);

private:
  static char *generate_simple_string(char *str, size_t size);

public:
  static TraceRunner *trace_runner(std::string packet_trace_filename,
                                   std::string event_trace_filename,
                                   bool is_wait_packets_of_prev_event);

private:
  TraceRunner(std::string packet_trace_filename,
              std::string event_trace_filename,
              bool is_wait_packets_of_prev_event) {
    this->mPacketTraceFilename.assign(packet_trace_filename);
    this->mEventTraceFilename.assign(event_trace_filename);
    this->mIsWaitPacketsOfPrevEvent = is_wait_packets_of_prev_event;
  }

private:
  /* Attributes */
  std::string mPacketTraceFilename;
  std::string mEventTraceFilename;
  bool mIsWaitPacketsOfPrevEvent;

  int mRecentTSUs = 0;

  /* Components */
  std::thread *mReceivingThread;
  std::thread *mImAliveThread;
  sc::BtServerAdapter *mBtAdapter;
  sc::WfdServerAdapter *mWfdAdapter;
}; /* class TraceRunner */

#endif /* !defined(__TRACE_RUNNER_H__) */