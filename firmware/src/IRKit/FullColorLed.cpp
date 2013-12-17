#include <Arduino.h>
#include "FullColorLed.h"
#include "timer.h"

FullColorLed::FullColorLed(int pinR, int pinG, int pinB) :
    pinR_(pinR),
    pinG_(pinG),
    pinB_(pinB),
    blinkOn_(0),
    isBlinking_(false),
    blink_timer_(TIMER_OFF)
{
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

    blink_timer_ = TIMER_OFF;
}

void FullColorLed::setLedColor(bool colorR, bool colorG, bool colorB, bool blink, uint8_t blink_timeout) {
    setLedColor( colorR, colorG, colorB, blink );

    TIMER_START(blink_timer_, blink_timeout);
}

void FullColorLed::off() {
    setLedColor( 0, 0, 0, false );
}

void FullColorLed::onTimer() {
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

    TIMER_TICK(blink_timer_);
    if (TIMER_FIRED(blink_timer_)) {
        TIMER_STOP(blink_timer_);
        isBlinking_ = false;
    }
}
