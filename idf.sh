#! /usr/bin/env bash

VERSION=v6.1

# Not using `-u $UID` for now, because it doesn't work as well with --device:
options=(--rm -v "$PWD:/project" -w /project)
options+=(-e IDF_GIT_SAFE_DIR=/project)

# The following would be ideal to automatically mount /dev/ttyUSB* and
# /dev/ttyACM* devices as they become available. Unfortunately, it is not
# supported in rootless mode:
#
#   options+=( --device-cgroup-rule='c 166:* rwm' )   # ttyACM devices
#   options+=( --device-cgroup-rule='c 188:* rwm' )   # ttyUSB devices
#
# Instead, we currently have to manually pass the device at start time:
for device in /dev/ttyUSB* /dev/ttyACM*; do
    if [[ -e "$device" ]]; then
        options+=(--device "$device")
    fi
done

# Needed to forward membership in dialout/uucp group to rootless container:
# (Note that you also need to setup /etc/subuid and /etc/subgid appropriately)
options+=(--runtime crun --group-add keep-groups)

if [[ $# -gt 0 ]]; then
    podman run "${options[@]}" -it docker.io/espressif/idf:$VERSION idf.py "$@"
else
    podman run "${options[@]}" -it docker.io/espressif/idf:$VERSION
fi
