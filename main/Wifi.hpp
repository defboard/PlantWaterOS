#pragma once
#include <WiFiType.h>
#include <Preferences.h>

extern WiFiMode_t WIFI_MODE;
extern String WIFI_SSID;
extern String WIFI_PASSWORD;
extern String WIFI_HOSTNAME;

void stopWifi();
void initWifi();
void loadWifiSettings(Preferences&);

bool disableWifi(Preferences&);
bool setWifiNetwork(Preferences&, WiFiMode_t mode, const String& ssid, const String& password);
bool setWifiHostname(Preferences&, const String& hostname="");
