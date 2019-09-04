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
#include "../inc/BtDevice.h"

#include "../../common/inc/ChildProcess.h"
#include "../../common/inc/Counter.h"
#include "../../device/inc/CommandRfkill.h"

#include "../../configs/BtConfig.h"

#include <mutex>
#include <thread>

#include <stdio.h>

using namespace sc;

bool BtDevice::turn_on_impl(void) {
  // int res = CommandRfkill::unblock_device(DEFAULT_BT_DEVICE_RFKILL_NAME);
  // if (res < 0)
  //   return false;
  // if(this->mIsTurnedOnOnce == true) {
  //   return true;
  // } else {
    int res;

    char buf[512];
    char const *const params1[] = {"hciconfig", DEFAULT_BT_INTERFACE_NAME, "up",
                                  "piscan", NULL};

    res = ChildProcess::run(HCICONFIG_PATH, params1, buf, 512, true);

    this->mIsTurnedOnOnce = true;
    return (res >= 0);
  // }
}

bool BtDevice::turn_off_impl(void) {
  // If Bluetooth device is turned off once, the initial bandwidth of Wi-fi direct is degraded.
  // On the other hand, keeping Bluetooth device powered on does not incur high power consumption.
  // Therefore, keeping Bluetooth device powered on and switching only Wi-fi direct on/off is better method.

  // char buf[512];
  // char const *const params1[] = {"hciconfig", DEFAULT_BT_INTERFACE_NAME, "down",
  //                                NULL};
  // int res = ChildProcess::run(HCICONFIG_PATH, params1, buf, 512, true);
  // if (res < 0)
  //   return res;

  // res = CommandRfkill::block_device(DEFAULT_BT_DEVICE_RFKILL_NAME);
  // if (res < 0)
  //   return false;

  // char const *const params2[] = {"invoke-rc.d", "bluetooth", "restart",
  // NULL}; res = ChildProcess::run(INVOKE_RC_D_PATH, params2, buf, 512, true);

  // return (res >= 0);
  return true;
}