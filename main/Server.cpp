#include "Server.hpp"

#include "Formatting.hpp"
#include "Json.hpp"
#include "PlantWaterOS.hpp"

#include <WebServer.h>
#include <uri/UriBraces.h>
#include <Stream.h>
#include <ElegantOTA.h>
#include <SD.h>
#include <Preferences.h>

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
void onHttpApiPumpActivate();
void onHttpApiPumpReset();
void onHttpApiPrefsPumpDuration();
void sendFile(int code, const char* content_type, const uint8_t* start, const uint8_t* end);


// Implementation

void initWebServer()
{
  server.on("/", onHttpRoot);
  server.on("/hyperapp.js", onHttpHyperappJs);
  server.on("/file/events.log", onHttpFileEventsLog);
  server.on("/file/sensor.log", onHttpFileSensorLog);
  server.on("/api/status", onHttpApiStatus);
  server.on("/api/pump/activate", onHttpApiPumpActivate);
  server.on("/api/pump/reset", onHttpApiPumpReset);

  server.on(UriBraces("/prefs/pump/duration/{}"), onHttpApiPrefsPumpDuration);

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
  response << '{';
  json_str(response, "serverTime", now);
  json_str(response, "bootTime", bootTime);
  json_str(response, "localIP", WiFi.localIP());
  json_str(response, "prevPumpTime", prevPumpTime);
  json_str(response, "nextPumpTime", nextPumpTime);
  json_str(response, "pumpInterval", pumpInterval);
  json_plain(response, "pumpDuration", getPumpDuration());
  json_plain(response, "numPumpEvents", numPumpEvents);
  json_plain(response, "sensorValue", sensorValue);
  json_str(response, "temperature", temperature, true);
  response << '}';
  server.send(200, "application/json", response);
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

void onHttpApiPrefsPumpDuration()
{
  int pumpDuration = server.pathArg(0).toInt();
  setPumpDuration(pumpDuration);
  onHttpApiStatus();
}
