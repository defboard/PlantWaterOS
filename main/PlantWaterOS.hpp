#pragma once
#include <StreamString.h>
#include <RTClib.h>


struct Temperature {
  int degreeCelsius;
};

enum class MessageType {
  ScreenRefresh,
  ScreenCycleNext,
  ScreenCyclePrev,
  ScreenInfoLine,
  PumpStart,
  PumpStop,
  PumpTimerReset,
  SensorRead,
};


extern StreamString eventLog;

extern const TimeSpan firstPumpDelay;
extern const TimeSpan pumpInterval;
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
