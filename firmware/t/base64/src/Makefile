BOARD_TAG    = uno
ARDUINO_PORT = /dev/cu.usb*
ARDUINO_LIBS =
ARDUINO_DIR  = /Applications/Arduino.app/Contents/Resources/Java
ARDMK_DIR    = ~/proj/Arduino-Makefile
USER_LIB_PATH = ${HOME}/shared/arduino/imote

include ~/proj/Arduino-Makefile/arduino-mk/Arduino.mk

### flymake

.PHONY: check-syntax
check-syntax:
	etags *.{c,h,cpp}
	$(CC) -x c++ $(CPPFLAGS) $(CXXFLAGS) -Wall -fsyntax-only $(CHK_SOURCES)
