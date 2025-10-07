#! /usr/bin/env bash

# Secure boot signing key
if [[ -e secure_boot_signing_key.pem ]]; then
    echo "secure_boot_signing_key.pem: Exists"
else
    echo "secure_boot_signing_key.pem: Creating"
    ./idf.sh idf.py secure-generate-signing-key --version 2 secure_boot_signing_key.pem
fi

# Wifi credentials
if  [[ -e main/WifiCredentials.cpp ]]; then
    echo "main/WifiCredentials.cpp: Exists"
else
    echo "main/WifiCredentials.cpp: Creating"
    read -r -p 'Enter WiFi SSID: ' WIFI_SSID
    read -r -p 'Enter password:  ' WIFI_PASSWORD

	cat >main/WifiCredentials.cpp <<EOF
#include "WifiCredentials.h"

const char* const WIFI_SSID = "$WIFI_SSID";
const char* const WIFI_PASSWORD = "$WIFI_PASSWORD";
EOF

fi
