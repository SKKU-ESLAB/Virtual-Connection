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

#ifndef __RFCOMM_SERVER_SOCKET_H__
#define __RFCOMM_SERVER_SOCKET_H__

#include "../../common/inc/Counter.h"
#include "../../common/inc/DebugLog.h"

#include "../../core/inc/ServerSocket.h"

#include "../../configs/BtConfig.h"

#include <mutex>
#include <thread>

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>
#include <bluetooth/sdp_lib.h>

namespace sc {
class ServerAdapter;
class RfcommServerSocket : public ServerSocket {
public:
  virtual bool open_impl(bool is_send_request);
  virtual bool close_impl(void);
  virtual int send_impl(const void *data_buffer, size_t data_length);
  virtual int receive_impl(void *data_buffer, size_t data_length);

  uuid_t get_service_uuid(void) { return this->mServiceUUID; }

  RfcommServerSocket(const char *name, const char *service_uuid,
                     ServerAdapter *owner)
      : ServerSocket(name, owner) {
    char buffer[100];
    snprintf(buffer, 100, "%s", service_uuid);
    this->str2uuid(buffer, &(this->mServiceUUID));
    this->mPort = -1;

    this->set_throttling_bandwidth_threshold(
        BT_THROTTLING_BANDWIDTH_THRESHOLD_NORMAL);
  }

  ~RfcommServerSocket(void) {}

  /* Switch preparation handling */
  void set_threshold_normal(void) {
    this->set_throttling_bandwidth_threshold(
        BT_THROTTLING_BANDWIDTH_THRESHOLD_NORMAL);
  }
  void set_threshold_switch(void) {
    this->set_throttling_bandwidth_threshold(
        BT_THROTTLING_BANDWIDTH_THRESHOLD_SWITCH);
  }

protected:
  void set_throttling_bandwidth_threshold(int new_threshold) {
    this->mThrottlingBandwidthThreshold = new_threshold;
  }
  int mThrottlingBandwidthThreshold;

protected:
  uuid_t mServiceUUID;
  int mPort;

  sdp_session_t *mSdpSession;
  int mServerSocket;
  int mClientSocket;

  Counter mTotalTransferredData;

private:
  int bt_dynamic_bind_rc(void);
  int bt_register_service(void);
  int str2uuid(char *str, uuid_t *uuid);
}; /* class RfcommServerSocket */
} /* namespace sc */

#endif /* !defined(__RFCOMM_SERVER_SOCKET_H__) */