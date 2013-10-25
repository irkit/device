/*
  FlexiTimer2.h - Using timer2 with a configurable resolution
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

#include "FlexiTimer2.h"

unsigned long FlexiTimer2::time_units;
void (*FlexiTimer2::func)();

void FlexiTimer2::set(unsigned long ms, void (*f)()) {
	// fix to ms timer

	time_units = ms;

	func = f;

	TCCR4B = 0;
	TCCR4A = 0;
	TCCR4C = 0;
	TCCR4D = 0;
	TCCR4E = 0;
	TCCR4B = (1<<CS43) | (1<<PSR4);

	// (16000_000 * 0.001 / 128) - 1 = 124
	OCR4C  = 124;
}

void FlexiTimer2::start() {
	TIFR4       = (1<<TOV4);
	TCNT4       = 0;
	TIMSK4      = (1<<TOIE4);
}

void FlexiTimer2::stop() {
	TIMSK4 = 0;
}

void FlexiTimer2::_overflow() {
	static uint8_t overflowing = 0;
	static unsigned long count = 0;

	count += 1;

	if (count >= time_units && !overflowing) {
		overflowing = 1;
		count = count - time_units; // subtract time_uints to catch missed overflows
					// set to 0 if you don't want this.
		(*func)();
		overflowing = 0;
	}
}

ISR(TIMER4_OVF_vect) {
	FlexiTimer2::_overflow();
}
