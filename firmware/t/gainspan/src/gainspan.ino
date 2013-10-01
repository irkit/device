#include "Arduino.h"
#include "pins.h"
#include "pgmStrToRAM.h"
#include "GSwifi.h"

GSwifi gs(&Serial1);

void setup() {
    // gs.setup();

    // enable 3.3V LDO
    pinMode( LDO33_ENABLE, OUTPUT );
    digitalWrite( LDO33_ENABLE, HIGH );

    // USB serial
    Serial.begin(115200);

    // wait for connection
    while ( ! Serial );

    printGuide();
}

void printGuide() {
    Serial.println(P("Menu:"));
    Serial.println(P("c) connect to wifi"));
    Serial.println(P("h) help (this)"));
    Serial.println(P("Command?"));
}

void loop() {
    // usb
    if (Serial.available()) {
        static uint8_t last_character = '0';

        last_character = Serial.read();

        Serial.print(P("last character: 0x")); Serial.println( last_character, HEX );
        /* Serial.print(P("free memory:    0x")); Serial.println( freeMemory(), HEX ); */

        uint8_t status;
        if (last_character == 'c') {
            gs.connect( GSwifi::GSSEC_WPA2_PSK, P("Rhodos"), P("aaaaaaaaaaaaa") );
        }
        else if (last_character == 'h') {
            printGuide();
        }
    }
}
