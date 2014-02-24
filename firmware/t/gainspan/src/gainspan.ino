#include "Arduino.h"
#include <avr/wdt.h>
#include "pins.h"
#include "pgmStrToRAM.h"
#include "MemoryFree.h"
#include "GSwifi.h"
#include "Keys.h"
#include "const.h"

GSwifi gs(&Serial1X);
Keys keys;
volatile char sharedbuffer[ SHARED_BUFFER_SIZE ];

int8_t onReset() {
    Serial.println(P("!!! onReset"));
    Serial.print(P("free memory: 0x")); Serial.println( freeMemory(), HEX );

    return 0;
}

int8_t onDisconnect() {
    Serial.println(P("!!! onDisconnect"));

    return 0;
}

void setup() {
    Serial.begin(115200);

    // wait for connection
    while ( ! Serial ); // debug

    // enable 3.3V LDO
    pinMode( LDO33_ENABLE, OUTPUT );
    digitalWrite( LDO33_ENABLE, HIGH );
    // gs.setup( &onDisconnect, &onReset );

    printGuide();
}

void printGuide() {
    Serial.println(P("Menu:"));
    Serial.println(P("a) start adhoc"));
    Serial.println(P("b) change baud rate to 9600"));
    Serial.println(P("B) change baud rate to 57600"));
    Serial.println(P("c) connect to wifi"));
    Serial.println(P("d) dump"));
    Serial.println(P("h) help (this)"));
    Serial.println(P("R) hardware reset"));
    Serial.println(P("y) set EEPROM with dev data"));
    Serial.println(P("z) clear EEPROM(ssid,password)"));
    Serial.println(P("Esc) command mode"));
    Serial.println(P("Command?"));
}

void loop() {
    // usb
    if (Serial.available()) {
        static uint8_t last_character = '0';
        static bool is_command_mode = false;

        last_character = Serial.read();

        Serial.print("> "); Serial.write(last_character);
        Serial.print(" 0x"); Serial.print(last_character, HEX);
        Serial.println();

        uint8_t status;
        if (is_command_mode) {
            Serial1X.write(last_character);

            if ( last_character == 0x1B ) {
                is_command_mode = false;
                Serial.println(P("<< command mode finished !!!!"));
            }
        }
        else if (last_character == 0x1B) {
            is_command_mode = true;
            Serial.println(P(">> entered command mode !!!!"));
        }
        else if (last_character == 'c') {
            gs.join( GSSECURITY_WPA2_PSK,
                     PB("Rhodos",2),
                     PB("aaaaaaaaaaaaa",3) );
        }
        else if (last_character == 'a') {
            Serial.println(P("start limited AP"));
            gs.startLimitedAP();
        }
        else if (last_character == 'b') {
            Serial1X.end();
            Serial1X.begin(9600);
        }
        else if (last_character == 'B') {
            Serial1X.end();
            Serial1X.begin(57600);
        }
        else if (last_character == 'd') {
            Serial.println(P("---keys---"));
            keys.dump();

            Serial.println(P("---wifi---"));
            gs.dump();
        }
        else if (last_character == 'h') {
            printGuide();
        }
        else if (last_character == 'R') {
            wifi_hardware_reset();
        }
        else if (last_character == 'y') {
            keys.set(GSSECURITY_WPA2_PSK,
                     PB("Rhodos",2),
                     PB("aaaaaaaaaaaaa",3));
            keys.save();
        }
        else if (last_character == 'z') {
            keys.clear();
            Serial.println(P("cleared EEPROM"));
        }
    }

    // gainspan
    if (Serial1X.available()) {
        static uint8_t last_character_gainspan = '0';
        last_character_gainspan = Serial1X.read();
        if (last_character_gainspan >= 0x0D ||
            last_character_gainspan == 0x0A) {
            Serial.write(last_character_gainspan);
        }
        else {
            Serial.print(" 0x");
            Serial.println(last_character_gainspan,HEX);
        }
    }
}

void wifi_hardware_reset () {
    Serial.println(P("!E25"));

    // UART line powers GS and pulls 3.3V line up to around 1.4V so GS won't reset without this
    Serial1X.end();

    digitalWrite( LDO33_ENABLE, LOW );
    delay( 1000 );
    digitalWrite( LDO33_ENABLE, HIGH );

    // wait til gs wakes up
    delay( 1000 );
}

void software_reset() {
    wdt_enable(WDTO_15MS);
    while (1) ;
}
