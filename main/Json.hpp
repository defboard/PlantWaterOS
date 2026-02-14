#pragma once
#include <Print.h>
#include "Formatting.hpp"

const char HEX_DIGITS[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
};


class JsonStringWriter : public Print
{
  Print& _p;
public:
  explicit JsonStringWriter(Print& p)
    : _p(p)
  {
    _p.write('"');
  }

  ~JsonStringWriter() final
  {
    _p.write('"');
  }

  size_t write(uint8_t c) final
  {
    switch (c) {
    case '"':
        return _p.write("\\\"", 2);
    case '\\':
        return _p.write("\\\\", 2);
    case '\b':
        return _p.write("\\b", 2);
    case '\f':
        return _p.write("\\f", 2);
    case '\n':
        return _p.write("\\n", 2);
    case '\r':
        return _p.write("\\r", 2);
    case '\t':
        return _p.write("\\t", 2);
    default:
        if (c <= '\x1f') {
            size_t result = 0;
            result += _p.write("\\u00", 4);
            result += _p.write(HEX_DIGITS[0x0f & (c >> 4)]);
            result += _p.write(HEX_DIGITS[0x0f & (c)]);
            return result;
        }
        break;
    }
    return _p.write(c);
  }
};


class JsonWriter {
  Print& _p;
  bool _comma = false;

  inline void _put_comma()
  {
    if (_comma) {
      _p.write(',');
    }
  }

public:
  explicit JsonWriter(Print& p)
    : _p(p)
  {
  }

  void put_object()
  {
    _put_comma();
    _p.write('{');
    _comma = false;
  }

  void end_object()
  {
    _p.write('}');
    _comma = true;
  }

  void put_array()
  {
    _put_comma();
    _p.write('[');
    _comma = false;
  }

  void end_array()
  {
    _p.write(']');
    _comma = true;
  }

  template <class V>
  void put_string(const V& value)
  {
    _put_comma();
    _comma = true;
    JsonStringWriter w(_p);
    w << value;
  }

  template <class V>
  void put_plain(const V& value)
  {
    _put_comma();
    _comma = true;
    _p << value;
  }

  void put_bool(bool value)
  {
    _put_comma();
    _comma = true;
    if (value) {
      _p.write("true", 4);
    }
    else {
      _p.write("false", 5);
    }
  }

  void put_null() {
    _put_comma();
    _comma = true;
    _p.write("null", 4);
  }


  template <class K>
  void put_key(const K& key)
  {
    put_string(key);
    _p.write(':');
    _comma = false;
  }

  template <class K>
  void put_object(const K& key)
  {
    put_key(key);
    put_object();
  }

  template <class K>
  void put_array(const K& key)
  {
    put_key(key);
    put_array();
  }

  template <class K, class V>
  inline void put_string(const K& key, const V& value)
  {
    put_key(key);
    put_string(value);
  }

  template <class K, class V>
  inline void put_plain(const K& key, const V& value)
  {
    put_key(key);
    put_plain(value);
  }

  template <class K>
  inline void put_bool(const K& key, bool value)
  {
    put_key(key);
    put_bool(value);
  }

  template <class K>
  inline void put_null(const K& key)
  {
    put_key(key);
    put_null();
  }
};
