/*
 Copyright (C) 2013-2014 Masakazu Ohtsuka
  
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.
  
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
  
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <Arduino.h>
#include "FullColorLed.h"
#include "timer.h"
#include "pins.h"

FullColorLed::FullColorLed() :
    blinkOn_(0),
    blinkMode_(ALWAYS_ON),
    blink_timer_(TIMER_OFF)
{
}

void FullColorLed::setLedColor(bool colorR, bool colorG, bool colorB, LightMode lightMode, uint8_t blinkTimeout) {
    colorR_      = colorR;
    colorG_      = colorG;
    colorB_      = colorB;
    blinkMode_   = lightMode;

    blink_timer_ = TIMER_OFF;
    if (blinkTimeout > 0) {
        TIMER_START(blink_timer_, blinkTimeout);
    }
}

void FullColorLed::off() {
    setLedColor( 0, 0, 0 );
}

void FullColorLed::onTimer() {
    blinkOn_ = ! blinkOn_;

    if ( blinkOn_ || (blinkMode_ == ALWAYS_ON) ) {
        // not blinking = always on
        digitalWrite(FULLCOLOR_LED_R, colorR_);
        digitalWrite(FULLCOLOR_LED_G, colorG_);
        digitalWrite(FULLCOLOR_LED_B, colorB_);
    }
    else {
        digitalWrite(FULLCOLOR_LED_R, LOW);
        digitalWrite(FULLCOLOR_LED_G, LOW);
        digitalWrite(FULLCOLOR_LED_B, LOW);
    }

    TIMER_TICK(blink_timer_);
    if (TIMER_FIRED(blink_timer_)) {
        TIMER_STOP(blink_timer_);
        if (blinkMode_ == BLINK_THEN_OFF) {
            off();
        } else if (blinkMode_ == BLINK_THEN_ON) {
            blinkMode_ = ALWAYS_ON;
        }
    }
}
