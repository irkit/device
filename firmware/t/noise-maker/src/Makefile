BOARD_TAG    = leonardo
ARDUINO_PORT = /dev/cu.usb*
# ARDUINO_LIBS = SoftwareSerial BGLib
ARDUINO_DIR  = /Applications/Arduino.app/Contents/Resources/Java
ARDMK_DIR    = ~/src/github.com/sudar/Arduino-Makefile
USER_LIB_PATH = ../../lib

include ~/src/github.com/sudar/Arduino-Makefile/Arduino.mk

### flymake

.PHONY: check-syntax
check-syntax:
	etags *.{c,h,cpp}
	$(CC) -x c++ $(CPPFLAGS) $(CXXFLAGS) -Wall -fsyntax-only $(CHK_SOURCES)
