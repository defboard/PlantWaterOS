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

// stdlib
#include <algorithm>


// Pin assignments
const int PIN_RTC_SDA       = 25;
const int PIN_RTC_SCL       = 26;

const int PIN_SD_MOSI       = 21;
const int PIN_SD_MISO       = 18;
const int PIN_SD_SCLK       = 19;
const int PIN_SD_CS         = 22;

const int PIN_SENSOR        = 35;
const int PIN_ENABLE_PUMP   = 27;

const int PIN_BTN_LEFT      = 13;
const int PIN_BTN_MIDDLE    = 12;
const int PIN_BTN_RIGHT     = 14;

// Configuration
const int SERIAL_BAUD_RATE = 115200;

const TimeSpan firstPumpDelay (1/*days*/, 0/*hours*/, 0/*minutes*/, 0/*seconds*/);
TimeSpan pumpInterval         (3/*days*/, 0/*hours*/, 0/*minutes*/, 0/*seconds*/);
int pumpDuration = 2000;

const int delaySensorRepeat = 1000;   // [ms]

const int SCREEN_TIMEOUT = 60;        // [s]

// Native display width is W x H = 128 x 64:
const int DISPLAY_WIDTH     = 128;  // OLED display width, in pixels
const int DISPLAY_HEIGHT    = 64;   // OLED display height, in pixels
const int LINE_HEIGHT       = 16;   // 8 * textHeight
const int DISPLAY_RESET_PIN = -1;   // Reset pin # (or -1 if sharing Arduino reset pin)
const int DISPLAY_ADDRESS   = 0x3C; // I2C address

const long MIN_BUTTON_HOLD_TIME = 30;   // [ms]


struct DeviceOperationMessage {
  MessageType type;
  const void* data;
};

enum ActionType
{
  Cancel = 0,
  ResetTimer = 1,
  PumpNow = 2
};


struct DisplaySection {
  int start_line, num_lines;
  String content;

  void write(const String& new_content);
};


// Globals
RTC_DS3231 rtc;

Adafruit_SSD1306 display(DISPLAY_WIDTH, DISPLAY_HEIGHT, &Wire, DISPLAY_RESET_PIN);
DisplaySection contentArea { 1, 2, "" };
DisplaySection notifyArea { 4, 1, "" };

StreamString eventLog;
LogFile logfile("/sensor.log", Serial);

DateTime prevPumpTime;
DateTime nextPumpTime;

int numPumpEvents = 0;

int sensorValue = 0;
Temperature temperature;
RingBuffer<SensorRecord, 30> sensorRecordsA;
RingBuffer<SensorRecord, 40> sensorRecordsB;
RingBuffer<SensorRecord, 1008> sensorRecordsC;

Button buttonLeft(PIN_BTN_LEFT);
Button buttonMiddle(PIN_BTN_MIDDLE);
Button buttonRight(PIN_BTN_RIGHT);

QueueHandle_t opMessageQueue = NULL;
TickType_t loopLastWakeTime = 0;
TimerHandle_t pumpStopTimer;

// Defined by arduino core
extern TaskHandle_t loopTaskHandle;


// Forward declarations
bool updateDisplay();
bool handleButtons();
void readSensor();
void enablePump(bool enable);
DateTime readNextPumpTime();
void writeNextPumpTime(const DateTime& nextPumpTime);
void opMessageTask(void*);
void onPumpStopTimer(TimerHandle_t);
bool _setSystemTime(DateTime newTime, bool adjustRtc);
void _setNextPumpTime();


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
  pinMode(PIN_BTN_LEFT, INPUT_PULLUP);
  pinMode(PIN_BTN_MIDDLE, INPUT_PULLUP);
  pinMode(PIN_BTN_RIGHT, INPUT_PULLUP);
  // buttonMiddle.begin();

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

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

  // Init boot time
  _setSystemTime(rtc.now(), false);
  const DateTime bootTime = getBootTime();
  logfile.open();
  logfile << bootTime << ": system booted - "
    << (rtc.lostPower()
        ? "RTC power loss"
        : "RTC remained powered")
    << endl;

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

  if (digitalRead(PIN_BTN_MIDDLE) == HIGH)
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
int screenTimeout = 0;
int actionScreen = 0;
int numActionScreens = 3;
bool isActionScreenActive = false;

bool pumpIsStarted = false;

void loop()
{
  ++counter;

  if (ulTaskNotifyTake(/* xClearCountOnExit */ pdTRUE, /* xTicksToWait */ 0)) {
    sendMessage(MessageType::PumpStop, nullptr, portMAX_DELAY);
  }

  bool anyButtonIsPressed = handleButtons();
  if (anyButtonIsPressed) {
    screenTimeout = SCREEN_TIMEOUT;
  }

  if (counter % 100 == 0) {
    counter = 0;
    if (screenTimeout >= 0) {
      --screenTimeout;
    }
    if (screenTimeout == 0) {
      screen = 0;
      updateDisplay();
    }
    sendMessage(MessageType::SensorRead);
    sendMessage(MessageType::ScreenRefresh);

    if (not anyButtonIsPressed) {
      if (now() >= nextPumpTime) {
        sendMessage(MessageType::PumpStart);
      }
    }
  }

  vTaskDelayUntil(&loopLastWakeTime, pdMS_TO_TICKS(10));
}


void updateActionInfoLine()
{
  if (isActionScreenActive) {
    switch (actionScreen) {
      case ActionType::Cancel:
        sendMessage(MessageType::ScreenInfoLine, "[OK]: Cancel");
        break;
      case ActionType::ResetTimer:
        sendMessage(MessageType::ScreenInfoLine, "[OK]: Reset timer");
        break;
      case ActionType::PumpNow:
        sendMessage(MessageType::ScreenInfoLine, "[OK]: Pump now!");
        break;
    }
  }
}

void executeSelectedAction()
{
  sendMessage(MessageType::ScreenInfoLine, "");
  switch (actionScreen) {
    case ActionType::Cancel:
      break;
    case ActionType::ResetTimer:
      sendMessage(MessageType::PumpTimerReset);
      break;
    case ActionType::PumpNow:
      sendMessage(MessageType::PumpStart);
      break;
  }
}


bool handleButtons()
{
  bool anyButtonPressed = false;

  {
    const ButtonEvent btnL = buttonLeft.getEvent();
    if (btnL.type == ButtonEvent::Release && btnL.millis > MIN_BUTTON_HOLD_TIME) {
      if (isActionScreenActive) {
        sendMessage(MessageType::ActionCyclePrev);
      }
      else {
        sendMessage(MessageType::ScreenCyclePrev);
      }
    }

    if (btnL.type != ButtonEvent::Up) {
      anyButtonPressed = true;
    }
  }

  {
    const ButtonEvent btnR = buttonRight.getEvent();
    if (btnR.type == ButtonEvent::Release && btnR.millis > MIN_BUTTON_HOLD_TIME) {
      if (isActionScreenActive) {
        sendMessage(MessageType::ActionCycleNext);
      }
      else {
        sendMessage(MessageType::ScreenCycleNext);
      }
    }

    if (btnR.type != ButtonEvent::Up) {
      anyButtonPressed = true;
    }
  }

  {
    const ButtonEvent btnM = buttonMiddle.getEvent();
    if (btnM.type == ButtonEvent::Release && btnM.millis > MIN_BUTTON_HOLD_TIME) {
      if (isActionScreenActive) {
        isActionScreenActive = false;
        executeSelectedAction();
      }
      else {
        actionScreen = 0;
        isActionScreenActive = true;
        updateActionInfoLine();
      }
    }

    if (btnM.type != ButtonEvent::Up) {
      anyButtonPressed = true;
    }
  }

  return anyButtonPressed;
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
  sensorValue = analogReadMilliVolts(PIN_SENSOR);
  temperature.degreeCelsius = rtc.getTemperature();

  sensorRecordsA.push_back( SensorRecord { now().unixtime(), sensorValue } );

  if (sensorRecordsA.pos() == 0) {
    // Compute median over first sensorRecordsA (i.e. short term log):
    const size_t numSensorRepeat = sensorRecordsA.capacity();
    std::array<SensorRecord, numSensorRepeat> sensorReadingRepetitions;
    std::copy(
        sensorRecordsA.begin(),
        sensorRecordsA.end(),
        sensorReadingRepetitions.begin());
    std::sort(
        sensorReadingRepetitions.begin(),
        sensorReadingRepetitions.end(),
        [](const SensorRecord& a, const SensorRecord& b) {
          return a.value < b.value;
        });

    const auto& q25 = sensorReadingRepetitions[numSensorRepeat * 1 / 4];
    const auto& q50 = sensorReadingRepetitions[numSensorRepeat * 2 / 4];
    const auto& q75 = sensorReadingRepetitions[numSensorRepeat * 3 / 4];
    sensorRecordsB.push_back(q50);

    // Move first median into long-term log:
    if (sensorRecordsB.pos() == 0) {
      sensorRecordsC.push_back(sensorRecordsB[0]);
    }

    // Log first median:
    if (sensorRecordsB.pos() == 1) {
      logfile.open();
      logfile << now() << ": " << temperature
        << " " << q25.value
        << " " << q50.value
        << " " << q75.value
        << endl;
    }
  }
}

bool updateDisplay()
{
  StreamString display;
  switch (screen) {
    case 0:
      // show black screen
      break;
    case 1:
      display << "- Plant Water OS -" << endl;
      display << now() << endl;
      break;
    case 2:
      display << "Soil moisture: " << sensorValue << endl;
      display << "Temperature: " << temperature << endl;
      break;
    case 3:
      display << "Next pouring: " << (nextPumpTime - now()) << endl;
      display << "Total pourings: " << numPumpEvents << endl;
      break;
    case 4:
      display << "IP: " << WiFi.localIP() << endl;
      display << "Uptime: " << (now() - getBootTime()) << endl;
      break;
  }
  contentArea.write(display);
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

DateTime _newSystemTime;

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
      notifyArea.write((const char*) message.data);
      break;

    case MessageType::ActionCyclePrev:
      actionScreen = wrapRange(actionScreen - 1, 0,  numActionScreens - 1);
      updateActionInfoLine();
      break;

    case MessageType::ActionCycleNext:
      actionScreen = wrapRange(actionScreen + 1, 0,  numActionScreens - 1);
      updateActionInfoLine();
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

      prevPumpTime = now();
      nextPumpTime = prevPumpTime + pumpInterval;
      writeNextPumpTime(nextPumpTime);

      numPumpEvents += 1;
      logfile.open();
      eventLog << prevPumpTime << ": Pump event " << numPumpEvents << " (" << pumpDuration << "ms)" << endl;

      notifyArea.write("PUMPING...");

      enablePump(true);
      break;

    case MessageType::PumpStop:
      enablePump(false);
      pumpIsStarted = false;

      notifyArea.write("");
      break;

    case MessageType::PumpTimerReset:
      nextPumpTime = now() + firstPumpDelay;
      writeNextPumpTime(nextPumpTime);
      break;

    case MessageType::SensorRead:
      readSensor();
      break;

    case MessageType::SetSystemTime:
      _setSystemTime(_newSystemTime, true);
      break;

    case MessageType::SetNextPumpTime:
      _setNextPumpTime();
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

void DisplaySection::write(const String& new_content)
{
  if (content != new_content) {
    content = new_content;
    display.fillRect(0, (start_line - 1) * LINE_HEIGHT, DISPLAY_WIDTH, LINE_HEIGHT * num_lines, SSD1306_BLACK);
    display.setCursor(1, (start_line - 1) * LINE_HEIGHT);
    display.print(content);
    display.display();
  }
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
  if (duration == pumpDuration) {
    return true;
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
  if (seconds == pumpInterval.totalseconds()) {
    return true;
  }

  pumpInterval = TimeSpan(seconds);

  return prefs.putLong("pump-interval", seconds);
}

DateTime now()
{
    uint32_t unixtime = time(NULL);
    return DateTime(unixtime);
}

DateTime getBootTime()
{
    uint32_t unixtime = time(NULL);
    uint64_t uptime = esp_timer_get_time() / 1'000'000;
    return DateTime(unixtime - uptime);
}

bool setSystemTime(DateTime systemTime)
{
  if (systemTime.isValid()) {
    _newSystemTime = systemTime;
    sendMessage(MessageType::SetSystemTime, nullptr, portMAX_DELAY);
    return true;
  }
  return false;
}

bool _setSystemTime(DateTime newTime, bool adjustRtc)
{
  if (not newTime.isValid()) {
    return false;
  }
  const TimeSpan delta = newTime - now();
  if (delta.totalseconds() == 0) {
    return true;
  }

  timeval tv;
  tv.tv_sec = newTime.unixtime();
  tv.tv_usec = 0;

  bool success = settimeofday(&tv, NULL) == 0;

  if (success and adjustRtc) {
    rtc.adjust(newTime);
  }
  return success;
}

DateTime _newPumpTime;

bool setNextPumpTime(DateTime pumpTime)
{
  if (pumpTime.isValid()) {
    _newPumpTime = pumpTime;
    sendMessage(MessageType::SetNextPumpTime, nullptr, portMAX_DELAY);
    return true;
  }
  return false;
}

void _setNextPumpTime()
{
  if (_newPumpTime == nextPumpTime) {
    return;
  }
  nextPumpTime = _newPumpTime;
  writeNextPumpTime(nextPumpTime);
}

// vim: sw=2 ts=2 sts=2
