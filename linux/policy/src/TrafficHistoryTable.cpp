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

#include "../inc/TrafficHistoryTable.h"

#include "../inc/ConfigHistoryBasedPolicy.h"

#include "../../common/inc/DebugLog.h"
#include "../../common/inc/csv.h"

#include <iostream>
#include <sstream>
#include <string>

using namespace sc;

// Internal reader class
class TrafficHistoryTableReader {
public:
  void read_header(void) {
    this->mCSVReader->read_header(io::ignore_extra_column, "AppName",
                                  "EventType", "TimeSec", "TrafficSequence");
  }

  bool read_a_row(std::string &app_name, int event_type, float &time_sec,
                  std::vector<int> &traffic_sequence) {
    std::string event_type_str, time_sec_str, traffic_sequence_str;
    bool ret = this->mCSVReader->read_row(app_name, event_type_str,
                                          time_sec_str, traffic_sequence_str);
    if (ret) {
      event_type = std::stoi(event_type_str);
      time_sec = std::stof(time_sec_str);

      std::istringstream sstream(traffic_sequence_str);
      std::string token;
      while (std::getline(sstream, token, ' ')) {
        int traffic = std::stoi(token);
        traffic_sequence.push_back(traffic);
      }
    }

    return ret;
  }

  TrafficHistoryTableReader(std::string filename) {
    this->mCSVReader =
        new io::CSVReader<4, io::trim_chars<>,
                          io::double_quote_escape<',', '\"'>>(filename.c_str());
  }

private:
  io::CSVReader<4, io::trim_chars<>, io::double_quote_escape<',', '\"'>>
      *mCSVReader;
}; /* class TrafficHistoryTableReader */

// Read TrafficHistoryTable file and construct the data structure
void TrafficHistoryTable::initialize(void) {
  std::string filename(TRAFFIC_HISTORY_TABLE_FILENAME);
  TrafficHistoryTableReader reader(filename);
  reader.read_header();

  std::string present_app_name;
  float present_app_start_time_sec = 0.0f;

  int num_rows = 0;
  while (true) {
    std::string app_name;
    int event_type;
    float absolute_time_sec;
    std::vector<int> traffic_sequence;
    if (!reader.read_a_row(app_name, event_type, absolute_time_sec,
                           traffic_sequence)) {
      break;
    }

    if (app_name.compare(present_app_name) != 0) {
      present_app_start_time_sec = absolute_time_sec;
    }
    present_app_name = app_name;
    float relative_time_sec = absolute_time_sec - present_app_start_time_sec;

    AppEntry *app_entry = this->getItem(app_name);
    if (app_entry == NULL) {
      AppEntry new_app_entry(app_name);
      this->addItem(new_app_entry);
      app_entry = this->getItem(app_name);
    }

    EventTypeEntry *event_type_entry = app_entry->getItem(event_type);
    if (event_type_entry == NULL) {
      EventTypeEntry new_event_type_entry(event_type);
      app_entry->addItem(new_event_type_entry);
      event_type_entry = app_entry->getItem(event_type);
    }

    TrafficEntry new_traffic_entry(relative_time_sec, traffic_sequence);
    event_type_entry->addItem(new_traffic_entry);
    num_rows++;
  }

  if (num_rows <= 0) {
    LOG_WARN("No data on traffic history table file!: %s",
             TRAFFIC_HISTORY_TABLE_FILENAME);
  }
}