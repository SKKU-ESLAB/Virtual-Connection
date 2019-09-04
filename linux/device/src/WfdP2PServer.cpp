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
#include "../inc/WfdP2PServer.h"

#include "../inc/CommandWpaCli.h"
#include "../inc/WfdServerAdapter.h"

#include "../../common/inc/ChildProcess.h"
#include "../../common/inc/Counter.h"
#include "../../common/inc/DebugLog.h"

#include "../../core/inc/Core.h"

#include "../../configs/WfdConfig.h"

#include <mutex>
#include <thread>

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace sc;

struct sigaction WfdP2PServer::sSigaction;
struct sigaction WfdP2PServer::sSigactionOld;
bool WfdP2PServer::sDhcpdMonitoring = false;
int WfdP2PServer::sDhcpdPid = 0;
WfdP2PServer *WfdP2PServer::sDhcpdCaller = NULL;
bool WfdP2PServer::sDhcpdEnabled = false;

bool WfdP2PServer::allow_discover_impl(void) {
  char buf[1024];
  bool ret;

  // Set Wi-fi WPS Device Name
  ret = CommandWpaCli::set_device_name(DEFAULT_WFD_INTERFACE_NAME,
                                       this->mDeviceName);
  if (!ret)
    return false;

  // Add Wi-fi Direct P2P Group
  ret = CommandWpaCli::p2p_group_add(DEFAULT_WFD_INTERFACE_NAME);
  if (!ret) {
    LOG_ERR("allow_discover(%s): p2p_group_add error", this->get_name());
    return false;
  }

  // Retrieve WPA Interface Name from wpa-cli
  std::string p2p_name;
  ret = CommandWpaCli::get_p2p_name(DEFAULT_WFD_INTERFACE_NAME, p2p_name);
  if (!ret) {
    LOG_ERR("allow_discover(%s): get_p2p_name error", this->get_name());
    return false;
  } else {
    this->mP2PName.assign(p2p_name);
  }
  LOG_DEBUG("P2P Name: %s", p2p_name.c_str());

  // Set Wi-fi Direct IP
  std::string default_ip_addr(DEFAULT_WFD_IP_ADDRESS);
  this->set_wfd_ip_addr(default_ip_addr);

  // Set DHCPD config
  WfdP2PServer::sDhcpdCaller = this;
  WfdP2PServer::sDhcpdPid = this->launch_dhcpd();

  // Retrieve and Send Wi-fi Direct Information
  std::string p2p_dev_addr, ssid, ip_addr;
  {
    char wfdInfo[1024] = {
        '\0',
    };

    // MAC Address & SSID
    ret = CommandWpaCli::get_p2p_info(p2p_name, p2p_dev_addr, ssid);
    if (!ret) {
      LOG_ERR("allow_discover(%s): get_wfd_p2p_device_addr error",
              this->get_name());
      return false;
    }

    snprintf(wfdInfo, 1024, "%s", p2p_dev_addr.c_str());
    strncat(wfdInfo, "\n", 1024);
    strncat(wfdInfo, ssid.c_str(), 1024);

    // Passphrase
    std::string passphrase = CommandWpaCli::p2p_get_passphrase(p2p_name);
    strncat(wfdInfo, "\n", 1024);
    strncat(wfdInfo, passphrase.c_str(), 1024);

    // WPS PIN
#define DEFAULT_WPS_PIN "12345670"
    ret = CommandWpaCli::set_wps_pin(p2p_name, DEFAULT_WPS_PIN);
    if (!ret) {
      LOG_ERR("allow_discover(%s): set_wps_pin error", this->get_name());
      return false;
    }
    strncat(wfdInfo, "\n", 1024);
    strncat(wfdInfo, DEFAULT_WPS_PIN, 1024);

    // Check if dhcpd is launched again
    if (WfdP2PServer::sDhcpdPid == 0) {
      LOG_DEBUG("%s: UDHCP is off > turn it up", this->get_name());
      WfdP2PServer::sDhcpdPid = this->launch_dhcpd();
    }

    // IP Address
    for (int ip_wait_it = 0; ip_wait_it < 30; ip_wait_it++) {
      ip_addr = CommandWpaCli::get_ip_address(p2p_name);
      if (!ip_addr.empty()) {
        break;
      }
      usleep(300000);
    }
    if (ip_addr.empty()) {
      LOG_ERR("allow_discover(%s): get_ip_address error", this->get_name());
      return false;
    }

    strncat(wfdInfo, "\n", 1024);
    strncat(wfdInfo, ip_addr.c_str(), 1024);

/* Send WFD Info
 *  <Server MAC Address>
 *  <WPS PIN>
 *  <Server IP Address>
 */
#ifndef EXP_DONT_SEND_PRIV_CONTROL_MESSAGE
    Core::singleton()->get_control_sender()->send_noti_private_data(
        PrivType::kPrivTypeWFDInfo, wfdInfo, strlen(wfdInfo));
#endif

    // Notify IP address to the listeners
    for (std::vector<WfdIpAddressListener *>::iterator it =
             this->mIpAddrListeners.begin();
         it != this->mIpAddrListeners.end(); it++) {
      WfdIpAddressListener *listener = (*it);
      listener->on_change_ip_address(ip_addr.c_str());
    }
  }

  return true;
}

int WfdP2PServer::set_wfd_ip_addr(std::string ip_addr) {
  assert(!this->mP2PName.empty());

  char const *const params[] = {"ifconfig", this->mP2PName.c_str(),
                                ip_addr.c_str(), NULL};
  char buf[256];

  return ChildProcess::run(IFCONFIG_PATH, params, buf, 256, true);
}

int WfdP2PServer::launch_dhcpd(void) {
  WfdP2PServer::sDhcpdEnabled = true;

  if (!WfdP2PServer::sDhcpdMonitoring) {
    WfdP2PServer::sSigaction.sa_flags = SA_SIGINFO;
    WfdP2PServer::sSigaction.sa_sigaction =
        WfdP2PServer::sighandler_monitor_udhcpd;
    sigaction(SIGCHLD, &WfdP2PServer::sSigaction, &WfdP2PServer::sSigactionOld);

    WfdP2PServer::sDhcpdMonitoring = true;
  }

  char const *const params[] = {"udhcpd", UDHCPD_CONFIG_PATH, "-f", NULL};

  // Generate dhcp configuration
  int config_fd = open(UDHCPD_CONFIG_PATH, O_CREAT | O_WRONLY | O_TRUNC, 0644);

  // Prevent file descriptor inheritance
  int fd_flag = fcntl(config_fd, F_GETFD, 0);
  fd_flag |= FD_CLOEXEC;
  fcntl(config_fd, F_SETFD, fd_flag);

#define LINEBUF_SIZE 128
#define SCRIPT_SIZE 512
  char linebuf[LINEBUF_SIZE];
  char script[SCRIPT_SIZE];

  assert(!this->mP2PName.empty());

  snprintf(linebuf, LINEBUF_SIZE, "start %s\n", WFD_DHCPD_LEASES_START_ADDRESS);
  strncpy(script, linebuf, SCRIPT_SIZE);
  snprintf(linebuf, LINEBUF_SIZE, "end %s\n", WFD_DHCPD_LEASES_END_ADDRESS);
  strncat(script, linebuf, SCRIPT_SIZE);
  snprintf(linebuf, LINEBUF_SIZE, "interface %s\n", this->mP2PName.c_str());
  strncat(script, linebuf, SCRIPT_SIZE);
  snprintf(linebuf, LINEBUF_SIZE, "max_leases %d\n", WFD_DHCPD_MAX_LEASES);
  strncat(script, linebuf, SCRIPT_SIZE);
  snprintf(linebuf, LINEBUF_SIZE, "option subnet %s\n", WFD_DHCPD_SUBNET_MASK);
  strncat(script, linebuf, SCRIPT_SIZE);
  snprintf(linebuf, LINEBUF_SIZE, "option router %s\n", DEFAULT_WFD_IP_ADDRESS);
  strncat(script, linebuf, SCRIPT_SIZE);
  snprintf(linebuf, LINEBUF_SIZE, "option lease %d\n", WFD_DHCPD_LEASE);
  strncat(script, linebuf, SCRIPT_SIZE);
  snprintf(linebuf, LINEBUF_SIZE, "option broadcast %s\n",
           WFD_DHCPD_BROADCAST_ADDRESS);
  strncat(script, linebuf, SCRIPT_SIZE);

  write(config_fd, script, strlen(script) + 1);
  close(config_fd);

  int dhcpdPid = ChildProcess::run(UDHCPD_PATH, params, false);

  return dhcpdPid;
}

void WfdP2PServer::sighandler_monitor_udhcpd(int signo, siginfo_t *sinfo,
                                             void *context) {
  if (signo != SIGCHLD || WfdP2PServer::sDhcpdPid == 0)
    return;

  if (sinfo->si_pid == WfdP2PServer::sDhcpdPid) {
    int status;
    while (waitpid(WfdP2PServer::sDhcpdPid, &status, WNOHANG) > 0) {
    }
    WfdP2PServer::sDhcpdPid = 0;

    LOG_DEBUG("udhcpd terminated");
    if (WfdP2PServer::sDhcpdCaller != NULL && WfdP2PServer::sDhcpdEnabled) {
      LOG_WARN("Relaunch udhcpd");
      WfdP2PServer::sDhcpdPid = WfdP2PServer::sDhcpdCaller->launch_dhcpd();
    }
  }
}

bool WfdP2PServer::disallow_discover_impl(void) {
  assert(!this->mP2PName.empty());

  this->kill_dhcpd();

  // Remove Wi-fi Direct P2P Group
  bool ret = CommandWpaCli::p2p_group_remove(DEFAULT_WFD_INTERFACE_NAME,
                                             this->mP2PName);
  if (!ret) {
    LOG_WARN("%s: Failed to remove p2p group", this->get_name());
  } else {
    LOG_VERB("%s: Succedded to remove p2p group", this->get_name());
  }
  return ret;

  return true;
}

int WfdP2PServer::kill_dhcpd(void) {
  WfdP2PServer::sDhcpdEnabled = false;
  WfdP2PServer::sDhcpdMonitoring = false;

  if (unlink(UDHCPD_CONFIG_PATH) != 0)
    LOG_WARN("%s: kill udhcpd failed %s", this->get_name(), strerror(errno));

  if (WfdP2PServer::sDhcpdPid == 0) {
    LOG_DEBUG("%s: no udhcpd to kill", this->get_name());
    return 0;
  }

  kill(WfdP2PServer::sDhcpdPid, SIGKILL);

  return 0;
}