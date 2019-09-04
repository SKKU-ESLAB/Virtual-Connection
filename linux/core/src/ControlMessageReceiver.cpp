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

#include "../inc/ControlMessageReceiver.h"

#include "../inc/Core.h"
#include "../inc/NetworkSwitcher.h"

#include "../../common/inc/DebugLog.h"

#include <iostream>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

#define THREAD_NAME "Control Message Receiving"

using namespace sc;

void ControlMessageReceiver::start_receiving_thread(void) {
  this->mReceivingThread = new std::thread(
      std::bind(&ControlMessageReceiver::receiving_thread_loop, this));
  this->mReceivingThread->detach();
}

void ControlMessageReceiver::stop_receiving_thread(void) {
  this->mReceivingThreadOn = false;
}

void ControlMessageReceiver::receiving_thread_loop(void) {
  this->mReceivingThreadOn = true;
  LOG_THREAD_LAUNCH(THREAD_NAME);

  while (this->mReceivingThreadOn) {
    this->receiving_thread_loop_internal();
  }

  LOG_THREAD_FINISH(THREAD_NAME);
  this->mReceivingThreadOn = false;
}

bool ControlMessageReceiver::receiving_thread_loop_internal(void) {
  void *message_buffer = NULL;
  Core::singleton()->receive(&message_buffer, true);
  if (message_buffer == NULL) {
    return false;
  }

  std::string message((char *)message_buffer);

  // Find separator location (between first line and other lines)
  std::size_t separator_pos = message.find('\n');

  // Divide the message into first line & other lines
  std::string first_line(message.substr(0, separator_pos));
  std::string other_lines(message.substr(separator_pos + 1));

  int control_message_code = std::stoi(first_line);

  switch (control_message_code) {
  case CMCode::kCMCodeConnect:
  case CMCode::kCMCodeSleep:
  case CMCode::kCMCodeWakeUp:
  case CMCode::kCMCodeDisconnectAck: {
    // Normal type
    int adapter_id = std::stoi(other_lines);
    this->on_receive_normal_message(control_message_code, adapter_id);
    break;
  }
  case CMCode::kCMCodeDisconnect: {
    // Disconnect type
    // Divide the message into second line, third line, fourth line
    separator_pos = other_lines.find('\n');
    std::string second_line(other_lines.substr(0, separator_pos));
    std::string third_fourth_line(other_lines.substr(separator_pos + 1));
    separator_pos = third_fourth_line.find('\n');
    std::string third_line(third_fourth_line.substr(0, separator_pos));
    std::string fourth_line(third_fourth_line.substr(separator_pos + 1));

    int adapter_id = std::stoi(second_line);
    uint32_t final_seq_no_control = std::stoul(third_line);
    uint32_t final_seq_no_data = std::stoul(fourth_line);

    this->on_receive_disconnect_message(adapter_id, final_seq_no_control,
                                        final_seq_no_data);
    break;
  }
  case CMCode::kCMCodePriv: {
    // Priv type
    this->on_receive_priv_message(other_lines);
    break;
  }
  case CMCode::kCMCodeRetransmit: {
    // Retransmit type
    // Divide the message into second line, third line, fourth line
    separator_pos = other_lines.find('\n');
    std::string second_line(other_lines.substr(0, separator_pos));
    std::string third_fourth_line(other_lines.substr(separator_pos + 1));
    separator_pos = third_fourth_line.find('\n');
    std::string third_line(third_fourth_line.substr(0, separator_pos));
    std::string fourth_line(third_fourth_line.substr(separator_pos + 1));

    int segment_type = std::stoi(second_line);
    uint32_t seq_no_start = std::stoul(third_line);
    uint32_t seq_no_end = std::stoul(fourth_line);

    this->on_receive_retransmit_request_message(segment_type, seq_no_start,
                                                seq_no_end);
    break;
  }
  case CMCode::kCMCodeQueueStatus: {
    // Queue Status type
    // Divide the message into second line, third line
    separator_pos = other_lines.find('\n');
    std::string second_line(other_lines.substr(0, separator_pos));
    std::string third_line(other_lines.substr(separator_pos + 1));
    uint32_t last_seq_no_control = std::stoul(second_line);
    uint32_t last_seq_no_data = std::stoul(third_line);

    this->on_receive_queue_status_message(last_seq_no_control,
                                          last_seq_no_data);
    break;
  }
  default: {
    LOG_ERR("Unknown Control Message Code(%d)!\n%s", control_message_code,
            message.c_str());
    break;
  }
  }

  free(message_buffer);
  return true;
}

void ControlMessageReceiver::on_receive_normal_message(int control_message_code,
                                                       int adapter_id) {
  NetworkSwitcher *ns = NetworkSwitcher::singleton();
  switch (control_message_code) {
  case CMCode::kCMCodeConnect: {
#ifdef VERBOSE_CONTROL_MESSAGE_RECEIVER
    LOG_DEBUG("Receive(Control Msg): Request(Connect %d)", adapter_id);
#endif
    ns->connect_adapter_by_peer(adapter_id);
    break;
  }
  case CMCode::kCMCodeSleep: {
#ifdef VERBOSE_CONTROL_MESSAGE_RECEIVER
    LOG_DEBUG("Receive(Control Msg): Request(Sleep %d)", adapter_id);
#endif
    ns->sleep_adapter_by_peer(adapter_id);
    break;
  }
  case CMCode::kCMCodeWakeUp: {
#ifdef VERBOSE_CONTROL_MESSAGE_RECEIVER
    LOG_DEBUG("Receive(Control Msg): Request(WakeUp %d)", adapter_id);
#endif
    ns->wake_up_adapter_by_peer(adapter_id);
    break;
  }
  case CMCode::kCMCodeDisconnectAck: {
#ifdef VERBOSE_CONTROL_MESSAGE_RECEIVER
    LOG_DEBUG("Receive(Control Msg): Request(DisconnectAck %d)", adapter_id);
#endif
    ServerAdapter *disconnect_adapter =
        Core::singleton()->find_adapter_by_id((int)adapter_id);
    if (disconnect_adapter == NULL) {
      LOG_WARN("Cannot find adapter %d", (int)adapter_id);
    } else {
      disconnect_adapter->peer_knows_disconnecting_on_purpose();
    }
    break;
  }
  default: {
    // Never reach here
    break;
  }
  }
}

void ControlMessageReceiver::on_receive_disconnect_message(
    int adapter_id, uint32_t final_seq_no_control, uint32_t final_seq_no_data) {
#ifdef VERBOSE_CONTROL_MESSAGE_RECEIVER
  LOG_DEBUG("Receive(Control Msg): Request(Disconnect %d / "
            "final_seq_no_control=%lu / final_seq_no_data=%lu)",
            adapter_id, final_seq_no_control, final_seq_no_data);
#endif
  NetworkSwitcher *ns = NetworkSwitcher::singleton();
  ns->disconnect_adapter_by_peer(adapter_id, final_seq_no_control,
                                 final_seq_no_data);
}

void ControlMessageReceiver::on_receive_retransmit_request_message(
    int segment_type, uint32_t seq_no_start, uint32_t seq_no_end) {
#ifdef VERBOSE_CONTROL_MESSAGE_RECEIVER
  LOG_DEBUG("Receive(Control Msg): Request(Retransmit type=%d / "
            "seq_no=%lu~%lu)",
            segment_type, seq_no_start, seq_no_end);
#endif
  SegmentManager *sm = SegmentManager::singleton();
  sm->retransmit_missing_segments_by_peer(segment_type, seq_no_start,
                                          seq_no_end);
}

void ControlMessageReceiver::on_receive_queue_status_message(
    uint32_t last_seq_no_control, uint32_t last_seq_no_data) {
#ifdef VERBOSE_CONTROL_MESSAGE_RECEIVER
  LOG_DEBUG(
      "Receive(Control Msg): Request(Queue Status last_seq_no_control=%d / "
      "last_seq_no_data=%lu)",
      last_seq_no_control, last_seq_no_data);
#endif
  SegmentManager *sm = SegmentManager::singleton();
  sm->deallocate_sent_segments_by_peer(last_seq_no_control, last_seq_no_data);
}

void ControlMessageReceiver::on_receive_priv_message(std::string contents) {
  // Find separator location (between second line and other lines)
  std::size_t separator_pos = contents.find('\n');

  // Divide the message into second line & other lines
  std::string second_line(contents.substr(0, separator_pos));
  std::string priv_message(contents.substr(separator_pos));

  int priv_type = std::stoi(second_line);

  // Notify the priv message
  for (std::vector<ControlPrivMessageListener *>::iterator it =
           this->mPrivMessageListeners.begin();
       it != this->mPrivMessageListeners.end(); it++) {
    ControlPrivMessageListener *listener = *it;
    if (listener != NULL) {
      listener->on_receive_control_priv_message(priv_type, priv_message);
    }
  }
}