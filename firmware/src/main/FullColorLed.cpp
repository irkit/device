#include <Arduino.h>
#include "FullColorLed.h"

FullColorLed::FullColorLed(int pinR, int pinG, int pinB) :
    pinR_(pinR),
    pinG_(pinG),
    pinB_(pinB),
    blinkOn_(0),
    cycleMillis_(0),
    nextMillis_(0)
{
    pinMode(pinR_, OUTPUT);
    pinMode(pinG_, OUTPUT);
    pinMode(pinB_, OUTPUT);
}

void FullColorLed::SetLedColor(bool colorR, bool colorG, bool colorB)
{
    SetLedColor(colorR, colorG, colorB, 0);
}

// blink cycleMillis[ms] ON, and cycleMillis[ms] OFF
void FullColorLed::SetLedColor(bool colorR, bool colorG, bool colorB, unsigned long cycleMillis)
{
    colorR_      = colorR;
    colorG_      = colorG;
    colorB_      = colorB;
    cycleMillis_ = cycleMillis;

    digitalWrite(pinR_, colorR);
    digitalWrite(pinG_, colorG);
    digitalWrite(pinB_, colorB);
}

void FullColorLed::LedOff()
{
    cycleMillis_ = 0;

    digitalWrite(pinR_, LOW);
    digitalWrite(pinG_, LOW);
    digitalWrite(pinB_, LOW);
}

void FullColorLed::Loop()
{
    if ( ! cycleMillis_ ) {
        return;
    }
    unsigned long now = millis();
    if (now > nextMillis_) {
        nextMillis_ = now + cycleMillis_;
        if (blinkOn_) {
            blinkOn_ = 0;
            // on -> off
            digitalWrite(pinR_, LOW);
            digitalWrite(pinG_, LOW);
            digitalWrite(pinB_, LOW);
        }
        else {
            blinkOn_ = 1;
            // off -> on
            digitalWrite(pinR_, colorR_);
            digitalWrite(pinG_, colorG_);
            digitalWrite(pinB_, colorB_);
        }
    }
}
