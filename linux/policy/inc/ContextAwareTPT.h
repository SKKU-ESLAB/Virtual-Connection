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
#ifndef __CONTEXT_AWARE_TPT_H__
#define __CONTEXT_AWARE_TPT_H__

#include <string>
#include <vector>

namespace sc {

class CABWTrafficEntry {
public:
  CABWTrafficEntry(float time_sec, float x, float y, float z, float bandwidth,
                 bool is_increase, std::vector<int> traffic_sequence) {
    this->mTimeSec = time_sec;
    this->mX = x;
    this->mY = y;
    this->mZ = z;
    this->mBandwidth = bandwidth;
    this->mIsIncrease = is_increase;
    this->mTrafficSequence.assign(traffic_sequence.begin(),
                                  traffic_sequence.end());
  }

  void getSensorData(float &x, float &y, float &z) {
    x = this->mX;
    y = this->mY;
    z = this->mZ;
  }
  float getX() { return this->mX; }
  float getY() { return this->mY; }
  float getZ() { return this->mZ; }
  float getTimeSec() { return this->mTimeSec; }
  float getBandwidth() { return this->mBandwidth; }
  bool isIncrease() { return this->mIsIncrease; }
  std::vector<int> &getTrafficSequence() { return this->mTrafficSequence; }
  void pushTraffic(int trafficEntry) {
    this->mTrafficSequence.push_back(trafficEntry);
  }

private:
  float mTimeSec;
  float mX;
  float mY;
  float mZ;
  float mBandwidth;
  bool mIsIncrease;
  std::vector<int> mTrafficSequence;
};

class ContextAwareTPT {
public:
  ContextAwareTPT(void) {}
  void initialize(void);

  void addItem(CABWTrafficEntry &bwTrafficEntry) {
    this->mBWTrafficList.push_back(bwTrafficEntry);
  }

  std::vector<CABWTrafficEntry> &getList() { return this->mBWTrafficList; }

private:
  std::vector<CABWTrafficEntry> mBWTrafficList;
}; /* class ContextAwareTPT */
} /* namespace sc */

#endif /* defined(__CONTEXT_AWARE_TPT_H__) */