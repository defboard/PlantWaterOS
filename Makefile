compile:
	arduino-cli compile -b arduino:avr:nano \
		--build-property 'compiler.cpp.extra_flags="-std=c++11"' \
		--build-property 'compiler.cpp.extra_flags="-Werror"' \
		--warnings all

upload:
	arduino-cli upload -b arduino:avr:nano -p /dev/ttyUSB0

monitor:
	arduino-cli monitor -p /dev/ttyUSB0 -b arduino:avr:nano

install-libs:
	arduino-cli lib install "Adafruit GFX Library" "Adafruit SSD1306" "SD" "DS3231"

.SILENT:
compile-silent: compile
