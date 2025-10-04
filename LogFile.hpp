#pragma once
#include <SD.h>

class LogFile : public Print
{
public:
  LogFile(const char* filename)
    : filename_(filename)
  { }

  ~LogFile()
  {
    file_.close();
  }

  bool open(bool close=false)
  {
    if (close || !file_) {
      file_.close();
      file_ = SD.open(filename_, FILE_APPEND, /* create */ true);
      Serial.println(file_
        ? F("Open logfile: SUCCESS!")
        : F("Open logfile: FAILED!"));
    }
    return file_;
  }

  size_t write(uint8_t c) final
  {
    Serial.write(c);
    return file_.write(c);
  }

  size_t write(const uint8_t *buffer, size_t size) final
  {
    Serial.write(buffer, size);
    return file_.write(buffer, size);
  }

  int availableForWrite() final
  {
    return file_.availableForWrite();
  }

  void flush() final
  {
    Serial.flush();
    file_.flush();
  }

  void close()
  {
    file_.close();
  }

  LogFile(const LogFile&) = delete;
  LogFile& operator= (const LogFile&) = delete;

  operator bool() const { return file_; }

private:
  const char* filename_;
  File file_;
};
