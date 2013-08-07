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
