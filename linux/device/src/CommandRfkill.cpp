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

#include "../inc/CommandRfkill.h"

#include "../../common/inc/DebugLog.h"
#include "../../common/inc/ChildProcess.h"

#include "../../configs/PathConfig.h"

#include <string.h>

using namespace sc;

int CommandRfkill::block_device(const char* device_name) {
  std::string rfkill_number = CommandRfkill::get_rfkill_number_str(device_name);
  if(rfkill_number.empty()) {
    LOG_ERR("Cannot achieve rfkill number of %s!", device_name);
    return false;
  }
  char const *const params[] = {"rfkill", "block", rfkill_number.c_str(),
                                NULL};
  int res = ChildProcess::run(RFKILL_PATH, params, true);
  return res;
}

int CommandRfkill::unblock_device(const char* device_name) {
  std::string rfkill_number = CommandRfkill::get_rfkill_number_str(device_name);
  if(rfkill_number.empty()) {
    LOG_ERR("Cannot achieve rfkill number of %s!", device_name);
    return false;
  }
  char const *const params[] = {"rfkill", "unblock", rfkill_number.c_str(),
                                NULL};
  int res = ChildProcess::run(RFKILL_PATH, params, true);
  return res;
}

std::string CommandRfkill::get_rfkill_number_str(const char* device_name) {
  std::string rfkill_number("");
  char buf[512];

  char const *const params[] = {"rfkill", "list", NULL};
  int res = ChildProcess::run(RFKILL_PATH, params, buf, 512, true);
  if (res < 0)
    return rfkill_number;

  const char separator[] = "\n";
  char *ptr = strtok(buf, separator);
  while (ptr != NULL) {
    if (strstr(ptr, device_name)) {
      const char separator2[] = ": ";
      char *ptr2 = strtok(ptr, separator2);
      if (ptr != NULL) {
        rfkill_number.assign(ptr2);
        return rfkill_number;
      }
    }
    ptr = strtok(NULL, separator);
  }
  return rfkill_number;
}