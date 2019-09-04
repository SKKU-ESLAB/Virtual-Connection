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

#ifndef __BT_CONFIG_H__
#define __BT_CONFIG_H__

/* BT device configuration */
#define DEFAULT_BT_INTERFACE_NAME "hci0"
#define DEFAULT_BT_DEVICE_RFKILL_NAME "hci0"

/* BT throttling */
#define BT_THROTTLING_SLEEP_US 100000
#define BT_THROTTLING_BANDWIDTH_THRESHOLD_NORMAL 75000
#define BT_THROTTLING_BANDWIDTH_THRESHOLD_SWITCH 25000
// #define BT_THROTTLING_ON

#endif /* defined(__BT_CONFIG_H__) */