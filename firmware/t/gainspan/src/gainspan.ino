#include "Arduino.h"
#include "pins.h"
#include "pgmStrToRAM.h"
#include "GSwifi.h"

GSwifi gs(&Serial1);

void setup() {
    // enable 3.3V LDO
    pinMode( LDO33_ENABLE, OUTPUT );
    digitalWrite( LDO33_ENABLE, HIGH );

    // wait til gs wakes up
    // sleep_ms( 100 );

    gs.setup();

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

        Serial.print(P("> 0x"));
        Serial.print(last_character, HEX);
        if (last_character > 0x0D) {
            Serial.print(P(" "));
            Serial.write(last_character);
        }
        Serial.println();
        /* Serial.print(P("free memory:    0x")); Serial.println( freeMemory(), HEX ); */

        uint8_t status;
        if (last_character == 'c') {
            gs.connect( GSwifi::GSSEC_WPA2_PSK, "Rhodos", "aaaaaaaaaaaaa" );
        }
        else if (last_character == 'h') {
            printGuide();
        }
        else {
            Serial1.write(last_character);
        }
    }

    // gainspan
    if (Serial1.available()) {
        static uint8_t last_character_gainspan = '0';
        last_character_gainspan = Serial1.read();
        Serial.print(P("< 0x"));
        Serial.print(last_character_gainspan, HEX);
        if (last_character_gainspan > 0x0D) {
            Serial.print(P(" "));
            Serial.write(last_character_gainspan);
        }
        Serial.println();
    }
}
