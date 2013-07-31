#include "Arduino.h"
#include "IRKit.h"

void setup() {
    Serial.begin(115200);

    IRKit_setup();
}

void loop() {
    IRKit_loop();
}
