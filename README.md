IRKit Device
===

IRKit device(here you are) and [IRKit SDK](https://github.com/irkit/ios-sdk) lets you control your home electronics from your iOS devices.
IRKit device has a Infrared LED and receiver, and a BluetoothLE module inside.
BluetoothLE enabled devices can connect with IRKit devices, and make it send IR signals for you.

## Get IRKit Device

Currently you can only make it yourself.

## How it looks when assembled

To be done..

## Schematics

Basically, it's a [Arduino Pro](http://arduino.cc/en/Main/ArduinoBoardPro) fork with following modifications:

* added [BLE112](http://www.bluegiga.com/BLE112_Bluetooth_Smart_module) ( [BlueGiga](http://www.bluegiga.com/) 's Bluetooth Low Energy module ).
* added infrared receiver and LED

<img src="https://raw.github.com/irkit/device/master/hardware/schematic.png" />

## Firmware

It's really just this.

```c
#include "Arduino.h"
#include "IRKit.h"

void setup() {
    Serial.begin(115200);

    IRKit_setup();

    // add your own code here!!
}

void loop() {
    IRKit_loop();

    // add your own code here!!
}
```

## Board

### Top

<img src="https://raw.github.com/irkit/device/master/hardware/top.png" />

### Bottom

<img src="https://raw.github.com/irkit/device/master/hardware/bottom.png" />

## Case

See [Case](https://github.com/irkit/device/tree/master/case)

## Extend it!

Following Arduino pins are pulled out for you.

* D9,D10,D11,D12,D13
* A0,A1,A7

## Manufacturers

Can you help us manufacture this?
Please contact us! at ohtsuka@kayac.com
