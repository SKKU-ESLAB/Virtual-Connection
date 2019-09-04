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

#ifndef __COMMAND_WPA_CLI_H__
#define __COMMAND_WPA_CLI_H__

#include <stdio.h>
#include <string>

// wlan_name: wlan device's interface name (ex. wlan0, wlan1, etc.)
// p2p_name: P2P device's interface name (ex. p2p-wlan0-0, wlan1, etc.)
// device_name: Wi-fi P2P device name shown via P2P discovery

namespace sc {
class CommandWpaCli {
public:
  static std::string ping(void);
  static std::string status(const std::string p2p_name);

  static bool get_p2p_name(const std::string wlan_name, std::string &p2p_name);
  static bool p2p_group_add(const std::string wlan_name);
  static bool p2p_group_remove(const std::string wlan_name,
                               const std::string p2p_name);
  static bool get_p2p_info(const std::string p2p_name, std::string &dev_addr,
                           std::string &ssid);

  static std::string p2p_get_passphrase(const std::string p2p_name);
  static std::string get_ip_address(const std::string p2p_name);

  static bool set_device_name(const std::string wlan_name,
                              const std::string device_name);
  static bool set_wps_pin(const std::string dev_name, const std::string pin);
}; /* class CommandWpaCli */
} /* namespace sc */

#endif /* defined(__COMMAND_WPA_CLI_H__) */