#define USE_SERIAL    1
#define USE_SD        1
#define USE_DISPLAY   1
#define USE_PUMP      1

#include "PlantWaterOS.h"

#include <Wire.h>
#include <RTClib.h>
#include <FreeStack.h>

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

const int BTN_OUT_PIN   = A1;
const int BTN_RCV_PIN   = A3;

const int PUMP_ON       = HIGH;
const int PUMP_OFF      = 1 - PUMP_ON;
const int PUMP_DURATION = 2500;     // [ms]

const TimeSpan logInterval    (0/*days*/, 1/*hours*/, 0/*minutes*/, 0/*seconds*/);
const TimeSpan firstPumpDelay (1/*days*/, 0/*hours*/, 0/*minutes*/, 0/*seconds*/);
const TimeSpan pumpInterval   (3/*days*/, 0/*hours*/, 0/*minutes*/, 0/*seconds*/);

bool isWaterTime(DateTime) {
  return true;
}

# define SPI_CLOCK    SD_SCK_MHZ(40) // 50MHz is maximum but might be unstable
# define SD_CONFIG    SdSpiConfig(SD_CS_PIN, SHARED_SPI, SPI_CLOCK)

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

#if USE_SERIAL
Print & Serial_ = Serial;
#else
DevNull Serial_;
#endif

#if USE_DISPLAY
Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, DISPLAY_RESET_PIN);
#else
NullDisplay display;
#endif

#if USE_SD
LogFile logfile("sensor.log", Serial_);
DateTime nextLogTime;
#else
Print& logfile = Serial_;
#endif

DateTime nextPumpTime;
int numPumpEvents = 0;

int sensorValue = 0;
Temperature temperature;


void setup() {
#if USE_SERIAL
  Serial.begin(9600);
  while (!Serial) { }
  Serial.println(F(""));
#endif

  pinMode(BTN_RCV_PIN, INPUT_PULLUP);
  pinMode(BTN_OUT_PIN, OUTPUT);
  digitalWrite(BTN_OUT_PIN, LOW);

  // needed for RTC and display:
  Wire.begin();
  rtc.begin(&Wire);
  rtc.disable32K();
  rtc.writeSqwPinMode(Ds3231SqwPinMode::DS3231_OFF);

#if USE_SD
  if (!SD.begin(SD_CONFIG)) {
    SD.initErrorHalt(&Serial);
    die();
  }
#endif

  bootTime = rtc.now();
  logfile << bootTime << F(": system booted - ")
    << (rtc.lostPower()
        ? F("RTC power loss")
        : F("RTC remained powered"))
    << endl;

#if USE_SD
  nextLogTime = bootTime;
#endif

#if USE_DISPLAY
  if (!display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDRESS, true, false)) {
    Serial_ << F("Display initialization failed.") << endl;
    die();
  }
  display.clearDisplay();
  display.display();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
#endif

#if USE_PUMP
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

  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, PUMP_OFF);
#endif
}

int counter = 0;
int screen = 0;
int num_screens = 5;
bool buttonWasDown = false;
long buttonDownStart = 0;

void loop()
{
  ++counter;

  if (handleButton()) {
  }
  else if (counter % 100 == 0) {
    counter = 0;
    readSensor();
    handlePump() || updateDisplay();
  }
  delay(10);
}

bool handleButton()
{
  bool buttonIsDown = digitalRead(BTN_RCV_PIN) == LOW;

  if (buttonIsDown) {
    if (buttonWasDown)
    {
      long buttonDownTime = millis() - buttonDownStart;
      if (buttonDownTime > 9000)
      {
        clearDisplay();
        display << F("It's over 9000!") << endl;
      }
      else if (buttonDownTime > 6000)
      {
        clearDisplay();
        display << F("Pump now!") << endl;
      }
      else if (buttonDownTime > 3000)
      {
        clearDisplay();
        display << F("Reset pump timer") << endl;
      }
      display.display();
    }
    else
    {
      buttonDownStart  = millis();
      buttonWasDown = true;
    }
    return true;
  }
  else if (buttonWasDown) {
    now = rtc.now();
    buttonWasDown = false;
    long buttonDownTime = millis() - buttonDownStart;
    if (buttonDownTime > 9000)
    {
      // action cancelled; do nothing
    }
    else if (buttonDownTime > 6000)
    {
      activatePump();
    }
    else if (buttonDownTime > 3000)
    {
      nextPumpTime = now + firstPumpDelay;
      writeNextPumpTime(nextPumpTime);
    }
    else if (buttonDownTime > 30)
    {
      screen = (screen + 1) % num_screens;
      updateDisplay();
    }
  }
  return false;
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

void readSensor()
{
  now = rtc.now();
  sensorValue = analogRead(SENSOR_PIN);
  temperature.degreeCelsius = rtc.getTemperature();

  // Log Value
#if USE_SD
  if (now > nextLogTime) {
    nextLogTime = now + logInterval;
    logfile << now << F(": ") << sensorValue << F(" ") << temperature << F(" ") << FreeStack() << "B" << endl;
  }
  else
#endif
  {
    Serial_ << now << F(": ") << sensorValue << F(" ") << temperature << F(" ") << FreeStack() << "B" << endl;
  }
}

bool updateDisplay()
{
#if USE_DISPLAY
  clearDisplay();
  switch (screen % num_screens) {
    case 0:
      // show black screen
      break;
    case 1:
      display << F("- Plant Water OS -") << endl;
      display << now << endl;
      break;
    case 2:
      display << F("Soil moisture: ") << sensorValue << endl;
      display << F("Temperature: ") << temperature << endl;
      break;
    case 3:
      display << F("Next pouring: ") << (nextPumpTime - now) << endl;
      display << F("Total pourings: ") << numPumpEvents << endl;
      break;
    case 4:
      display << F("Uptime: ") << (now - bootTime) << endl;
      display << F("Free RAM: ") << FreeStack() << F(" Byte") << endl;
      break;
  }
  display.display();
#endif
  return true;
}

bool handlePump()
{
#if USE_PUMP
  if (now > nextPumpTime and isWaterTime(now)) {
    activatePump();
    return true;
  }
#endif
  return false;
}

void activatePump()
{
    nextPumpTime = now + pumpInterval;
    writeNextPumpTime(nextPumpTime);
    numPumpEvents += 1;

    clearDisplay();

    logfile << now << F(": Pump event ") << numPumpEvents << F(" starting") << endl;
    display << F("PUMPING..") << endl;
    display.display();

    digitalWrite(PUMP_PIN, PUMP_ON);
    delay(PUMP_DURATION);
    digitalWrite(PUMP_PIN, PUMP_OFF);

    now = rtc.now();
    logfile << now << F(": Pump event ") << numPumpEvents << F(" stopped") << endl;

    display << F("DONE..") << endl;
    display.display();
}

void clearDisplay() {
#if USE_DISPLAY
  display.clearDisplay();
  display.setCursor(1, 0);
#endif
}
