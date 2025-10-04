#include "SensorBoard.hpp"

#include <Wire.h>
#include <RTClib.h>
#include <SD.h>


// Pin assignments
const int PIN_RTC_SDA                     = 27;
const int PIN_RTC_SCL                     = 14;

const int PIN_SD_MOSI                     = 18;
const int PIN_SD_MISO                     = 21;
const int PIN_SD_SCLK                     = 19;
const int PIN_SD_CS                       = 22;

const int PIN_SENSOR_L0_CAP_5V            = 32;
const int PIN_SENSOR_R0_CAP_3V            = 36;
const int PIN_SENSOR_L1_RES_ALWAYSON      = 35;
const int PIN_SENSOR_R1_RES_CONTROLLED    = 39;
const int PIN_SENSOR_R2_WORST_CONTROLLED  = 34;

const int PIN_ENABLE_PUMP                 = 26;
const int PIN_ENABLE_SENSOR_R1            = 25;
const int PIN_ENABLE_SENSOR_R2            = 4;

const int SENSOR_PINS[] = {
  PIN_SENSOR_L0_CAP_5V,
  PIN_SENSOR_R0_CAP_3V,
  PIN_SENSOR_L1_RES_ALWAYSON,
  PIN_SENSOR_R1_RES_CONTROLLED,
  PIN_SENSOR_R2_WORST_CONTROLLED,
};
const int NUM_SENSORS = sizeof(SENSOR_PINS) / sizeof(*SENSOR_PINS);


// Configuration
const int PUMP_DURATION = 2500;     // [ms]

const TimeSpan firstPumpDelay (1/*days*/, 0/*hours*/, 0/*minutes*/, 0/*seconds*/);
const TimeSpan pumpInterval   (3/*days*/, 0/*hours*/, 0/*minutes*/, 0/*seconds*/);
const TimeSpan logInterval    (0/*days*/, 0/*hours*/, 20/*minutes*/, 0/*seconds*/);

const int numSensorRepeat = 20;
const int delaySensorRepeat = 1000;   // [ms]


// Globals
RTC_DS3231 rtc;
File logfile;

DateTime now;
DateTime bootTime;
DateTime nextLogTime;
DateTime nextPumpTime;

int numPumpEvents = 0;


void setup()
{
  // Init Serial
  Serial.begin(115200);
  while (!Serial) {
    // wait for Serial connection
  }

  // Init control pins
  pinMode(PIN_ENABLE_PUMP, OUTPUT);
  pinMode(PIN_ENABLE_SENSOR_R1, OUTPUT);
  pinMode(PIN_ENABLE_SENSOR_R2, OUTPUT);
  enablePump(false);
  enableSensors(false);

  // Init RTC
  Serial << F("Init Wire: ") << CheckSuccess(Wire.begin(PIN_RTC_SDA, PIN_RTC_SCL));
  Serial << F("Init RTC: ") << CheckSuccess(rtc.begin(&Wire));
  rtc.disable32K();
  rtc.writeSqwPinMode(Ds3231SqwPinMode::DS3231_OFF);

  // Init SD
  Serial << F("Init SPI: ") << CheckSuccess(SPI.begin(PIN_SD_SCLK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS));
  Serial << F("Init SD: ") << CheckSuccess(SD.begin(PIN_SD_CS, SPI));
  if (SD.cardType() == CARD_NONE) {
    Serial.println(F("No SD card attached"));
  }
  uint32_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial << F("SDCard Size: ") << cardSize << "MB" << endl;

  // Init nextLogTime
  bootTime = rtc.now();
  Serial << bootTime << F(": system booted - ")
    << (rtc.lostPower()
        ? F("RTC power loss")
        : F("RTC remained powered"))
    << endl;
  nextLogTime = bootTime;

  // Init nextPumpTime
  const DateTime storedTime = readNextPumpTime();
  if (storedTime > bootTime and
      storedTime < bootTime + pumpInterval)
  {
    logfile << F("Using RTC pump timer: ") << storedTime << endl;
    nextPumpTime = storedTime;
  }
  else
  {
    logfile << F("Reset RTC pump timer: ") << storedTime << endl;
    nextPumpTime = bootTime + firstPumpDelay;
    writeNextPumpTime(nextPumpTime);
  }
}


void loop()
{
  now = rtc.now();

  if (now >= nextLogTime) {
    nextLogTime = now + logInterval;
    logSensorReadings();
  }

  if (now >= nextPumpTime) {
    nextPumpTime = now + pumpInterval;
    writeNextPumpTime(nextPumpTime);
    activatePump();
  }

  delay(1000);
}


void logSensorReadings()
{
  enableSensors(true);

  if (!logfile) {
    const bool createFile = true;
    logfile = SD.open("/sensors.log", FILE_APPEND, createFile);
  }
  Serial << "Open logfile: " << CheckSuccess(logfile);

  for (int i = 0; i < numSensorRepeat; ++i) {
    DateTime now = rtc.now();
    Temperature temp{ (int) rtc.getTemperature() };
    Serial << now << ": " << temp;
    logfile << now << ": " << temp;

    for (int i = 0; i < NUM_SENSORS; ++i) {
      int sensorValue = analogRead(SENSOR_PINS[i]);
      Serial << " " << sensorValue;
      logfile << " " << sensorValue;
      delayMicroseconds(100);
    }

    Serial << endl;
    logfile << endl;

    delay(delaySensorRepeat);
  }
  enableSensors(false);
}


void enableSensors(bool enable)
{
  digitalWrite(PIN_ENABLE_SENSOR_R1, enable ? HIGH : LOW);
  digitalWrite(PIN_ENABLE_SENSOR_R2, enable ? HIGH : LOW);
}

void enablePump(bool enable)
{
  digitalWrite(PIN_ENABLE_PUMP, enable ? HIGH : LOW);
}


void activatePump()
{
  numPumpEvents += 1;
  Serial << now << F(": Pump event ") << numPumpEvents << " (" << PUMP_DURATION << "ms)" << endl;

  enablePump(true);
  delay(PUMP_DURATION);
  enablePump(false);
}


DateTime readNextPumpTime()
{
  // The information stored with either one of the alarms is not enough to
  // reconstruct a complete DateTime:
  //
  // Alarm1 only stores DAY/HOUR/MINUTE/SECOND, but not YEAR/MONTH.
  // Alarm2 only stores DAY/HOUR/MINUTE,        but not YEAR/MONTH/SECOND
  //
  // We therefore use both alarms as RTC memory to reconstruct the full
  // DateTime information:
  const DateTime alarm1 = rtc.getAlarm1();
  const DateTime alarm2 = rtc.getAlarm2();
  return DateTime(
      alarm2.minute() + 2000,   // use alarm2.minute as year [2000-2059]
      alarm2.hour(),            // use alarm2.hour as month [0-24]
      alarm1.day(),
      alarm1.hour(),
      alarm1.minute(),
      alarm1.second()
  );
}

void writeNextPumpTime(const DateTime& nextPumpTime)
{
  const uint8_t year = nextPumpTime.year();
  const uint8_t month = nextPumpTime.month();
  const DateTime& alarm1 = nextPumpTime;
  const DateTime alarm2(
      2000, 5,      // year/month is ignored
      1,            // day is saved, but we currently don't use it
      month,        // use alarm2.hour as month [0-24]
      year - 2000,  // use alarm2.minute as as year [2000-2059]
      0);           // second is ignored
  rtc.setAlarm1(alarm1, DS3231_A1_Date);
  rtc.setAlarm2(alarm2, DS3231_A2_Date);
}
