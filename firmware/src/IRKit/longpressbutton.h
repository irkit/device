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
#ifndef __LONGPRESSBUTTON_H__
#define __LONGPRESSBUTTON_H__

// reverse logic
#define BUTTON_ON  LOW
#define BUTTON_OFF HIGH

struct long_press_button_state_t {
    int              pin;
    uint8_t          threshold_time;
    volatile uint8_t timer;
    void             (*callback)();
};

#ifdef __cplusplus
extern "C" {
#endif

void long_press_button_ontimer( struct long_press_button_state_t* state );

#ifdef  __cplusplus
}
#endif

#endif // __LONGPRESSBUTTON_H__
