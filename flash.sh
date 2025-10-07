#! /usr/bin/env bash
set -e

cd build
python -m esptool --chip esp32 -b 460800 \
    --before default_reset \
    --after hard_reset write_flash "@flash_args"
