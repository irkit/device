#include "Arduino.h"
#include "pins.h"
#include "SetSwitch.h"

SetSwitch set( AUTH_SWITCH );

void authorized() {
    Serial.println("authorized");
}

void cleared() {
    Serial.println("cleared");
}

void setup() {
    pinMode(BUSY_LED,         OUTPUT);
    digitalWrite(BUSY_LED,    LOW);

    // pull-up
    pinMode(AUTH_SWITCH,      INPUT);
    digitalWrite(AUTH_SWITCH, HIGH);

    set.setup();
    set.callback      = authorized;
    set.clearCallback = cleared;

    // USB serial
    Serial.begin(115200);
}

void loop() {
    static uint8_t writeCount = 1;
    static uint8_t lastCharacter = '0';

    Serial.println("Operations Menu:");
    Serial.println("a) Dump set data");
    Serial.println("z) Clear Switch Auth saved data");
    Serial.println("Command?");
    while (1) {
        // check for auth switch pressed
        set.loop(1); // 1 is stored in set as authorized

        // check for input from the user
        if (Serial.available()) {

            lastCharacter = Serial.read();
            Serial.print("0x"); Serial.println( lastCharacter, HEX );

            uint8_t status;
            if (lastCharacter == 'a') {
                Serial.print("data: { ");
                for (uint8_t i=0; i<set.count(); i++) {
                    Serial.print(set.data(i));
                    Serial.print(" ");
                }
                Serial.println("}");
            }
            else if (lastCharacter == 'z') {
                Serial.println("cleared set data");
                set.clear();
            }
        }
    }
}
