#!/bin/sh
set -xe

# remove Arduino IDE build files
rm -rf src/IRKit/build-uno/

VERSION=`git describe --tags --long | sed -e "s/^v//" | sed -e "s/\-/./g"`
sed -e "s/__VERSION__/$VERSION/" src/IRKit/version.template > src/IRKit/version.c
~/src/ino/bin/ino build -m irkit
