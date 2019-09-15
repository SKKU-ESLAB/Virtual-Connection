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

#include "../inc/ContextAwareTPT.h"

#include "../inc/ConfigContextAwarePolicy.h"

#include "../../common/inc/DebugLog.h"
#include "../../common/inc/csv.h"

#include <iostream>
#include <sstream>
#include <string>

using namespace sc;

class ContextAwareTPTReader {
public:
  void read_header(void) {
    this->mCSVReader->read_header(io::ignore_extra_column, "TimeSec", "X", "Y",
                                  "Z", "Bandwidth", "IncFlag",
                                  "TrafficSequence");
  }

  bool read_a_row(float &time_sec, float &x, float &y, float &z,
                  float &bandwidth, bool &is_increase,
                  std::vector<int> &traffic_sequence) {
    std::string time_sec_str, x_str, y_str, z_str, bandwidth_str, inc_flag_str,
        traffic_sequence_str;
    bool ret = this->mCSVReader->read_row(time_sec_str, x_str, y_str, z_str,
                                          bandwidth_str, inc_flag_str,
                                          traffic_sequence_str);
    if (ret) {
      // LOG_VERB("ROW: %s / %s %s %s / %s / %s / %s", time_sec_str.c_str(),
      //          x_str.c_str(), y_str.c_str(), z_str.c_str(),
      //          bandwidth_str.c_str(), inc_flag_str.c_str(),
      //          traffic_sequence_str.c_str());
      time_sec = std::stof(time_sec_str);
      x = std::stof(x_str);
      y = std::stof(y_str);
      z = std::stof(z_str);
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

  ContextAwareTPTReader(std::string filename) {
    this->mCSVReader =
        new io::CSVReader<7, io::trim_chars<>,
                          io::double_quote_escape<',', '\"'>>(filename.c_str());
  }

private:
  io::CSVReader<7, io::trim_chars<>, io::double_quote_escape<',', '\"'>>
      *mCSVReader;
};

// Read ContextAwareTPT file and construct the data structure
void ContextAwareTPT::initialize(void) {
  std::string filename(CONTEXT_AWARE_TPT_FILENAME);
  ContextAwareTPTReader reader(filename);
  reader.read_header();

  int num_rows = 0;
  while (true) {
    float time_sec, bandwidth;
    float x, y, z;
    bool is_increase;
    std::vector<int> traffic_sequence;
    if (!reader.read_a_row(time_sec, x, y, z, bandwidth, is_increase,
                           traffic_sequence)) {
      break;
    }

    BWTrafficEntry new_bw_traffic_entry(time_sec, x, y, z, bandwidth,
                                        is_increase, traffic_sequence);
    this->addItem(new_bw_traffic_entry);
    num_rows++;
  }

  if (num_rows <= 0) {
    LOG_WARN("No data on TPT file!: %s", CONTEXT_AWARE_TPT_FILENAME);
  }
}