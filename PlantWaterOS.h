#pragma once
#include <Print.h>
#include <RTClib.h>

# define endl   "\r\n"


class DevNull : public Print {
  size_t write(uint8_t) final { return 0; };
};

struct NullDisplay : public DevNull {
    void display() {}
};

struct Temperature {
  int degreeCelsius;
};

struct DoubleDigits {
  unsigned number;
};

struct DatePart { DateTime dt; };
struct TimePart { DateTime dt; };


template <class T>
inline Print& operator<< (Print& out, T value)
{
  out.print(value);
  return out;
}

Print& operator<< (Print& out, DoubleDigits num)
{
  return out
      << (num.number / 10)
      << (num.number % 10);
}

Print& operator<< (Print& out, DatePart date)
{
  return out
    << DoubleDigits{date.dt.year() / 100}
    << DoubleDigits{date.dt.year() % 100}
    << F("-") << date.dt.month()
    << F("-") << date.dt.day();
}

Print& operator<< (Print& out, TimePart time)
{
  return out
    << DoubleDigits{time.dt.hour()} << F(":")
    << DoubleDigits{time.dt.minute()} << F(":")
    << DoubleDigits{time.dt.second()};
}

Print& operator<< (Print& out, DateTime datetime)
{
  return out
      << DatePart{datetime} << F(" ")
      << TimePart{datetime};
}

Print& operator<< (Print& out, Temperature temperature)
{
  return out
      << (int)(temperature.degreeCelsius)           << F(".")
      << (int)(temperature.degreeCelsius * 10) % 10 << F("C");
}

void die()
{
  for (;;);  // Don't proceed, loop forever
}
