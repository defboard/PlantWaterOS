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

  logfile = SD.open("/sensors.log", FILE_WRITE);

  pinMode(ENABLE_SENSOR_R1, OUTPUT);
  pinMode(ENABLE_SENSOR_R2, OUTPUT);
  digitalWrite(ENABLE_SENSOR_R1, HIGH);
  digitalWrite(ENABLE_SENSOR_R2, HIGH);
}


void loop() {

  DateTime now = rtc.now();

  int sensor_values[5];
  
  sensor_values[0] = analogRead(SENSOR_L0_CAP_5V);
  analogRead(SENSOR_R0_CAP_3V),
    analogRead(SENSOR_L1_RES_ALWAYSON),
    analogRead(SENSOR_R1_RES_CONTROLLED),
    analogRead(SENSOR_R2_WORST_CONTROLLED),
  };

  Serial << now << ":"
    << " " << sensor_values[0]
    << " " << sensor_values[1]
    << " " << sensor_values[2]
    << " " << sensor_values[3]
    << " " << sensor_values[4]
    << endl;

  delay(1000);
}
