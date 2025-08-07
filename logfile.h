#pragma once
#include <SD.h>

class LogFile : public Print
{
public:
  LogFile(const char* filename)
    : _filename(filename)
  { }

  ~LogFile() { _file.close(); }

  void open()
  {
    _file.close();
    _file = SD.open(_filename, FILE_WRITE);
    if (not _file) {
      Serial.println(F("Failed to open logfile!"));
    }
  }

  size_t write(uint8_t c) override
  {
    if (!_file) {
      open();
    }
    return _file.write(c);
  }

  void flush() override
  {
    _file.flush();
  }

  void close()
  {
    _file.close();
  }

  LogFile(const LogFile&) = delete;
  LogFile& operator= (const LogFile&) = delete;

private:
  const char* _filename;
  File _file;
};
