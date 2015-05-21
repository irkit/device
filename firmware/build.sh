#!/bin/sh
set -xe

# Use Arduino-1.0.5 on /Applications/Arduino.app
AVRBIN=/Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/bin

# remove Arduino IDE build files
rm -rf src/IRKit/build-uno/

VERSION=`git describe --tags --long | sed -e "s/^v//" | sed -e "s/\-/./g"`
sed -e "s/__VERSION__/$VERSION/" src/IRKit/version.template > src/IRKit/version.c
~/src/ino/bin/ino build -m irkit --ldflags="-Os --gc-sections -Map=.build/irkit/firmware.map"
${AVRBIN}/avr-nm --size-sort .build/irkit/firmware.elf > .build/irkit/firmware.nm.size.log
${AVRBIN}/avr-nm --numeric-sort .build/irkit/firmware.elf > .build/irkit/firmware.nm.numeric.log
