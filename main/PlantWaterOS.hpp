#pragma once
#include "RingBuffer.hpp"

#include <Preferences.h>
#include <StreamString.h>
#include <RTClib.h>


const char* const PREFS_NAMESPACE = "PlantWaterOS";     // size <= 16!


struct Temperature {
  int degreeCelsius;
};

enum class MessageType {
  ScreenRefresh,
  ScreenCycleNext,
  ScreenCyclePrev,
  ScreenInfoLine,
  ActionCycleNext,
  ActionCyclePrev,
  PumpStart,
  PumpStop,
  PumpTimerReset,
  SensorRead,
};


struct SensorRecord {
    uint32_t time;
    int value;
};
extern RingBuffer<SensorRecord, 1000> sensorRecords;

extern StreamString eventLog;

extern const TimeSpan firstPumpDelay;
extern const TimeSpan logInterval;

extern DateTime now;
extern DateTime bootTime;
extern DateTime nextLogTime;
extern DateTime prevPumpTime;
extern DateTime nextPumpTime;

extern int numPumpEvents;

extern int sensorValue;
extern Temperature temperature;


bool sendMessage(
    MessageType type,
    const void* data=nullptr,
    TickType_t waitTime=portMAX_DELAY);


extern int getPumpDuration();
extern bool setPumpDuration(Preferences& prefs, int duration);

extern int32_t getPumpInterval();
extern bool setPumpInterval(Preferences& prefs, int32_t interval);
