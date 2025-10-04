ifeq ($(PORT),)
PORT := /dev/ttyUSB0
endif

ifeq ($(BOARD),)
BOARD := esp32:esp32:esp32
endif

ifeq ($(BAUD),)
BAUD := 115200
endif

compile:
	arduino-cli compile -b $(BOARD) \
		--build-property 'compiler.cpp.extra_flags="-std=c++17"' \
		--build-property 'compiler.cpp.extra_flags="-Werror"' \
		--build-property 'compiler.cpp.extra_flags="-Wno-deprecated-copy"' \
		--warnings all

upload:
	arduino-cli upload -b $(BOARD) -p $(PORT)

monitor:
	arduino-cli monitor -b $(BOARD) -p $(PORT) --config $(BAUD)

install-libs:
	arduino-cli lib install "RTClib" "SD"

.SILENT:
compile-silent: compile
