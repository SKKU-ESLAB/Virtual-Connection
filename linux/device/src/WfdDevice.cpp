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
#include "../inc/WfdDevice.h"

#include "../inc/CommandIwpriv.h"

#include "../../common/inc/ChildProcess.h"
#include "../../common/inc/Counter.h"

#include "../../configs/WfdConfig.h"

#include <mutex>
#include <thread>

#include <stdio.h>

using namespace sc;

bool WfdDevice::turn_on_impl(void) {
  // this->ifup();
  // this->ifconfig_up();

#if CONFIG_REALTEK_MODE == 1
  // RTL81xx series require iwpriv setting for Wi-fi P2P.
  CommandIwpriv::p2p_set_enable(DEFAULT_WFD_INTERFACE_NAME,
                                IwprivP2PRole::kP2PGroupOwner);
#endif

  return true;
}

bool WfdDevice::turn_off_impl(void) {
  // this->ifconfig_down();
  // this->ifdown();
  return true;
}

bool WfdDevice::ifup() {
  char const *const params[] = {"ifup", DEFAULT_WFD_INTERFACE_NAME, NULL};
  sleep(1);
  return ChildProcess::run(IFUP_PATH, params, true);
}

bool WfdDevice::ifdown() {
  char const *const params[] = {"ifdown", DEFAULT_WFD_INTERFACE_NAME, NULL};

  return ChildProcess::run(IFDOWN_PATH, params, true);
}

bool WfdDevice::ifconfig_up() {
  char const *const params[] = {"ifconfig", DEFAULT_WFD_INTERFACE_NAME, "up",
                                NULL};
  sleep(1);
  return ChildProcess::run(IFCONFIG_PATH, params, true);
}

bool WfdDevice::ifconfig_down() {
  char const *const params[] = {"ifconfig", DEFAULT_WFD_INTERFACE_NAME, "down",
                                NULL};

  return ChildProcess::run(IFCONFIG_PATH, params, true);
}