#!/bin/sh
set -xe

# remove Arduino IDE build files
rm -rf src/build-*/
~/src/ino/bin/ino build -m leonardo
