#include "Arduino.h"
#include "pgmStrToRAM.h"

void setup() {
    // USB serial
    Serial.begin(115200);

    // wait for leonardo
    while ( ! Serial );

    // Serial.print("3");
}

void loop() {
}
