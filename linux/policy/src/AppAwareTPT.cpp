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

#include "../inc/AppAwareTPT.h"

#include "../inc/ConfigAppAwarePolicy.h"

#include "../../common/inc/DebugLog.h"
#include "../../common/inc/csv.h"

#include <iostream>
#include <sstream>
#include <string>

using namespace sc;

class AppAwareTPTReader {
public:
  void read_header(void) {
    this->mCSVReader->read_header(io::ignore_extra_column, "AppName", "TimeSec",
                                  "Bandwidth", "IncFlag", "TrafficSequence");
  }

  bool read_a_row(std::string &app_name, float &time_sec, float &bandwidth,
                  bool &is_increase, std::vector<int> &traffic_sequence) {
    std::string time_sec_str, bandwidth_str, inc_flag_str, traffic_sequence_str;
    bool ret = this->mCSVReader->read_row(app_name, time_sec_str, bandwidth_str,
                                          inc_flag_str, traffic_sequence_str);
    if (ret) {
      LOG_VERB("ROW: %s / %s / %s / %s / %s", app_name.c_str(),
               time_sec_str.c_str(), bandwidth_str.c_str(),
               inc_flag_str.c_str(), traffic_sequence_str.c_str());
      time_sec = std::stof(time_sec_str);
      bandwidth = std::stof(bandwidth_str);
      is_increase = (std::stoi(inc_flag_str) > 0) ? true : false;

      std::istringstream sstream(traffic_sequence_str);
      std::string token;
      while (std::getline(sstream, token, ' ')) {
        int traffic = std::stoi(token);
        traffic_sequence.push_back(traffic);
      }
    }

    return ret;
  }

  AppAwareTPTReader(std::string filename) {
    this->mCSVReader =
        new io::CSVReader<5, io::trim_chars<>,
                          io::double_quote_escape<',', '\"'>>(filename.c_str());
  }

private:
  io::CSVReader<5, io::trim_chars<>, io::double_quote_escape<',', '\"'>>
      *mCSVReader;
};

// Read AppAwareTPT file and construct the data structure
void AppAwareTPT::initialize(void) {
  std::string filename(APP_AWARE_TPT_FILENAME);
  AppAwareTPTReader reader(filename);
  reader.read_header();

  int num_rows = 0;
  while (true) {
    std::string app_name;
    float time_sec, bandwidth;
    bool is_increase;
    std::vector<int> traffic_sequence;
    if (!reader.read_a_row(app_name, time_sec, bandwidth, is_increase,
                           traffic_sequence)) {
      break;
    }

    AppTrafficEntry *app_entry = this->getItem(app_name);
    if (app_entry == NULL) {
      AppTrafficEntry new_app_entry(app_name);
      this->addItem(new_app_entry);
      app_entry = this->getItem(app_name);
    }

    BWTrafficEntry new_bw_traffic_entry(time_sec, bandwidth, is_increase,
                                        traffic_sequence);
    app_entry->addItem(new_bw_traffic_entry);
    num_rows++;
  }

  if (num_rows <= 0) {
    LOG_WARN("No data on TPT file!: %s", APP_AWARE_TPT_FILENAME);
  }
}