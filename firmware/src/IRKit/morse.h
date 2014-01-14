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
#ifndef __MORSE_LISTENER__
#define __MORSE_LISTENER__

#include "Arduino.h"
#include "stdbool.h"

struct morse_t {
    // called when a letter is detected
    void (*letter_callback)(char letter);
    // called when a word space is detected
    void (*word_callback)();
    int           pin;
    uint16_t      wpm;
    int8_t        index;       // index of morseTable
    bool          enabled;
    bool          is_on;
    bool          word_started;
    bool          did_call_letter_callback;
    unsigned long last_changed;
    unsigned long last_on;

    // if raw input was higher than threshold more than once per period: it's HIGH
    // if raw input was always lower than threshold for period: it's LOW
    uint16_t      debounce_period;
    uint16_t      min_letter_space;
    uint16_t      min_word_space;
};

#ifdef __cplusplus
extern "C" {
#endif

void morse_setup( struct morse_t *state, uint16_t wpm );
void morse_loop( struct morse_t *state );
void morse_enable( struct morse_t *state, bool enabled );

#ifdef __cplusplus
}
#endif

#endif
