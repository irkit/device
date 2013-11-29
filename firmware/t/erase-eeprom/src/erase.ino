#include "Arduino.h"
#include "EEPROM.h"

void setup() {
    while ( ! Serial ) ; // wait for leonardo

    // write a 0..10, this should invalid crc check
    for (int i = 0; i < 10; i++) {
        EEPROM.write(i, 0);
    }

    Serial.println("erased!");
}

void loop() {
}
