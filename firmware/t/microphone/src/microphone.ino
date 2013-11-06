#include "Arduino.h"
#include "pins.h"
#include "MemoryFree.h"
#include "pgmStrToRAM.h"
#include "MorseListener.h"
#include "Global.h"
#include "Keys.h"

MorseListener listener(MICROPHONE,13);

Keys keys;

void letterCallback( char letter ) {
    Serial.print(P("letter: ")); Serial.write(letter); Serial.println();
    int8_t result = keys.put( letter );
    if ( result != 0 ) {
        keys.clear();
        Serial.println(P("cleared"));
    }
}

void wordCallback() {
    Serial.println(P("word"));
    int8_t result = keys.putDone();
    if ( result != 0 ) {
        keys.clear();
        Serial.println(P("cleared"));
    }
    else {
        Serial.println(P("let's try connecting to wifi"));
        keys.dump();
    }
}

void errorCallback() {
    Serial.println(P("error"));
    keys.clear();
}

void setup() {
    pinMode(MICROPHONE,  INPUT);

    listener.letterCallback = &letterCallback;
    listener.wordCallback   = &wordCallback;
    listener.errorCallback  = &errorCallback;

    // USB serial
    Serial.begin(115200);

    // wait for leonardo
    while ( ! Serial );

    listener.setup();
    listener.enable(true);
}

void loop() {
    static uint8_t last_character = '0';

    global.loop(); // always run first

    listener.loop();

    // check for input from the user
    if (Serial.available()) {
        static bool is_command_mode = false;

        last_character = Serial.read();
        Serial.print("0x"); Serial.println( last_character, HEX );

        uint8_t status;
        if (is_command_mode) {
            if ( last_character == 0x1B ) {
                is_command_mode = false;
                Serial.println(P("<< command mode finished !!!!"));
            }
            else if ( last_character == 'w' ) {
                wordCallback();
            }
            else {
                letterCallback( last_character );
            }
        }
        else if (last_character == 0x1B) {
            is_command_mode = true;
            Serial.println(P(">> entered command mode !!!!"));
        }
        else if (last_character == '1') {
            Serial.println(P("WPM set to 13"));
            listener.setWPM(13);
            listener.setup(); // for debug output
        }
        else if (last_character == '2') {
            Serial.println(P("WPM set to 20"));
            listener.setWPM(20);
            listener.setup(); // for debug output
        }
        else if (last_character == '3') {
            Serial.println(P("WPM set to 40"));
            listener.setWPM(40);
            listener.setup(); // for debug output
        }
        else if (last_character == '4') {
            Serial.println(P("WPM set to 100"));
            listener.setWPM(100);
            listener.setup(); // for debug output
        }
        else if (last_character == '5') {
            Serial.println(P("WPM set to 1000"));
            listener.setWPM(1000);
            listener.setup(); // for debug output
        }
        else if (last_character == 'd') {
            keys.dump();
        }
    }

}
