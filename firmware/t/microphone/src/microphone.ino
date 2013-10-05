#include "Arduino.h"
#include "pins.h"

void setup() {
    pinMode(MICROPHONE,  INPUT);

    // USB serial
    Serial.begin(115200);

    // wait for leonardo
    while ( ! Serial );
}

void loop() {
    int mic = analogRead(MICROPHONE);
    Serial.println(mic);

    delay(100);
}
