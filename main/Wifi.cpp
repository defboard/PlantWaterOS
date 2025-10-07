#include "Wifi.h"
#include "WifiCredentials.h"

#include <WiFi.h>


void onWifiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.print(F("Wifi connected: IP "));
  Serial.print(WiFi.localIP());
  Serial.print(F(" / RSSI "));
  Serial.print(WiFi.RSSI());
  Serial.println(F("dB"));
  Serial.println();
}


void onWifiDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.print(F("WiFi disconnected: "));
  Serial.println(info.wifi_sta_disconnected.reason);
  WiFi.reconnect();
}


void initWifi()
{
  Serial.print(F("Connecting to WiFi: "));
  Serial.println(WIFI_SSID);
  WiFi.onEvent(onWifiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(onWifiDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}
