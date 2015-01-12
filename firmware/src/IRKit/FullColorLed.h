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
#ifndef __FULLCOLORLED_H__
#define __FULLCOLORLED_H__

class FullColorLed
{
public:

    // The different modes for lighting the LED
    typedef enum  {
        ALWAYS_ON       = 0,    // LED stays always on.
        BLINK_THEN_ON   = 1,    // LED blinks, then stays on.
        BLINK_THEN_OFF  = 2     // LED blinks, then turns off.
    } LightMode;

    FullColorLed(int pinR, int pinG, int pinB);

    // Lights-up the LED with a specific color, with optional blinking parameters.
    void setLedColor(
            bool colorR,
            bool colorG,
            bool colorB,
            LightMode lightMode = ALWAYS_ON,
            uint8_t blinkTimeout = 0);

    // Turns-off the LED
    void off();

    void onTimer();

private:
    int pinR_;
    int pinG_;
    int pinB_;
    bool colorR_;
    bool colorG_;
    bool colorB_;
    LightMode blinkMode_; // defaults to ALWAYS_ON
    volatile bool blinkOn_; // altered inside timer ISR
    volatile uint8_t blink_timer_;
};

#endif // __FULLCOLORLED_H__
