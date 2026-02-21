#include "Server.hpp"

#include "Formatting.hpp"
#include "Json.hpp"
#include "PlantWaterOS.hpp"
#include "Wifi.hpp"

#include <WebServer.h>
#include <Stream.h>
#include <ElegantOTA.h>
#include <SD.h>
#include <Preferences.h>

#include <optional>

// Globals
WebServer server(80);

extern const uint8_t static_index_html_start[]  asm("_binary_index_html_gz_start");
extern const uint8_t static_index_html_end[]    asm("_binary_index_html_gz_end");

extern const uint8_t static_hyperapp_js_start[] asm("_binary_hyperapp_js_gz_start");
extern const uint8_t static_hyperapp_js_end[]   asm("_binary_hyperapp_js_gz_end");


// Forward declarations
void onHttpRoot();
void onHttpHyperappJs();
void onHttpFileEventsLog();
void onHttpFileSensorLog();
void onHttpApiStatus();
void onHttpApiServerReboot();
void onHttpApiServerTime();
void onHttpApiPumpActivate();
void onHttpApiPumpReset();
void onHttpApiPrefsPost();
void sendFile(int code, const char* content_type, const uint8_t* start, const uint8_t* end);


namespace {
  std::optional<WiFiMode_t> strToWifiMode(const String& wifi_mode)
  {
    if (wifi_mode == "OFF") { return WIFI_OFF; }
    if (wifi_mode == "STA") { return WIFI_STA; }
    if (wifi_mode == "AP") { return WIFI_AP; }
    return std::nullopt;
  }

  const char* dumpWifiMode(WiFiMode_t wifi_mode)
  {
    if (wifi_mode == WIFI_OFF) { return "OFF"; }
    if (wifi_mode == WIFI_STA) { return "STA"; }
    if (wifi_mode == WIFI_AP) { return "AP"; }
    return "OFF";
  }
}


// Implementation

void initWebServer()
{
  server.on("/", onHttpRoot);
  server.on("/hyperapp.js", onHttpHyperappJs);
  server.on("/file/events.log", onHttpFileEventsLog);
  server.on("/file/sensor.log", onHttpFileSensorLog);
  server.on("/api/status", onHttpApiStatus);
  server.on("/api/server/reboot", onHttpApiServerReboot);
  server.on("/api/server/time", onHttpApiServerTime);
  server.on("/api/pump/activate", onHttpApiPumpActivate);
  server.on("/api/pump/reset", onHttpApiPumpReset);
  server.on("/prefs", HTTPMethod::HTTP_POST, onHttpApiPrefsPost);

  ElegantOTA.begin(&server);
  server.begin();
}

void handleServer(void* args)
{
  while (true) {
    server.handleClient();
    ElegantOTA.loop();
  }
}

void sendFile(int code, const char* content_type, const uint8_t* start, const uint8_t* end)
{
  const int size = end - start;
  server.setContentLength(size);
  server.send(code, content_type);
  server.sendContent((const char*) start, size);
}

void onHttpRoot()
{
  server.sendHeader("Content-Encoding", "gzip");
  sendFile(200, "text/html", static_index_html_start, static_index_html_end);
}

void onHttpHyperappJs()
{
  server.sendHeader("Content-Encoding", "gzip");
  sendFile(200, "text/javascript", static_hyperapp_js_start, static_hyperapp_js_end);
}

void onHttpFileEventsLog()
{
  server.send(200, "text/plain", eventLog);
}

void onHttpFileSensorLog()
{
  // The File will be able to read all the content that was there at the
  // time of opening, but not more. It might be nice to guard the SD.open()
  // call with a mutex against multi-line write operations to ensure that
  // only fully written lines can be seen.
  File logfile = SD.open("/sensor.log", FILE_READ, /* create */ false);

  // No mutex guards needed to protect read access to the log file because the
  // SD API is thread-safe as long as CONFIG_DISABLE_HAL_LOCKS is not set.
  server.streamFile(logfile, "text/plain", 200);
}

void onHttpApiStatus()
{
  StreamString response;
  JsonWriter json(response);

  json.put_object();
  json.put_string("serverTime", now);
  json.put_string("bootTime", bootTime);
  json.put_string("localIP", WiFi.localIP());
  json.put_string("wifiMode", dumpWifiMode(WIFI_MODE));
  json.put_string("wifiSsid", WIFI_SSID);
  json.put_string("wifiHostname", WIFI_HOSTNAME);
  json.put_string("prevPumpTime", prevPumpTime);
  json.put_string("nextPumpTime", nextPumpTime);
  json.put_string("pumpInterval", getPumpInterval());

  json.put_plain("pumpDuration", getPumpDuration());
  json.put_plain("numPumpEvents", numPumpEvents);
  json.put_plain("sensorValue", sensorValue);

  json.put_array("sensorData");
  for (const SensorRecord& record : sensorRecords) {
    json.put_array();
    json.put_plain(record.time);
    json.put_plain(record.value);
    json.end_array();
  }
  json.end_array();

  json.put_string("temperature", temperature);
  json.end_object();

  server.send(200, "application/json", response);
}

void onHttpApiServerReboot()
{
  server.send(200, "application/json", "{}");
  delay(100);
  esp_restart();
}

void onHttpApiServerTime()
{
  if (server.hasArg("serverTime")) {
    String serverTime = server.arg("serverTime");
    setSystemTime(serverTime.c_str());            // ISO 8601 format
    delay(100);
  }
  onHttpApiStatus();
}

void onHttpApiPumpActivate()
{
  sendMessage(MessageType::PumpStart);
  delay(100);
  onHttpApiStatus();
}

void onHttpApiPumpReset()
{
  sendMessage(MessageType::PumpTimerReset);
  delay(100);
  onHttpApiStatus();
}

void onHttpApiPrefsPost()
{
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, /* readOnly */ false);

  // Pump settings
  if (server.hasArg("pumpDuration")) {
    int pumpDuration = server.arg("pumpDuration").toInt();
    setPumpDuration(prefs, pumpDuration);
  }

  if (server.hasArg("pumpInterval")) {
    int32_t pumpInterval = server.arg("pumpInterval").toInt();
    setPumpInterval(prefs, pumpInterval);
  }

  // WiFi settings
  String wifi_mode = server.arg("wifiMode");
  String wifi_ssid = server.arg("wifiSsid");
  String wifi_password = server.arg("wifiPassword");
  String wifi_hostname = server.arg("wifiHostname");

  std::optional<WiFiMode_t> mode = strToWifiMode(wifi_mode);

  bool restart_wifi = false;

  if (mode == WIFI_OFF and WIFI_MODE != WIFI_OFF) {
    disableWifi(prefs);
    restart_wifi = true;
  }
  if (mode and mode != WIFI_OFF and wifi_ssid.length() > 0 and wifi_password.length() > 0) {
    setWifiNetwork(prefs, *mode, wifi_ssid, wifi_password);
    restart_wifi = true;
  }
  if (mode and mode != WIFI_OFF and wifi_hostname.length() > 0) {
    setWifiHostname(prefs, wifi_hostname);
    restart_wifi = true;
  }

  // Send status update before disconnecting WiFi
  onHttpApiStatus();

  if (restart_wifi) {
    stopWifi();
    initWifi();
  }
}
