#include "Arduino.h"
#include "pins.h"
#include "FullColorLed.h"

FullColorLed color( FULLCOLOR_LED_R, FULLCOLOR_LED_G, FULLCOLOR_LED_B );
#define LED_BLINK_INTERVAL 200

void setup() {
    color.SetLedColor( 1, 1, 1 );

    // USB serial
    Serial.begin(115200);

    Serial.println("Operations Menu:");
    Serial.println("r) R");
    Serial.println("g) G");
    Serial.println("b) B");
    Serial.println("c) C");
    Serial.println("m) M");
    Serial.println("y) Y");
    Serial.println("k) K");
    Serial.println("R) R blink");
    Serial.println("G) G blink");
    Serial.println("B) B blink");
    Serial.println("C) C blink");
    Serial.println("M) M blink");
    Serial.println("Y) Y blink");
    Serial.println("Command?");
}

void loop() {
    static uint8_t lastCharacter = '0';

    color.Loop();

    // check for input from the user
    if (Serial.available()) {

        lastCharacter = Serial.read();
        Serial.print("0x"); Serial.println( lastCharacter, HEX );

        uint8_t status;
        if (lastCharacter == 'k') {
            color.LedOff();
        }
        else if (lastCharacter == 'r') {
            color.SetLedColor(1,0,0);
        }
        else if (lastCharacter == 'g') {
            color.SetLedColor(0,1,0);
        }
        else if (lastCharacter == 'b') {
            color.SetLedColor(0,0,1);
        }
        else if (lastCharacter == 'c') {
            color.SetLedColor(0,1,1);
        }
        else if (lastCharacter == 'm') {
            color.SetLedColor(1,0,1);
        }
        else if (lastCharacter == 'y') {
            color.SetLedColor(1,1,0);
        }
        else if (lastCharacter == 'w') {
            color.SetLedColor(1,1,1);
        }
        else if (lastCharacter == 'R') {
            color.SetLedColor(1,0,0,LED_BLINK_INTERVAL);
        }
        else if (lastCharacter == 'G') {
            color.SetLedColor(0,1,0,LED_BLINK_INTERVAL);
        }
        else if (lastCharacter == 'B') {
            color.SetLedColor(0,0,1,LED_BLINK_INTERVAL);
        }
        else if (lastCharacter == 'C') {
            color.SetLedColor(0,1,1,LED_BLINK_INTERVAL);
        }
        else if (lastCharacter == 'M') {
            color.SetLedColor(1,0,1,LED_BLINK_INTERVAL);
        }
        else if (lastCharacter == 'Y') {
            color.SetLedColor(1,1,0,LED_BLINK_INTERVAL);
        }
        else if (lastCharacter == 'W') {
            color.SetLedColor(1,1,1,LED_BLINK_INTERVAL);
        }
    }
}
