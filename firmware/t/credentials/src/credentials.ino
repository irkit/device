#include "Arduino.h"
#include "pins.h"
#include "pgmStrToRAM.h"
#include "MemoryFree.h"
#include "WifiCredentials.h"

void setup() {
    // USB serial
    Serial.begin(115200);

    // wait for connection
    while ( ! Serial ); // debug

    // load wifi credentials from EEPROM
    {
        WifiCredentials credentials;

        if (credentials.isValid()) {
            Serial.println(credentials.getSecurity());
            Serial.println(credentials.getSSID());
            Serial.println(credentials.getPassword());
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
            WifiCredentials credentials;
            uint8_t security = WIFICREDENTIALS_SECURITY_WPA2PSK;
            credentials.set(GSwifi::GSSECURITY_WPA2_PSK,
                            PB("Rhodos",2),
                            PB("aaaaaaaaaaaaa",3));
            credentials.save();
        }
        else if (last_character == 'z') {
            WifiCredentials credentials;
            credentials.clear();
            Serial.println(P("cleared EEPROM"));
        }
    }
}
