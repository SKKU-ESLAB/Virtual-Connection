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

#include "TestRunner.h"

#include "../common/inc/ChildProcess.h"
#include "../configs/PathConfig.h"
using namespace sc;

#include <string>
#include <thread>

int main(int argc, char **argv) {
  /* Parse arguments */
  if (argc != 3 && argc != 4) {
    printf("[Usage] %s <send when?> <user-side throughput (B/s)>\n", argv[0]);
    printf("  - on BT: 0\n");
    printf("  - on WFD: 1\n");
    printf("  - BT -> WFD: 2\n");
    printf("  - WFD -> BT: 3\n");
    return -1;
  }

  int send_when = atoi(argv[1]);
  int throughput = atoi(argv[2]);

  TestRunner *test_runner;
  if (send_when < 0 || send_when > 3) {
    printf("Invalid send-when!: %d\n", send_when);
    return -1;
  } else if(throughput <= 0) {
    printf("Invalid throughput!: %d\n", throughput);
    return -1;
  }

  test_runner = TestRunner::test_runner(send_when, throughput);

  if (test_runner == NULL) {
    return -2;
  }

  test_runner->start();
  return 0;
}