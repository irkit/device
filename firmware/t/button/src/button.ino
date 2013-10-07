#include "Arduino.h"
#include "pins.h"
#include "LongPressButton.h"

LongPressButton button( RESET_SWITCH, 3000 );

void longPressed() {
    Serial.println("long pressed");
}

void setup() {
    button.callback = &longPressed;

    // USB serial
    Serial.begin(115200);

    while (! Serial) ;
}

void loop() {
    while (1) {
        button.loop();

        delay(100);
        Serial.print(".");
    }
}
