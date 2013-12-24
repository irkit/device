#include "Arduino.h"
#include "pins.h"
#include "FullColorLed.h"
#include "timer.h"

FullColorLed color( FULLCOLOR_LED_R, FULLCOLOR_LED_G, FULLCOLOR_LED_B );
#define LED_BLINK_INTERVAL 200

void timerFired() {
    Serial.println(".");
    color.onTimer();
}

void setup() {
    timer_init( &timerFired );
    timer_start( LED_BLINK_INTERVAL );

    color.setLedColor( 1, 1, 1 );

    // USB serial
    Serial.begin(115200);

    // wait for leonardo
    while ( ! Serial ) ;

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

    // check for input from the user
    if (Serial.available()) {

        lastCharacter = Serial.read();
        Serial.print("0x"); Serial.println( lastCharacter, HEX );

        uint8_t status;
        if (lastCharacter == 'k') {
            color.off();
        }
        else if (lastCharacter == 'r') {
            color.setLedColor(1,0,0);
        }
        else if (lastCharacter == 'g') {
            color.setLedColor(0,1,0);
        }
        else if (lastCharacter == 'b') {
            color.setLedColor(0,0,1);
        }
        else if (lastCharacter == 'c') {
            color.setLedColor(0,1,1);
        }
        else if (lastCharacter == 'm') {
            color.setLedColor(1,0,1);
        }
        else if (lastCharacter == 'y') {
            color.setLedColor(1,1,0);
        }
        else if (lastCharacter == 'w') {
            color.setLedColor(1,1,1);
        }
        else if (lastCharacter == 'R') {
            color.setLedColor(1,0,0,true,1);
        }
        else if (lastCharacter == 'G') {
            color.setLedColor(0,1,0,true,1);
        }
        else if (lastCharacter == 'B') {
            color.setLedColor(0,0,1,true,1);
        }
        else if (lastCharacter == 'C') {
            color.setLedColor(0,1,1,true,1);
        }
        else if (lastCharacter == 'M') {
            color.setLedColor(1,0,1,true,1);
        }
        else if (lastCharacter == 'Y') {
            color.setLedColor(1,1,0,true,1);
        }
        else if (lastCharacter == 'W') {
            color.setLedColor(1,1,1,true,1);
        }
    }
}
