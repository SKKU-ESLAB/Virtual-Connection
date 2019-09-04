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

#ifndef __COMMAND_IWPRIV_H__
#define __COMMAND_IWPRIV_H__

typedef enum {
  kP2PDisable = 0,
  kP2PDevice = 1,
  kP2PClient = 2,
  kP2PGroupOwner = 3
} IwprivP2PRole;

namespace sc {
class CommandIwpriv {
public:
  static int p2p_set_enable(const char* device_name, IwprivP2PRole p2p_role);
}; /* class CommandIwpriv */
} /* namespace sc */

#endif /* !defined(__COMMAND_IWPRIV_H__) */