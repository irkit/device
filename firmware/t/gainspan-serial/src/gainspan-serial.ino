#include "Arduino.h"
#include "pins.h"
#include "pgmStrToRAM.h"
#include "HardwareSerialX.h"

void setup() {
    // USB serial
    Serial.begin(115200);

    // wait for connection
    while ( ! Serial ) ;

    pinMode( LDO33_ENABLE, OUTPUT );
    digitalWrite( LDO33_ENABLE, HIGH );

    // gainspan
    Serial1X.begin(38400);
}

void loop() {
    // gainspan -> usb
    if (Serial1X.available()) {
        Serial.write(Serial1X.read());
    }

    // usb -> gainspan
    if (Serial.available()) {
        static uint8_t last_character = '0';

        last_character = Serial.read();

        Serial1X.write(last_character);
        Serial.write(last_character);
    }
}
