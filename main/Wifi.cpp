#include "Wifi.hpp"
#include "PlantWaterOS.hpp"

#include <WiFi.h>

#ifndef WIFI_DEFAULT_SSID
# define WIFI_DEFAULT_SSID "PlantWaterNet"
#endif
#ifndef WIFI_DEFAULT_PASSWORD
# define WIFI_DEFAULT_PASSWORD "WaterMyPlants"
#endif
#ifndef WIFI_DEFAULT_HOSTNAME
# define WIFI_DEFAULT_HOSTNAME "PlantWaterOS"
#endif

WiFiMode_t WIFI_MODE = WIFI_AP;
String WIFI_SSID = WIFI_DEFAULT_SSID;
String WIFI_PASSWORD = WIFI_DEFAULT_PASSWORD;
String WIFI_HOSTNAME = WIFI_DEFAULT_HOSTNAME;


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


void stopWifi()
{
  WiFi.mode(WIFI_MODE_NULL);
}


void initWifi()
{
  Serial.print(F("Connecting to WiFi: "));
  Serial.println(WIFI_SSID);
  WiFi.onEvent(onWifiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(onWifiDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  WiFi.setHostname(WIFI_HOSTNAME.c_str());
  WiFi.mode(WIFI_MODE);
  if (WIFI_MODE == WIFI_MODE_STA) {
    WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
    WiFi.begin(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());
  }
  else {
    WiFi.softAP(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());
  }
}


void loadWifiSettings(Preferences& prefs)
{
  WIFI_MODE = prefs.getInt("wifi-mode", WIFI_MODE) == WIFI_STA ? WIFI_MODE_STA : WIFI_MODE_AP;
  WIFI_SSID = prefs.getString("wifi-ssid", WIFI_SSID);
  WIFI_PASSWORD = prefs.getString("wifi-password", WIFI_PASSWORD);
  WIFI_HOSTNAME = prefs.getString("wifi-hostname", WIFI_HOSTNAME);
}


bool setWifiNetwork(Preferences& prefs, WiFiMode_t mode, const String& ssid, const String& password)
{
  // sanity checks
  if (mode != WIFI_MODE_AP and mode != WIFI_MODE_STA) {
    return false;
  }
  if (ssid.length() < 1 or ssid.length() > 32) {
    return false;
  }
  if (password.length() < 1) {
    return false;
  }

  // store settings
  bool success = true;

  if (WIFI_MODE != mode) {
    WIFI_MODE = mode;
    success &= prefs.putInt("wifi-mode", (int) WIFI_MODE);
  }

  if (WIFI_SSID != ssid) {
    WIFI_SSID = ssid;
    success &= prefs.putString("wifi-ssid", WIFI_SSID);
  }

  if (WIFI_PASSWORD != password) {
    WIFI_PASSWORD = password;
    success &= prefs.putString("wifi-password", WIFI_PASSWORD);
  }

  return success;
}

bool setWifiHostname(Preferences& prefs, const String& hostname)
{
  // sanity checks
  if (hostname.length() < 1) {
    return false;
  }

  // store settings
  bool success = true;

  if (WIFI_HOSTNAME != hostname) {
    WIFI_HOSTNAME = hostname;
    success &= prefs.putString("wifi-hostname", WIFI_HOSTNAME);
  }

  return success;
}
