#include "Arduino.h"
#include "pins.h"
#include "pgmStrToRAM.h"

void setup() {
    // USB serial
    Serial.begin(115200);

    // gainspan
    Serial1.begin(9600);

    // wait for connection
    while ( ! Serial ) ;
}

void loop() {
    // gainspan -> usb
    if (Serial1.available()) {
        Serial.write(Serial1.read());
    }

    // usb -> gainspan
    if (Serial.available()) {
        static uint8_t is_new_line    = 1;
        static uint8_t last_character = '0';

        last_character = Serial.read();

        if (is_new_line) {
            is_new_line = 0;
            Serial.print(P("> "));
        }

        if (last_character == 0x0D) {
            is_new_line = 1;
        }

        // GS module echoes back
        // Serial.write( last_character );
        Serial1.write( last_character );
    }
}
