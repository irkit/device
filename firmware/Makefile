# This Makefile is only used to check .lst files

SOURCEDIR = src/IRKit
BUILDDIR = .build/irkit
CPPSOURCES = $(wildcard $(SOURCEDIR)/*.cpp)
CSOURCES = $(wildcard $(SOURCEDIR)/*.c)
CPPLISTS = $(patsubst %.cpp,$(BUILDDIR)/%.lst,$(CPPSOURCES))
CLISTS = $(patsubst %.c,$(BUILDDIR)/%.lst,$(CSOURCES))

INO = ~/src/ino/bin/ino

# Use Arduino-1.0.5 on /Applications/Arduino.app
AVRBIN = /Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/bin
DATASIZE = $(shell $(AVRBIN)/avr-size -B .build/irkit/firmware.hex | tail -n 1 | cut -b 11-15)
VERSION = $(shell git describe --tags --long | sed -e "s/^v//" | sed -e "s/\-/./g")

build:
	$(INO) build -m irkit --ldflags="-Os --gc-sections -Map=.build/irkit/firmware.map"
	$(AVRBIN)/avr-nm --size-sort .build/irkit/firmware.elf > .build/irkit/firmware.nm.size.log
	$(AVRBIN)/avr-nm --numeric-sort .build/irkit/firmware.elf > .build/irkit/firmware.nm.numeric.log

version:
	sed -e s/__VERSION__/$(VERSION)/ src/IRKit/version.template > src/IRKit/version.c

upload:
	@if [ $(DATASIZE) -gt 28672 ] ; then \
		echo "size is $(DATASIZE), NG" ; \
	else \
		echo "size is $(DATASIZE), OK" ; \
		$(INO) upload -m irkit ; \
	fi

serial:
	sleep 2
	$(INO) serial -b 115200

# remove Arduino IDE build files
clean:
	rm -rf src/IRKit/build-uno/

$(BUILDDIR)/$(SOURCEDIR)/%.lst : $(SOURCEDIR)/%.cpp
	@mkdir -p $(BUILDDIR)/$(SOURCEDIR)
	/Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/bin/avr-g++ -mmcu=atmega32u4 -DF_CPU=16000000L -DARDUINO=105 -I/Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/cores/arduino -ffunction-sections -fdata-sections -g -Os -w -DUSB_VID=0x1d50 -DUSB_PID=0x6085 -I/Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/variants/leonardo -I/Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/cores/arduino -I/Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/cores/arduino/avr-libc -Wa,-aln=$@ -fno-exceptions -o $(subst .lst,.o,$@) -c $<

$(BUILDDIR)/$(SOURCEDIR)/%.lst : $(SOURCEDIR)/%.c
	@mkdir -p $(BUILDDIR)/$(SOURCEDIR)
	/Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/bin/avr-gcc -mmcu=atmega32u4 -DF_CPU=16000000L -DARDUINO=105 -I/Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/cores/arduino -ffunction-sections -fdata-sections -g -Os -w -DUSB_VID=0x1d50 -DUSB_PID=0x6085 -I/Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/variants/leonardo -I/Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/cores/arduino -I/Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/cores/arduino/avr-libc -Wa,-aln=$@ -o $(subst .lst,.o,$@) -c $<

.PHONY: assembly build version clean
assembly: $(CPPLISTS) $(CLISTS)
