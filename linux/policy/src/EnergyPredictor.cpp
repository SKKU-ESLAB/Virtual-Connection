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

#include "../inc/EnergyPredictor.h"

#include <limits>

// #define PRINT_TRAFFIC_SEQ 1

using namespace sc;

float EnergyPredictor::predictEnergy(int segment_queue_length,
                                     std::vector<int> &traffic_seq,
                                     int scenario_id) {
  // Step 1. Get total predicted bandwidth sequence
  //   (segment queue length + predicted traffic sequence)
  std::vector<int> bandwidth_seq;
  bool res = predictBandwidthSeq(segment_queue_length, traffic_seq, scenario_id,
                                 bandwidth_seq);
  if (!res) {
    LOG_ERR("Failed to predict energy");
    return -1.0f;
  }

#if PRINT_TRAFFIC_SEQ == 1
  printf("Traffic Seq: ");
  for (std::vector<int>::iterator it = traffic_seq.begin();
       it != traffic_seq.end(); it++) {
    printf("%d ", *it);
  }
  printf("\n");

  printf("Bandwidth Seq: ");
  for (std::vector<int>::iterator it = bandwidth_seq.begin();
       it != bandwidth_seq.end(); it++) {
    printf("%d ", *it);
  }
  printf("\n");
#endif

  // Step 2. Get predicted energy (apply energy prediction regression model)
  float predicted_energy = _predictEnergy(bandwidth_seq, scenario_id);
  return predicted_energy;
}

bool EnergyPredictor::predictBandwidthSeq(int segment_queue_length,
                                          std::vector<int> &traffic_seq,
                                          int scenario_id,
                                          std::vector<int> &bandwidth_seq) {
  if (!bandwidth_seq.empty()) {
    LOG_WARN("bandwidth_seq is not empty!");
    return false;
  }

  // Simulate the scenario and predict bandwidth
  int present_adapter_id = getInitialAdapter(scenario_id);
  if (present_adapter_id <= 0) {
    LOG_WARN("invalid adapter id or scenario id!");
    return false;
  }
  int remainder_data_size = segment_queue_length;
  float remain_switch_latency = getSwitchLatency(scenario_id);
  bool is_switching = (remain_switch_latency > 0.0f) ? true : false;
  bool is_delayed_switch = false; // WFD b.w. should be applied delayingly
  for (std::vector<int>::iterator it = traffic_seq.begin();
       it != traffic_seq.end(); it++) {
    // Predict switch
    if (is_switching) {
      remain_switch_latency = remain_switch_latency - 1.0f;
      if (remain_switch_latency < 0.0f) {
        if (present_adapter_id == ADAPTER_BT_TO_WFD) {
          is_delayed_switch = true;
        } else if (present_adapter_id == ADAPTER_WFD_TO_BT) {
          present_adapter_id = ADAPTER_BT;
        }
        is_switching = false;
      }
    } else if (is_delayed_switch) {
      present_adapter_id = ADAPTER_WFD;
      is_delayed_switch = false;
    }

    int traffic = *(it);

    // Predict bandwidth of a time item
    int data_prepared = remainder_data_size + traffic;
    int bandwidth = (data_prepared < getMaxBW(present_adapter_id))
                        ? data_prepared
                        : getMaxBW(present_adapter_id);
    remainder_data_size = data_prepared - bandwidth;
    bandwidth_seq.push_back(bandwidth);
  }

  // Remainder data will be distributed to extra timeslot
  while (remainder_data_size > 0) {
    int data_prepared = remainder_data_size; // assume that no data is generated
    int bandwidth = (data_prepared < getMaxBW(present_adapter_id))
                        ? data_prepared
                        : getMaxBW(present_adapter_id);
    remainder_data_size = data_prepared - bandwidth;
    bandwidth_seq.push_back(bandwidth);
  }
  return true;
}

float EnergyPredictor::_predictEnergy(std::vector<int> &bandwidth_seq,
                                      int scenario_id) {
  // Simulate the scenario and predict energy
  int present_adapter_id = getInitialAdapter(scenario_id);
  if (present_adapter_id <= 0) {
    LOG_WARN("invalid adapter id or scenario id!");
    return -1.0f;
  }

  if (bandwidth_seq.size() > PREDICTION_WINDOW_SEC) {
    return std::numeric_limits<float>::max();
  }

  float total_energy = 0.0f;
  float remain_switch_latency = getSwitchLatency(scenario_id);
  bool is_switching = (remain_switch_latency > 0.0f) ? true : false;
  for (int bandwidth : bandwidth_seq) {
    // Predict unit energy
    float power = getPower(bandwidth, present_adapter_id);
    if (power < 0.0f) {
      LOG_WARN("Invalid power: bandwidth %d - scenario %d - adapter %d - %f",
               bandwidth, scenario_id, present_adapter_id, power);
      return -1.0f;
    }

    // Predict switch and update energy
    bool is_switch_unit = false;
    if (is_switching) {
      remain_switch_latency = remain_switch_latency - 1.0f;
      if (remain_switch_latency < 0.0f) {
        if (present_adapter_id == ADAPTER_BT_TO_WFD) {
          present_adapter_id = ADAPTER_WFD;
          is_switch_unit = true;
        } else if (present_adapter_id == ADAPTER_WFD_TO_BT) {
          present_adapter_id = ADAPTER_BT;
          is_switch_unit = true;
        }
        is_switching = false;
      }
    }

    // Update energy
    if (is_switch_unit) {
      // If switch occurs, use different power for the unit energy
      float time_before_switch = remain_switch_latency + 1.0f;
      float time_after_switch = 1.0f - time_before_switch;
      float power_after_switch = getPower(bandwidth, present_adapter_id);
      total_energy = total_energy + (power * time_before_switch) +
                     (power_after_switch * time_after_switch);
    } else {
      total_energy = total_energy + power;
    }
  }
  if (total_energy < 0.0f) {
    total_energy = 0.0f;
  }
  return total_energy;
}

float EnergyPredictor::getMaxBW(int adapter_id) {
  switch (adapter_id) {
  case ADAPTER_BT:
    return MAX_BANDWIDTH_BT;
    break;
  case ADAPTER_WFD:
    return MAX_BANDWIDTH_WFD;
    break;
  case ADAPTER_BT_TO_WFD:
    return MAX_BANDWIDTH_BT_TO_WFD;
    break;
  case ADAPTER_WFD_TO_BT:
    return MAX_BANDWIDTH_WFD_TO_BT;
    break;
  default:
    return -1.0f;
    break;
  }
}

float EnergyPredictor::getPower(int bandwidth, int adapter_id) {
  float bw_kb = (float)bandwidth / 1000.0f;
  float b0 = 0.0f, b1 = 0.0f, b2 = 0.0f;
  switch (adapter_id) {
  case ADAPTER_BT:
    b0 = RM_BT_B0;
    b1 = RM_BT_B1;
    b2 = RM_BT_B2;
    break;
  case ADAPTER_WFD:
    b0 = RM_WFD_B0;
    b1 = RM_WFD_B1;
    b2 = RM_WFD_B2;
    break;
  case ADAPTER_BT_TO_WFD:
    b0 = RM_BT_TO_WFD_B0;
    b1 = RM_BT_TO_WFD_B1;
    b2 = RM_BT_TO_WFD_B2;
    break;
  case ADAPTER_WFD_TO_BT:
    b0 = RM_WFD_TO_BT_B0;
    b1 = RM_WFD_TO_BT_B1;
    b2 = RM_WFD_TO_BT_B2;
    break;
  default:
    return -1.0f;
    break;
  }
  return b0 + (b1 * bw_kb) + (b2 * bw_kb * bw_kb);
}

int EnergyPredictor::getInitialAdapter(int scenario_id) {
  switch (scenario_id) {
  case SCENARIO_BT:
    return SCENARIO_BT;
    break;
  case SCENARIO_WFD:
    return SCENARIO_WFD;
    break;
  case SCENARIO_BT_TO_WFD:
    return SCENARIO_BT_TO_WFD;
    break;
  case SCENARIO_WFD_TO_BT:
    return SCENARIO_WFD_TO_BT;
    break;
  default:
    return -1;
    break;
  }
}

float EnergyPredictor::getSwitchLatency(int scenario_id) {
  if (scenario_id == SCENARIO_BT_TO_WFD) {
    return LATENCY_BT_TO_WFD;
  } else if (scenario_id == SCENARIO_WFD_TO_BT) {
    return LATENCY_WFD_TO_BT;
  } else {
    return -1.0f;
  }
}