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

const int PUMP_ON       = LOW;
const int PUMP_OFF      = 1 - PUMP_ON;
const int PUMP_DURATION = 2500;     // [ms]

const uint32_t PUMP_INTERVAL = 3 * _DAYS;

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
#endif

#if USE_SD
LogFile logfile("sensor.log");
#endif

#if USE_PUMP
DateTime lastPumpTime;
#endif

int counter = 0;
int sensorValue = 0;


void setup() {
#if USE_SERIAL
  Serial.begin(9600);
  while (!Serial) { }
  Serial.println(F(""));
#endif

  // needed for RTC and display:
  Wire.begin();

#if USE_SD
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(F("SD initialization failed."));
    die();
  }
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
  lastPumpTime = RTClib::now(Wire);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, PUMP_OFF);
#endif
}

void loop()
{
  now = RTClib::now(Wire);
  sensorValue = analogRead(SENSOR_PIN);

  // Log Value
  logSensorValue(Serial, now, sensorValue);
  if (counter % 10 == 2) {
    logSensorValue(logfile, now, sensorValue);
  }
  counter += 1;

#if USE_DISPLAY
  clearDisplay();
  printDateTime(display, now);
  display.println(F(""));
  display.print(F("A: "));
  display.println(sensorValue);
  display.display();
#endif

#if USE_PUMP
  if (now.unixtime() - lastPumpTime.unixtime() > PUMP_INTERVAL) {
    lastPumpTime = now;

    clearDisplay();

    logTo(Serial, now, F("Pump starting"));
    logTo(logfile, now, F("Pump starting"));
#if USE_DISPLAY
    display.println(F("PUMPING.."));
    display.display();
#endif

    digitalWrite(PUMP_PIN, PUMP_ON);
    delay(PUMP_DURATION);
    digitalWrite(PUMP_PIN, PUMP_OFF);

    now = RTClib::now(Wire);
    logTo(Serial, now, F("Pump stopped"));
    logTo(logfile, now, F("Pump stopped"));
#if USE_DISPLAY
    display.println(F("DONE.."));
    display.display();
#endif
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
