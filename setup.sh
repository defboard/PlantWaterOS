#! /usr/bin/env bash

# Secure boot signing key
if [[ -e secure_boot_signing_key.pem ]]; then
    echo "secure_boot_signing_key.pem: Exists"
else
    echo "secure_boot_signing_key.pem: Creating"
    ./idf.sh secure-generate-signing-key --version 2 secure_boot_signing_key.pem
fi
