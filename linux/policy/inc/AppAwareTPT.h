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
#ifndef __APP_AWARE_TPT_H__
#define __APP_AWARE_TPT_H__

#include <string>
#include <vector>

namespace sc {

class BWTrafficEntry {
public:
  BWTrafficEntry(float time_sec, float bandwidth, bool is_increase,
                 std::vector<int> traffic_sequence) {
    this->mTimeSec = time_sec;
    this->mBandwidth = bandwidth;
    this->mIsIncrease = is_increase;
    this->mTrafficSequence.assign(traffic_sequence.begin(),
                                  traffic_sequence.end());
  }

  float getTimeSec() { return this->mTimeSec; }
  float getBandwidth() { return this->mBandwidth; }
  bool isIncrease() { return this->mIsIncrease; }
  std::vector<int> &getTrafficSequence() { return this->mTrafficSequence; }
  void pushTraffic(int trafficEntry) {
    this->mTrafficSequence.push_back(trafficEntry);
  }

private:
  float mTimeSec;
  float mBandwidth;
  bool mIsIncrease;
  std::vector<int> mTrafficSequence;
};

class AppTrafficEntry {
public:
  AppTrafficEntry(std::string app_name) { this->mAppName = app_name; }
  std::string getAppName() { return this->mAppName; }

  BWTrafficEntry *getItem(float bandwidth) {
    for (std::vector<BWTrafficEntry>::iterator it =
             this->mBWTrafficList.begin();
         it != this->mBWTrafficList.end(); it++) {
      BWTrafficEntry *entry = &(*it);
      int _bandwidth = entry->getBandwidth();
      if (bandwidth == _bandwidth) {
        return entry;
      }
    }
    return NULL;
  }

  void addItem(BWTrafficEntry &bwTrafficEntry) {
    this->mBWTrafficList.push_back(bwTrafficEntry);
  }

  std::vector<BWTrafficEntry> &getList() { return this->mBWTrafficList; }

private:
  std::string mAppName;
  std::vector<BWTrafficEntry> mBWTrafficList;
};

class AppAwareTPT {
public:
  AppAwareTPT(void) {}
  void initialize(void);

  AppTrafficEntry *getItem(std::string app_name) {
    for (std::vector<AppTrafficEntry>::iterator it =
             this->mAppTrafficList.begin();
         it != this->mAppTrafficList.end(); it++) {
      AppTrafficEntry *entry = &(*it);
      std::string _app_name = entry->getAppName();
      if (app_name.compare(_app_name) == 0) {
        return entry;
      }
    }
    return NULL;
  }

  void addItem(AppTrafficEntry &appTrafficEntry) {
    this->mAppTrafficList.push_back(appTrafficEntry);
  }

  std::vector<AppTrafficEntry> &getList() { return this->mAppTrafficList; }

private:
  std::vector<AppTrafficEntry> mAppTrafficList;
}; /* class AppAwareTPT */
} /* namespace sc */

#endif /* defined(__APP_AWARE_TPT_H__) */