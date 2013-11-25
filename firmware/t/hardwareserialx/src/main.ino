#include "Arduino.h"
#include "HardwareSerialX.h"

extern volatile uint8_t test;
void setup() {
    while (! Serial) ;
    // USB serial
    Serial1X.begin(115200);
}

void loop() {
}
