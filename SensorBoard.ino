#include "SensorBoard.hpp"

#include <Wire.h>
#include <RTClib.h>
#include <SD.h>


#define RTC_SDA                     27
#define RTC_SCL                     14

#define SD_MOSI                     18
#define SD_MISO                     21
#define SD_SCLK                     19
#define SD_CS                       22

#define SENSOR_L0_CAP_5V            32
#define SENSOR_R0_CAP_3V            36
#define SENSOR_L1_RES_ALWAYSON      35
#define SENSOR_R1_RES_CONTROLLED    39
#define SENSOR_R2_WORST_CONTROLLED  34

#define ENABLE_PUMP                 26
#define ENABLE_SENSOR_R1            25
#define ENABLE_SENSOR_R2            4

const int sensor_pins[] = {
  SENSOR_L0_CAP_5V,
  SENSOR_R0_CAP_3V,
  SENSOR_L1_RES_ALWAYSON,
  SENSOR_R1_RES_CONTROLLED,
  SENSOR_R2_WORST_CONTROLLED,
};
const int num_sensors = sizeof(sensor_pins) / sizeof(*sensor_pins);

#define SECONDS   (1000L)
#define MINUTES   (SECONDS * 60)


RTC_DS3231 rtc;
File logfile;


void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }

  Serial << F("Init Wire: ") << CheckSuccess(Wire.begin(RTC_SDA, RTC_SCL));
  Serial << F("Init RTC: ") << CheckSuccess(rtc.begin(&Wire));
  rtc.disable32K();
  rtc.writeSqwPinMode(Ds3231SqwPinMode::DS3231_OFF);

  Serial << F("Init SPI: ") << CheckSuccess(SPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS));
  Serial << F("Init SD: ") << CheckSuccess(SD.begin(SD_CS, SPI));
  if (SD.cardType() == CARD_NONE) {
    Serial.println("No SD card attached");
  }

  uint32_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.println("SDCard Size: " + String(cardSize) + "MB");

  pinMode(ENABLE_SENSOR_R1, OUTPUT);
  pinMode(ENABLE_SENSOR_R2, OUTPUT);
  digitalWrite(ENABLE_SENSOR_R1, LOW);
  digitalWrite(ENABLE_SENSOR_R2, LOW);
}


void loop() {
  digitalWrite(ENABLE_SENSOR_R1, HIGH);
  digitalWrite(ENABLE_SENSOR_R2, HIGH);

  if (!logfile) {
    const bool createFile = true;
    logfile = SD.open("/sensors.log", FILE_APPEND, createFile);
  }
  Serial << "Open logfile: " << CheckSuccess(logfile);

  for (int i = 0; i < 20; ++i) {
    DateTime now = rtc.now();
    Temperature temp{ rtc.getTemperature() };
    Serial << now << ": " << temp;
    logfile << now << ": " << temp;

    for (int i = 0; i < num_sensors; ++i) {
      int sensorValue = analogRead(sensor_pins[i]);
      Serial << " " << sensorValue;
      logfile << " " << sensorValue;
      delayMicroseconds(100);
    }

    Serial << endl;
    logfile << endl;

    delay(1 * SECONDS);
  }
  digitalWrite(ENABLE_SENSOR_R1, LOW);
  digitalWrite(ENABLE_SENSOR_R2, LOW);

  delay(30 * MINUTES);
}
