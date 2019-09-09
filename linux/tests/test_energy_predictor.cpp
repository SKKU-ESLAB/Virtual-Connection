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

#include "../policy/inc/EnergyPredictor.h"

#include <stdio.h>
#include <stdlib.h>

using namespace sc;

int main(int argc, char **argv) {
  /* Parse arguments */
  if (argc != 4) {
    printf("[Usage] %s <mode (1-BT / 2-WFD / 3-BT-WFD / 4-WFD-BT)>"
           " <throughput (B/s)> <elapsed time (s)>\n",
           argv[0]);
    return -1;
  }

  int mode = atoi(argv[1]);
  int throughput = atoi(argv[2]);
  int elapsed_time = atoi(argv[3]);

  std::vector<int> traffic_seq;
  for (int i = 0; i < elapsed_time; i++) {
    traffic_seq.push_back(throughput);
  }

  float predicted_energy = EnergyPredictor::predictEnergy(0, traffic_seq, mode);
  printf("%dB/s * %ds (mode: %d) => %3.02f\n", elapsed_time, throughput, mode,
         predicted_energy);
  return 0;
}