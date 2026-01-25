#pragma once
#include "PlantWaterOS.hpp"

#include <Print.h>
#include <RTClib.h>


using StreamFormatter = void(Print&);

struct DoubleDigits {
  int number;
};

struct DatePart { DateTime dt; };
struct TimePart { DateTime dt; };


inline void endl(Print& out) {
  out.println();
  out.flush();
}

template <class T>
inline Print& operator<< (Print& out, T value)
{
  out.print(value);
  return out;
}

inline Print& operator<< (Print& out, DoubleDigits num)
{
  return out
      << (num.number / 10)
      << (num.number % 10);
}

inline Print& operator<< (Print& out, DatePart date)
{
  return out
    << DoubleDigits{date.dt.year() / 100}
    << DoubleDigits{date.dt.year() % 100}
    << F("-") << date.dt.month()
    << F("-") << date.dt.day();
}

inline Print& operator<< (Print& out, TimePart time)
{
  return out
    << DoubleDigits{time.dt.hour()} << F(":")
    << DoubleDigits{time.dt.minute()} << F(":")
    << DoubleDigits{time.dt.second()};
}

inline Print& operator<< (Print& out, DateTime datetime)
{
  return out
      << DatePart{datetime} << F(" ")
      << TimePart{datetime};
}

inline Print& operator<< (Print& out, TimeSpan timespan)
{
  int days = timespan.days();
  int hours = timespan.hours();
  int minutes = timespan.minutes();

  if (days > 0) {
    return out << days << F("d ")
               << hours << F("h");
  }
  if (hours > 0) {
    out << hours << F("h ");
  }
  return out << minutes << F("m");
}

inline Print& operator<< (Print& out, Temperature temperature)
{
  return out
      << (int)(temperature.degreeCelsius)           << F(".")
      << (int)(temperature.degreeCelsius * 10) % 10 << F("C");
}

inline Print& operator<< (Print& out, SensorRecord record)
{
  return out << '[' << record.time << ',' << record.value << ']';
}

inline Print& operator<< (Print& out, StreamFormatter f)
{
  f(out);
  return out;
}

const char* CheckSuccess(bool success)
{
  return success ? "SUCCESS!" : "FAILED!";
}
