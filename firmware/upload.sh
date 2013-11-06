#!/bin/sh
set -xe

# don't want to write boot loader area of leonardo
ruby ./check_flash_size.rb ".build/irkit/firmware.hex" 28672

~/src/ino/bin/ino upload -m irkit
