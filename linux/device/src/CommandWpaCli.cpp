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

#include "../inc/CommandWpaCli.h"

#include "../../common/inc/ChildProcess.h"
#include "../../common/inc/DebugLog.h"

#include "../../configs/WfdConfig.h"

#include <stdio.h>
#include <string.h>
#include <string>

using namespace sc;

bool CommandWpaCli::set_device_name(const std::string wlan_name,
                                    const std::string device_name) {
  assert(!wlan_name.empty() && !device_name.empty());

  char const *const params[] = {"wpa_cli", "-i",          wlan_name.c_str(),
                                "set",     "device_name", device_name.c_str(),
                                NULL};
  char buf[512];
  int ret = ChildProcess::run(WPA_CLI_PATH, params, buf, 512, true);
  return (ret >= 0);
}

std::string CommandWpaCli::ping(void) {
  // Selected interface 'p2p-wlan0-0' is expected
  char const *const params[] = {"wpa_cli", "ping", NULL};
  char buf[512];
  int ret = ChildProcess::run(WPA_CLI_PATH, params, buf, 512, true);
  if (ret < 0) {
    return std::string("");
  } else {
    return std::string(buf);
  }
}

std::string CommandWpaCli::status(const std::string p2p_name) {
  assert(!p2p_name.empty());

  char const *const params[] = {"wpa_cli", "-i", p2p_name.c_str(), "status",
                                NULL};
  char buf[512];
  int ret = ChildProcess::run(WPA_CLI_PATH, params, buf, 512, true);
  if (ret < 0) {
    return std::string("");
  } else {
    return std::string(buf);
  }
}

bool CommandWpaCli::get_p2p_name(const std::string wlan_name,
                                 std::string &p2p_name) {
  // In the case of Realtek Wi-fi Module, it uses 'wlanX'.
  // In the case of Broadcom Wi-fi Module, it uses 'p2p-wlanX-Y'.
  assert(!wlan_name.empty());

  std::string ping_ret = ping();
  if (ping_ret.empty())
    return false;
  LOG_DEBUG("Ping return: %s", ping_ret.c_str());

  char buf[512];
  strncpy(buf, ping_ret.c_str(), 512);

  char *ptrptr;
  char *ptr = strtok_r(buf, "\t \n\'", &ptrptr);
  while (ptr != NULL) {
    if (strstr(ptr, "p2p-wlan")) {
      p2p_name.assign(ptr);
      return true;
    } else if (strstr(ptr, "FAIL")) {
      return false;
    }
    ptr = strtok_r(NULL, "\t \n\'", &ptrptr);
  }

  p2p_name.assign(wlan_name);
  return true;
}

bool CommandWpaCli::p2p_group_add(const std::string wlan_name) {
  assert(!wlan_name.empty());

#if CONFIG_REALTEK_MODE == 1
  char const *const params[] = {"wpa_cli",         "-i",
                                wlan_name.c_str(), "p2p_group_add",
                                "persistent=0",    NULL};
#else
  char const *const params[] = {"wpa_cli", "-i", wlan_name.c_str(),
                                "p2p_group_add", NULL};
#endif
  char buf[512];
  int ret = ChildProcess::run(WPA_CLI_PATH, params, buf, 512, true);
  if (ret < 0) {
    return false;
  }

  if (strstr(buf, "OK")) {
    return true;
  } else {
    return false;
  }
}

bool CommandWpaCli::p2p_group_remove(const std::string wlan_name,
                                     const std::string p2p_name) {
  assert(!wlan_name.empty() && !p2p_name.empty());

  char const *const params[] = {"wpa_cli",         "-i",
                                wlan_name.c_str(), "p2p_group_remove",
                                p2p_name.c_str(),  NULL};
  char buf[512];
  int ret = ChildProcess::run(WPA_CLI_PATH, params, buf, 512, true);
  if (ret < 0) {
    return false;
  }

  if (strstr(buf, "OK")) {
    return true;
  } else {
    return false;
  }
}

bool CommandWpaCli::get_p2p_info(const std::string p2p_name,
                                 std::string &dev_addr, std::string &ssid) {
  assert(!p2p_name.empty());
  std::string status_ret = CommandWpaCli::status(p2p_name);
  if (status_ret.empty()) {
    return false;
  }

  // WFD Device Address
  {
    int header_pos = status_ret.find("p2p_device_address=");
    if (header_pos == std::string::npos) {
      LOG_WARN("Get p2p device address failed");
      return -1;
    }

    int target_pos = header_pos + strlen("p2p_device_address=");
    int end_pos = status_ret.find("\n", target_pos);
    int target_length = end_pos - target_pos;
    std::string target_str = status_ret.substr(target_pos, target_length);
    dev_addr.assign(target_str);
  }

  // WFD SSID
  {
    int header_pos = status_ret.find("\nssid=");
    if (header_pos == std::string::npos) {
      ssid.assign("any");
      return true;
    }

    int target_pos = header_pos + 1 + strlen("ssid=");
    int end_pos = status_ret.find("\n", target_pos);
    int target_length = end_pos - target_pos;
    std::string target_str = status_ret.substr(target_pos, target_length);
    ssid.assign(target_str);
  }

  return true;
}

bool CommandWpaCli::set_wps_pin(const std::string dev_name,
                                const std::string pin) {
  char const *const params[] = {
      "wpa_cli", "-i", dev_name.c_str(), "wps_pin", "any", pin.c_str(), NULL};

  int ret = ChildProcess::run(WPA_CLI_PATH, params, true);
  return (ret >= 0);
}

#define PASSPHRASE_LENGTH 8
std::string CommandWpaCli::p2p_get_passphrase(const std::string p2p_name) {
  assert(!p2p_name.empty());

  char const *const params[] = {"wpa_cli", "-i", p2p_name.c_str(),
                                "p2p_get_passphrase", NULL};
  char buf[512];
  int res = ChildProcess::run(WPA_CLI_PATH, params, buf, 512, true);
  if (res < 0) {
    return std::string("");
  }

  buf[PASSPHRASE_LENGTH] = '\0';
  return std::string(buf);
}

std::string CommandWpaCli::get_ip_address(const std::string p2p_name) {
  assert(!p2p_name.empty());

  std::string status_ret = CommandWpaCli::status(p2p_name);
  if (status_ret.empty())
    return std::string("");

  char buf[512];
  strncpy(buf, status_ret.c_str(), 512);

  char *ptrptr;
  char *ptr = strtok_r(buf, "\t \n\'", &ptrptr);
  while (ptr != NULL) {
    if (strstr(ptr, "ip_address") || strstr(ptr, "p2p_device_address")) {
      sscanf(ptr, "%*[^=]=%s", buf);
      return std::string(buf);
    }

    ptr = strtok_r(NULL, "\t \n\'", &ptrptr);
  }

  LOG_WARN("Get IP address failed");
  return std::string("");
}