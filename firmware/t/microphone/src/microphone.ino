#include "Arduino.h"
#include "pins.h"
#include "MemoryFree.h"
#include "pgmStrToRAM.h"
#include "MorseListener.h"
#include "Global.h"

MorseListener listener(MICROPHONE,13);

void letterCallback( uint8_t letter ) {
    Serial.print(P("letter: ")); Serial.write(letter); Serial.println();
}

void wordCallback() {
    Serial.println(P("word"));
}

void errorCallback() {
    Serial.println(P("error"));
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
    static uint8_t lastCharacter = '0';

    global.loop(); // always run first

    listener.loop();

    // check for input from the user
    if (Serial.available()) {

        lastCharacter = Serial.read();
        Serial.print("0x"); Serial.println( lastCharacter, HEX );

        uint8_t status;
        if (lastCharacter == '1') {
            Serial.println(P("WPM set to 13"));
            listener.setWPM(13);
            listener.setup(); // for debug output
        }
        else if (lastCharacter == '2') {
            Serial.println(P("WPM set to 20"));
            listener.setWPM(20);
            listener.setup(); // for debug output
        }
        else if (lastCharacter == '3') {
            Serial.println(P("WPM set to 40"));
            listener.setWPM(40);
            listener.setup(); // for debug output
        }
        else if (lastCharacter == '4') {
            Serial.println(P("WPM set to 100"));
            listener.setWPM(100);
            listener.setup(); // for debug output
        }
        else if (lastCharacter == '5') {
            Serial.println(P("WPM set to 1000"));
            listener.setWPM(1000);
            listener.setup(); // for debug output
        }
    }

}
