
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

#include "../inc/NetworkInitializer.h"

#include "../inc/CommandRfkill.h"

#include "../../common/inc/ChildProcess.h"
#include "../../common/inc/DebugLog.h"

#include "../../configs/BtConfig.h"
#include "../../configs/ExpConfig.h"
#include "../../configs/PathConfig.h"
#include "../../configs/WfdConfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

using namespace sc;

void NetworkInitializer::initialize(void) {
  char cmdLine[500] = {
      0,
  };

  // Step 1. Bluetooth OFF
  LOG_VERB("Init (1/3): BT OFF");

  CommandRfkill::block_device(DEFAULT_BT_DEVICE_RFKILL_NAME);

  snprintf(cmdLine, 500, "%s %s down", HCICONFIG_PATH,
           DEFAULT_BT_INTERFACE_NAME);
  system(cmdLine);

  // Step 2. Bluetooth ON
  LOG_VERB("Init (2/3): BT ON");

  CommandRfkill::unblock_device(DEFAULT_BT_DEVICE_RFKILL_NAME);

  snprintf(cmdLine, 500, "%s %s up piscan", HCICONFIG_PATH,
           DEFAULT_BT_INTERFACE_NAME);
  system(cmdLine);

  // Step 3. Wi-fi Direct OFF
  LOG_VERB("Init (3/4). Wi-fi Direct OFF");

  snprintf(cmdLine, 500, "killall udhcpd");
  system(cmdLine);

  // snprintf(cmdLine, 500, "%s %s down", IFCONFIG_PATH,
  //          DEFAULT_WFD_INTERFACE_NAME);
  // system(cmdLine);

#if CONFIG_REALTEK_MODE == 1
#else
  // snprintf(cmdLine, 500, "%s %s", IFDOWN_PATH, DEFAULT_WFD_INTERFACE_NAME);
  // system(cmdLine);
#endif

  // Step 4. Wi-fi ON
  LOG_VERB("Init (4/4). Wi-fi Direct ON");

#if CONFIG_REALTEK_MODE == 1
#else
  // snprintf(cmdLine, 500, "%s %s", IFUP_PATH,
  //          DEFAULT_WFD_INTERFACE_NAME);
  // system(cmdLine);
#endif

  // snprintf(cmdLine, 500, "%s %s up", IFCONFIG_PATH, DEFAULT_WFD_INTERFACE_NAME);
  // system(cmdLine);

#if CONFIG_REALTEK_MODE == 1
  // Restart wpa_supplicant
  snprintf(cmdLine, 500, "%s wpa_supplicant", KILLALL_PATH);
  system(cmdLine);

  sleep(3);

  FILE *p2p_conf_file = NULL;
  p2p_conf_file = fopen("p2p.conf", "w");
  if (p2p_conf_file == NULL) {
    LOG_ERR("Cannot write p2p.conf file");
    return;
  }
  fprintf(p2p_conf_file, "ctrl_interface=/var/run/wpa_supplicant \
  \nap_scan=1 \
  \ndevice_name=SelCon \
  \ndevice_type=1-0050F204-1 \
  \ndriver_param=p2p_device=1 \
  \n\nnetwork={ \
  \n\tmode=3 \
  \n\tdisabled=2 \
  \n\tssid=\"DIRECT-SelCon\" \
  \n\tkey_mgmt=WPA-PSK \
  \n\tproto=RSN \
  \n\tpairwise=CCMP \
  \n\tpsk=\"12345670\" \
  \n}");
  fclose(p2p_conf_file);
  
  snprintf(cmdLine, 500, "%s -Dnl80211 -iwlan1 -cp2p.conf -Bd",
           WPA_SUPPLICANT_PATH);
  system(cmdLine);
#endif
}

int NetworkInitializer::ping_wpa_cli(char ret[], size_t len) {
  char const *const params[] = {"wpa_cli", "-i", DEFAULT_WFD_INTERFACE_NAME,
                                "ping", NULL};

  return ChildProcess::run(WPA_CLI_PATH, params, ret, len, true);
}

void NetworkInitializer::retrieve_wpa_interface_name(std::string &wpaIntfName) {
  char wpaIntfNameCstr[100];
  char buf[1024];

  // In the case of Wi-fi USB Dongle, it uses 'wlanX'.
  snprintf(wpaIntfNameCstr, sizeof(wpaIntfNameCstr), DEFAULT_WFD_INTERFACE_NAME,
           strlen(DEFAULT_WFD_INTERFACE_NAME));

  // In the case of Raspberry Pi 3 Internal Wi-fi Module, it uses 'p2p-wlanX-Y'.
  int ret = this->ping_wpa_cli(buf, 1024);
  if (ret < 0) {
    LOG_ERR("P2P ping call failed");
    return;
  } else {
    char *ptrptr;
    char *ptr = strtok_r(buf, "\t \n\'", &ptrptr);
    while (ptr != NULL) {
      if (strstr(ptr, "p2p-wlan")) {
        snprintf(wpaIntfNameCstr, sizeof(wpaIntfNameCstr), "%s", ptr);
      } else if (strstr(ptr, "FAIL")) {
        LOG_ERR("P2P ping failed");
        return;
      }
      ptr = strtok_r(NULL, "\t \n\'", &ptrptr);
    }
  }
  wpaIntfName.assign(wpaIntfNameCstr, strlen(wpaIntfNameCstr));
}