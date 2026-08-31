#pragma once
#include "RingBuffer.hpp"

#include <Preferences.h>
#include <StreamString.h>
#include <RTClib.h>


constexpr char PREFS_NAMESPACE[] = "PlantWaterOS";
static_assert(sizeof(PREFS_NAMESPACE) <= 16);


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
  SetSystemTime,
  SetNextPumpTime,
};


struct SensorRecord {
    uint32_t time;
    int value;
};
extern RingBuffer<SensorRecord, 30> sensorRecordsA;         // one value every  1s for 30s
extern RingBuffer<SensorRecord, 40> sensorRecordsB;         // one value every 30s for 20m
extern RingBuffer<SensorRecord, 1008> sensorRecordsC;       // one value every 20m for 14d

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


extern bool sendMessage(
    MessageType type,
    const void* data=nullptr,
    TickType_t waitTime=portMAX_DELAY);


extern int getPumpDuration();
extern bool setPumpDuration(Preferences& prefs, int duration);

extern int32_t getPumpInterval();
extern bool setPumpInterval(Preferences& prefs, int32_t interval);

extern bool setSystemTime(DateTime systemTime);

extern bool setNextPumpTime(DateTime pumpTime);
