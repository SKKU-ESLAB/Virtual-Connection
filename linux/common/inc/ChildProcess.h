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

#ifndef __UTIL_H__
#define __UTIL_H__

#include "../../configs/PathConfig.h"

#include <stdio.h>

namespace sc {
class ChildProcess {
public:
  static int run(const char *path, char const *const params[], char *res_buf,
                 size_t len, bool is_wait_child);
  static int run(const char *path, char const *const params[],
                 bool is_wait_child);
}; /* class ChildProcess */

} /* namespace sc */

#endif /* !defined(__UTIL_H__) */