#include "Wifi.h"

#include <WebServer.h>

#include <ElegantOTA.h>


WebServer server(80);


void setup()
{
  Serial.begin(115200);
  Serial.println();

  initWifi();

  server.on("/", []() {
    server.send(200, "text/plain", "Hello from esp32!");
  });

  ElegantOTA.begin(&server);
  server.begin();
}

void loop()
{
  server.handleClient();
  ElegantOTA.loop();
}
