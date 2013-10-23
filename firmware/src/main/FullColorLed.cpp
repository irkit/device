#include <Arduino.h>
#include "FullColorLed.h"

FullColorLed::FullColorLed(int pinR, int pinG, int pinB) :
    pinR_(pinR),
    pinG_(pinG),
    pinB_(pinB),
    blinkOn_(0),
    isBlinking_(false)
{
    pinMode(pinR_, OUTPUT);
    pinMode(pinG_, OUTPUT);
    pinMode(pinB_, OUTPUT);
}

void FullColorLed::setLedColor(bool colorR, bool colorG, bool colorB) {
    setLedColor(colorR, colorG, colorB, false);
}

void FullColorLed::setLedColor(bool colorR, bool colorG, bool colorB, bool blink) {
    colorR_      = colorR;
    colorG_      = colorG;
    colorB_      = colorB;
    isBlinking_  = blink;

    digitalWrite(pinR_, colorR);
    digitalWrite(pinG_, colorG);
    digitalWrite(pinB_, colorB);
}

void FullColorLed::off() {
    setLedColor( 0, 0, 0, false );
}

void FullColorLed::toggleBlink() {
    blinkOn_ = ! blinkOn_;

    if ( blinkOn_ || ! isBlinking_ ) {
        // not blinking = always on
        digitalWrite(pinR_, colorR_);
        digitalWrite(pinG_, colorG_);
        digitalWrite(pinB_, colorB_);
    }
    else {
        digitalWrite(pinR_, LOW);
        digitalWrite(pinG_, LOW);
        digitalWrite(pinB_, LOW);
    }
}
