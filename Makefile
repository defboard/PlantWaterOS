ifeq ($(PORT),)
PORT := /dev/ttyUSB0
endif

compile:
	arduino-cli compile -b arduino:avr:nano \
		--build-property 'compiler.cpp.extra_flags="-std=c++11"' \
		--build-property 'compiler.cpp.extra_flags="-Werror"' \
		--warnings all

upload:
	arduino-cli upload -b arduino:avr:nano -p $(PORT)

monitor:
	arduino-cli monitor -p $(PORT) -b arduino:avr:nano

install-libs:
	arduino-cli lib install "Adafruit GFX Library" "Adafruit SSD1306" "SdFat" "RTClib"

.SILENT:
compile-silent: compile
