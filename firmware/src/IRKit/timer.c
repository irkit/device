/*
  timer.c,h - Using timer with a configurable resolution
  Wim Leers <work@wimleers.com>

  Based on MsTimer2
  Javier Valencia <javiervalencia80@gmail.com>

  History:
    25/Oct/2013 - Optimize for 32U4 to reduce program memory (mash)
    16/Dec/2011 - Added Teensy/Teensy++ support (bperrybap)
           note: teensy uses timer4 instead of timer2
    25/April/10 - Based on MsTimer2 V0.5 (from 29/May/09)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifdef __AVR__
#include <avr/interrupt.h>
#else
#error timer library only works on AVR architecture
#endif

#include "timer.h"

static uint16_t interval_ms;
static void (*callback)();

void timer_init( void (*callback_)() ) {
    callback = callback_;

    TCCR4B = 0;
    TCCR4A = 0;
    TCCR4C = 0;
    TCCR4D = 0;
    TCCR4E = 0;
    TCCR4B = (1<<CS43) | (1<<PSR4);

    // (16000_000 * 0.001 / 128) - 1 = 124
    OCR4C  = 124;
}

void timer_start( uint16_t interval_ms_ ) {
    interval_ms = interval_ms_;

    TIFR4       = (1<<TOV4);
    TCNT4       = 0;
    TIMSK4      = (1<<TOIE4);
}

void timer_stop() {
    TIMSK4 = 0;
}

static void _overflow() {
    static uint8_t overflowing = 0;
    static uint16_t count = 0;

    count += 1;

    if ((count >= interval_ms) && ! overflowing) {
        overflowing = 1;
        count = count - interval_ms; // subtract interval to catch missed overflows
                    // set to 0 if you don't want this.
        (*callback)();
        overflowing = 0;
    }
}

ISR(TIMER4_OVF_vect) {
    _overflow();
}
