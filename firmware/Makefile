# This Makefile is only used to check .lst files

SOURCEDIR = src/IRKit
BUILDDIR = .build/irkit
CPPSOURCES = $(wildcard $(SOURCEDIR)/*.cpp)
CSOURCES = $(wildcard $(SOURCEDIR)/*.c)
CPPLISTS = $(patsubst %.cpp,$(BUILDDIR)/%.lst,$(CPPSOURCES))
CLISTS = $(patsubst %.c,$(BUILDDIR)/%.lst,$(CSOURCES))


.PHONY: assembly
assembly: $(CPPLISTS) $(CLISTS)


$(BUILDDIR)/$(SOURCEDIR)/%.lst : $(SOURCEDIR)/%.cpp
	@mkdir -p $(BUILDDIR)/$(SOURCEDIR)
	/Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/bin/avr-g++ -mmcu=atmega32u4 -DF_CPU=16000000L -DARDUINO=105 -I/Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/cores/arduino -ffunction-sections -fdata-sections -g -Os -w -DUSB_VID=0x1d50 -DUSB_PID=0x6085 -I/Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/variants/leonardo -I/Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/cores/arduino -I/Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/cores/arduino/avr-libc -Wa,-aln=$@ -fno-exceptions -o $(subst .lst,.o,$@) -c $<

$(BUILDDIR)/$(SOURCEDIR)/%.lst : $(SOURCEDIR)/%.c
	@mkdir -p $(BUILDDIR)/$(SOURCEDIR)
	/Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/bin/avr-gcc -mmcu=atmega32u4 -DF_CPU=16000000L -DARDUINO=105 -I/Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/cores/arduino -ffunction-sections -fdata-sections -g -Os -w -DUSB_VID=0x1d50 -DUSB_PID=0x6085 -I/Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/variants/leonardo -I/Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/cores/arduino -I/Applications/Arduino.app/Contents/Resources/Java/hardware/arduino/cores/arduino/avr-libc -Wa,-aln=$@ -o $(subst .lst,.o,$@) -c $<


