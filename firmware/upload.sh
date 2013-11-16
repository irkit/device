#!/bin/sh
set -e

# don't want to write boot loader area of leonardo
AVRBIN=/Applications/Arduino.app/Contents/Resources/Java/hardware/tools/avr/bin
DATASIZE=`$AVRBIN/avr-size -B .build/irkit/firmware.hex | tail -n 1 | cut -b 11-15`
if [ "$DATASIZE" -gt 28672 ]; then
    echo "size is $DATASIZE > 28672, NG"
    exit 1
else
    echo "size is $DATASIZE < 28672, OK"
fi

~/src/ino/bin/ino upload -m irkit
