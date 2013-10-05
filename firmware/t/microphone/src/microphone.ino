#include "Arduino.h"
#include "pins.h"
#include "MemoryFree.h"
#include "pgmStrToRAM.h"
#include "MorseListener.h"
#include "Global.h"

MorseListener listener(MICROPHONE,13);

void letterCallback( uint8_t letter ) {
    Serial.print(P("letter: ")); Serial.println(letter);
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
}

void loop() {
    global.loop(); // always run first

    listener.loop();
}
