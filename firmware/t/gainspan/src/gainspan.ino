#include "Arduino.h"
#include "pins.h"
#include "pgmStrToRAM.h"
#include "MemoryFree.h"
#include "GSwifi.h"
#include "WifiCredentials.h"

GSwifi gs(&Serial1);

void setup() {
    // enable 3.3V LDO
    pinMode( LDO33_ENABLE, OUTPUT );
    digitalWrite( LDO33_ENABLE, HIGH );

    // USB serial
    Serial.begin(115200);

    // wait for connection
    while ( ! Serial ); // debug

    reset3V3();

    // wait til gs wakes up
    delay( 100 );

    gs.setup();

    // load wifi credentials from EEPROM
    {
        WifiCredentials credentials;

        if (credentials.isValid()) {
            /* gs.connect(credentials.get(WIFICREDENTIALS_KEY_SECURITY), */
            /*            credentials.get(WIFICREDENTIALS_KEY_SSID), */
            /*            credentials.get(WIFICREDENTIALS_KEY_PASSWORD)); */
            // gs.startup();
        }
    }

    printGuide();
}

void printGuide() {
    Serial.println(P("Menu:"));
    Serial.println(P("b) change baud rate to 9600"));
    Serial.println(P("B) change baud rate to 115200"));
    Serial.println(P("c) connect to wifi"));
    Serial.println(P("d) dump"));
    Serial.println(P("h) help (this)"));
    Serial.println(P("R) hardware reset"));
    Serial.println(P("s) setup"));
    Serial.println(P("z) clear EEPROM(ssid,password)"));
    Serial.println(P("Esc) command mode"));
    Serial.println(P("Command?"));
}

void loop() {
    // usb
    if (Serial.available()) {
        static uint8_t last_character = '0';
        static bool isCommandMode = false;

        last_character = Serial.read();

        Serial.print(P("> 0x"));
        Serial.print(last_character, HEX);
        Serial.println();
        Serial.print(P("free memory: 0x")); Serial.println( freeMemory(), HEX );


        uint8_t status;
        if (isCommandMode) {
            Serial1.write(last_character);

            if ( last_character == 0x1B ) {
                isCommandMode = false;
                Serial.println(P("<< command mode finished !!!!"));
            }
        }
        else if (last_character == 0x1B) {
            isCommandMode = true;
            Serial.println(P(">> entered command mode !!!!"));
        }
        else if (last_character == 'c') {
            gs.connect( GSwifi::GSSEC_WPA2_PSK,
                        PB("Rhodos",2),
                        PB("aaaaaaaaaaaaa",3) );
        }
        else if (last_character == 'b') {
            gs.setBaud(9600);
        }
        else if (last_character == 'B') {
            gs.setBaud(115200);
        }
        else if (last_character == 'd') {
            WifiCredentials credentials;
            credentials.dump();

            gs.dump();
        }
        else if (last_character == 'h') {
            printGuide();
        }
        else if (last_character == 'R') {
            reset3V3();
        }
        else if (last_character == 's') {
            gs.setup();
        }
        else if (last_character == 'y') {
            WifiCredentials credentials;
            uint8_t security = 4;
            credentials.set(WIFICREDENTIALS_KEY_SECURITY, &security, 1);
            credentials.set(WIFICREDENTIALS_KEY_SSID,     (const uint8_t*)P("Rhodos"), 6);
            credentials.set(WIFICREDENTIALS_KEY_PASSWORD, (const uint8_t*)P("aaaaaaaaaaaaa"), 13);
            credentials.save();
        }
        else if (last_character == 'z') {
            WifiCredentials credentials;
            credentials.clear();
            Serial.println(P("cleared EEPROM"));
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

void reset3V3 () {
    Serial.println(P("hardware reset"));
    digitalWrite( LDO33_ENABLE, LOW );
    delay( 100 );
    digitalWrite( LDO33_ENABLE, HIGH );
}
