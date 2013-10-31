#include "Arduino.h"
#include "pins.h"
#include "pgmStrToRAM.h"

void setup() {
    // USB serial
    Serial.begin(115200);

    // wait for connection
    while ( ! Serial ) ;

    pinMode( LDO33_ENABLE, OUTPUT );
    digitalWrite( LDO33_ENABLE, HIGH );

    // gainspan
    Serial1.begin(9600);
}

void loop() {
    // gainspan -> usb
    if (Serial1.available()) {
        Serial.write(Serial1.read());
    }

    // usb -> gainspan
    if (Serial.available()) {
        static uint8_t last_character = '0';

        last_character = Serial.read();

        Serial1.write(last_character);
        Serial.write(last_character);
    }
}
