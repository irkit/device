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
#include <avr/pgmspace.h>

// largest used in our program
#define PROGMEM_CACHE_SIZE 48

// rule
// index 0 is for directly using Serial.println(P("hoge"))
// index 1,.. is for passing char* into next function

// choose different index to use simultaneously
char to_print[3][PROGMEM_CACHE_SIZE];

char *pgmStrToRAM(PROGMEM char *theString, uint8_t index) {
    strcpy_P( to_print[ index ], theString );
    return to_print[index];
}
