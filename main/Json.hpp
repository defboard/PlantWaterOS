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

template <class T>
void json_plain_array(Stream& out, const char* key, const T& array, bool last=false)
{
  out << '"' << key << "\": [";
  for (size_t i = 0; i < array.size(); i++) {
    out << array[i];
    if (i + 1 < array.size()) {
      out << ',';
    }
  }
  out << ']';
  if (not last) {
      out << ',';
  }
}
