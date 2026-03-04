#! /usr/bin/env bash

VERSION=v5.5

# Not using `-u $UID` for now, because it doesn't work as well with --device:
options=(--rm -v "$PWD:/project" -w /project)

if [[ -e /dev/ttyUSB0 ]]; then
    options+=(--device /dev/ttyUSB0:/dev/ttyUSB0)
fi

if [[ $# -gt 0 ]]; then
    docker run "${options[@]}" -it espressif/idf:$VERSION idf.py "$@"
else
    docker run "${options[@]}" -it espressif/idf:$VERSION
fi
