#pragma once
#include <SD.h>

class LogFile : public Print
{
public:
  LogFile(const char* filename, Print& print)
    : filename_(filename)
    , print_(print)
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
      print_.println(file_
        ? "Open logfile: SUCCESS!"
        : "Open logfile: FAILED!");
    }
    return file_;
  }

  size_t write(uint8_t c) final
  {
    print_.write(c);
    return file_.write(c);
  }

  size_t write(const uint8_t *buffer, size_t size) final
  {
    print_.write(buffer, size);
    return file_.write(buffer, size);
  }

  int availableForWrite() final
  {
    return file_.availableForWrite();
  }

  void flush() final
  {
    print_.flush();
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
  Print& print_;
};
