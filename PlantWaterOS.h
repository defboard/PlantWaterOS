#pragma once
#include <Print.h>
#include <DS3231.h>

#define _SECONDS   (1l)
#define _MINUTES   (60l * _SECONDS)
#define _HOURS     (60l * _MINUTES)
#define _DAYS      (24l * _HOURS)


class DevNull : public Print {
  size_t write(uint8_t) final { return 0; };
};

struct NullDisplay : public DevNull {
    void display() {}
};


template <class T>
void print(Print& out, T value)
{
  out.print(value);
}

template <class Head, class ...Tail>
void print(Print& out, Head head, Tail... tail)
{
  print(out, head);
  print(out, tail...);
}

template <class Head, class ...Tail>
void println(Print& out, Head head, Tail... tail)
{
  print(out, head);
  println(out, tail...);
}

void println(Print& out)
{
  out.println();
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

void print(Print& out, DateTime datetime)
{
  printDate(out, datetime);
  out.print(F(" "));
  printTime(out, datetime);
}

void die()
{
  for (;;);  // Don't proceed, loop forever
}
