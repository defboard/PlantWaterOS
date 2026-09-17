#pragma once
#include "PlantWaterOS.hpp"

#include <Print.h>
#include <RTClib.h>


using StreamFormatter = void(Print&);

struct DatePart { DateTime dt; };
struct TimePart { DateTime dt; };


inline void endl(Print& out) {
  out.println();
  out.flush();
}

inline char digit(int number)
{
  return '0' + (number % 10);
}

template <class T>
inline Print& operator<< (Print& out, T value)
{
  out.print(value);
  return out;
}

inline Print& operator<< (Print& out, DatePart date)
{
  return out
    << date.dt.year() / 1000
    << digit(date.dt.year() / 100)
    << digit(date.dt.year() / 10)
    << digit(date.dt.year())
    << '-'
    << digit(date.dt.month() / 10)
    << digit(date.dt.month())
    << '-'
    << digit(date.dt.day() / 10)
    << digit(date.dt.day());
}

inline Print& operator<< (Print& out, TimePart time)
{
  return out
    << digit(time.dt.hour() / 10)
    << digit(time.dt.hour())
    << ':'
    << digit(time.dt.minute() / 10)
    << digit(time.dt.minute())
    << ':'
    << digit(time.dt.second() / 10)
    << digit(time.dt.second());
}

inline Print& operator<< (Print& out, DateTime datetime)
{
  return out
      << DatePart{datetime} << ' '
      << TimePart{datetime};
}

inline Print& operator<< (Print& out, TimeSpan timespan)
{
  int days = timespan.days();
  int hours = timespan.hours();
  int minutes = timespan.minutes();

  if (days > 0) {
    return out << days << "d "
               << hours << 'h';
  }
  if (hours > 0) {
    out << hours << "h ";
  }
  return out << minutes << 'm';
}

inline Print& operator<< (Print& out, Temperature temperature)
{
  return out << temperature.degreeCelsius << 'C';
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

inline const char* CheckSuccess(bool success)
{
  return success ? "SUCCESS!" : "FAILED!";
}
