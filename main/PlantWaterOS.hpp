#pragma once
#include <StreamString.h>
#include <RTClib.h>


struct Temperature {
  int degreeCelsius;
};

extern StreamString eventLog;

extern const TimeSpan firstPumpDelay;
extern const TimeSpan pumpInterval;
extern const TimeSpan logInterval;

extern DateTime now;
extern DateTime bootTime;
extern DateTime nextLogTime;
extern DateTime nextPumpTime;

extern int numPumpEvents;

extern int sensorValue;
extern Temperature temperature;
