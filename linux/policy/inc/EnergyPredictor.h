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
#ifndef __ENERGY_PREDICTOR_H__
#define __ENERGY_PREDICTOR_H__

#include "../../common/inc/DebugLog.h"

#include <string>
#include <vector>

#define PREDICTION_WINDOW_SEC 30

/* Switch scenario case */
#define SCENARIO_BT 1
#define SCENARIO_WFD 2
#define SCENARIO_BT_TO_WFD 3
#define SCENARIO_WFD_TO_BT 4

/* Adapter status */
#define ADAPTER_BT 1
#define ADAPTER_WFD 2
#define ADAPTER_BT_TO_WFD 3
#define ADAPTER_WFD_TO_BT 4

/* Measured switch latency */
#define LATENCY_BT_TO_WFD 5.4015f
#define LATENCY_WFD_TO_BT 6.8679f

/* Measured maximum bandwidth (B/s) */
#define MAX_BANDWIDTH_BT 75000.0f
#define MAX_BANDWIDTH_WFD 3500000.0f
#define MAX_BANDWIDTH_BT_TO_WFD 200000.0f
#define MAX_BANDWIDTH_WFD_TO_BT 3500000.0f

/* Power regression model */
#define RM_BT_B0 20.56f
#define RM_BT_B1 1.756f
#define RM_BT_B2 -0.001641

#define RM_WFD_B0 143.8f
#define RM_WFD_B1 0.1964f
#define RM_WFD_B2 -0.000006769f

#define RM_BT_TO_WFD_B0 217.7f
#define RM_BT_TO_WFD_B1 4.433f
#define RM_BT_TO_WFD_B2 -0.02262f

#define RM_WFD_TO_BT_B0 203.3f
#define RM_WFD_TO_BT_B1 0.1869f
#define RM_WFD_TO_BT_B2 -0.00001695f

namespace sc {
class EnergyPredictor {
public:
  static float predictEnergy(int segment_queue_length,
                             std::vector<int> &traffic_seq, int scenario_id);

private:
  static bool predictBandwidthSeq(int segment_queue_length,
                                  std::vector<int> &traffic_seq,
                                  int scenario_id,
                                  std::vector<int> &bandwidth_seq);
  static float _predictEnergy(std::vector<int> &bandwidth_seq, int scenario_id);

private:
  static float getMaxBW(int adapter_id);
  static float getPower(int bandwidth, int adapter_id);
  static int getInitialAdapter(int scenario_id);
  static float getSwitchLatency(int scenario_id);
  static float getSwitchBackLatency(int scenario_id);
}; /* class EnergyPredictor */
} /* namespace sc */

#endif /* !defined(__ENERGY_PREDICTOR_H__) */