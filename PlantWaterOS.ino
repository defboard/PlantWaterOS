#define USE_SERIAL    1
#define USE_SD        1
#define USE_DISPLAY   1
#define USE_PUMP      1

#include "PlantWaterOS.h"

#include <Wire.h>
#include <DS3231.h>

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

const uint32_t LOG_INTERVAL = 1 * _HOURS;
const uint32_t PUMP_FIRST_TIME = 1 * _DAYS;
const uint32_t PUMP_INTERVAL = 3 * _DAYS;

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

#if USE_PUMP
DateTime nextPumpTime;
#endif
int numPumpEvents = 0;

int sensorValue = 0;


void setup() {
#if USE_SERIAL
  Serial.begin(9600);
  while (!Serial) { }
  Serial.println(F(""));
#endif

  // needed for RTC and display:
  Wire.begin();
  now = RTClib::now(Wire);

#if USE_SD
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(F("SD initialization failed."));
    die();
  }
  nextLogTime = now;
#endif

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
  nextPumpTime = DateTime(now.unixtime() + PUMP_FIRST_TIME);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, PUMP_OFF);
#endif
}

void loop()
{
  now = RTClib::now(Wire);
  sensorValue = analogRead(SENSOR_PIN);

  // Log Value
  println(Serial, now, F(": "), sensorValue);
#if USE_SD
  if (now.unixtime() > nextLogTime.unixtime()) {
    nextLogTime = DateTime(now.unixtime() + LOG_INTERVAL);
    println(logfile, now, F(": "), sensorValue);
  }
#endif

#if USE_DISPLAY
  clearDisplay();
  println(display, now);
  println(display, F("A: "), sensorValue);
  display.display();
#endif

#if USE_PUMP
  if (now.unixtime() > nextPumpTime.unixtime() and isWaterTime(now)) {
    nextPumpTime = DateTime(now.unixtime() + PUMP_INTERVAL);
    numPumpEvents += 1;

    clearDisplay();

    println(Serial, now, F(": Pump event "), numPumpEvents, F("starting"));
    println(logfile, now, F(": Pump event "), numPumpEvents, F(" starting"));
    println(display, F("PUMPING.."));
    display.display();

    digitalWrite(PUMP_PIN, PUMP_ON);
    delay(PUMP_DURATION);
    digitalWrite(PUMP_PIN, PUMP_OFF);

    now = RTClib::now(Wire);
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
