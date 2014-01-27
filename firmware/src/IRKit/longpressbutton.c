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
#include "longpressbutton.h"
#include "timer.h"

void long_press_button_ontimer( struct long_press_button_state_t* state ) {
    static uint8_t button_state = BUTTON_OFF;

    uint8_t next_button_state = digitalRead( state->pin );

    if ((BUTTON_OFF == button_state) &&
        (BUTTON_ON  == next_button_state)) {
        // OFF -> ON
        TIMER_START( state->timer, state->threshold_time );
    }
    else if ((BUTTON_ON == button_state) &&
             (BUTTON_ON == next_button_state)) {
        // still pressing

        TIMER_TICK( state->timer );

        if (TIMER_FIRED( state->timer )) {
            state->callback();

            // fires again after state->threshold_time
            button_state = BUTTON_OFF;
            return;
        }
    }

    button_state = next_button_state;
}
