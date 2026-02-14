// Local includes
#include "PlantWaterOS.hpp"

#include "Button.hpp"
#include "Formatting.hpp"
#include "LogFile.hpp"
#include "Wifi.hpp"
#include "Server.hpp"

// Builtin libraries
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <SD.h>
#include <WiFi.h>
#include <Wire.h>
#include <Preferences.h>

// 3rdparty components
#include <Adafruit_SSD1306.h>
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

const int BTN_RCV_PIN       = 23;


// Configuration
const int SERIAL_BAUD_RATE = 9600;

const TimeSpan firstPumpDelay (1/*days*/, 0/*hours*/, 0/*minutes*/, 0/*seconds*/);
const TimeSpan logInterval    (0/*days*/, 0/*hours*/, 20/*minutes*/, 0/*seconds*/);
TimeSpan pumpInterval         (3/*days*/, 0/*hours*/, 0/*minutes*/, 0/*seconds*/);
int pumpDuration = 2000;

const int numSensorRepeat = 20;
const int delaySensorRepeat = 1000;   // [ms]

// Native display width is W x H = 128 x 64:
const int DISPLAY_WIDTH     = 128;  // OLED display width, in pixels
const int DISPLAY_HEIGHT    = 64;   // OLED display height, in pixels
const int LINE_HEIGHT       = 16;   // 8 * textHeight
const int DISPLAY_RESET_PIN = -1;   // Reset pin # (or -1 if sharing Arduino reset pin)
const int DISPLAY_ADDRESS   = 0x3C; // I2C address


struct DeviceOperationMessage {
  MessageType type;
  const void* data;
};


// Globals
RTC_DS3231 rtc;

Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, DISPLAY_RESET_PIN);

StreamString eventLog;
LogFile logfile("/sensor.log", Serial);

DateTime now;
DateTime bootTime;
DateTime nextLogTime;
DateTime prevPumpTime;
DateTime nextPumpTime;

int numPumpEvents = 0;

int sensorValue = 0;
Temperature temperature;
RingBuffer<SensorRecord, 1000> sensorRecords;

Button mainButton(BTN_RCV_PIN);

QueueHandle_t opMessageQueue = NULL;
TickType_t loopLastWakeTime = 0;
TimerHandle_t pumpStopTimer;

// Defined by arduino core
extern TaskHandle_t loopTaskHandle;


// Forward declarations
bool handleButton(ButtonEvent event);
void readSensor();
void enablePump(bool enable);
DateTime readNextPumpTime();
void writeNextPumpTime(const DateTime& nextPumpTime);
void clearDisplayLines(int firstLine, int num=1);
void opMessageTask(void*);
void onPumpStopTimer(TimerHandle_t);


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
  pinMode(BTN_RCV_PIN, INPUT_PULLUP);
  // mainButton.begin();

  // Init RTC
  eventLog << "Init Wire: " << CheckSuccess(Wire.begin(PIN_RTC_SDA, PIN_RTC_SCL)) << endl;
  eventLog << "Init RTC: " << CheckSuccess(rtc.begin(&Wire)) << endl;
  rtc.disable32K();
  rtc.writeSqwPinMode(Ds3231SqwPinMode::DS3231_OFF);

  // Init SD
  eventLog << "Init SPI: " << CheckSuccess(SPI.begin(PIN_SD_SCLK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS)) << endl;
  eventLog << "Init SD: " << CheckSuccess(SD.begin(PIN_SD_CS, SPI)) << endl;
  if (SD.cardType() == CARD_NONE) {
    eventLog.println("No SD card attached");
  }
  uint32_t cardSize = SD.cardSize() / (1024 * 1024);
  eventLog << "SDCard Size: " << cardSize << "MB" << endl;

  // Init nextLogTime
  bootTime = rtc.now();
  logfile.open();
  logfile << bootTime << ": system booted - "
    << (rtc.lostPower()
        ? "RTC power loss"
        : "RTC remained powered")
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
    logfile << "Using RTC pump timer: " << storedTime << endl;
    nextPumpTime = storedTime;
  }
  else
  {
    logfile << "Reset RTC pump timer: " << storedTime << endl;
    nextPumpTime = bootTime + firstPumpDelay;
    writeNextPumpTime(nextPumpTime);
  }

  if (digitalRead(BTN_RCV_PIN) == HIGH)
  {
    Preferences prefs;
    bool open = prefs.begin(PREFS_NAMESPACE, /* readOnly */ true);
    eventLog << "Open prefs: " << CheckSuccess(open) << endl;
    if (open) {
      pumpDuration = prefs.getInt("pump-duration", pumpDuration);
      pumpInterval = TimeSpan(prefs.getLong("pump-interval", pumpInterval.totalseconds()));
      loadWifiSettings(prefs);
    }
  }

  initWifi();
  initWebServer();

  pumpStopTimer = xTimerCreate(
      "pumpStop", pdMS_TO_TICKS(pumpDuration),
      /* xAutoReload */ pdFALSE,
      /* pvTimerID */ (void*) loopTaskHandle,
      onPumpStopTimer);

  // Should not need more than a couple (4) entries, because this task has priority on core 1.
  opMessageQueue = xQueueCreate(4, sizeof(DeviceOperationMessage));
  xTaskCreatePinnedToCore(opMessageTask, "operations", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(handleServer, "server", 4096, NULL, 1, NULL, 0);

  loopLastWakeTime = xTaskGetTickCount();
}

int counter = 0;
int screen = 0;
int num_screens = 5;
int curSensorRepeat = numSensorRepeat;

bool pumpIsStarted = false;

void loop()
{
  ++counter;

  if (ulTaskNotifyTake(/* xClearCountOnExit */ pdTRUE, /* xTicksToWait */ 0)) {
    sendMessage(MessageType::PumpStop, nullptr, portMAX_DELAY);
  }

  ButtonEvent btn1 = mainButton.getEvent();

  bool buttonIsPressed = handleButton(btn1);

  if (counter % 100 == 0) {
    counter = 0;
    sendMessage(MessageType::SensorRead);
    sendMessage(MessageType::ScreenRefresh);

    if (not buttonIsPressed) {
      if (now >= nextPumpTime) {
        sendMessage(MessageType::PumpStart);
      }
    }
  }

  vTaskDelayUntil(&loopLastWakeTime, pdMS_TO_TICKS(10));
}


bool handleButton(ButtonEvent event)
{
  if (event.type == ButtonEvent::Down) {
    if (event.millis > 9000)
    {
      if (event.prevMillis <= 9000) {
        sendMessage(MessageType::ScreenInfoLine, "It's over 9000!");
      }
    }
    else if (event.millis > 6000)
    {
      if (event.prevMillis <= 6000) {
        sendMessage(MessageType::ScreenInfoLine, "Pump now!");
      }
    }
    else if (event.millis > 3000)
    {
      if (event.prevMillis <= 3000) {
        sendMessage(MessageType::ScreenInfoLine, "Reset pump timer");
      }
    }

    return true;
  }

  else if (event.type == ButtonEvent::Release) {
    sendMessage(MessageType::ScreenInfoLine, "");

    if (event.millis > 9000)
    {
      // action cancelled; do nothing
    }
    else if (event.millis > 6000)
    {
      sendMessage(MessageType::PumpStart);
    }
    else if (event.millis > 3000)
    {
      sendMessage(MessageType::PumpTimerReset);
    }
    else if (event.millis > 30)
    {
      sendMessage(MessageType::ScreenCycleNext);
    }

    return true;
  }

  return false;
}


void enablePump(bool enable)
{
  digitalWrite(PIN_ENABLE_PUMP, enable ? HIGH : LOW);
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
    if (curSensorRepeat == numSensorRepeat/2) {
      sensorRecords.push_back(SensorRecord{now.unixtime(), sensorValue});
    }
    ++curSensorRepeat;
  }
}

bool updateDisplay()
{
  clearDisplayLines(1, 2);
  switch (screen) {
    case 0:
      // show black screen
      break;
    case 1:
      display << "- Plant Water OS -" << endl;
      display << now << endl;
      break;
    case 2:
      display << "Soil moisture: " << sensorValue << endl;
      display << "Temperature: " << temperature << endl;
      break;
    case 3:
      display << "Next pouring: " << (nextPumpTime - now) << endl;
      display << "Total pourings: " << numPumpEvents << endl;
      break;
    case 4:
      display << "IP: " << WiFi.localIP() << endl;
      display << "Uptime: " << (now - bootTime) << endl;
      break;
  }
  display.display();
  return true;
}

void onPumpStopTimer(TimerHandle_t timer)
{
  xTaskNotifyGive((TaskHandle_t) pvTimerGetTimerID(timer));
}

bool sendMessage(MessageType type, const void* data, TickType_t waitTime)
{
  const DeviceOperationMessage message { type, data };
  return xQueueSend(opMessageQueue, &message, waitTime) == pdPASS;
}

int wrapRange(int num, int range_min, int range_max)
{
  if (num < range_min) {
    return range_max;
  }
  if (num > range_max) {
    return range_min;
  }
  return num;
}

void dispatchMessage(DeviceOperationMessage message)
{
  switch (message.type) {

    case MessageType::ScreenRefresh:
      if (screen != 0) {
        updateDisplay();
      }
      break;

    case MessageType::ScreenCyclePrev:
      screen = wrapRange(screen - 1, 0, num_screens - 1);
      updateDisplay();
      break;

    case MessageType::ScreenCycleNext:
      screen = wrapRange(screen + 1, 0, num_screens - 1);
      updateDisplay();
      break;

    case MessageType::ScreenInfoLine:
      clearDisplayLines(4);
      display << ((const char*) message.data);
      display.display();
      break;

    case MessageType::PumpStart:
      if (pumpIsStarted) {
        break;
      }
      pumpIsStarted = true;
      if (not xTimerStart(pumpStopTimer, 10)) {
        // Failed to schedule the PumpStop event, so it is not safe to continue;
        pumpIsStarted = false;
        break;
      }

      now = rtc.now();
      prevPumpTime = now;
      nextPumpTime = now + pumpInterval;
      writeNextPumpTime(nextPumpTime);

      numPumpEvents += 1;
      logfile.open();
      eventLog << now << ": Pump event " << numPumpEvents << " (" << pumpDuration << "ms)" << endl;

      clearDisplayLines(4);
      display << "PUMPING...";
      display.display();

      enablePump(true);
      break;

    case MessageType::PumpStop:
      enablePump(false);
      pumpIsStarted = false;

      clearDisplayLines(4);
      display.display();
      break;

    case MessageType::PumpTimerReset:
      now = rtc.now();
      nextPumpTime = now + firstPumpDelay;
      writeNextPumpTime(nextPumpTime);
      break;

    case MessageType::SensorRead:
      readSensor();
      break;

  };
}

void opMessageTask(void* args)
{
  DeviceOperationMessage message;
  while (true) {
    if (xQueueReceive(opMessageQueue, &message, portMAX_DELAY)) {
      dispatchMessage(message);
    }
  }
}

void clearDisplayLines(int line, int num)
{
  display.fillRect(0, (line - 1) * LINE_HEIGHT, DISPLAY_WIDTH, LINE_HEIGHT * num, SSD1306_BLACK);
  display.setCursor(1, (line - 1) * LINE_HEIGHT);
}


int getPumpDuration()
{
  return pumpDuration;
}

bool setPumpDuration(Preferences& prefs, int duration)
{
  if (duration < 100 or duration > 10 * 1000) {
    return false;
  }

  pumpDuration = duration;

  xTimerChangePeriod(
      pumpStopTimer,
      pdMS_TO_TICKS(duration),
      /* blockTime */ 0);

  return prefs.putInt("pump-duration", duration);
}

int32_t getPumpInterval()
{
  return pumpInterval.totalseconds();
}

bool setPumpInterval(Preferences& prefs, int32_t seconds)
{
  if (seconds < 6 * 3600) {
    return false;
  }

  pumpInterval = TimeSpan(seconds);

  return prefs.putLong("pump-interval", seconds);
}

// vim: sw=2 ts=2 sts=2
