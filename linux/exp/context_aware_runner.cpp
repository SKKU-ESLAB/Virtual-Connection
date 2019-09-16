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

#include "../common/inc/ChildProcess.h"
#include "../configs/PathConfig.h"

#include "../policy/inc/NMPolicyContextAware.h"
using namespace sc;

#include <string>
#include <thread>

int main(int argc, char **argv) {
  /* Parse arguments */
  if (argc != 3 && argc != 1) {
    printf("[Usage] %s <packet trace file> <context trace file>\n", argv[0]);
    return -1;
  }

  std::string packet_trace_filename("");
  std::string context_trace_filename("");

  if (argc == 1) {
    packet_trace_filename.assign("final-packet-trace.csv");
    context_trace_filename.assign("final-sensor-trace.csv");
  } else {
    packet_trace_filename.assign(argv[1]);
    context_trace_filename.assign(argv[2]);
  }

  TraceRunner *trace_runner = TraceRunner::trace_runner(
      packet_trace_filename, context_trace_filename, true);

  NMPolicyContextAware *switch_policy = new NMPolicyContextAware();
  trace_runner->start(switch_policy);

  delete switch_policy;
  return 0;
}