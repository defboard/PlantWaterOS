Run ./setup.sh in order to create the following files required for building:

    secure_boot_signing_key.pem
    main/WifiCredentials.cpp

# Flashing

For flashing via USB either `flash.sh` or `idf.py flash` can be used.

For flashing via OTA, the webinterface on `http://host:/` provides an "Update" tab.

There is also a minimal webinterface on `http://host:/update` in case the default
webinterface is broken for some reason (e.g. javascript exception).
Alternatively, you could use curl to upload directly, e.g.:

    curl -X POST -F file=@build/PlantWaterOS.bin http://host/update
