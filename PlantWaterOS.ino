#define USE_SERIAL    1
#define USE_SD        1
#define USE_DISPLAY   1
#define USE_PUMP      1

#include "PlantWaterOS.h"
#include "buildtime.h"

#include <Wire.h>
#include <RTClib.h>

#if USE_DISPLAY
# include <Adafruit_GFX.h>
# include <Adafruit_SSD1306.h>
#endif

#if USE_SD
# include "logfile.h"
#endif


// Configuration:
const int SENSOR_PIN    = A6;
const int SD_CS_PIN     = 10;
const int PUMP_PIN      = 6;

const int PUMP_ON       = HIGH;
const int PUMP_OFF      = 1 - PUMP_ON;
const int PUMP_DURATION = 2500;     // [ms]

const TimeSpan logInterval    (0/*days*/, 1/*hours*/, 0/*minutes*/, 0/*seconds*/);
const TimeSpan firstPumpDelay (1/*days*/, 0/*hours*/, 0/*minutes*/, 0/*seconds*/);
const TimeSpan pumpInterval   (3/*days*/, 0/*hours*/, 0/*minutes*/, 0/*seconds*/);

bool isWaterTime(DateTime t) {
  return t.hour() >= 7 and t.hour() <= 18;
}

// Native display width is W x H = 128 x 64, but we lower resolution
// to save precious memory:
const int DISPLAY_WIDTH     = 128;  // OLED display width, in pixels
const int DISPLAY_HEIGHT    = 16;   // OLED display height, in pixels
const int DISPLAY_RESET_PIN = -1;   // Reset pin # (or -1 if sharing Arduino reset pin)
const int DISPLAY_ADDRESS   = 0x3C; // I2C address


// Global variables:
RTC_DS3231 rtc;
DateTime bootTime;
DateTime now;

#if USE_DISPLAY
Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, DISPLAY_RESET_PIN);
#else
NullDisplay display;
#endif

#if USE_SD
LogFile logfile("sensor.log");
DateTime nextLogTime;
#else
DevNull logfile;
#endif

DateTime nextPumpTime;
int numPumpEvents = 0;

int sensorValue = 0;
float temperature = 0;


void setup() {
#if USE_SERIAL
  Serial.begin(9600);
  while (!Serial) { }
  Serial.println(F(""));
#endif

  // needed for RTC and display:
  Wire.begin();
  rtc.begin(&Wire);
  bootTime = DateTime(
      BUILD_YEAR, BUILD_MONTH, BUILD_DAY,
      BUILD_HOUR, BUILD_MINUTE, BUILD_SECOND);
  rtc.adjust(bootTime);

#if USE_SD
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(F("SD initialization failed."));
    die();
  }
  nextLogTime = bootTime;
  println(logfile, bootTime, F(": system booted"));
#endif
  println(Serial, bootTime, F(": system booted"));

#if USE_DISPLAY
  if(!display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDRESS, true, false)) {
    Serial.println(F("Display initialization failed."));
    die();
  }
  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
#endif

#if USE_PUMP
  nextPumpTime = bootTime + firstPumpDelay;
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, PUMP_OFF);
#endif
}

void loop()
{
  now = rtc.now();
  sensorValue = analogRead(SENSOR_PIN);
  temperature = rtc.getTemperature();

  // Log Value
  println(Serial, now, F(": "), sensorValue, F(" "), temperature);
#if USE_SD
  if (now > nextLogTime) {
    nextLogTime = now + logInterval;
    println(logfile, now, F(": "), sensorValue, F(" "), temperature);
  }
#endif

#if USE_DISPLAY
  clearDisplay();
  switch ((now - bootTime).totalseconds() / 3 % 7) {
    case 0:
      println(display, F("Welcome to:"));
      println(display, F("- Plant Water OS -"));
      break;
    case 1:
      println(display, F("Soil moisture:"));
      println(display, sensorValue);
      break;
    case 2:
      println(display, F("Temperature:"));
      println(display, temperature);
      break;
    case 3:
      println(display, F("Current time:"));
      println(display, now);
      break;
    case 4:
      println(display, F("Next pump time:"));
      println(display, nextPumpTime);
      break;
    case 5:
      println(display, F("Online since:"));
      println(display, bootTime);
      break;
    case 6:
      println(display, F("Total pump events:"));
      println(display, numPumpEvents);
      break;
  }
  display.display();
#endif

#if USE_PUMP
  if (now > nextPumpTime and isWaterTime(now)) {
    nextPumpTime = now + pumpInterval;
    numPumpEvents += 1;

    clearDisplay();

    println(Serial, now, F(": Pump event "), numPumpEvents, F("starting"));
    println(logfile, now, F(": Pump event "), numPumpEvents, F(" starting"));
    println(display, F("PUMPING.."));
    display.display();

    digitalWrite(PUMP_PIN, PUMP_ON);
    delay(PUMP_DURATION);
    digitalWrite(PUMP_PIN, PUMP_OFF);

    now = rtc.now();
    println(Serial, now, F(": Pump event "), numPumpEvents, F(" stopped"));
    println(logfile, now, F(": Pump event "), numPumpEvents, F(" stopped"));

    println(display, F("DONE.."));
    display.display();
  }
#endif

  delay(1000);
}

void clearDisplay() {
#if USE_DISPLAY
  display.clearDisplay();
  display.setCursor(1, 0);
#endif
}
