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

#include "SwitchTestRunner.h"

#include "../common/inc/ChildProcess.h"
#include "../configs/PathConfig.h"
using namespace sc;

#include <string>
#include <thread>

int main(int argc, char **argv) {
  /* Parse arguments */
  if (argc != 3 && argc != 4) {
    printf("[Usage] %s <user-side throughput (B/s)> <# iterations>\n", argv[0]);
    return -1;
  }

  int throughput = atoi(argv[1]);
  int iterations = atoi(argv[2]);

  SwitchTestRunner *test_runner;
  if(throughput <= 0) {
    printf("Invalid throughput!: %d\n", throughput);
    return -1;
  } else if(iterations <= 0) {
    printf("Invalid # iterations!: %d\n", iterations);
    return -1;
  }

  test_runner = SwitchTestRunner::test_runner(throughput, iterations);

  if (test_runner == NULL) {
    return -2;
  }

  test_runner->start();
  return 0;
}