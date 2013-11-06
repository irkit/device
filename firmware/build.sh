#!/bin/sh
set -xe

# remove Arduino IDE build files
rm -rf src/main/build-uno/

VERSION=`git describe --tags --long | sed -e "s/^v//" | sed -e "s/\-/./g"`
sed -e "s/__VERSION__/$VERSION/" src/main/version.template > src/main/version.c
~/src/ino/bin/ino build -m irkit
