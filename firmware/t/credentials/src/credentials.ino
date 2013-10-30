#include "Arduino.h"
#include "pins.h"
#include "pgmStrToRAM.h"
#include "MemoryFree.h"
#include "Keys.h"

void setup() {
    // USB serial
    Serial.begin(115200);

    // wait for connection
    while ( ! Serial ); // debug

    // load wifi credentials from EEPROM
    {
        Keys keys;

        if (keys.isValid()) {
            Serial.println(keys.getSecurity());
            Serial.println(keys.getSSID());
            Serial.println(keys.getPassword());
        }
    }

    printGuide();
}

void printGuide() {
    Serial.println(P("Menu:"));
    Serial.println(P("Esc) command mode"));
    Serial.println(P("Command?"));
}

void loop() {
    // usb
    if (Serial.available()) {
        static uint8_t last_character = '0';
        static bool is_command_mode = false;

        last_character = Serial.read();

        Serial.print(P("> 0x"));
        Serial.print(last_character, HEX);
        Serial.println();
        Serial.print(P("free memory: 0x")); Serial.println( freeMemory(), HEX );


        uint8_t status;
        if (last_character == 'y') {
            Keys keys;
            keys.set(GSwifi::GSSECURITY_WPA2_PSK,
                     PB("Rhodos",2),
                     PB("aaaaaaaaaaaaa",3));
            keys.save();
        }
        else if (last_character == 'z') {
            Keys keys;
            keys.clear();
            Serial.println(P("cleared EEPROM"));
        }
    }
}
