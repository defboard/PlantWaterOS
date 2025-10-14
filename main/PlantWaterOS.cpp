// Local includes
#include "PlantWaterOS.hpp"

#include "Button.h"
#include "LogFile.hpp"
#include "Wifi.h"

// Builtin libraries
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <SD.h>
#include <StreamString.h>
#include <WebServer.h>
#include <Wire.h>

// 3rdparty components
#include <Adafruit_SSD1306.h>
#include <ElegantOTA.h>
#include <RTClib.h>


// Pin assignments
const int PIN_RTC_SDA       = 33;
const int PIN_RTC_SCL       = 32;

const int PIN_SD_MOSI       = 5;
const int PIN_SD_MISO       = 19;
const int PIN_SD_SCLK       = 18;
const int PIN_SD_CS         = 21;

const int PIN_SENSOR        = 35;
const int PIN_ENABLE_PUMP   = 4;

const int PIN_STICK_BTN     = 25;
const int PIN_STICK_X       = 36;
const int PIN_STICK_Y       = 39;

const int BTN_RCV_PIN       = 23;


// Configuration
const int SERIAL_BAUD_RATE = 9600;
const int PUMP_DURATION = 2000;     // [ms]

const TimeSpan firstPumpDelay (1/*days*/, 0/*hours*/, 0/*minutes*/, 0/*seconds*/);
const TimeSpan pumpInterval   (3/*days*/, 0/*hours*/, 0/*minutes*/, 0/*seconds*/);
const TimeSpan logInterval    (0/*days*/, 0/*hours*/, 20/*minutes*/, 0/*seconds*/);

const int numSensorRepeat = 20;
const int delaySensorRepeat = 1000;   // [ms]

// Native display width is W x H = 128 x 64:
const int DISPLAY_WIDTH     = 128;  // OLED display width, in pixels
const int DISPLAY_HEIGHT    = 64;   // OLED display height, in pixels
const int LINE_HEIGHT       = 16;   // 8 * textHeight
const int DISPLAY_RESET_PIN = -1;   // Reset pin # (or -1 if sharing Arduino reset pin)
const int DISPLAY_ADDRESS   = 0x3C; // I2C address

// Globals
WebServer server(80);

RTC_DS3231 rtc;

Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, DISPLAY_RESET_PIN);

StreamString eventLog;
LogFile logfile("/sensor.log", Serial);

DateTime now;
DateTime bootTime;
DateTime nextLogTime;
DateTime nextPumpTime;

int numPumpEvents = 0;

int sensorValue = 0;
Temperature temperature;

Button mainButton(BTN_RCV_PIN);
Button stickButton(PIN_STICK_BTN);


// Forward declarations
void handleServer(void*);
bool handlePump();
bool handleButton(ButtonEvent event);
void handleJoystick();
void readSensor();
void enablePump(bool enable);
void activatePump();
DateTime readNextPumpTime();
void writeNextPumpTime(const DateTime& nextPumpTime);
void onHttpRoot();
void onHttpEventLog();
bool updateDisplay();
void clearDisplayLines(int firstLine, int num=1);


// Implementation

void setup()
{
  // Init Serial
  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial) {
    // wait for Serial connection
  }
  Serial.println();

  // Init control pins
  pinMode(PIN_ENABLE_PUMP, OUTPUT);
  enablePump(false);
  pinMode(PIN_STICK_X, INPUT);
  pinMode(PIN_STICK_Y, INPUT);
  pinMode(PIN_STICK_BTN, INPUT_PULLUP);
  pinMode(BTN_RCV_PIN, INPUT_PULLUP);
  // mainButton.begin();

  // Init RTC
  eventLog << F("Init Wire: ") << CheckSuccess(Wire.begin(PIN_RTC_SDA, PIN_RTC_SCL));
  eventLog << F("Init RTC: ") << CheckSuccess(rtc.begin(&Wire));
  rtc.disable32K();
  rtc.writeSqwPinMode(Ds3231SqwPinMode::DS3231_OFF);

  // Init SD
  eventLog << F("Init SPI: ") << CheckSuccess(SPI.begin(PIN_SD_SCLK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS));
  eventLog << F("Init SD: ") << CheckSuccess(SD.begin(PIN_SD_CS, SPI));
  if (SD.cardType() == CARD_NONE) {
    eventLog.println(F("No SD card attached"));
  }
  uint32_t cardSize = SD.cardSize() / (1024 * 1024);
  eventLog << F("SDCard Size: ") << cardSize << F("MB") << endl;

  // Init nextLogTime
  bootTime = rtc.now();
  logfile.open();
  logfile << bootTime << F(": system booted - ")
    << (rtc.lostPower()
        ? F("RTC power loss")
        : F("RTC remained powered"))
    << endl;
  nextLogTime = bootTime;

  if (!display.begin(SSD1306_SWITCHCAPVCC, DISPLAY_ADDRESS, true, false)) {
    eventLog << "Display initialization failed." << endl;
  }
  display.clearDisplay();
  display.display();
  display.setTextSize(1, 2);
  display.setTextColor(SSD1306_WHITE);

  // Init nextPumpTime
  const DateTime storedTime = readNextPumpTime();
  if (storedTime > bootTime - TimeSpan(1 /* days */) and
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

  initWifi();

  server.on("/", onHttpRoot);
  server.on("/eventlog", onHttpEventLog);

  ElegantOTA.begin(&server);
  server.begin();

  xTaskCreatePinnedToCore(handleServer, "server", 4096, NULL, 1, NULL, 0);
}

int counter = 0;
int screen = 0;
int num_screens = 5;
int curSensorRepeat = numSensorRepeat;

void loop()
{
  ++counter;

  ButtonEvent btn1 = mainButton.getEvent();
  ButtonEvent btn2 = stickButton.getEvent();

  bool blockPump = handleButton(btn1) or handleButton(btn2);
  handleJoystick();

  if (counter % 100 == 0) {
    counter = 0;
    readSensor();
    if (not blockPump) {
      handlePump();
    }
    updateDisplay();
  }
  delay(10);
}


void handleServer(void* args)
{
  while (true) {
    server.handleClient();
    ElegantOTA.loop();
  }
}


void onHttpRoot()
{
  StreamString response;
  response << R"(<html>
<head>
  <title>PlantWaterOS</title>
</head>
<body>
  <h1>PlantWaterOS</h1>
  <div>Next pump time: )" << nextPumpTime
  << R"(</div>
</body>
</html>
)";

  server.send(200, "text/html", response);
}

void onHttpEventLog()
{
  server.send(200, "text/plain", eventLog);
}

bool handleButton(ButtonEvent event)
{
  if (event.type == ButtonEvent::Down) {
    clearDisplayLines(4);
    if (event.millis > 9000)
    {
      display << F("It's over 9000!") << endl;
    }
    else if (event.millis > 6000)
    {
      display << F("Pump now!") << endl;
    }
    else if (event.millis > 3000)
    {
      display << F("Reset pump timer") << endl;
    }
    display.display();

    return true;
  }

  else if (event.type == ButtonEvent::Release) {
    clearDisplayLines(4);

    now = rtc.now();
    if (event.millis > 9000)
    {
      // action cancelled; do nothing
    }
    else if (event.millis > 6000)
    {
      activatePump();
    }
    else if (event.millis > 3000)
    {
      nextPumpTime = now + firstPumpDelay;
      writeNextPumpTime(nextPumpTime);
    }
    else if (event.millis > 30)
    {
      screen = (screen + 1) % num_screens;
      updateDisplay();
    }

    return true;
  }

  return false;
}


int old_xdir = 0;
int old_ydir = 0;
TickType_t old_xtick = 0;
TickType_t old_ytick = 0;


int joystick_direction(int val) {
  const int MID = 2048;
  const int THRESH = MID / 2;
  if (val < MID - THRESH) {
    return -1;
  }
  if (val > MID + THRESH) {
    return 1;
  }
  return 0;
}

void handleJoystick()
{
  const TickType_t tick = xTaskGetTickCount();
  const TickType_t delta_xtick = tick - old_xtick;
  const TickType_t delta_ytick = tick - old_ytick;

  const int new_vx = analogRead(PIN_STICK_X);
  const int new_vy = analogRead(PIN_STICK_Y);
  const int new_xdir = -joystick_direction(new_vx);
  const int new_ydir = joystick_direction(new_vy);

  if (new_xdir == old_xdir) {
    old_xtick = tick;
  }
  else if (delta_xtick > 50) {
    old_xtick = tick;
    old_xdir = new_xdir;

    if (new_xdir != 0) {
      screen = (screen + num_screens + new_xdir) % num_screens;
      updateDisplay();
    }
  }

  if (new_ydir == old_ydir) {
    old_ytick = tick;
  }
  else if (delta_ytick > 50) {
    old_ytick = tick;
    old_ydir = new_ydir;

    clearDisplayLines(4);
    if (new_ydir < 0) {
      display << "Up!";
    }
    else if (new_ydir > 0) {
      display << "Down!";
    }
    display.display();
  }
}

void enablePump(bool enable)
{
  digitalWrite(PIN_ENABLE_PUMP, enable ? HIGH : LOW);
}

void activatePump()
{
  logfile.open();

  numPumpEvents += 1;
  eventLog << now << F(": Pump event ") << numPumpEvents << F(" (") << PUMP_DURATION << F("ms)") << endl;

  clearDisplayLines(4);
  display << "PUMPING...";
  display.display();

  enablePump(true);
  delay(PUMP_DURATION);
  enablePump(false);

  display << " DONE" << endl;
  display.display();
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
  sensorValue = analogRead(PIN_SENSOR);
  temperature.degreeCelsius = rtc.getTemperature();

  if (now >= nextLogTime) {
    nextLogTime = now + logInterval;
    curSensorRepeat = 0;
  }
  if (curSensorRepeat < numSensorRepeat) {
    logfile.open();
    logfile << now << ": " << curSensorRepeat << " " << temperature << " " << sensorValue << endl;
    ++curSensorRepeat;
  }
}

bool updateDisplay()
{
  clearDisplayLines(1, 2);
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
      display << F("IP: ") << WiFi.localIP() << endl;
      display << F("Uptime: ") << (now - bootTime) << endl;
      break;
  }
  display.display();
  return true;
}

bool handlePump()
{
  if (now >= nextPumpTime) {
    nextPumpTime = now + pumpInterval;
    writeNextPumpTime(nextPumpTime);
    activatePump();
    return true;
  }
  return false;
}

void clearDisplayLines(int line, int num)
{
  display.fillRect(0, (line - 1) * LINE_HEIGHT, DISPLAY_WIDTH, LINE_HEIGHT * num, SSD1306_BLACK);
  display.setCursor(1, (line - 1) * LINE_HEIGHT);
}
