#pragma once
#include <Stream.h>


template <class T>
void json_str(Stream& out, const char* key, const T& value, bool last=false)
{
  out << '"' << key << "\": \"" << value << '"';
  if (not last) {
    out << ',';
  }
}

template <class T>
void json_plain(Stream& out, const char* key, const T& value, bool last=false)
{
  out << '"' << key << "\": " << value;
  if (not last) {
    out << ',';
  }
}
