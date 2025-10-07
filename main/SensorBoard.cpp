#include "WifiCredentials.h"

#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);


void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();
  Serial.print(F("Local IP: "));
  Serial.println(WiFi.localIP());

  server.on("/", []() {
    server.send(200, "text/plain", "Hello from esp32!");
  });

  server.begin();
}

void loop()
{
  server.handleClient();
}
