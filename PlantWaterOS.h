#pragma once
#include <Print.h>
#include <DS3231.h>

#define _SECONDS   (1l)
#define _MINUTES   (60l * _SECONDS)
#define _HOURS     (60l * _MINUTES)
#define _DAYS      (24l * _HOURS)

#define endl        "\r\n"


template <class T>
Print& operator<< (Print& out, T value)
{
  out.print(value);
  return out;
}

void printDigits(Print& out, int num, int digits)
{
  int place = 1;
  while (digits > 1) {
    --digits;
    place *= 10;
  }
  while (place > 0) {
    out.print(num / place % 10);
    place /= 10;
  }
}

void printDate(Print& out, const DateTime& date)
{
  printDigits(out, date.year(), 4);
  out.print(F("-"));
  printDigits(out, date.month(), 2);
  out.print(F("-"));
  printDigits(out, date.day(), 2);
}

void printTime(Print& out, const DateTime& time)
{
  printDigits(out, time.hour(), 2);
  out.print(F(":"));
  printDigits(out, time.minute(), 2);
  out.print(F(":"));
  printDigits(out, time.second(), 2);
}

Print& operator<< (Print& out, DateTime datetime)
{
  printDate(out, datetime);
  out.print(F(" "));
  printTime(out, datetime);
  return out;
}

void die()
{
  for (;;);  // Don't proceed, loop forever
}
