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
#ifndef __TRAFFIC_HISTORY_TABLE_H__
#define __TRAFFIC_HISTORY_TABLE_H__

#include "../../common/inc/DebugLog.h"

#include <cmath>
#include <limits>
#include <map>
#include <string>
#include <vector>

namespace sc {
/*
 * TrafficHistoryTable
 *  =(map; id=string appName)=> AppEntry
 *  =(map; id=int eventType)=> EventTypeEntry
 *  =(vector)=> TrafficEntry
 */

class TrafficEntry {
public:
  TrafficEntry(float time_sec, std::vector<int> traffic_sequence) {
    this->mTimeSec = time_sec;
    this->mTrafficSequence.assign(traffic_sequence.begin(),
                                  traffic_sequence.end());
  }
  float getTimeSec() { return this->mTimeSec; }
  std::vector<int> &getTrafficSequence() { return this->mTrafficSequence; }
  void pushTraffic(int trafficEntry) {
    this->mTrafficSequence.push_back(trafficEntry);
  }

private:
  float mTimeSec;
  std::vector<int> mTrafficSequence;
}; /* class TrafficEntry */

class EventTypeEntry {
public:
  EventTypeEntry(int event_type) { this->mEventType = event_type; }
  int getEventType() { return this->mEventType; }

  TrafficEntry *findItem(float time_sec) {
    TrafficEntry *closest_entry = NULL;
    float closest_time_diff = std::numeric_limits<float>::max();

    for (std::vector<TrafficEntry>::iterator it = this->mTrafficList.begin();
         it != this->mTrafficList.end(); it++) {
      TrafficEntry *entry = &(*it);
      float entry_time_diff = abs(time_sec - entry->getTimeSec());
      LOG_DEBUG("%7.3f? %7.3f vs. %7.3f", time_sec, closest_time_diff, entry_time_diff);
      if (closest_time_diff > entry_time_diff) {
        closest_entry = entry;
        closest_time_diff = entry_time_diff;
      }
    }
    return closest_entry;
  }

  void addItem(TrafficEntry &trafficEntry) {
    this->mTrafficList.push_back(trafficEntry);
  }

  std::vector<TrafficEntry> &getList() { return this->mTrafficList; }

private:
  int mEventType;
  std::vector<TrafficEntry> mTrafficList;
}; /* class EventTypeEntry */

class AppEntry {
public:
  AppEntry(std::string app_name) { this->mAppName = app_name; }
  std::string getAppName() { return this->mAppName; }

  EventTypeEntry *getItem(int eventType) {
    std::map<int, EventTypeEntry>::iterator foundItem =
        this->mEventTypeMap.find(eventType);
    if (foundItem == this->mEventTypeMap.end()) {
      return NULL;
    } else {
      return &(foundItem->second);
    }
  }

  void addItem(EventTypeEntry &eventTypeEntry) {
    int eventType = eventTypeEntry.getEventType();
    this->mEventTypeMap.insert(
        std::pair<int, EventTypeEntry>(eventType, eventTypeEntry));
  }

  std::map<int, EventTypeEntry> &getMap() { return this->mEventTypeMap; }

private:
  std::string mAppName;
  std::map<int, EventTypeEntry> mEventTypeMap;
}; /* class AppEntry */

class TrafficHistoryTable {
public:
  TrafficHistoryTable(void) {}
  void initialize(void);

  AppEntry *getItem(std::string appName) {
    std::map<std::string, AppEntry>::iterator foundItem =
        this->mAppMap.find(appName);
    if (foundItem == this->mAppMap.end()) {
      return NULL;
    } else {
      return &(foundItem->second);
    }
  }

  void addItem(AppEntry &appEntry) {
    std::string appName = appEntry.getAppName();
    this->mAppMap.insert(std::pair<std::string, AppEntry>(appName, appEntry));
  }

  std::map<std::string, AppEntry> &getMap() { return this->mAppMap; }

private:
  std::map<std::string, AppEntry> mAppMap;
}; /* class TrafficHistoryTable */
} /* namespace sc */

#endif /* defined(__TRAFFIC_HISTORY_TABLE_H__) */