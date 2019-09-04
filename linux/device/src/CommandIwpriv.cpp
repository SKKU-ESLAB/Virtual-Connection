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

#include "../inc/CommandIwpriv.h"

#include "../../common/inc/DebugLog.h"
#include "../../common/inc/ChildProcess.h"

#include "../../configs/PathConfig.h"

#include <string.h>
#include <stdio.h>

using namespace sc;

int CommandIwpriv::p2p_set_enable(const char* device_name, IwprivP2PRole p2p_role) {
  char enable_str[512];
  snprintf(enable_str, 512, "enable=%d", p2p_role);

  char const *const params[] = {"iwpriv", device_name, "p2p_set", enable_str,
                                NULL};
  int res = ChildProcess::run(IWPRIV_PATH, params, true);
  return res;
}