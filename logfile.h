#pragma once
#include <SdFat.h>

# define SD_FAT_TYPE 0

# if SD_FAT_TYPE == 0
using SdType   = SdFat;
using FileType = File;
# elif SD_FAT_TYPE == 1
using SdType   = SdFat32;
using FileType = File32;
# elif SD_FAT_TYPE == 2
using SdType   = SdExFat;
using FileType = ExFile;
# elif SD_FAT_TYPE == 3
using SdType   = SdFs;
using FileType = FsFile;
# else
#  error Invalid SD_FAT_TYPE
# endif

SdType SD;


class LogFile : public Print
{
public:
  LogFile(const char* filename, Print& serial)
    : _filename(filename)
    , _serial(serial)
  { }

  ~LogFile() { _file.close(); }

  void open()
  {
    _file.close();
    _file.open(_filename, O_RDWR | O_CREAT | O_APPEND);
    if (not _file) {
      Serial.println(F("Failed to open logfile!"));
    }
  }

  size_t write(uint8_t c) override
  {
    if (!_file) {
      open();
    }
    _serial.write(c);
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
  FileType _file;
  Print& _serial;
};
