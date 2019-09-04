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

#ifndef __NETWORK_SWITCHER_CONFIG_H__
#define __NETWORK_SWITCHER_CONFIG_H__

#define NETWORK_MONITOR_SLEEP_USECS (100 * 1000)

/* Max Bandwidth (B/s; float) */
#define MAX_BANDWIDTH_BT 175413.0f
#define MAX_BANDWIDTH_WFD 2031406.0f
#define MAX_BANDWIDTH_BT_ON 759893.0f
#define MAX_BANDWIDTH_BT_OFF 104674.0f
#define MAX_BANDWIDTH_WFD_ON 104674.0f
#define MAX_BANDWIDTH_WFD_OFF 286047.0f

/* Switch Latency (sec; float) */
#define LATENCY_BT_ON 3.400f
#define LATENCY_BT_OFF 0.661f
#define LATENCY_WFD_ON 3.363f
#define LATENCY_WFD_OFF 1.660f

/* Power (mW; float) */
#define POWER_BT_TRANSFER 327.88f
#define POWER_WFD_TRANSFER 950.72f
#define POWER_BT_ON 348.20f
#define POWER_BT_OFF 212.64f
#define POWER_WFD_ON 302.38f
#define POWER_WFD_OFF 312.72f
#define POWER_BT_IDLE 68.88f
#define POWER_WFD_IDLE 180.93f

#endif /* !defined(__NETWORK_SWITCHER_CONFIG_H__) */